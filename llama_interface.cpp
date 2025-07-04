#include "llama_interface.h"
#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <filesystem>
#include <thread>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <chrono>

// Use ollama's enhanced llama.cpp headers
#include "llama.h"
#include "common.h"

/**
 * Wrapper structs to hide llama.cpp implementation details from the header
 * This provides a clean interface while using ollama's enhanced llama.cpp
 */
struct LlamaModel {
    llama_model* model = nullptr;
    ~LlamaModel() {
        if (model) {
            llama_model_free(model);
        }
    }
};

struct LlamaContext {
    llama_context* ctx = nullptr;
    ~LlamaContext() {
        if (ctx) {
            llama_free(ctx);
        }
    }
};

/**
 * Constructor - Initialize llama backend
 * Uses ollama's enhanced llama.cpp with production patches
 */
LlamaInterface::LlamaInterface() 
    : model(std::make_unique<LlamaModel>())
    , context(std::make_unique<LlamaContext>()) {
    
    // Initialize ollama's enhanced llama.cpp backend
    llama_backend_init();
    
}

/**
 * Destructor - Clean up resources
 * Resources are automatically cleaned up by the wrapper structs
 */
LlamaInterface::~LlamaInterface() {
    // Resources are automatically cleaned up by LlamaModel and LlamaContext destructors
    llama_backend_free();
}

/**
 * Load a GGUF model from the specified path
 * Uses ollama's enhanced model loading with stability patches
 */
bool LlamaInterface::loadModel(const std::string& path) {
    return loadModel(path, hardware_config);
}

bool LlamaInterface::loadModel(const std::string& path, const HardwareConfig& config) {
    model_path = path;
    hardware_config = config;
    
    // Validate file exists and is readable
    if (!std::filesystem::exists(path)) {
        return false;
    }
    
    if (!std::filesystem::is_regular_file(path)) {
        return false;
    }
    
    try {
        // Set up model parameters based on hardware configuration
        llama_model_params model_params = llama_model_default_params();
        
        // Configure GPU usage based on detected/configured mode
        HardwareConfig effective_config = (config.gpu_mode == GPUMode::AUTO) ? 
            detectOptimalConfig() : config;
            
        switch (effective_config.gpu_mode) {
            case GPUMode::CPU_ONLY:
                model_params.n_gpu_layers = 0;
                break;
            case GPUMode::SINGLE_GPU:
                model_params.n_gpu_layers = (effective_config.gpu_layers == -1) ? 999 : effective_config.gpu_layers;
                model_params.main_gpu = effective_config.main_gpu;
                break;
            case GPUMode::DUAL_GPU:
                model_params.n_gpu_layers = (effective_config.gpu_layers == -1) ? 999 : effective_config.gpu_layers;
                model_params.main_gpu = 0; // Use GPU 0 as primary
                // Note: tensor_split configuration would go here but current llama.cpp version
                // may have different API. For dual GPU, ollama handles this automatically.
                break;
            default:
                model_params.n_gpu_layers = 0; // Fallback to CPU
        }
        
        model_params.use_mmap = effective_config.use_mmap;
        model_params.use_mlock = effective_config.use_mlock;
        model_params.vocab_only = false;
        
        hardware_config = effective_config; // Store the effective config
        
        // Load the model using ollama's enhanced loader
        model->model = llama_model_load_from_file(path.c_str(), model_params);
        if (!model->model) {
            return false;
        }
        
        // Set up context parameters optimized for this hardware
        llama_context_params ctx_params = llama_context_default_params();
        ctx_params.n_ctx = 2048;      // Context window size
        ctx_params.n_batch = 512;     // Batch size for processing
        
        // Configure threads based on GPU mode and available hardware
        int optimal_threads;
        if (effective_config.cpu_threads != -1) {
            optimal_threads = effective_config.cpu_threads;
        } else {
            optimal_threads = getOptimalCPUThreads();
            // Adjust for GPU usage - use fewer CPU threads when GPUs are active
            if (effective_config.gpu_mode != GPUMode::CPU_ONLY) {
                optimal_threads = std::max(1, optimal_threads / 2);
            }
        }
        
        ctx_params.n_threads = optimal_threads;
        ctx_params.n_threads_batch = optimal_threads;
        ctx_params.type_k = GGML_TYPE_F16; // Use F16 for KV cache to save memory
        ctx_params.type_v = GGML_TYPE_F16;
        
        // Create context with ollama's enhanced context management
        context->ctx = llama_init_from_model(model->model, ctx_params);
        if (!context->ctx) {
            return false;
        }
        
        
        return true;
        
    } catch (const std::exception& e) {
        return false;
    }
}

std::string LlamaInterface::generate(const std::string& prompt, int max_tokens) {
    if (!model || !model->model || !context || !context->ctx) {
        throw std::runtime_error("Model not properly initialized");
    }
    
    if (prompt.empty()) {
        throw std::runtime_error("Prompt cannot be empty");
    }
    
    if (max_tokens <= 0 || max_tokens > 4096) {
        throw std::runtime_error("max_tokens must be between 1 and 4096");
    }
    
    // Tokenize the prompt
    const auto vocab = llama_model_get_vocab(model->model);
    std::vector<llama_token> tokens;
    tokens.resize(prompt.length() + 1);
    int n_tokens = llama_tokenize(vocab, prompt.c_str(), prompt.length(), 
                                  tokens.data(), tokens.size(), true, true);
    if (n_tokens < 0) {
        throw std::runtime_error("Failed to tokenize prompt");
    }
    tokens.resize(n_tokens);
    
    // Clear the KV cache
    llama_kv_self_clear(context->ctx);
    
    // Process the prompt in optimal batches
    const size_t optimal_batch_size = 256; // Smaller batches for better memory usage
    for (size_t i = 0; i < tokens.size(); i += optimal_batch_size) {
        size_t batch_size = std::min(optimal_batch_size, tokens.size() - i);
        if (llama_decode(context->ctx, llama_batch_get_one(&tokens[i], batch_size))) {
            throw std::runtime_error("Failed to decode prompt");
        }
    }
    
    // Generate response
    std::string response;
    llama_sampler* sampler = llama_sampler_chain_init(llama_sampler_chain_default_params());
    llama_sampler_chain_add(sampler, llama_sampler_init_top_k(top_k));
    llama_sampler_chain_add(sampler, llama_sampler_init_top_p(top_p, 1));
    llama_sampler_chain_add(sampler, llama_sampler_init_temp(temperature));
    
    for (int i = 0; i < max_tokens; i++) {
        llama_token new_token = llama_sampler_sample(sampler, context->ctx, -1);
        
        // Check for end of sequence
        if (llama_vocab_is_eog(vocab, new_token)) {
            break;
        }
        
        // Convert token to text
        char token_str[256];
        int token_len = llama_token_to_piece(vocab, new_token, token_str, sizeof(token_str), 0, true);
        if (token_len > 0) {
            response.append(token_str, token_len);
        }
        
        // Process the new token
        if (llama_decode(context->ctx, llama_batch_get_one(&new_token, 1))) {
            break;
        }
    }
    
    llama_sampler_free(sampler);
    return response;
}

void LlamaInterface::setTemperature(float temperature) {
    this->temperature = temperature;
}

void LlamaInterface::setTopP(float top_p) {
    this->top_p = top_p;
}

void LlamaInterface::setTopK(int top_k) {
    this->top_k = top_k;
}

void LlamaInterface::clearCache() {
    if (context && context->ctx) {
        llama_kv_self_clear(context->ctx);
    }
}

void LlamaInterface::setHardwareConfig(const HardwareConfig& config) {
    hardware_config = config;
}

HardwareConfig LlamaInterface::getHardwareConfig() const {
    return hardware_config;
}

int LlamaInterface::detectGPUCount() {
    // Try to get GPU count from nvidia-smi
    FILE* pipe = popen("nvidia-smi --query-gpu=count --format=csv,noheader,nounits 2>/dev/null | wc -l", "r");
    if (pipe) {
        char buffer[32];
        if (fgets(buffer, sizeof(buffer), pipe)) {
            int count = std::atoi(buffer);
            pclose(pipe);
            return count;
        }
        pclose(pipe);
    }
    
    // Fallback: try to detect via /proc
    std::ifstream nvidia_smi("/proc/driver/nvidia/gpus");
    if (!nvidia_smi.is_open()) {
        return 0; // No NVIDIA driver or GPUs
    }
    
    int gpu_count = 0;
    std::string line;
    while (std::getline(nvidia_smi, line)) {
        if (line.find("GPU-") != std::string::npos) {
            gpu_count++;
        }
    }
    
    return gpu_count;
}

std::vector<std::string> LlamaInterface::getGPUInfo() {
    std::vector<std::string> gpu_info;
    
    // Try to get GPU info from nvidia-ml-py or nvidia-smi
    FILE* pipe = popen("nvidia-smi --query-gpu=name,memory.total --format=csv,noheader,nounits 2>/dev/null", "r");
    if (pipe) {
        char buffer[256];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            std::string line(buffer);
            if (!line.empty() && line.back() == '\n') {
                line.pop_back();
            }
            if (!line.empty()) {
                gpu_info.push_back(line);
            }
        }
        pclose(pipe);
    }
    
    return gpu_info;
}

int LlamaInterface::getOptimalCPUThreads() {
    int total_cores = std::thread::hardware_concurrency();
    
    // For dual Xeon setup, we want to use most cores but leave some for system
    if (total_cores >= 16) {
        return total_cores - 2; // Leave 2 cores for system
    } else if (total_cores >= 8) {
        return total_cores - 1; // Leave 1 core for system
    } else {
        return std::max(1, total_cores / 2);
    }
}

HardwareConfig LlamaInterface::detectOptimalConfig() {
    HardwareConfig config;
    
    int gpu_count = detectGPUCount();
    
    if (gpu_count >= 2) {
        config.gpu_mode = GPUMode::DUAL_GPU;
        config.gpu_layers = 999; // Use all layers on GPU
        config.tensor_split_enabled = true;
        config.tensor_split = {0.5f, 0.5f}; // 50/50 split
    } else if (gpu_count == 1) {
        config.gpu_mode = GPUMode::SINGLE_GPU;
        config.gpu_layers = 999; // Use all layers on GPU
        config.main_gpu = 0;
    } else {
        config.gpu_mode = GPUMode::CPU_ONLY;
        config.gpu_layers = 0;
    }
    
    config.cpu_threads = getOptimalCPUThreads();
    config.use_mmap = true;
    config.use_mlock = false; // Don't lock memory by default
    
    return config;
}

LlamaInterface::PerformanceStats LlamaInterface::getPerformanceStats() const {
    PerformanceStats stats;
    
    // Get GPU count and memory usage
    stats.active_gpus = detectGPUCount();
    
    // Get VRAM usage if GPUs are available
    if (stats.active_gpus > 0) {
        FILE* pipe = popen("nvidia-smi --query-gpu=memory.used --format=csv,noheader,nounits 2>/dev/null", "r");
        if (pipe) {
            char buffer[64];
            if (fgets(buffer, sizeof(buffer), pipe)) {
                stats.vram_usage_mb = std::stoul(buffer);
            }
            pclose(pipe);
        }
    }
    
    // Get system memory usage
    std::ifstream meminfo("/proc/meminfo");
    if (meminfo.is_open()) {
        std::string line;
        size_t mem_total = 0, mem_available = 0;
        while (std::getline(meminfo, line)) {
            if (line.substr(0, 9) == "MemTotal:") {
                std::istringstream iss(line);
                std::string label, unit;
                iss >> label >> mem_total >> unit;
            } else if (line.substr(0, 12) == "MemAvailable:") {
                std::istringstream iss(line);
                std::string label, unit;
                iss >> label >> mem_available >> unit;
            }
        }
        if (mem_total > 0) {
            stats.memory_usage_mb = (mem_total - mem_available) / 1024;
        }
    }
    
    return stats;
}