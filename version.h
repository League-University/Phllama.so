#ifndef PHLLAMA_VERSION_H
#define PHLLAMA_VERSION_H

/**
 * Phllama.so Version Information
 * 
 * High-performance PHP extension for AI model inference
 * using ollama's enhanced llama.cpp with production patches
 */

#define PHLLAMA_VERSION_MAJOR 1
#define PHLLAMA_VERSION_MINOR 0
#define PHLLAMA_VERSION_PATCH 0
#define PHLLAMA_VERSION_SUFFIX "alpha"

#define PHLLAMA_VERSION_STRING "1.0.0-alpha"
#define PHLLAMA_BUILD_DATE __DATE__
#define PHLLAMA_BUILD_TIME __TIME__

// Feature flags for alpha release
#define PHLLAMA_FEATURE_OLLAMA_INTEGRATION 1
#define PHLLAMA_FEATURE_DIRECT_GGUF_LOADING 1
#define PHLLAMA_FEATURE_MODEL_CACHING 1
#define PHLLAMA_FEATURE_GPU_ACCELERATION 0  // Coming in beta
#define PHLLAMA_FEATURE_MULTI_MODEL 0       // Coming in beta
#define PHLLAMA_FEATURE_STREAMING 0         // Coming in beta

// Architecture info
#define PHLLAMA_USES_OLLAMA_LLAMA_CPP 1
#define PHLLAMA_USES_ENHANCED_PATCHES 1
#define PHLLAMA_ZERO_RUNTIME_OVERHEAD 1

#endif // PHLLAMA_VERSION_H