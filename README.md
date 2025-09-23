# μTRON Edge AI OCR

🎯 **Real-time OCR with tactile feedback using μTRON OS on STM32N6570-DK**

## 🌟 Project Overview

A μTRON OS competition project demonstrating edge AI capabilities with accessibility features. This system provides real-time text recognition with voice synthesis and tactile feedback for visually impaired users.

### Key Features
- 🧠 **Neural-ART NPU acceleration** for fast OCR inference
- 🔄 **Multi-task real-time system** using μTRON OS
- 🔊 **Voice synthesis** for audio feedback
- ✋ **Solenoid tactile feedback** for Morse code output
- 📷 **MIPI CSI-2 camera integration** with ISP
- ⚡ **Sub-20ms response time** guaranteed

## 🏗️ System Architecture

```
┌─────────────────────────────────────┐
│ Task 1: Camera Capture (20ms)       │ ← Real-time imaging
├─────────────────────────────────────┤
│ Task 2: AI Inference (Neural-ART)   │ ← NPU acceleration
├─────────────────────────────────────┤  
│ Task 3: Voice Synthesis + Output    │ ← Audio feedback
├─────────────────────────────────────┤
│ Task 4: Solenoid Control            │ ← Tactile feedback
├─────────────────────────────────────┤
│ Task 5: System Monitor + UI         │ ← Management
└─────────────────────────────────────┘
```

## 🔧 Hardware Requirements

- **STM32N6570-DK** Development Kit
- **Push Solenoids** (2x for Morse code, 4x for Braille optional)
- **MIPI CSI-2 Camera** (included in DK)
- **Audio output** (speaker/headphones)
- **microSD card** (for audio samples)

## 📊 Technical Specifications

| Component | Specification |
|-----------|---------------|
| **MCU** | STM32N657X0H3Q (800MHz Cortex-M55) |
| **NPU** | Neural-ART Accelerator (600 GOPS) |
| **RAM** | 4.2MB embedded + external PSRAM |
| **Flash** | External Octo-SPI (model storage) |
| **OS** | μTRON OS (real-time multitasking) |
| **AI Framework** | STM32Cube.AI + Neural-ART |

## 🚀 Getting Started

### Prerequisites
- STM32CubeIDE 1.15.0+
- STM32CubeMX with STM32CubeN6 package
- STM32Cube.AI 10.0+
- μTRON OS SDK

### Quick Start
```bash
git clone https://github.com/wwlapaki310/utron-edge-ai-ocr.git
cd utron-edge-ai-ocr
# Follow setup instructions in docs/setup.md
```

## 📁 Project Structure

```
utron-edge-ai-ocr/
├── docs/                    # Documentation
│   ├── setup.md            # Setup instructions
│   ├── architecture.md     # System architecture
│   └── api.md              # API documentation
├── src/                     # Source code
│   ├── tasks/              # μTRON OS task implementations
│   ├── ai/                 # AI model integration
│   ├── drivers/            # Hardware drivers
│   └── utils/              # Utility functions
├── models/                  # AI models
│   ├── ocr/                # OCR models
│   └── preprocessing/      # Image preprocessing
├── assets/                  # Audio samples and resources
├── hardware/               # Hardware schematics
├── tests/                  # Unit tests
└── examples/               # Usage examples
```

## 🎯 Development Roadmap

### Phase 1 (Week 1): Foundation
- [x] μTRON OS setup and basic task structure
- [ ] Hardware abstraction layer (HAL)
- [ ] Camera interface implementation
- [ ] Solenoid control driver

### Phase 2 (Week 2): AI Integration
- [ ] OCR model optimization for Neural-ART
- [ ] Real-time inference pipeline
- [ ] Memory management optimization
- [ ] Task synchronization

### Phase 3 (Week 3): User Interface
- [ ] Voice synthesis integration
- [ ] Morse code output system
- [ ] Error handling and recovery
- [ ] Performance optimization

## 🧪 Performance Targets

- **Latency**: < 20ms end-to-end
- **Accuracy**: > 95% for printed text
- **Power**: < 300mW active mode
- **Memory**: < 4MB runtime usage

## 🤝 Contributing

This is a competition project, but feedback and suggestions are welcome!

1. Fork the repository
2. Create a feature branch
3. Commit your changes
4. Push to the branch
5. Open a Pull Request

## 📄 License

MIT License - see [LICENSE](LICENSE) file for details.

## 📞 Contact

For questions about this μTRON OS competition project:
- Create an issue in this repository
- Technical discussions welcome!

---

**🏆 μTRON OS Competition Entry 2025**  
*Demonstrating real-time edge AI with accessibility focus*