<?php

// Test script for Phllama extension

echo "Phllama.so Extension Test\n";
echo "========================\n\n";

if (!extension_loaded('phllama')) {
    echo "Error: Phllama extension is not loaded.\n";
    echo "Make sure the extension is installed and enabled.\n";
    exit(1);
}

try {
// Test with ollama model name (if available)
    echo "Test 1: Testing with ollama model name...\n";
    
// Check if ollama is available
    $output = shell_exec('which ollama 2>/dev/null');
    if (empty($output)) {
        echo "Ollama not found, skipping ollama model test.\n\n";
    } else {
        try {
        // Use a smaller model for testing
            $agent1 = new Phllama('phi3:latest');
            $agent1->setTemperature(0.7);
            $agent1->setTopP(0.9);
            $response1 = $agent1->sendMessage('Hello! If you are correctly functioning, please respond with just "Hi there!"');
            echo "Response: " . $response1 . "\n\n";
        } catch (Exception $e) {
            echo "Ollama model test failed: " . $e->getMessage() . "\n\n";
        }
    }

    // Test with direct file path (example path)
    echo "Test 2: Testing with direct model file path...\n";
    
    // This will fail unless you have an actual model file
    $test_path = '/path/to/your/model.gguf';
    echo "Note: Update the path '$test_path' to point to an actual model file.\n";

    if (file_exists($test_path)) {
        try {
            $agent2 = new Phllama($test_path);
            $agent2->setTemperature(0.5);
            $response2 = $agent2->sendMessage('How are you?');
            echo "Response: " . $response2 . "\n\n";
        } catch (Exception $e) {
            echo "Direct file test failed: " . $e->getMessage() . "\n\n";
        }
    } else {
        echo "Test file not found, skipping direct file test.\n\n";
    }

    echo "Testing completed.\n";
} catch (Exception $e) {
    echo "Error: " . $e->getMessage() . "\n";
}
