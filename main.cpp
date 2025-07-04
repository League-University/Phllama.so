#include <phpcpp.h>
#include <iostream>
#include <string>
#include <memory>
#include <filesystem>
#include <regex>
#include <cmath>
#include "ollama_interface.h"
#include "llama_interface.h"

/**
 * Phllama PHP Extension
 * 
 * A high-performance PHP extension for AI model inference using ollama's
 * enhanced llama.cpp with production-tested stability patches.
 * 
 * Features:
 * - Zero runtime overhead after model loading
 * - Direct C++ inference without CLI processes
 * - Enhanced stability with ollama's patches
 * - Automatic model discovery and validation
 * 
 * @version 1.0.0-alpha
 * @author Phllama.so Project
 */
class Phllama : public Php::Base
{
private:
    std::string model_path;
    std::string model_identifier;
    bool is_ollama_model;
    std::unique_ptr<LlamaInterface> llama_engine;
    
public:
    Phllama() = default;
    virtual ~Phllama() = default;
    
    /**
     * Constructor - Initialize with model identifier
     * 
     * @param model_identifier Either an ollama model name (e.g., "phi3:latest") 
     *                        or a direct path to a GGUF file
     */
    void __construct(Php::Parameters &params)
    {
        if (params.size() != 1) {
            throw Php::Exception("Phllama constructor requires exactly one parameter: model identifier");
        }
        
        model_identifier = static_cast<std::string>(params[0]);
        
        // Security: Input validation and sanitization
        if (model_identifier.empty()) {
            throw Php::Exception("Model identifier cannot be empty");
        }
        
        // Check for maximum length to prevent memory issues
        if (model_identifier.length() > 512) {
            throw Php::Exception("Model identifier too long (max 512 characters)");
        }
        
        // Security: Prevent directory traversal and command injection
        if (model_identifier.find("../") != std::string::npos ||
            model_identifier.find("..\\") != std::string::npos ||
            model_identifier.find("\\") != std::string::npos ||
            model_identifier.find(";") != std::string::npos ||
            model_identifier.find("|") != std::string::npos ||
            model_identifier.find("&") != std::string::npos ||
            model_identifier.find("$") != std::string::npos ||
            model_identifier.find("`") != std::string::npos) {
            throw Php::Exception("Invalid characters in model identifier");
        }
        
        // Security: Check for valid characters (alphanumeric, dash, underscore, dot, colon, slash for paths)
        if (!std::regex_match(model_identifier, std::regex("^[a-zA-Z0-9._:/-]+$"))) {
            throw Php::Exception("Model identifier contains invalid characters");
        }
        
        // Determine if this is a file path or ollama model name
        if (model_identifier.find('/') != std::string::npos || 
            model_identifier.find(".gguf") != std::string::npos ||
            model_identifier.find('\\') != std::string::npos) {
            // File path
            model_path = model_identifier;
            is_ollama_model = false;
        } else {
            // Ollama model name
            model_path = model_identifier;
            is_ollama_model = true;
        }
        
        try {
            initializeModel();
        } catch (const std::exception& e) {
            throw Php::Exception("Failed to initialize model '" + model_identifier + "': " + e.what());
        }
    }
    
    /**
     * Generate a response to the given message
     * 
     * @param message The input message/prompt
     * @return Generated response string
     */
    Php::Value sendMessage(Php::Parameters &params)
    {
        if (params.size() != 1) {
            throw Php::Exception("sendMessage requires exactly one parameter: message");
        }
        
        std::string message = static_cast<std::string>(params[0]);
        
        // Security: Input validation
        if (message.empty()) {
            throw Php::Exception("Message cannot be empty");
        }
        
        // Security: Prevent extremely large inputs that could cause memory issues
        if (message.length() > 100000) {
            throw Php::Exception("Message too long (max 100KB)");
        }
        
        try {
            return generateResponse(message);
        } catch (const std::exception& e) {
            throw Php::Exception("Failed to generate response: " + std::string(e.what()));
        }
    }
    
    /**
     * Set the sampling temperature (0.0 to 1.0)
     * Higher values make output more random, lower values more deterministic
     * 
     * @param temperature Float between 0.0 and 1.0
     */
    void setTemperature(Php::Parameters &params)
    {
        if (params.size() != 1) {
            throw Php::Exception("setTemperature requires exactly one parameter: temperature");
        }
        
        if (!llama_engine) {
            throw Php::Exception("Model not initialized. Cannot set temperature.");
        }
        
        double temperature = static_cast<double>(params[0]);
        
        // Security: Validate numeric input ranges
        if (std::isnan(temperature) || std::isinf(temperature)) {
            throw Php::Exception("Invalid temperature value");
        }
        
        if (temperature < 0.0 || temperature > 2.0) {
            throw Php::Exception("Temperature must be between 0.0 and 2.0, got: " + std::to_string(temperature));
        }
        
        llama_engine->setTemperature(static_cast<float>(temperature));
    }
    
    /**
     * Set the top-p sampling parameter (0.0 to 1.0)
     * Controls diversity via nucleus sampling
     * 
     * @param top_p Float between 0.0 and 1.0
     */
    void setTopP(Php::Parameters &params)
    {
        if (params.size() != 1) {
            throw Php::Exception("setTopP requires exactly one parameter: top_p");
        }
        
        if (!llama_engine) {
            throw Php::Exception("Model not initialized. Cannot set top_p.");
        }
        
        double top_p = static_cast<double>(params[0]);
        
        // Security: Validate numeric input
        if (std::isnan(top_p) || std::isinf(top_p)) {
            throw Php::Exception("Invalid top_p value");
        }
        
        if (top_p < 0.0 || top_p > 1.0) {
            throw Php::Exception("top_p must be between 0.0 and 1.0, got: " + std::to_string(top_p));
        }
        
        llama_engine->setTopP(static_cast<float>(top_p));
    }
    
    /**
     * Clear the KV cache to free memory
     * Useful for long-running processes or when switching contexts
     */
    void clearCache()
    {
        if (!llama_engine) {
            throw Php::Exception("Model not initialized. Cannot clear cache.");
        }
        
        llama_engine->clearCache();
    }
    
    /**
     * Get information about the loaded model
     * 
     * @return Array with model information
     */
    Php::Value getModelInfo()
    {
        if (!llama_engine) {
            throw Php::Exception("Model not initialized");
        }
        
        Php::Array info;
        info["identifier"] = model_identifier;
        info["path"] = model_path;
        info["type"] = is_ollama_model ? "ollama" : "direct";
        info["version"] = "1.0.0-alpha";
        
        return info;
    }
    
    /**
     * Set custom ollama models directory
     * 
     * @param directory Path to ollama models directory
     */
    static void setOllamaModelsDir(Php::Parameters &params)
    {
        if (params.size() != 1) {
            throw Php::Exception("setOllamaModelsDir requires exactly one parameter: directory path");
        }
        
        std::string directory = static_cast<std::string>(params[0]);
        
        // Security: Validate directory path
        if (directory.empty()) {
            throw Php::Exception("Directory path cannot be empty");
        }
        
        if (directory.length() > 512) {
            throw Php::Exception("Directory path too long (max 512 characters)");
        }
        
        // Security: Basic path validation
        if (directory.find("..") != std::string::npos) {
            throw Php::Exception("Directory traversal not allowed");
        }
        
        OllamaInterface::setModelsDirectory(directory);
    }
    
    /**
     * Get current ollama models directory
     * 
     * @return Current models directory path
     */
    static Php::Value getOllamaModelsDir()
    {
        return OllamaInterface::getModelsDirectory();
    }

private:
    /**
     * Initialize the model based on whether it's an ollama model or direct file
     */
    void initializeModel()
    {
        if (is_ollama_model) {
            setupOllamaModel();
        } else {
            setupDirectModel();
        }
    }
    
    /**
     * Setup model through ollama (download if needed, then load)
     */
    void setupOllamaModel()
    {
        try {
            std::string actual_path = OllamaInterface::downloadModel(model_path);
            model_path = actual_path;  // Update to actual file path
            
            llama_engine = std::make_unique<LlamaInterface>();
            if (!llama_engine->loadModel(actual_path)) {
                throw std::runtime_error("Failed to load ollama model: " + model_identifier);
            }
        } catch (const std::exception& e) {
            throw std::runtime_error("Ollama model setup failed for '" + model_identifier + "': " + e.what());
        }
    }
    
    /**
     * Setup model from direct file path
     */
    void setupDirectModel()
    {
        // Validate file exists
        if (!std::filesystem::exists(model_path)) {
            throw std::runtime_error("Model file not found: " + model_path);
        }
        
        // Validate file extension
        if (model_path.find(".gguf") == std::string::npos) {
            throw std::runtime_error("Only GGUF files are supported. File: " + model_path);
        }
        
        llama_engine = std::make_unique<LlamaInterface>();
        if (!llama_engine->loadModel(model_path)) {
            throw std::runtime_error("Failed to load model from path: " + model_path);
        }
    }
    
    /**
     * Generate response using the loaded model
     */
    std::string generateResponse(const std::string& message)
    {
        if (!llama_engine) {
            throw std::runtime_error("Model not initialized");
        }
        
        return llama_engine->generate(message);
    }
};

extern "C" {
    /**
     * PHP Extension Module Entry Point
     * 
     * Exports the Phllama class to PHP with all its methods
     */
    PHPCPP_EXPORT void *get_module() 
    {
        static Php::Extension extension("phllama", "1.0.0-alpha");
        
        Php::Class<Phllama> phllama("Phllama");
        
        // Core functionality
        phllama.method<&Phllama::__construct>("__construct", {
            Php::ByVal("model", Php::Type::String)
        });
        
        phllama.method<&Phllama::sendMessage>("sendMessage", {
            Php::ByVal("message", Php::Type::String)
        });
        
        // Configuration methods
        phllama.method<&Phllama::setTemperature>("setTemperature", {
            Php::ByVal("temperature", Php::Type::Float)
        });
        
        phllama.method<&Phllama::setTopP>("setTopP", {
            Php::ByVal("top_p", Php::Type::Float)
        });
        
        phllama.method<&Phllama::clearCache>("clearCache");
        
        // Utility methods
        phllama.method<&Phllama::getModelInfo>("getModelInfo");
        
        // Static configuration methods
        extension.add(Php::Constant("PHLLAMA_VERSION", "1.0.0-alpha"));
        extension.add("Phllama_setOllamaModelsDir", &Phllama::setOllamaModelsDir, {
            Php::ByVal("directory", Php::Type::String)
        });
        extension.add("Phllama_getOllamaModelsDir", &Phllama::getOllamaModelsDir);
        
        extension.add(std::move(phllama));
        
        return extension;
    }
}