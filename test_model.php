<?php
// Test actual model functionality

echo "=== Phllama Model Functionality Test ===\n\n";

try {
    echo "Creating Phllama instance with mistral model...\n";
    $phllama = new Phllama('mistral:latest');
    echo "✓ Model loaded successfully!\n";
    
    // Get model info
    echo "\nGetting model info...\n";
    $info = $phllama->getModelInfo();
    echo "Model info: " . $info . "\n";
    
    // Test message generation with a simple prompt
    echo "\nTesting message generation...\n";
    $response = $phllama->sendMessage("Hello! Please respond with just 'Hi there!'");
    echo "Response: " . $response . "\n";
    
    echo "\n✓ All tests completed successfully!\n";
    
} catch (Exception $e) {
    echo "Error: " . $e->getMessage() . "\n";
    echo "This might be expected if the model is too large for this test environment.\n";
}

echo "\n=== Test completed ===\n";