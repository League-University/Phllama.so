#include <phpcpp.h>
#include <iostream>
#include <string>
#include <memory>
#include "ollama_interface.h"
#include "llama_interface.h"

class Phllama : public Php::Base
{
private:
    std::string model_path;
    bool is_ollama_model;
    std::unique_ptr<LlamaInterface> llama_engine;
    
public:
    Phllama() = default;
    virtual ~Phllama() = default;
    
    void __construct(Php::Parameters &params)
    {
        if (params.size() != 1) {
            throw Php::Exception("Phllama constructor requires exactly one parameter");
        }
        
        std::string model_identifier = params[0];
        
        if (model_identifier.find('/') != std::string::npos || 
            model_identifier.find('.gguf') != std::string::npos) {
            model_path = model_identifier;
            is_ollama_model = false;
        } else {
            model_path = model_identifier;
            is_ollama_model = true;
        }
        
        initializeModel();
    }
    
    Php::Value sendMessage(Php::Parameters &params)
    {
        if (params.size() != 1) {
            throw Php::Exception("sendMessage requires exactly one parameter");
        }
        
        std::string message = params[0];
        
        return generateResponse(message);
    }
    
    void setTemperature(Php::Parameters &params)
    {
        if (params.size() != 1) {
            throw Php::Exception("setTemperature requires exactly one parameter");
        }
        
        if (!llama_engine) {
            throw Php::Exception("Model not initialized");
        }
        
        double temperature = params[0];
        llama_engine->setTemperature(static_cast<float>(temperature));
    }
    
    void setTopP(Php::Parameters &params)
    {
        if (params.size() != 1) {
            throw Php::Exception("setTopP requires exactly one parameter");
        }
        
        if (!llama_engine) {
            throw Php::Exception("Model not initialized");
        }
        
        double top_p = params[0];
        llama_engine->setTopP(static_cast<float>(top_p));
    }

private:
    void initializeModel()
    {
        if (is_ollama_model) {
            setupOllamaModel();
        } else {
            setupDirectModel();
        }
    }
    
    void setupOllamaModel()
    {
        try {
            std::string actual_path = OllamaInterface::downloadModel(model_path);
            llama_engine = std::make_unique<LlamaInterface>();
            if (!llama_engine->loadModel(actual_path)) {
                throw Php::Exception("Failed to load model: " + model_path);
            }
        } catch (const std::exception& e) {
            throw Php::Exception("Ollama model setup failed: " + std::string(e.what()));
        }
    }
    
    void setupDirectModel()
    {
        llama_engine = std::make_unique<LlamaInterface>();
        if (!llama_engine->loadModel(model_path)) {
            throw Php::Exception("Failed to load model from path: " + model_path);
        }
    }
    
    std::string generateResponse(const std::string& message)
    {
        if (!llama_engine) {
            throw Php::Exception("Model not initialized");
        }
        
        try {
            return llama_engine->generate(message);
        } catch (const std::exception& e) {
            throw Php::Exception("Generation failed: " + std::string(e.what()));
        }
    }
};

extern "C" {
    PHPCPP_EXPORT void *get_module() 
    {
        static Php::Extension extension("phllama", "1.0");
        
        Php::Class<Phllama> phllama("Phllama");
        phllama.method<&Phllama::__construct>("__construct");
        phllama.method<&Phllama::sendMessage>("sendMessage");
        phllama.method<&Phllama::setTemperature>("setTemperature");
        phllama.method<&Phllama::setTopP>("setTopP");
        
        extension.add(std::move(phllama));
        
        return extension;
    }
}