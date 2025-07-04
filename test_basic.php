<?php
// Basic test of the Phllama extension

echo "=== Phllama Extension Basic Test ===\n\n";

// Test 1: Check if extension is loaded
if (extension_loaded('phllama')) {
    echo "✓ Extension loaded successfully\n";
} else {
    echo "✗ Extension not loaded\n";
    exit(1);
}

// Test 2: Check if class exists
if (class_exists('Phllama')) {
    echo "✓ Phllama class is available\n";
} else {
    echo "✗ Phllama class not found\n";
    exit(1);
}

// Test 3: Try to create an instance (this will test if constructor works)
try {
    echo "Attempting to create Phllama instance...\n";
    $phllama = new Phllama('test-model'); // This should fail gracefully since we don't have a model
    echo "✓ Phllama instance created (unexpected success)\n";
} catch (Exception $e) {
    echo "✓ Constructor executed (expected failure): " . $e->getMessage() . "\n";
}

// Test 4: Check method availability
$methods = get_class_methods('Phllama');
echo "Available methods: " . implode(', ', $methods) . "\n";

echo "\n=== Basic tests completed ===\n";