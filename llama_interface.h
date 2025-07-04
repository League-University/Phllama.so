#ifndef LLAMA_INTERFACE_H
#define LLAMA_INTERFACE_H

#include <string>
#include <vector>
#include <memory>

struct LlamaContext;
struct LlamaModel;

class LlamaInterface {
private:
    std::unique_ptr<LlamaModel> model;
    std::unique_ptr<LlamaContext> context;
    std::string model_path;
    float temperature = 0.7f;
    float top_p = 0.9f;
    int top_k = 40;
    
public:
    LlamaInterface();
    ~LlamaInterface();
    
    bool loadModel(const std::string& path);
    std::string generate(const std::string& prompt, int max_tokens = 512);
    void setTemperature(float temperature);
    void setTopP(float top_p);
    void setTopK(int top_k);
    void clearCache(); // Clear KV cache for memory management
};

#endif