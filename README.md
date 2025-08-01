# LibreOffice
[![Coverity Scan Build Status](https://scan.coverity.com/projects/211/badge.svg)](https://scan.coverity.com/projects/211) [![CII Best Practices](https://bestpractices.coreinfrastructure.org/projects/307/badge)](https://bestpractices.coreinfrastructure.org/projects/307) [![Translation status](https://weblate.documentfoundation.org/widgets/libo_ui-master/-/svg-badge.svg)](https://weblate.documentfoundation.org/engage/libo_ui-master/?utm_source=widget)

<img align="right" width="150" src="https://opensource.org/wp-content/uploads/2009/06/OSIApproved.svg">

LibreOffice is an integrated office suite based on copyleft licenses
and compatible with most document formats and standards. Libreoffice
is backed by The Document Foundation, which represents a large
independent community of enterprises, developers and other volunteers
moved by the common goal of bringing to the market the best software
for personal productivity. LibreOffice is open source, and free to
download, use and distribute.

A quick overview of the LibreOffice code structure.

## Overview

You can develop for LibreOffice in one of two ways, one
recommended and one much less so. First the somewhat less recommended
way: it is possible to use the SDK to develop an extension,
for which you can read the [API docs](https://api.libreoffice.org/)
and [Developers Guide](https://wiki.documentfoundation.org/Documentation/DevGuide).
This re-uses the (extremely generic) UNO APIs that are also used by
macro scripting in StarBasic.

The best way to add a generally useful feature to LibreOffice
is to work on the code base however. Overall this way makes it easier
to compile and build your code, it avoids any arbitrary limitations of
our scripting APIs, and in general is far more simple and intuitive -
if you are a reasonably able C++ programmer.

## The Build Chain and Runtime Baselines

These are the current minimal operating system and compiler versions to
run and compile LibreOffice, also used by the TDF builds:

* Windows:
    * Runtime: Windows 10
    * Build: Cygwin + Visual Studio 2019 version 16.10
* macOS:
    * Runtime: 11
    * Build: 13 or later + Xcode 14.3 or later (using latest version available for a given version of macOS)
* Linux:
    * Runtime: RHEL 8 or CentOS 8 and comparable
    * Build: either GCC 12; or Clang 12 with libstdc++ 10
* iOS (only for LibreOfficeKit):
    * Runtime: 14.5 (only support for newer i devices == 64 bit)
    * Build: Xcode 12.5 and iPhone SDK 14.5
* Android:
    * Build: NDK 27 and SDK 30.0.3
* Emscripten / WASM:
    * Runtime: a browser with SharedMemory support (threads + atomics)
    * Build: Qt 5.15 with Qt supported Emscripten 1.39.8
    * See [README.wasm](static/README.wasm.md)

Java is required for building many parts of LibreOffice. In TDF Wiki article
[Development/Java](https://wiki.documentfoundation.org/Development/Java), the
exact modules that depend on Java are listed.

The baseline for Java is Java Development Kit (JDK) Version 17 or later.

The baseline for Python is version 3.11. It follows the version available
in SUSE Linux Enterprise Desktop and the Maintenance Support version of
Red Hat Enterprise Linux.

If you want to use Clang with the LibreOffice compiler plugins, the minimal
version of Clang is 12.0.1. Since Xcode doesn't provide the compiler plugin
headers, you have to compile your own Clang to use them on macOS.

You can find the TDF configure switches in the `distro-configs/` directory.

To setup your initial build environment on Windows and macOS, we provide
the LibreOffice Development Environment
([LODE](https://wiki.documentfoundation.org/Development/lode)) scripts.

For more information see the build instructions for your platform in the
[TDF wiki](https://wiki.documentfoundation.org/Development/How_to_build).

## Environment Variables

LibreOffice supports several environment variables for optional features:

### Google Docs Integration
To enable Google Docs integration, set your Google API credentials:

```bash
# Linux/macOS
export GOOGLE_API_CLIENT_ID="your-client-id.apps.googleusercontent.com"
export GOOGLE_API_CLIENT_SECRET="your-client-secret"

# Windows Command Prompt
set GOOGLE_API_CLIENT_ID=your-client-id.apps.googleusercontent.com
set GOOGLE_API_CLIENT_SECRET=your-client-secret

# Windows PowerShell
$env:GOOGLE_API_CLIENT_ID="your-client-id.apps.googleusercontent.com"
$env:GOOGLE_API_CLIENT_SECRET="your-client-secret"
```

To obtain these credentials:
1. Go to [Google Cloud Console](https://console.cloud.google.com/)
2. Create a new project or select existing one
3. Enable Google Drive API
4. Create OAuth 2.0 credentials (Desktop application type)
5. Download the credentials and use the client ID and secret

### OpenAI Whisper Integration
To enable speech-to-text functionality with OpenAI Whisper:

```bash
# Linux/macOS
export OPENAI_API_KEY="sk-your-api-key-here"

# Windows Command Prompt
set OPENAI_API_KEY=sk-your-api-key-here

# Windows PowerShell
$env:OPENAI_API_KEY="sk-your-api-key-here"
```

To get an API key:
1. Sign up at [OpenAI Platform](https://platform.openai.com/)
2. Navigate to API keys section
3. Create a new secret key

### Making Environment Variables Permanent

**Linux/macOS:**
Add the export commands to your shell configuration file:
- Bash: `~/.bashrc` or `~/.bash_profile`
- Zsh: `~/.zshrc`

**Windows:**
Use System Properties → Environment Variables to add them permanently, or use:
```cmd
setx GOOGLE_API_CLIENT_ID "your-client-id.apps.googleusercontent.com"
setx GOOGLE_API_CLIENT_SECRET "your-client-secret"
setx OPENAI_API_KEY "sk-your-api-key-here"
```

## The Important Bits of Code

Each module should have a `README.md` file inside it which has some
degree of documentation for that module; patches are most welcome to
improve those. We have those turned into a web page here:

<https://docs.libreoffice.org/>

However, there are two hundred modules, many of them of only
peripheral interest for a specialist audience. So - where is the
good stuff, the code that is most useful. Here is a quick overview of
the most important ones:

Module    | Description
----------|-------------------------------------------------
[sal/](sal)             | this provides a simple System Abstraction Layer
[tools/](tools)         | this provides basic internal types: `Rectangle`, `Color` etc.
[vcl/](vcl)             | this is the widget toolkit library and one rendering abstraction
[framework/](framework) | UNO framework, responsible for building toolbars, menus, status bars, and the chrome around the document using widgets from VCL, and XML descriptions from `/uiconfig/` files
[sfx2/](sfx2)           | legacy core framework used by Writer/Calc/Draw: document model / load/save / signals for actions etc.
[svx/](svx)             | drawing model related helper code, including much of Draw/Impress

Then applications

Module    | Description
----------|-------------------------------------------------
[desktop/](desktop)  | this is where the `main()` for the application lives, init / bootstrap. the name dates back to an ancient StarOffice that also drew a desktop
[sw/](sw/)           | Writer
[sc/](sc/)           | Calc
[sd/](sd/)           | Draw / Impress

There are several other libraries that are helpful from a graphical perspective:

Module    | Description
----------|-------------------------------------------------
[basegfx/](basegfx)  | algorithms and data-types for graphics as used in the canvas
[canvas/](canvas)   | new (UNO) canvas rendering model with various backends
[cppcanvas/](cppcanvas) | C++ helper classes for using the UNO canvas
[drawinglayer/](drawinglayer) | View code to render drawable objects and break them down into primitives we can render more easily.

## Rules for #include Directives (C/C++)

Use the `"..."` form if and only if the included file is found next to the
including file. Otherwise, use the `<...>` form. (For further details, see the
mail [Re: C[++]: Normalizing include syntax ("" vs
<>)](https://lists.freedesktop.org/archives/libreoffice/2017-November/078778.html).)

The UNO API include files should consistently use double quotes, for the
benefit of external users of this API.

`loplugin:includeform (compilerplugins/clang/includeform.cxx)` enforces these rules.


## Finding Out More

Beyond this, you can read the `README.md` files, send us patches, ask
on the mailing list libreoffice@lists.freedesktop.org (no subscription
required) or poke people on IRC `#libreoffice-dev` on irc.libera.chat -
we're a friendly and generally helpful mob. We know the code can be
hard to get into at first, and so there are no silly questions.

## SAST Tools

[PVS-Studio](https://pvs-studio.com/en/pvs-studio/?utm_source=website&utm_medium=github&utm_campaign=open_source) - static analyzer for C, C++, C#, and Java code.
