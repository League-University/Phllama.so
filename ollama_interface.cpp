#include "ollama_interface.h"
#include <iostream>
#include <filesystem>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <regex>
#include <unordered_map>
#include <chrono>

// Static member definition
std::string OllamaInterface::models_directory = "";

namespace {
    // Cache for model paths to avoid repeated filesystem scans
    std::unordered_map<std::string, std::string> model_cache;
    
    /**
     * Execute a command and capture its output
     * Only used for ollama operations when absolutely necessary
     */
    std::string executeCommand(const std::string& command) {
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            throw std::runtime_error("Failed to execute command: " + command);
        }
        
        std::string result;
        char buffer[128];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
        
        int exit_code = pclose(pipe);
        if (exit_code != 0) {
            throw std::runtime_error("Command failed with exit code " + std::to_string(exit_code) + ": " + command);
        }
        
        return result;
    }
}

/**
 * Check if model is available locally by scanning ollama's directory
 * Avoids CLI calls when possible
 */
bool OllamaInterface::isModelAvailable(const std::string& model_name) {
    try {
        // Try to get model path directly (this will check filesystem)
        getModelPath(model_name);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

/**
 * Download model if needed, then return path to GGUF file
 * Uses CLI only when model is not found locally
 */
std::string OllamaInterface::downloadModel(const std::string& model_name) {
    // First try to find existing model
    try {
        std::string existing_path = getModelPath(model_name);
        return existing_path;
    } catch (const std::exception&) {
        // Model not found locally, need to download
    }
    
    
    // Use ollama CLI to download (only time we use system calls)
    try {
        std::string command = "ollama pull " + model_name + " 2>&1";
        executeCommand(command);
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to download model '" + model_name + "': " + e.what());
    }
    
    // Now try to find the downloaded model
    try {
        return getModelPath(model_name);
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Model downloaded but could not be located: ") + e.what());
    }
}

/**
 * Get the filesystem path to a model's GGUF file
 * Uses intelligent caching and filesystem scanning
 */
std::string OllamaInterface::getModelPath(const std::string& model_name) {
    // Security: Validate model name
    if (model_name.empty() || model_name.length() > 256) {
        throw std::runtime_error("Invalid model name");
    }
    
    // Check cache first
    auto cache_it = model_cache.find(model_name);
    if (cache_it != model_cache.end()) {
        // Verify cached path still exists
        if (std::filesystem::exists(cache_it->second)) {
            return cache_it->second;
        } else {
            // Remove stale cache entry
            model_cache.erase(cache_it);
        }
    }
    
    // Get ollama directory from configuration or default
    std::string ollama_dir;
    if (!models_directory.empty()) {
        ollama_dir = models_directory;
    } else {
        const char* home_env = std::getenv("HOME");
        if (!home_env) {
            throw std::runtime_error("HOME environment variable not set");
        }
        std::string home = home_env;
        ollama_dir = home + "/.ollama/models";
    }
    
    if (!std::filesystem::exists(ollama_dir)) {
        throw std::runtime_error("Ollama models directory not found: " + ollama_dir);
    }
    
    // Search for GGUF files in ollama's blobs directory
    std::string blobs_dir = ollama_dir + "/blobs";
    
    if (!std::filesystem::exists(blobs_dir)) {
        throw std::runtime_error("Ollama blobs directory not found: " + blobs_dir);
    }
    
    std::vector<std::pair<std::string, std::uintmax_t>> gguf_files;
    
    try {
        for (const auto& entry : std::filesystem::directory_iterator(blobs_dir)) {
            if (!entry.is_regular_file()) continue;
            
            // Check if it's a GGUF file by reading magic bytes
            std::ifstream file(entry.path(), std::ios::binary);
            if (!file) continue;
            
            char magic[4];
            file.read(magic, 4);
            if (file && std::string(magic, 4) == "GGUF") {
                auto size = std::filesystem::file_size(entry.path());
                gguf_files.emplace_back(entry.path().string(), size);
            }
        }
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to scan ollama blobs directory: " + std::string(e.what()));
    }
    
    if (gguf_files.empty()) {
        throw std::runtime_error("No GGUF files found in ollama blobs directory for model: " + model_name);
    }
    
    // For now, return the largest GGUF file (most likely the main model)
    // TODO: In the future, we could parse ollama's manifest files to be more precise
    auto largest = std::max_element(gguf_files.begin(), gguf_files.end(),
        [](const auto& a, const auto& b) { return a.second < b.second; });
    
    std::string model_path = largest->first;
    
    // Cache the result
    model_cache[model_name] = model_path;
    
    
    return model_path;
}

/**
 * Tokenize text using the model's tokenizer
 * NOTE: Currently placeholder - tokenization is handled by llama.cpp directly
 * This method is kept for potential future API compatibility
 */
std::vector<int> OllamaInterface::tokenize(const std::string& text, const std::string& model_path) {
    // TODO: Implement proper tokenization using ollama's tokenizer
    // For now, this is a placeholder since llama.cpp handles tokenization directly
    throw std::runtime_error("Direct tokenization not yet implemented. Use llama.cpp tokenization instead.");
}

/**
 * Detokenize tokens back to text using the model's tokenizer  
 * NOTE: Currently placeholder - detokenization is handled by llama.cpp directly
 * This method is kept for potential future API compatibility
 */
std::string OllamaInterface::detokenize(const std::vector<int>& tokens, const std::string& model_path) {
    // TODO: Implement proper detokenization using ollama's tokenizer
    // For now, this is a placeholder since llama.cpp handles detokenization directly
    throw std::runtime_error("Direct detokenization not yet implemented. Use llama.cpp detokenization instead.");
}

/**
 * Set custom models directory (useful for non-standard installations)
 */
void OllamaInterface::setModelsDirectory(const std::string& directory) {
    models_directory = directory;
    // Clear cache when directory changes
    model_cache.clear();
}

/**
 * Get current models directory
 */
std::string OllamaInterface::getModelsDirectory() {
    if (!models_directory.empty()) {
        return models_directory;
    }
    
    const char* home_env = std::getenv("HOME");
    if (!home_env) {
        return "";
    }
    
    return std::string(home_env) + "/.ollama/models";
}