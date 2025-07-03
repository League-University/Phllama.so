#include "llama_interface.h"
#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstring>

#include "llama.h"
#include "common.h"

// Wrapper structs to hide llama.cpp details
struct LlamaModel {
    llama_model* model = nullptr;
};

struct LlamaContext {
    llama_context* ctx = nullptr;
};

LlamaInterface::LlamaInterface() 
    : model(std::make_unique<LlamaModel>())
    , context(std::make_unique<LlamaContext>()) {
    llama_backend_init();
}

LlamaInterface::~LlamaInterface() {
    if (context->ctx) {
        llama_free(context->ctx);
    }
    if (model->model) {
        llama_free_model(model->model);
    }
    llama_backend_free();
}

bool LlamaInterface::loadModel(const std::string& path) {
    model_path = path;
    
    // Set up model parameters
    llama_model_params model_params = llama_model_default_params();
    model_params.n_gpu_layers = 0; // CPU only for now
    
    // Load the model
    model->model = llama_load_model_from_file(path.c_str(), model_params);
    if (!model->model) {
        std::cerr << "Failed to load model from: " << path << std::endl;
        return false;
    }
    
    // Set up context parameters
    llama_context_params ctx_params = llama_context_default_params();
    ctx_params.n_ctx = 2048; // Context size
    ctx_params.n_batch = 512; // Batch size
    ctx_params.n_threads = 4; // Number of threads
    
    // Create context
    context->ctx = llama_new_context_with_model(model->model, ctx_params);
    if (!context->ctx) {
        std::cerr << "Failed to create context" << std::endl;
        llama_free_model(model->model);
        model->model = nullptr;
        return false;
    }
    
    std::cout << "Successfully loaded model from: " << path << std::endl;
    return true;
}

std::string LlamaInterface::generate(const std::string& prompt, int max_tokens) {
    if (!model->model || !context->ctx) {
        throw std::runtime_error("Model not loaded");
    }
    
    // Tokenize the prompt
    std::vector<llama_token> tokens;
    tokens.resize(prompt.length() + 1);
    int n_tokens = llama_tokenize(model->model, prompt.c_str(), prompt.length(), 
                                  tokens.data(), tokens.size(), true, true);
    if (n_tokens < 0) {
        throw std::runtime_error("Failed to tokenize prompt");
    }
    tokens.resize(n_tokens);
    
    // Clear the KV cache
    llama_kv_cache_clear(context->ctx);
    
    // Process the prompt
    for (size_t i = 0; i < tokens.size(); i += 512) {
        size_t batch_size = std::min(static_cast<size_t>(512), tokens.size() - i);
        if (llama_decode(context->ctx, llama_batch_get_one(&tokens[i], batch_size, i, 0))) {
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
        if (llama_token_is_eog(model->model, new_token)) {
            break;
        }
        
        // Convert token to text
        char token_str[256];
        int token_len = llama_token_to_piece(model->model, new_token, token_str, sizeof(token_str), 0, true);
        if (token_len > 0) {
            response.append(token_str, token_len);
        }
        
        // Process the new token
        if (llama_decode(context->ctx, llama_batch_get_one(&new_token, 1, tokens.size() + i, 0))) {
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