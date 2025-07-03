#include "ollama_interface.h"
#include <iostream>
#include <filesystem>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <regex>

bool OllamaInterface::isModelAvailable(const std::string& model_name) {
    std::string command = "ollama list | grep -q '" + model_name + "'";
    int result = system(command.c_str());
    return result == 0;
}

std::string OllamaInterface::downloadModel(const std::string& model_name) {
    if (!isModelAvailable(model_name)) {
        std::string command = "ollama pull " + model_name;
        int result = system(command.c_str());
        if (result != 0) {
            throw std::runtime_error("Failed to download model: " + model_name);
        }
    }
    return getModelPath(model_name);
}

std::string OllamaInterface::getModelPath(const std::string& model_name) {
    std::string home = std::getenv("HOME");
    std::string ollama_dir = home + "/.ollama/models";
    
    // Simplified approach: search for GGUF files in ollama's blobs directory
    std::string blobs_dir = ollama_dir + "/blobs";
    
    if (std::filesystem::exists(blobs_dir)) {
        // Find the largest GGUF file (likely the model)
        std::string largest_gguf;
        std::uintmax_t largest_size = 0;
        
        for (const auto& entry : std::filesystem::directory_iterator(blobs_dir)) {
            if (entry.is_regular_file()) {
                // Check if it's a GGUF file by reading magic bytes
                std::ifstream file(entry.path(), std::ios::binary);
                char magic[4];
                file.read(magic, 4);
                if (file && std::string(magic, 4) == "GGUF") {
                    auto size = std::filesystem::file_size(entry.path());
                    if (size > largest_size) {
                        largest_size = size;
                        largest_gguf = entry.path().string();
                    }
                }
            }
        }
        
        if (!largest_gguf.empty()) {
            return largest_gguf;
        }
    }
    
    throw std::runtime_error("Model path not found for: " + model_name);
}

std::vector<int> OllamaInterface::tokenize(const std::string& text, const std::string& model_path) {
    // TODO: Implement tokenization using ollama's tokenizer
    // This is a placeholder implementation
    std::vector<int> tokens;
    for (char c : text) {
        tokens.push_back(static_cast<int>(c));
    }
    return tokens;
}

std::string OllamaInterface::detokenize(const std::vector<int>& tokens, const std::string& model_path) {
    // TODO: Implement detokenization using ollama's tokenizer
    // This is a placeholder implementation
    std::string result;
    for (int token : tokens) {
        result += static_cast<char>(token);
    }
    return result;
}