# LibreOffice Architecture Documentation

## Table of Contents
1. [Overview](#overview)
2. [Project Structure](#project-structure)
3. [Core Architecture](#core-architecture)
4. [Module Dependencies](#module-dependencies)
5. [Data Flows](#data-flows)
6. [Build System](#build-system)
7. [Key Technologies](#key-technologies)
8. [Platform Support](#platform-support)

## Overview

LibreOffice is a comprehensive, cross-platform office productivity suite that provides word processing (Writer), spreadsheet (Calc), presentation (Impress), drawing (Draw), database (Base), and formula editing (Math) applications. The codebase represents over 20 years of development, originating from StarOffice and evolving through OpenOffice.org.

### Key Characteristics
- **Modular Architecture**: Over 200 modules with clear separation of concerns
- **Cross-Platform**: Supports Windows, Linux, macOS, Android, iOS, and WebAssembly
- **Multi-Language**: Core in C++, with Java, Python, and BASIC scripting support
- **Extensible**: Plugin architecture with UNO (Universal Network Objects) framework
- **Standards-Based**: Native support for ODF (Open Document Format) and compatibility with Microsoft Office formats

## Project Structure

The LibreOffice codebase is organized into functional modules:

### Core System Modules
- **sal/** - System Abstraction Layer: Platform-independent OS abstractions
- **cppu/** - C++ UNO runtime: Core component model implementation
- **cppuhelper/** - UNO helper classes: Utilities for component development
- **comphelper/** - Component helpers: Common utilities for all modules

### UI and Graphics Modules
- **vcl/** - Visual Class Library: Cross-platform GUI toolkit abstraction
- **svx/** - Shared View eXtensions: Drawing layer and common UI components
- **basegfx/** - Basic graphics algorithms and primitives
- **drawinglayer/** - Advanced rendering and primitive decomposition
- **canvas/** - Modern canvas-based rendering framework

### Framework Modules
- **framework/** - UNO-based application framework: Menus, toolbars, dispatch
- **sfx2/** - StarView Framework 2: Legacy document/view/controller architecture
- **svtools/** - StarView Tools: Common controls and utilities
- **unotools/** - UNO tools: High-level UNO utilities

### Application Modules
- **sw/** - Writer: Word processor
- **sc/** - Calc: Spreadsheet  
- **sd/** - Draw/Impress: Vector graphics and presentations
- **starmath/** - Math: Formula editor
- **dbaccess/** - Base: Database frontend

### Filter and Import/Export
- **filter/** - Document format filters
- **oox/** - Office Open XML filters
- **writerfilter/** - DOCX import
- **writerperfect/** - Import filters for various formats

## Core Architecture

LibreOffice follows a layered architecture:

```
┌─────────────────────────────────────────────────┐
│         Applications (Writer, Calc, etc.)        │
├─────────────────────────────────────────────────┤
│    Frameworks (Framework, SFX2, SVX)            │
├─────────────────────────────────────────────────┤
│      UI Toolkit (VCL, Basegfx, Canvas)         │
├─────────────────────────────────────────────────┤
│        Component Model (UNO, CPPU)              │
├─────────────────────────────────────────────────┤
│    System Abstraction Layer (SAL, OSL)         │
├─────────────────────────────────────────────────┤
│        Operating System / Platform              │
└─────────────────────────────────────────────────┘
```

### Component Model (UNO)

UNO (Universal Network Objects) is the component model that enables:
- Language-independent component interfaces
- Remote procedure calls
- Service-based architecture
- Extension mechanisms

Key concepts:
- **Services**: Named component implementations
- **Interfaces**: Contract definitions (IDL-based)
- **Properties**: Named attributes with type information
- **Events**: Observer pattern implementation

### Document Model

Each application maintains its own document model:
- **Writer**: Node-based model with SwDoc, SwNodes
- **Calc**: Column-oriented storage with ScDocument
- **Impress/Draw**: Page-based with SdrModel

Common aspects:
- Undo/Redo via command pattern
- Style management
- Document properties and metadata

### Rendering Architecture

LibreOffice uses multiple rendering paths:

1. **Traditional VCL Path**
   - OutputDevice abstraction
   - Platform-specific backends (GDI+, Quartz, Cairo)
   - Immediate mode rendering

2. **Modern Canvas Path**
   - Retained mode graphics
   - Hardware acceleration support
   - Better animation support

3. **Primitive-Based Rendering**
   - Drawinglayer primitives
   - Decomposition for complex objects
   - Resolution-independent

## Module Dependencies

### Dependency Hierarchy

```
Applications (sw, sc, sd)
    ↓
Framework Layer (framework, sfx2, svx)
    ↓
UI/Graphics (vcl, basegfx, drawinglayer)
    ↓
Core Services (comphelper, unotools, svl)
    ↓
UNO Runtime (cppu, cppuhelper)
    ↓
System Abstraction (sal)
```

### Key Dependencies

1. **SAL** - No dependencies (base layer)
2. **VCL** - Depends on: sal, basegfx, comphelper
3. **SFX2** - Depends on: sal, vcl, svl, comphelper
4. **Framework** - Depends on: sal, vcl, comphelper, cppu
5. **SVX** - Depends on: sal, vcl, sfx2, basegfx, drawinglayer
6. **SW/SC/SD** - Depend on all lower layers

## Data Flows

### Document Loading

```
File → ImportFilter → DocumentModel → View → Screen
         ↓                ↓
    Format Detection   Styles/Content
                          ↓
                      Layout Engine
```

### Command Execution

```
User Input → VCL Event → Dispatch Framework → Shell/Controller
                              ↓
                         Command Handler
                              ↓
                        Document Model
                              ↓
                         View Update
```

### Rendering Pipeline

```
Document Model → Primitives → Processor → OutputDevice → Platform API
                    ↓            ↓
              Decomposition  Optimization
```

### Extension Loading

```
Extension Package → Package Manager → Component Registration
                          ↓
                    Service Manager
                          ↓
                    Runtime Loading
```

## Build System

LibreOffice uses **gbuild**, a GNU Make-based build system:

### Key Features
- **Modular builds**: Each module builds independently
- **Dependency tracking**: Automatic resolution of build order
- **Cross-compilation**: Support for different target platforms
- **External dependencies**: Managed through download.lst
- **Parallel builds**: Full support for make -j

### Build Components
- **Module_*.mk**: Module definition files
- **Library_*.mk**: Library build rules
- **Executable_*.mk**: Binary build rules
- **CppunitTest_*.mk**: Unit test definitions

### Configuration System
- **configure.ac**: Autoconf-based configuration
- **config_host.mk.in**: Build configuration template
- **distro-configs/**: Platform-specific configurations

## Key Technologies

### Programming Languages
- **C++**: Core implementation (C++17 standard)
- **Java**: Some filters, database drivers, extensions
- **Python**: Scripting, build tools, some extensions
- **BASIC**: Macro language (StarBasic)
- **Objective-C++**: macOS-specific code

### External Libraries
- **boost**: Utilities and data structures
- **ICU**: Unicode and localization
- **libxml2**: XML parsing
- **cairo**: 2D graphics (Linux/GTK)
- **freetype**: Font rendering
- **OpenSSL**: Cryptography

### File Formats
- **Native**: ODF (Open Document Format)
- **Microsoft**: DOC/DOCX, XLS/XLSX, PPT/PPTX
- **Others**: PDF, RTF, CSV, HTML, EPUB

## Platform Support

### Desktop Platforms
- **Windows**: 10 and later (GDI+/Direct2D rendering)
- **Linux**: GTK3/4, Qt5/6, KF5 backends
- **macOS**: 11 and later (Quartz rendering)

### Mobile Platforms
- **Android**: Via LibreOfficeKit
- **iOS**: Via LibreOfficeKit (limited)

### Web Platform
- **WebAssembly**: Experimental support
- **LibreOffice Online**: Server-based deployment

### Platform Abstraction
- VCL provides platform abstraction for:
  - Window management
  - Input handling
  - File dialogs
  - Clipboard
  - Printing
  - Font management

Each platform has specific backends in vcl/:
- **win/**: Windows implementation
- **osx/**: macOS implementation  
- **unx/**: Unix/Linux implementations
- **gtk3/**, **qt5/**: Toolkit-specific backends

## Development Workflow

### Code Organization
- Headers in module/inc/ or module/source/inc/
- Implementation in module/source/
- Tests in module/qa/

### Testing Infrastructure
- **Unit tests**: CppUnit-based
- **Integration tests**: UITests, Python tests
- **Performance tests**: PerfCheck framework

### Quality Assurance
- **Compiler plugins**: Custom Clang plugins for code analysis
- **Include-what-you-use**: Header dependency optimization
- **Coverity**: Static analysis
- **Address/UB Sanitizers**: Runtime checking

## Additional Documentation

This repository contains detailed documentation for specific subsystems and APIs:

### Architecture Documentation

- **[ARCHITECTURE-UNO.md](ARCHITECTURE-UNO.md)** - Universal Network Objects (UNO) component model
  - Core concepts, interfaces, services, and types
  - Binary UNO, language bindings, and bridges
  - Threading model and inter-process communication
  - Extension mechanisms and best practices

- **[ARCHITECTURE-VCL.md](ARCHITECTURE-VCL.md)** - Visual Class Library
  - Platform abstraction layer (SAL)
  - Window system and widget hierarchy
  - Rendering pipeline and graphics subsystems
  - Event handling and layout management

- **[ARCHITECTURE-WRITER.md](ARCHITECTURE-WRITER.md)** - Writer word processor
  - Document model and node system
  - Layout engine and text formatting
  - Tables, fields, and numbering
  - Redlining and change tracking

- **[ARCHITECTURE-CALC.md](ARCHITECTURE-CALC.md)** - Calc spreadsheet
  - Column-based storage with MDDS
  - Formula engine and interpreter
  - Shared formulas and performance features
  - Charts and external references

- **[ARCHITECTURE-DRAW-IMPRESS.md](ARCHITECTURE-DRAW-IMPRESS.md)** - Draw and Impress
  - Shared drawing layer architecture
  - Slide show engine and animations
  - 3D capabilities and media playback
  - Presenter console and views

- **[ARCHITECTURE-BUILD.md](ARCHITECTURE-BUILD.md)** - Build system (gbuild)
  - Build classes and target types
  - Platform abstraction and cross-compilation
  - Dependency management
  - Performance optimization

- **[ARCHITECTURE-FRAMEWORK.md](ARCHITECTURE-FRAMEWORK.md)** - Application frameworks
  - Frame-Controller-Model pattern
  - Dispatch framework and UI integration
  - SFX2 legacy framework
  - Docking and sidebar systems

- **[ARCHITECTURE-FILTERS.md](ARCHITECTURE-FILTERS.md)** - Import/Export filters
  - Type detection and filter factory
  - XML and binary format support
  - Filter chains and transformations
  - Storage handling

- **[ARCHITECTURE-DATABASE.md](ARCHITECTURE-DATABASE.md)** - Database connectivity (Base/SDBC)
  - SDBC API and driver architecture
  - Native and bridge drivers
  - Connection pooling and transactions
  - Query designer and forms integration

### API Documentation

- **[API-UNO.md](API-UNO.md)** - UNO API programming guide
  - Core services and document APIs
  - Dialog and forms programming
  - Event handling and configuration
  - Best practices and common patterns

- **[API-SCRIPTING.md](API-SCRIPTING.md)** - Scripting APIs
  - StarBasic/LibreOffice Basic
  - Python, JavaScript, and BeanShell scripting
  - Script provider framework
  - Security and deployment

- **[API-EXTENSIONS.md](API-EXTENSIONS.md)** - Extension development
  - Extension structure and packaging
  - UI integration (menus, toolbars)
  - Component implementation
  - Calc add-ins and protocol handlers

- **[API-LIBREOFFICEKIT.md](API-LIBREOFFICEKIT.md)** - LibreOfficeKit API
  - Document rendering and user input
  - Callback system and view management
  - Mobile platform integration
  - Performance optimization

---

This architecture documentation provides a high-level overview of LibreOffice's structure and design. For detailed information about specific subsystems, refer to the documentation files listed above. For module-specific details, see the README.md files in each module's directory.