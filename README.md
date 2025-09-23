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

## 📚 Documentation

### Core Documentation
- 📖 **[Project Overview](docs/project-overview.md)** - Comprehensive project explanation and social impact
- 🏛️ **[System Architecture](docs/architecture.md)** - Detailed system design and component relationships
- 🔧 **[Technical Stack](docs/technical-stack.md)** - Technology choices and implementation details
- 💡 **[Implementation Guide](docs/implementation-guide.md)** - Best practices and optimization techniques
- ⚙️ **[Setup Instructions](docs/setup.md)** - Development environment configuration

### Quick Navigation
| Document | Purpose | Target Audience |
|----------|---------|-----------------|
| [Project Overview](docs/project-overview.md) | Project goals, social impact, technology explanation | Everyone |
| [System Architecture](docs/architecture.md) | High-level system design and data flow | Engineers, System Architects |
| [Technical Stack](docs/technical-stack.md) | Technology decisions and detailed specifications | Developers, Technical Leads |
| [Implementation Guide](docs/implementation-guide.md) | Coding best practices and optimization tips | Software Engineers |
| [Setup Instructions](docs/setup.md) | Environment setup and build instructions | Developers |

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
# Follow detailed setup instructions in docs/setup.md
```

## 📁 Project Structure

```
utron-edge-ai-ocr/
├── docs/                    # Documentation
│   ├── setup.md            # Setup instructions
│   ├── architecture.md     # System architecture  
│   ├── technical-stack.md  # Technology stack details
│   ├── implementation-guide.md # Best practices
│   └── project-overview.md # Project explanation
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
- [x] System architecture documentation
- [x] Technical stack definition
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

## 🌟 Key Innovations

### Real-time Edge AI
- **Sub-20ms response time** using Neural-ART NPU acceleration
- **Offline operation** with no cloud dependency
- **Optimized memory usage** for edge deployment

### Accessibility Focus
- **Multi-modal feedback** - Audio + Tactile (Morse code)
- **Universal design** for visually impaired users
- **Customizable output** speed and intensity

### μTRON OS Integration
- **Deterministic real-time** scheduling
- **Task-based architecture** with strict timing guarantees
- **Resource-efficient** multitasking

## 🤝 Contributing

This is a competition project, but feedback and suggestions are welcome!

1. Fork the repository
2. Create a feature branch
3. Commit your changes
4. Push to the branch
5. Open a Pull Request

### Areas for Contribution
- 🔍 **AI Model Optimization** - Improve OCR accuracy and speed
- 🎵 **Audio Enhancement** - Better voice synthesis quality
- 🔧 **Hardware Integration** - Additional sensor support
- 📖 **Documentation** - Translations and improvements
- 🧪 **Testing** - User experience testing and validation

## 📄 License

MIT License - see [LICENSE](LICENSE) file for details.

## 📞 Contact

For questions about this μTRON OS competition project:
- Create an issue in this repository
- Technical discussions welcome!
- Read our [Project Overview](docs/project-overview.md) for detailed information

---

**🏆 μTRON OS Competition Entry 2025**  
*Demonstrating real-time edge AI with accessibility focus*

**"見える"を"聞こえる""感じる"に変える技術** - *Technology that transforms "seeing" into "hearing" and "feeling"*
