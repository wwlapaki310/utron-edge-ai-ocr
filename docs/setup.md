# Setup Guide

## Development Environment Setup

### Required Software
1. **STM32CubeIDE 1.15.0+**
   - Download from STMicroelectronics website
   - Install with STM32CubeMX integration

2. **STM32CubeMX** with STM32CubeN6 package
   - Enable STM32N6 series support
   - Install latest firmware package

3. **STM32Cube.AI 10.0.1+**
   - Neural-ART NPU support required
   - Install via STM32CubeMX Additional Software

4. **μTRON OS SDK**
   - Download from official μTRON website
   - Configure for STM32N6 target

5. **Git**
   - For version control and repository cloning

### Hardware Setup

#### STM32N6570-DK Configuration
1. **Main Connection**
   ```
   PC ←[USB-C]→ STM32N6570-DK (CN6: STLINK-V3EC)
   ```

2. **Power Supply**
   ```
   External Power ←[USB-C]→ DK (CN8: USB1) [Optional for high power]
   ```

3. **Camera Module**
   - MIPI CSI-2 camera (included with DK)
   - 5MP CMOS sensor with ISP support

#### Solenoid Connection
```
STM32N6570-DK GPIO:
├── PA0 (SOLENOID_1) → Solenoid Driver → Solenoid #1
├── PA1 (SOLENOID_2) → Solenoid Driver → Solenoid #2
├── PA2 (Optional)   → Solenoid Driver → Solenoid #3
└── PA3 (Optional)   → Solenoid Driver → Solenoid #4
```

#### Audio Output
```
STM32N6570-DK:
├── 3.5mm Audio Jack → Headphones/Speaker
└── USB Audio (CN6) → PC Audio [Development]
```

#### Storage
```
STM32N6570-DK:
├── microSD Slot → Audio samples (WAV files)
└── External Flash → AI models, application code
```

### Software Installation

#### 1. STM32CubeIDE Setup
```bash
# Linux
sudo dpkg -i stm32cubeide_1.15.0_*.deb

# Windows
# Run installer as administrator
```

#### 2. μTRON OS SDK Installation
```bash
# Extract SDK
tar -xzf utron-sdk-stm32n6.tar.gz
cd utron-sdk-stm32n6

# Configure environment
export UTRON_SDK_PATH=$(pwd)
export PATH=$PATH:$UTRON_SDK_PATH/bin
```

#### 3. Project Clone and Build
```bash
# Clone repository
git clone https://github.com/wwlapaki310/utron-edge-ai-ocr.git
cd utron-edge-ai-ocr

# Initialize submodules (if any)
git submodule update --init --recursive

# Open in STM32CubeIDE
# File → Import → Existing Projects into Workspace
# Select utron-edge-ai-ocr directory
```

### Build Configuration

#### 1. Compiler Settings
```c
// In STM32CubeIDE Project Properties:
// C/C++ Build → Settings → Tool Settings

// MCU GCC Compiler → Optimization
// Optimization Level: -O2 (Optimize for speed)

// MCU GCC Compiler → Preprocessor
// Define symbols:
USE_HAL_DRIVER
STM32N657xx
UTRON_OS_ENABLE
AI_NPU_ENABLE
```

#### 2. Linker Configuration
```
// Memory Layout (STM32N657X0H3Q_FLASH.ld)
MEMORY
{
  FLASH (rx)    : ORIGIN = 0x70000000, LENGTH = 2048K
  RAM (rwx)     : ORIGIN = 0x24000000, LENGTH = 4352K
  AI_RAM (rwx)  : ORIGIN = 0x24440000, LENGTH = 2560K
}
```

#### 3. Debug Configuration
```
// Debug Configurations
// Debugger: ST-LINK (OpenOCD)
// Interface: SWD
// Target: STM32N657X0H3Q
// Reset Mode: Hardware reset
```

### Neural-ART NPU Setup

#### 1. Model Conversion
```bash
# Convert ONNX model to Neural-ART format
stm32ai convert --model ocr_model.onnx \
                --output ocr_model_nart.h \
                --target neural-art \
                --optimization 3 \
                --quantization int8
```

#### 2. Memory Configuration
```c
// In utron_config.h
#define AI_ACTIVATION_SIZE  (2500 * 1024)  // 2.5MB
#define AI_WEIGHTS_SIZE     (8 * 1024 * 1024)  // 8MB (external flash)
```

### Boot Configuration

#### 1. Boot Pins Setup
```
STM32N6570-DK Boot Switches:
├── Development Mode: BOOT1=1, BOOT2=1 (both right)
└── Flash Boot Mode:  BOOT1=0, BOOT2=0 (both left)
```

#### 2. Flashing Sequence
```bash
# 1. Set Development Boot Mode
# 2. Flash FSBL (First Stage Boot Loader)
# 3. Flash Application
# 4. Flash AI Model Weights
# 5. Set Flash Boot Mode
# 6. Reset board
```

### Verification Steps

#### 1. Hardware Check
```bash
# Connect via ST-LINK Utility
# Verify device detection: STM32N657X0H3Q
# Check memory mapping
# Test GPIO functionality
```

#### 2. μTRON OS Check
```c
// In system startup, verify:
// - Task creation successful
// - Semaphore initialization
// - Message queue creation
// - Timer functionality
```

#### 3. AI Model Check
```c
// Verify Neural-ART NPU:
// - Model loading successful
// - Inference pipeline working
// - Memory allocation correct
// - Performance meets targets
```

### Troubleshooting

#### Common Issues

1. **Boot Loop**
   ```
   Solution: Check boot pin configuration
   Verify external flash programming
   ```

2. **AI Model Load Failure**
   ```
   Solution: Verify model size < 8MB
   Check quantization format (int8)
   Validate Neural-ART compatibility
   ```

3. **Camera Not Detected**
   ```
   Solution: Check MIPI CSI-2 connections
   Verify ISP configuration
   Test camera module separately
   ```

4. **Audio Issues**
   ```
   Solution: Check codec initialization
   Verify audio file formats (16kHz, 16-bit)
   Test DAC output signals
   ```

5. **Real-time Performance**
   ```
   Solution: Optimize task priorities
   Adjust stack sizes
   Enable compiler optimizations
   Profile memory usage
   ```

### Performance Optimization

#### 1. Memory Optimization
```c
// Use memory pools for frequent allocations
// Optimize buffer sizes
// Enable cache for external memory access
```

#### 2. Real-time Tuning
```c
// Adjust task priorities based on deadlines
// Optimize synchronization mechanisms
// Minimize interrupt latency
```

#### 3. AI Optimization
```c
// Use Neural-ART optimized operators
// Optimize input preprocessing
// Pipeline inference with capture
```

---

For detailed API documentation, see [api.md](api.md)  
For system architecture details, see [architecture.md](architecture.md)