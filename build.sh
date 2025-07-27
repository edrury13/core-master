#!/bin/bash
# Bash script to manage LibreOffice Docker build

set -e

CONTAINER_NAME="libreoffice-dev"
IMAGE_NAME="libreoffice-builder"
SOURCE_PATH="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

echo -e "${CYAN}LibreOffice Docker Build Script${NC}"
echo -e "${CYAN}===============================${NC}"

# Parse arguments
CLEAN=false
FULL_BUILD=false
RUN_AFTER_BUILD=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --clean)
            CLEAN=true
            shift
            ;;
        --full-build)
            FULL_BUILD=true
            shift
            ;;
        --run)
            RUN_AFTER_BUILD=true
            shift
            ;;
        *)
            echo "Unknown option: $1"
            echo "Usage: $0 [--clean] [--full-build] [--run]"
            exit 1
            ;;
    esac
done

# Function to check if container exists
container_exists() {
    docker ps -a --filter "name=$CONTAINER_NAME" --format "{{.Names}}" 2>/dev/null | grep -q "^$CONTAINER_NAME$"
}

# Function to check if container is running
container_running() {
    docker ps --filter "name=$CONTAINER_NAME" --format "{{.Names}}" 2>/dev/null | grep -q "^$CONTAINER_NAME$"
}

# Check if container exists
if container_exists; then
    if container_running; then
        echo -e "${GREEN}Container '$CONTAINER_NAME' is already running.${NC}"
    else
        echo -e "${YELLOW}Starting existing container '$CONTAINER_NAME'...${NC}"
        docker start $CONTAINER_NAME
        sleep 2
    fi
else
    echo -e "${YELLOW}Container '$CONTAINER_NAME' not found. Creating from docker-compose...${NC}"
    docker-compose up -d
    sleep 5
fi

# Wait for container to be ready
echo -e "${YELLOW}Waiting for container to be ready...${NC}"
MAX_ATTEMPTS=30
ATTEMPT=0
while [ $ATTEMPT -lt $MAX_ATTEMPTS ]; do
    if docker exec $CONTAINER_NAME bash -c "echo 'ready'" 2>/dev/null | grep -q "ready"; then
        echo -e "${GREEN}Container is ready!${NC}"
        break
    fi
    ATTEMPT=$((ATTEMPT + 1))
    sleep 1
done

if [ $ATTEMPT -eq $MAX_ATTEMPTS ]; then
    echo -e "${RED}Container failed to become ready. Check docker logs.${NC}"
    exit 1
fi

# Sync source code
echo -e "\n${YELLOW}Syncing source code to container...${NC}"
EXCLUDES=(
    "--exclude=.git"
    "--exclude=workdir"
    "--exclude=instdir"
    "--exclude=autom4te.cache"
    "--exclude=*.o"
    "--exclude=*.lo"
    "--exclude=*.la"
    "--exclude=.deps"
    "--exclude=.libs"
)

docker exec $CONTAINER_NAME bash -c "rsync -av --delete ${EXCLUDES[*]} /source/ /build/"

if [ $? -ne 0 ]; then
    echo -e "${RED}Failed to sync source code!${NC}"
    exit 1
fi

# Build LibreOffice
if [ "$CLEAN" = true ]; then
    echo -e "\n${YELLOW}Performing clean build...${NC}"
    docker exec $CONTAINER_NAME bash -c "cd /build && make clean" 2>/dev/null || true
fi

if [ "$FULL_BUILD" = true ] || ! docker exec $CONTAINER_NAME bash -c "test -f /build/instdir/program/soffice" 2>/dev/null; then
    echo -e "\n${YELLOW}Running full build (this will take a while)...${NC}"
    docker exec $CONTAINER_NAME bash -c "cd /build && ./autogen.sh --without-java --without-help --without-myspell-dicts --with-parallelism=4 && make -j4"
else
    echo -e "\n${YELLOW}Running incremental build...${NC}"
    docker exec $CONTAINER_NAME bash -c "cd /build && make -j4"
fi

if [ $? -ne 0 ]; then
    echo -e "${RED}Build failed! Check the logs with: docker logs $CONTAINER_NAME${NC}"
    exit 1
fi

echo -e "\n${GREEN}Build completed successfully!${NC}"
echo -e "${CYAN}LibreOffice binary location: /build/instdir/program/soffice${NC}"

if [ "$RUN_AFTER_BUILD" = true ]; then
    echo -e "\n${YELLOW}Starting LibreOffice...${NC}"
    ./run-libreoffice-gui.sh
fi

echo -e "\n${CYAN}Useful commands:${NC}"
echo -e "  Check logs:        docker logs -f $CONTAINER_NAME"
echo -e "  Enter container:   docker exec -it $CONTAINER_NAME bash"
echo -e "  Run LibreOffice:   ./run-libreoffice-gui.sh"