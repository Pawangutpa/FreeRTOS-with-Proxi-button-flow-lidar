FROM python:3.10-slim

# Install system dependencies
RUN apt-get update && apt-get install -y \
    git \
    build-essential \
    wget \
    && rm -rf /var/lib/apt/lists/*

# Install PlatformIO
RUN pip install platformio

# Create workspace
WORKDIR /workspace

# Default command
CMD ["bash"]