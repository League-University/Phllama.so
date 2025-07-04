# Phllama.so - PHP Extension for Llama Models

A PHP extension that integrates ollama's model management with ollama's patched llama.cpp for enhanced stability and performance.

## Features

- Automatic model downloading via ollama
- Direct GGUF file loading
- Fast inference using ollama's enhanced llama.cpp
- Production-tested stability patches
- Enhanced CUDA operations
- Zero runtime overhead after model loading
- Simple PHP API

## Dependencies

- PHP 8.x with development headers
- PHP-CPP library
- ollama (for initial model download only)
- C++17 compatible compiler
- CMake

## Installation

### Prerequisites

```bash
# Install system dependencies
sudo apt-get update
sudo apt-get install php-dev cmake build-essential git

# Install PHP-CPP
git clone https://github.com/CopernicaMarketingSoftware/PHP-CPP.git
cd PHP-CPP
make && sudo make install

# Install ollama (for model downloads)
curl -fsSL https://ollama.com/install.sh | sh
```

### Build the Extension

```bash
# Use the automated build script
./build.sh

# Or build manually:
git submodule update --init --recursive
make
sudo make install
```

### Enable the Extension

Add to your PHP configuration:
```ini
extension=phllama.so
```

Or copy the provided ini file:
```bash
sudo cp phllama.ini /etc/php/8.2/mods-available/
sudo phpenmod phllama
```

## Usage

```php
<?php

// Using ollama model name (downloads automatically on first use)
$agent = new Phllama('phi4:latest');
$agent->setTemperature(0.7);
$response = $agent->sendMessage('Hello world!');
echo $response;

// Using direct GGUF file path
$agent = new Phllama('/path/to/model.gguf');
$response = $agent->sendMessage('How are you?');
echo $response;

?>
```

## Methods

- `__construct(string $model)` - Initialize with ollama model name or GGUF file path
- `sendMessage(string $message)` - Generate response using ollama's llama.cpp
- `setTemperature(float $temp)` - Set sampling temperature
- `setTopP(float $top_p)` - Set top-p sampling parameter

## Architecture

- **Ollama's llama.cpp**: Enhanced inference engine with production patches
  - Memory management fixes
  - Better KV cache defragmentation
  - Enhanced CUDA operations
  - Cross-compiler compatibility
- **Ollama integration**: Model download and path discovery (fallback only)
- **PHP-CPP**: Extension framework

## Performance Benefits

- **Zero runtime overhead**: Direct C++ function calls, no CLI processes
- **Enhanced stability**: Production-tested patches from ollama
- **Unified codebase**: Single source of truth for both model management and inference
