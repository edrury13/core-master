# Dockerfile - Ubuntu-based LibreOffice build environment with VNC GUI support
FROM ubuntu:22.04

# Prevent interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=UTC

# Install VNC server, desktop environment, and basic build tools
RUN apt-get update && apt-get install -y \
    software-properties-common \
    && add-apt-repository ppa:ubuntu-toolchain-r/test -y \
    && apt-get update && apt-get install -y \
    # Basic tools
    gcc-12 \
    g++-12 \
    build-essential \
    git \
    autoconf \
    automake \
    libtool \
    pkg-config \
    ccache \
    # Python and Java
    python3-dev \
    python3-pip \
    openjdk-11-jdk \
    ant \
    # Build dependencies
    bison \
    flex \
    gperf \
    nasm \
    libxml2-utils \
    xsltproc \
    # Development libraries
    libx11-dev \
    libxext-dev \
    libxrender-dev \
    libxrandr-dev \
    libxtst-dev \
    libgl1-mesa-dev \
    libglu1-mesa-dev \
    libgtk-3-dev \
    libcups2-dev \
    libfontconfig1-dev \
    libxinerama-dev \
    libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev \
    # More dependencies
    libcairo2-dev \
    libkrb5-dev \
    libnss3-dev \
    libxml2-dev \
    libxslt1-dev \
    libpython3-dev \
    libboost-dev \
    libhunspell-dev \
    libhyphen-dev \
    libmythes-dev \
    liblpsolve55-dev \
    libcppunit-dev \
    libclucene-dev \
    libexpat1-dev \
    libmysqlclient-dev \
    libpq-dev \
    firebird-dev \
    libcurl4-openssl-dev \
    libssl-dev \
    # VNC and Desktop Environment
    tigervnc-standalone-server \
    tigervnc-viewer \
    xfce4 \
    xfce4-goodies \
    xfce4-terminal \
    novnc \
    websockify \
    supervisor \
    dbus-x11 \
    # Utilities
    wget \
    curl \
    zip \
    unzip \
    sudo \
    locales \
    nano \
    vim \
    htop \
    x11-apps \
    firefox \
    && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-12 100 \
    && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-12 100 \
    && rm -rf /var/lib/apt/lists/*

# Generate locale
RUN locale-gen en_US.UTF-8
ENV LANG=en_US.UTF-8
ENV LANGUAGE=en_US:en
ENV LC_ALL=en_US.UTF-8

# Set up ccache
RUN mkdir -p /ccache && chmod 777 /ccache
ENV CCACHE_DIR=/ccache
ENV CCACHE_MAXSIZE=5G
ENV PATH="/usr/lib/ccache:${PATH}"

# Create build user
RUN useradd -m -s /bin/bash builder && \
    echo "builder ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers && \
    usermod -aG sudo builder

# Set Java environment
ENV JAVA_HOME=/usr/lib/jvm/java-11-openjdk-amd64

# Create directories for persistent storage and VNC
RUN mkdir -p /build-artifacts && \
    chown builder:builder /build-artifacts && \
    mkdir -p /home/builder/.vnc && \
    chown -R builder:builder /home/builder/.vnc

# Set up VNC configuration
USER builder
WORKDIR /home/builder

# Configure VNC password and startup
RUN echo "libreoffice" | vncpasswd -f > /home/builder/.vnc/passwd && \
    chmod 600 /home/builder/.vnc/passwd && \
    echo '#!/bin/bash\nxfce4-session &' > /home/builder/.vnc/xstartup && \
    chmod +x /home/builder/.vnc/xstartup

# Set up desktop shortcuts and menu items
RUN mkdir -p /home/builder/Desktop /home/builder/.config/xfce4/desktop && \
    echo '[Desktop Entry]\nType=Application\nName=LibreOffice Writer\nComment=Build and run LibreOffice Writer\nExec=/home/builder/run-libreoffice.sh writer\nIcon=libreoffice-writer\nTerminal=false\nCategories=Office;' > /home/builder/Desktop/LibreOffice-Writer.desktop && \
    echo '[Desktop Entry]\nType=Application\nName=LibreOffice Calc\nComment=Build and run LibreOffice Calc\nExec=/home/builder/run-libreoffice.sh calc\nIcon=libreoffice-calc\nTerminal=false\nCategories=Office;' > /home/builder/Desktop/LibreOffice-Calc.desktop && \
    echo '[Desktop Entry]\nType=Application\nName=Build LibreOffice\nComment=Configure and build LibreOffice from source\nExec=/home/builder/build-libreoffice.sh\nIcon=applications-development\nTerminal=true\nCategories=Development;' > /home/builder/Desktop/Build-LibreOffice.desktop && \
    chmod +x /home/builder/Desktop/*.desktop

# Environment variables for VNC and build
ENV DISPLAY=:1
ENV VNC_RESOLUTION=1920x1080
ENV VNC_PASSWORD=libreoffice
ENV PARALLELISM=4

WORKDIR /core

# Copy startup and utility scripts
COPY --chown=builder:builder supervisord.conf /home/builder/supervisord.conf
COPY --chown=builder:builder build-libreoffice.sh /home/builder/build-libreoffice.sh
COPY --chown=builder:builder run-libreoffice.sh /home/builder/run-libreoffice.sh
COPY --chown=builder:builder start-vnc.sh /home/builder/start-vnc.sh

RUN chmod +x /home/builder/*.sh

# Expose VNC and noVNC ports
EXPOSE 5901 6080

# Entry point script
CMD ["/home/builder/start-vnc.sh"]