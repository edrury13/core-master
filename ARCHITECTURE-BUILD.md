# LibreOffice Build System (gbuild) Architecture

## Overview

LibreOffice uses a custom GNU Make-based build system called gbuild. It provides a declarative interface for defining build targets, handles cross-platform compilation, manages dependencies, and supports various build configurations. The system is designed to build a massive codebase efficiently across multiple platforms.

## Core Architecture

### Build System Layers

```
configure.ac (Autoconf)
        ↓
config_host.mk (Configuration)
        ↓
Makefile.gbuild (Core gbuild)
        ↓
Module makefiles (Module_*.mk)
        ↓
Target makefiles (Library_*.mk, etc.)
        ↓
Platform-specific rules
        ↓
Compiler/Linker invocation
```

### Directory Structure

```
solenv/gbuild/          # Core gbuild implementation
    ├── gbuild.mk       # Main entry point
    ├── Library.mk      # Library class definition
    ├── Executable.mk   # Executable class definition
    ├── Module.mk       # Module class definition
    ├── platform/       # Platform-specific rules
    │   ├── linux.mk
    │   ├── windows.mk
    │   └── macos.mk
    └── extensions/     # Optional features

<module>/
    ├── Module_<module>.mk      # Module definition
    ├── Library_<name>.mk       # Library definitions
    ├── Executable_<name>.mk    # Executable definitions
    └── CppunitTest_<name>.mk   # Unit test definitions
```

## Build Classes

### Module Class

Defines a build module and its targets:

```makefile
# Module_vcl.mk
$(eval $(call gb_Module_Module,vcl))

$(eval $(call gb_Module_add_targets,vcl,\
    Library_vcl \
    Library_vclplug_gtk3 \
    $(if $(ENABLE_QT5),Library_vclplug_qt5) \
))

$(eval $(call gb_Module_add_check_targets,vcl,\
    CppunitTest_vcl_lifecycle \
    CppunitTest_vcl_bitmap_test \
))
```

### Library Class

Defines shared/static libraries:

```makefile
# Library_vcl.mk
$(eval $(call gb_Library_Library,vcl))

$(eval $(call gb_Library_set_include,vcl,\
    $$(INCLUDE) \
    -I$(SRCDIR)/vcl/inc \
))

$(eval $(call gb_Library_use_libraries,vcl,\
    basegfx \
    comphelper \
    sal \
))

$(eval $(call gb_Library_add_exception_objects,vcl,\
    vcl/source/app/svapp \
    vcl/source/window/window \
))
```

### Executable Class

Defines executable programs:

```makefile
# Executable_soffice_bin.mk
$(eval $(call gb_Executable_Executable,soffice_bin))

$(eval $(call gb_Executable_use_libraries,soffice_bin,\
    sal \
    sofficeapp \
))

$(eval $(call gb_Executable_add_exception_objects,soffice_bin,\
    desktop/source/app/main \
))
```

## Dependency Management

### Internal Dependencies

gbuild automatically tracks dependencies:

```makefile
# Declare library dependencies
$(eval $(call gb_Library_use_libraries,mylib,\
    sal \
    vcl \
    sfx2 \
))

# Declare API dependencies
$(eval $(call gb_Library_use_sdk_api,mylib))

# Use internal headers
$(eval $(call gb_Library_use_package,mylib,\
    mymodule_inc \
))
```

### External Dependencies

External libraries via `RepositoryExternal.mk`:

```makefile
# Define external library
define gb_ExternalProject__use_libxml2
$(call gb_ExternalProject_use_package,$(1),libxml2)
endef

# Use in module
$(eval $(call gb_Library_use_externals,mylib,\
    libxml2 \
    boost_headers \
))
```

### Dependency Resolution

gbuild resolves dependencies automatically:
1. Topological sort of modules
2. Prerequisite tracking
3. Parallel build support
4. Incremental rebuilds

## Platform Abstraction

### Platform Detection

```makefile
# platform/linux.mk
gb_COMPILERDEFS := -DLINUX -DUNIX
gb_CFLAGS := -fPIC -Wreturn-type
gb_CXXFLAGS := -fPIC -std=c++17

# platform/windows.mk  
gb_COMPILERDEFS := -DWIN32 -DWNT
gb_CFLAGS := /EHsc /MD
gb_CXXFLAGS := /EHsc /MD /std:c++17
```

### Compiler Abstraction

```makefile
# Compiler commands
gb_CC := $(CC)
gb_CXX := $(CXX)
gb_LINK := $(CXX)

# Compiler flags
gb_CFLAGS_COMMON := $(CFLAGS)
gb_CXXFLAGS_COMMON := $(CXXFLAGS)
gb_LDFLAGS := $(LDFLAGS)
```

### Cross-Compilation

Support for cross-compilation:

```makefile
# Host vs Build
gb_Executable_EXT_for_build := $(gb_Executable_EXT)
gb_Library_DLLEXT_for_build := $(gb_Library_DLLEXT)

# Custom tools for build platform
$(call gb_ExternalExecutable_set_precommand,python,\
    $(gb_Helper_set_ld_path) \
)
```

## Target Types

### Primary Targets

1. **Library**: Shared/static libraries (.so, .dll, .a)
2. **Executable**: Binary executables
3. **Module**: Logical grouping of targets
4. **Package**: File installation/deployment
5. **ExternalProject**: Third-party projects
6. **CustomTarget**: Custom build rules

### Test Targets

```makefile
# Unit test
$(eval $(call gb_CppunitTest_CppunitTest,sc_ucalc))

# UI test
$(eval $(call gb_UITest_UITest,sc_options))

# Python test
$(eval $(call gb_PythonTest_PythonTest,sc_python))

# JUnit test
$(eval $(call gb_JunitTest_JunitTest,sc_complex))
```

### Documentation Targets

```makefile
# Doxygen documentation
$(eval $(call gb_CustomTarget_CustomTarget,docs/cpp))

# Help compilation
$(eval $(call gb_AllLangHelp_AllLangHelp,scalc))
```

## Build Process

### Configuration Phase

1. **autogen.sh**: Generate configure script
2. **configure**: Detect system, set options
3. **config_host.mk**: Generated configuration

Example configure options:
```bash
./configure \
    --enable-debug \
    --enable-dbgutil \
    --with-system-libs \
    --with-lang="en-US de fr"
```

### Compilation Phase

```makefile
# Object compilation
$(call gb_CxxObject_get_target,%) : $(call gb_CxxObject_get_source,%)
    $(call gb_CxxObject__command,$@,$*,$<,$(DEFS),$(T_CXXFLAGS))

# Linking
$(call gb_Library_get_target,%) :
    $(call gb_Library__command,$@,$*)
```

### Build Flow

```
make
  ↓
Read Makefile.gbuild
  ↓
Include all Module_*.mk
  ↓
Build dependency graph
  ↓
Execute targets in parallel
  ↓
Link final products
```

## Advanced Features

### Precompiled Headers

```makefile
# Enable PCH for module
$(eval $(call gb_Library_set_precompiled_header,vcl,\
    vcl/inc/pch/precompiled_vcl \
))

# Define PCH content
// precompiled_vcl.hxx
#include <sal/config.h>
#include <vcl/dllapi.h>
#include <memory>
#include <vector>
```

### Code Generation

```makefile
# Custom code generation
$(eval $(call gb_CustomTarget_CustomTarget,vcl/unx/gtk3))

$(call gb_CustomTarget_get_target,vcl/unx/gtk3) : \
    $(call gb_CustomTarget_get_workdir,vcl/unx/gtk3)/generated.cxx

$(call gb_CustomTarget_get_workdir,vcl/unx/gtk3)/generated.cxx :
    $(call gb_Helper_execute,codegen) > $@
```

### Localization

```makefile
# Localization targets
$(eval $(call gb_AllLangMoTarget_AllLangMoTarget,vcl))

# UI translation
$(eval $(call gb_UIConfig_UIConfig,vcl))
$(eval $(call gb_UIConfig_add_uifiles,vcl,\
    vcl/uiconfig/ui/aboutdialog \
))
```

### Packaging

```makefile
# Installation package
$(eval $(call gb_Package_Package,vcl_opengl,$(SRCDIR)/vcl/opengl))

$(eval $(call gb_Package_add_files,vcl_opengl,$(LIBO_ETC_FOLDER)/opengl,\
    fragment_shaders.glsl \
    vertex_shaders.glsl \
))
```

## Build Optimization

### Parallelization

```makefile
# Parallel job support
PARALLELISM := $(shell $(GNUMAKE) -j 2>&1 | \
    $(GREP) "\\-j" | $(SED) "s/.*\\-j *\\([0-9]*\\).*/\\1/")

# Module dependencies for parallel builds
$(call gb_Module_add_moduledirs,libreoffice,\
    sal \
    $(call gb_Helper_optional,DESKTOP,desktop) \
)
```

### Incremental Builds

gbuild tracks:
- Source file timestamps
- Header dependencies
- Build flags changes
- External library updates

### ccache Integration

```makefile
# Enable ccache if available
ifeq ($(USE_CCACHE),TRUE)
gb_CC := ccache $(gb_CC)
gb_CXX := ccache $(gb_CXX)
endif
```

### Distributed Building

Support for icecream/distcc:
```makefile
# Icecream setup
export ICECC_VERSION := $(WORKDIR)/lo-icecream.tar.gz
gb_CC := icecc $(gb_CC)
gb_CXX := icecc $(gb_CXX)
```

## Debugging Builds

### Debug Options

```makefile
# Debug/development build
ifeq ($(ENABLE_DEBUG),TRUE)
gb_CFLAGS += -g
gb_CXXFLAGS += -g
gb_LDFLAGS += -g
endif

# Verbose output
$(eval $(call gb_Module_VERBOSEBUILD,vcl))
```

### Build Analysis

```bash
# Dependency analysis
make vcl.showdeliverables

# Build timing
make TIMELOG=1

# Verbose mode
make VERBOSE=t

# Specific target
make Library_vcl
```

### Common Issues

1. **Missing dependencies**: Check use_libraries/use_externals
2. **Link errors**: Verify symbol visibility
3. **Header not found**: Check include paths
4. **Parallel build failures**: Add order dependencies

## Extension Points

### Custom Build Rules

```makefile
# Define custom class
gb_MyClass_MyClass = $(call gb_MyClass__init,$(1))

define gb_MyClass__init
$(call gb_MyClass_get_target,$(1)) : MYVAR := value
$(call gb_MyClass_get_target,$(1)) : $(call gb_MyClass_get_source,$(1))
    $$(call gb_Output_announce,$(1),$(true),MYC,1)
    $$(call gb_Helper_run,mytool) $$< > $$@
endef
```

### Build Hooks

```makefile
# Pre/post build hooks
define gb_Module__post_build
    @echo "Build completed for $(1)"
endef

# Clean hooks
define gb_Module__clean_post
    rm -rf $(WORKDIR)/cache/$(1)
endef
```

## Performance Tips

### Best Practices

1. **Use object libraries**: Share common code
2. **Minimize dependencies**: Only what's needed
3. **Precompiled headers**: For large modules
4. **Static libraries**: For build-only tools

### Optimization Flags

```makefile
# Release optimization
gb_COMPILEROPTFLAGS := -O2
gb_COMPILEROPT1FLAGS := -O1

# Link-time optimization
ifeq ($(ENABLE_LTO),TRUE)
gb_LTOFLAGS := -flto=$(PARALLELISM)
endif
```

## Future Enhancements

### Planned Improvements

1. **Meson/CMake evaluation**: Modern build systems
2. **Better caching**: Improved incremental builds
3. **Cloud builds**: Distributed compilation
4. **Build reproducibility**: Deterministic outputs
5. **Faster configuration**: Reduced configure time

### Ongoing Work

- Module dependency optimization
- Build parallelization improvements
- Cross-compilation enhancements
- Developer experience improvements

---

This documentation covers gbuild's architecture and usage. For specific module builds, consult the module's .mk files.