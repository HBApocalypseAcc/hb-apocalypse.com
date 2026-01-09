FROM ubuntu:22.04

# Set working directory
WORKDIR /opt/helbreath

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    git \
    wget \
    unzip \
    && rm -rf /var/lib/apt/lists/*

# Create necessary directories
RUN mkdir -p /opt/helbreath/server/HGServer \
    && mkdir -p /opt/helbreath/server/LoginServer \
    && mkdir -p /opt/helbreath/config \
    && mkdir -p /opt/helbreath/logs

# Copy configuration files
COPY config/ /opt/helbreath/config/

# Copy start script
COPY start.sh /opt/helbreath/start.sh
RUN chmod +x /opt/helbreath/start.sh

# Expose ports
# 2848 - Login Server
# 1234 - Game Server
EXPOSE 2848 1234

# Run start script
CMD ["/opt/helbreath/start.sh"]
