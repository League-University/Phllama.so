#include "llama_interface.h"
#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <filesystem>
#include <thread>
#include <algorithm>

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
    model_path = path;
    
    // Validate file exists and is readable
    if (!std::filesystem::exists(path)) {
        return false;
    }
    
    if (!std::filesystem::is_regular_file(path)) {
        return false;
    }
    
    try {
        // Set up model parameters with ollama's optimizations
        llama_model_params model_params = llama_model_default_params();
        model_params.n_gpu_layers = 0; // CPU only for alpha release
        model_params.use_mmap = true;  // Use memory mapping for efficiency
        model_params.use_mlock = false; // Don't lock memory for now
        model_params.vocab_only = false; // Load full model
        
        // Load the model using ollama's enhanced loader
        model->model = llama_model_load_from_file(path.c_str(), model_params);
        if (!model->model) {
            return false;
        }
        
        // Set up context parameters optimized for typical usage
        llama_context_params ctx_params = llama_context_default_params();
        ctx_params.n_ctx = 2048;      // Context window size
        ctx_params.n_batch = 512;     // Batch size for processing
        // Use optimal thread count: half of available cores, min 1, max 8
        int optimal_threads = std::max(1, std::min(8, static_cast<int>(std::thread::hardware_concurrency()) / 2));
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