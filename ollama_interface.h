#ifndef OLLAMA_INTERFACE_H
#define OLLAMA_INTERFACE_H

#include <string>
#include <vector>

class OllamaInterface {
public:
    static bool isModelAvailable(const std::string& model_name);
    static std::string downloadModel(const std::string& model_name);
    static std::string getModelPath(const std::string& model_name);
    static std::vector<int> tokenize(const std::string& text, const std::string& model_path);
    static std::string detokenize(const std::vector<int>& tokens, const std::string& model_path);
    
    // Configuration methods
    static void setModelsDirectory(const std::string& directory);
    static std::string getModelsDirectory();
    
private:
    static std::string models_directory;
};

#endif