#ifndef LLAMA_INTERFACE_H
#define LLAMA_INTERFACE_H

#include <string>
#include <vector>
#include <memory>

// GPU Configuration options
enum class GPUMode {
    CPU_ONLY = 0,
    SINGLE_GPU = 1,
    DUAL_GPU = 2,
    AUTO = -1
};

struct HardwareConfig {
    GPUMode gpu_mode = GPUMode::AUTO;
    int gpu_layers = -1;  // -1 = auto-detect
    int main_gpu = 0;     // Primary GPU for single GPU mode
    bool use_mmap = true;
    bool use_mlock = false;
    int cpu_threads = -1; // -1 = auto-detect
    bool tensor_split_enabled = false;
    std::vector<float> tensor_split;
};

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
    HardwareConfig hardware_config;
    
public:
    LlamaInterface();
    ~LlamaInterface();
    
    bool loadModel(const std::string& path);
    bool loadModel(const std::string& path, const HardwareConfig& config);
    std::string generate(const std::string& prompt, int max_tokens = 512);
    void setTemperature(float temperature);
    void setTopP(float top_p);
    void setTopK(int top_k);
    void clearCache(); // Clear KV cache for memory management
    
    // Hardware configuration methods
    void setHardwareConfig(const HardwareConfig& config);
    HardwareConfig getHardwareConfig() const;
    static int detectGPUCount();
    static std::vector<std::string> getGPUInfo();
    static int getOptimalCPUThreads();
    static HardwareConfig detectOptimalConfig();
    
    // Performance monitoring
    struct PerformanceStats {
        double tokens_per_second = 0.0;
        size_t memory_usage_mb = 0;
        size_t vram_usage_mb = 0;
        int active_gpus = 0;
        double cpu_usage_percent = 0.0;
    };
    PerformanceStats getPerformanceStats() const;
};

#endif