# Phllama.so - PHP Extension for Llama Models

A PHP extension that combines ollama.cpp for model management and llama.cpp for fast inference.

## Features

- Automatic model downloading via ollama
- Direct file loading
- Fast inference using llama.cpp
- Simple PHP API

## Dependencies

- PHP 8.x with development headers
- PHP-CPP library
- ollama (for model management)
- llama.cpp (included as submodule)
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

# Install ollama
curl -fsSL https://ollama.ai/install.sh | sh
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

// Using ollama model name (downloads automatically)
$agent = new Phllama('phi4:latest');
$agent->setTemperature(0.7);
$response = $agent->sendMessage('Hello world!');
echo $response;

// Using direct file path
$agent = new Phllama('/path/to/model.gguf');
$response = $agent->sendMessage('How are you?');
echo $response;

?>
```

## Methods

- `__construct(string $model)` - Initialize with ollama model name or model file path
- `sendMessage(string $message)` - Generate response using llama.cpp
- `setTemperature(float $temp)` - Set sampling temperature
- `setTopP(float $top_p)` - Set top-p sampling parameter

## Architecture

- **ollama.cpp**: Model download, management, and tokenization
- **llama.cpp**: Fast inference engine
- **PHP-CPP**: PHP extension framework

## Notes

This is a basic implementation. For production use, you'll need to:

1. Integrate actual ollama.cpp and llama.cpp libraries
2. Implement proper model loading and inference
3. Add comprehensive error handling
4. Optimize memory usage and performance
5. Add more sampling parameters and configuration options