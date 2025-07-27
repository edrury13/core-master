#!/bin/bash
# Bash script to run LibreOffice with GUI from Docker container

set -e

CONTAINER_NAME="libreoffice-dev"
PROGRAM="soffice"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

echo -e "${CYAN}LibreOffice GUI Launcher${NC}"
echo -e "${CYAN}========================${NC}"

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --writer)
            PROGRAM="swriter"
            shift
            ;;
        --calc)
            PROGRAM="scalc"
            shift
            ;;
        --impress)
            PROGRAM="simpress"
            shift
            ;;
        --draw)
            PROGRAM="sdraw"
            shift
            ;;
        --base)
            PROGRAM="sbase"
            shift
            ;;
        --math)
            PROGRAM="smath"
            shift
            ;;
        *)
            echo "Unknown option: $1"
            echo "Usage: $0 [--writer|--calc|--impress|--draw|--base|--math]"
            exit 1
            ;;
    esac
done

# Check if container is running
if ! docker ps --filter "name=$CONTAINER_NAME" --format "{{.Names}}" 2>/dev/null | grep -q "^$CONTAINER_NAME$"; then
    echo -e "${YELLOW}Container '$CONTAINER_NAME' is not running. Starting it...${NC}"
    docker start $CONTAINER_NAME
    sleep 3
fi

echo -e "\n${YELLOW}Setting up display...${NC}"

# Detect platform and set display accordingly
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    # Linux
    export DISPLAY=${DISPLAY:-:0}
    DOCKER_DISPLAY=$DISPLAY
    XSOCK=/tmp/.X11-unix
    XAUTH=/tmp/.docker.xauth
    
    # Create xauth file
    touch $XAUTH
    xauth nlist $DISPLAY | sed -e 's/^..../ffff/' | xauth -f $XAUTH nmerge -
    
    echo -e "${GREEN}Linux detected. Using DISPLAY=$DISPLAY${NC}"
    
    DOCKER_RUN_OPTS="-v $XSOCK:$XSOCK -v $XAUTH:$XAUTH -e XAUTHORITY=$XAUTH"
    
elif [[ "$OSTYPE" == "darwin"* ]]; then
    # macOS
    if ! pgrep -x "XQuartz" > /dev/null; then
        echo -e "${RED}XQuartz is not running!${NC}"
        echo -e "${YELLOW}Please install and run XQuartz:${NC}"
        echo "  brew install --cask xquartz"
        echo "  open -a XQuartz"
        echo "  In XQuartz preferences, enable 'Allow connections from network clients'"
        exit 1
    fi
    
    IP=$(ifconfig en0 | grep inet | awk '$1=="inet" {print $2}')
    export DISPLAY=$IP:0
    DOCKER_DISPLAY=$DISPLAY
    echo -e "${GREEN}macOS detected. Using DISPLAY=$DISPLAY${NC}"
    
    DOCKER_RUN_OPTS=""
    
elif [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "cygwin" ]] || [[ "$OSTYPE" == "win32" ]]; then
    # Windows
    echo -e "${YELLOW}Windows detected. Make sure your X server is running.${NC}"
    
    # Get host IP for WSL2
    HOST_IP=$(docker exec $CONTAINER_NAME bash -c "grep nameserver /etc/resolv.conf | cut -d' ' -f2" 2>/dev/null | tr -d '\r\n')
    if [ -z "$HOST_IP" ]; then
        HOST_IP=$(hostname -I | awk '{print $1}')
    fi
    
    export DISPLAY=$HOST_IP:0.0
    DOCKER_DISPLAY=$DISPLAY
    echo -e "${GREEN}Using DISPLAY=$DISPLAY${NC}"
    
    DOCKER_RUN_OPTS=""
fi

# Build the docker exec command
PROGRAM_PATH="/build/instdir/program/$PROGRAM"

echo -e "\n${YELLOW}Starting LibreOffice...${NC}"
echo -e "Program: $PROGRAM_PATH"

# Run LibreOffice
docker exec -it \
    -e DISPLAY=$DOCKER_DISPLAY \
    -e LIBGL_ALWAYS_INDIRECT=1 \
    -e LIBGL_ALWAYS_SOFTWARE=1 \
    -e SAL_USE_VCLPLUGIN=gen \
    -e GDK_BACKEND=x11 \
    $DOCKER_RUN_OPTS \
    $CONTAINER_NAME \
    bash -c "
        if [ -f $PROGRAM_PATH ]; then
            $PROGRAM_PATH
        else
            echo 'LibreOffice not found. Has the build completed?'
            echo 'Run: ./build.sh --full-build'
            exit 1
        fi
    "

if [ $? -ne 0 ]; then
    echo -e "\n${YELLOW}Troubleshooting tips:${NC}"
    echo "1. Make sure X server is running and allows connections"
    echo "2. Check firewall settings - X server uses port 6000"
    echo "3. For VcXsrv on Windows: Run with 'Disable access control' checked"
    echo "4. Check container logs: docker logs $CONTAINER_NAME"
fi