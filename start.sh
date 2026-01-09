#!/bin/bash

# Script de inicio para Helbreath Apocalypse Server
# Start script for Helbreath Apocalypse Server

set -e

echo "==================================="
echo "Helbreath Apocalypse Server"
echo "==================================="
echo ""

# Verificar que los directorios necesarios existan
echo "Verificando estructura de directorios..."
mkdir -p /opt/helbreath/logs
mkdir -p /opt/helbreath/data
mkdir -p /opt/helbreath/server/HGServer
mkdir -p /opt/helbreath/server/LoginServer

# Verificar archivos de configuración
echo "Verificando archivos de configuración..."
if [ ! -f "/opt/helbreath/config/GServer.cfg" ]; then
    echo "ADVERTENCIA: No se encontró GServer.cfg"
fi

if [ ! -f "/opt/helbreath/config/LServer.cfg" ]; then
    echo "ADVERTENCIA: No se encontró LServer.cfg"
fi

# Verificar ejecutables del servidor
echo "Verificando ejecutables del servidor..."
GAME_SERVER="/opt/helbreath/server/HGServer/HGServer"
LOGIN_SERVER="/opt/helbreath/server/LoginServer/LoginServer"

if [ ! -f "$LOGIN_SERVER" ]; then
    echo "ADVERTENCIA: No se encontró el ejecutable del Login Server"
    echo "Por favor, coloque los binarios del servidor en las carpetas correspondientes"
fi

if [ ! -f "$GAME_SERVER" ]; then
    echo "ADVERTENCIA: No se encontró el ejecutable del Game Server"
    echo "Por favor, coloque los binarios del servidor en las carpetas correspondientes"
fi

echo ""
echo "Configuración:"
echo "- Login Server Port: 2848"
echo "- Game Server Port: 1234"
echo ""

# Función para manejar señales de terminación
cleanup() {
    echo ""
    echo "Deteniendo servidores..."
    # Aquí se agregarían comandos para detener los servidores
    exit 0
}

trap cleanup SIGTERM SIGINT

echo "==================================="
echo "Servidor listo para iniciar"
echo "==================================="
echo ""
echo "NOTA: Para iniciar el servidor, necesita los archivos binarios"
echo "      de Helbreath en las carpetas server/HGServer y server/LoginServer"
echo ""
echo "Consulte la documentación en docs/INSTALLATION.md para más información"
echo ""

# Mantener el contenedor corriendo
tail -f /dev/null
