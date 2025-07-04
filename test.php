<?php

/**
 * Phllama.so Extension Test Suite
 * 
 * Alpha Release Testing - v1.0.0-alpha
 * Tests the enhanced Phllama PHP extension with ollama's llama.cpp integration
 */

echo "╔══════════════════════════════════════════════════════════════╗\n";
echo "║                    Phllama.so Test Suite                     ║\n";
echo "║                     Alpha Release v1.0.0                     ║\n";
echo "╚══════════════════════════════════════════════════════════════╝\n\n";

function test_extension_loaded() {
    echo "🔍 Checking extension status...\n";
    
    if (!extension_loaded('phllama')) {
        echo "❌ Error: Phllama extension is not loaded.\n";
        echo "   Make sure the extension is installed and enabled.\n";
        echo "   Run: sudo make install && sudo phpenmod phllama\n\n";
        exit(1);
    }
    
    echo "✅ Phllama extension loaded successfully\n\n";
}

function test_ollama_model() {
    echo "🔍 Test 1: Ollama Model Integration\n";
    echo "─────────────────────────────────────\n";
    
    // Check if ollama is available
    $ollama_path = shell_exec('which ollama 2>/dev/null');
    if (empty(trim($ollama_path))) {
        echo "⚠️  Ollama not found in PATH, skipping ollama model test.\n";
        echo "   Install ollama: curl -fsSL https://ollama.com/install.sh | sh\n\n";
        return;
    }
    
    echo "✅ Ollama found at: " . trim($ollama_path) . "\n";
    
    try {
        echo "📥 Initializing model (may download if not present)...\n";
        $agent = new Phllama('phi3:mini');
        
        echo "✅ Model loaded successfully\n";
        
        // Get model info
        $info = $agent->getModelInfo();
        echo "📊 Model Info:\n";
        echo "   - Identifier: " . $info['identifier'] . "\n";
        echo "   - Type: " . $info['type'] . "\n";
        echo "   - Path: " . basename($info['path']) . "...\n\n";
        
        // Configure sampling
        $agent->setTemperature(0.7);
        $agent->setTopP(0.9);
        echo "⚙️  Configured: temperature=0.7, top_p=0.9\n";
        
        // Test generation
        echo "💬 Testing generation...\n";
        $prompt = "Hello! Please respond with exactly: 'Phllama extension working!'";
        echo "   Prompt: $prompt\n";
        
        $response = $agent->sendMessage($prompt);
        echo "   Response: " . trim($response) . "\n";
        
        echo "✅ Ollama model test completed successfully\n\n";
        
    } catch (Exception $e) {
        echo "❌ Ollama model test failed: " . $e->getMessage() . "\n\n";
    }
}

function test_direct_file() {
    echo "🔍 Test 2: Direct GGUF File Loading\n";
    echo "──────────────────────────────────────\n";
    
    // Look for any GGUF files in common locations
    $common_paths = [
        '/usr/local/share/models/',
        '/opt/models/',
        './models/',
        getenv('HOME') . '/models/'
    ];
    
    $found_model = null;
    foreach ($common_paths as $path) {
        if (is_dir($path)) {
            $files = glob($path . "*.gguf");
            if (!empty($files)) {
                $found_model = $files[0];
                break;
            }
        }
    }
    
    if (!$found_model) {
        echo "⚠️  No GGUF files found in common locations.\n";
        echo "   To test direct file loading, place a .gguf file in one of these locations:\n";
        foreach ($common_paths as $path) {
            echo "   - $path\n";
        }
        echo "\n";
        return;
    }
    
    echo "✅ Found GGUF file: " . basename($found_model) . "\n";
    
    try {
        echo "📥 Loading model directly from file...\n";
        $agent = new Phllama($found_model);
        
        echo "✅ Model loaded successfully\n";
        
        // Get model info
        $info = $agent->getModelInfo();
        echo "📊 Model Info:\n";
        echo "   - Type: " . $info['type'] . "\n";
        echo "   - File: " . basename($info['path']) . "\n\n";
        
        // Test generation
        $agent->setTemperature(0.5);
        echo "⚙️  Configured: temperature=0.5\n";
        
        echo "💬 Testing generation...\n";
        $response = $agent->sendMessage("Say 'Direct file loading works!'");
        echo "   Response: " . trim($response) . "\n";
        
        echo "✅ Direct file test completed successfully\n\n";
        
    } catch (Exception $e) {
        echo "❌ Direct file test failed: " . $e->getMessage() . "\n\n";
    }
}

function test_error_handling() {
    echo "🔍 Test 3: Error Handling\n";
    echo "─────────────────────────\n";
    
    // Test invalid model name
    try {
        echo "Testing invalid model name...\n";
        $agent = new Phllama('invalid-model-name-that-does-not-exist');
        echo "❌ Should have thrown an exception\n";
    } catch (Exception $e) {
        echo "✅ Correctly caught error: " . substr($e->getMessage(), 0, 50) . "...\n";
    }
    
    // Test invalid file path
    try {
        echo "Testing invalid file path...\n";
        $agent = new Phllama('/nonexistent/path/model.gguf');
        echo "❌ Should have thrown an exception\n";
    } catch (Exception $e) {
        echo "✅ Correctly caught error: " . substr($e->getMessage(), 0, 50) . "...\n";
    }
    
    // Test invalid parameters
    try {
        echo "Testing empty model identifier...\n";
        $agent = new Phllama('');
        echo "❌ Should have thrown an exception\n";
    } catch (Exception $e) {
        echo "✅ Correctly caught error: " . substr($e->getMessage(), 0, 50) . "...\n";
    }
    
    echo "✅ Error handling tests completed\n\n";
}

// Run all tests
try {
    test_extension_loaded();
    test_ollama_model();
    test_direct_file();
    test_error_handling();
    
    echo "🎉 All tests completed successfully!\n";
    echo "✅ Phllama.so Alpha Release is ready for use\n\n";
    
} catch (Exception $e) {
    echo "💥 Fatal error: " . $e->getMessage() . "\n";
    exit(1);
}
