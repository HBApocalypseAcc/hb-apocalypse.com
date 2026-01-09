# Helbreath Apocalypse Server - Referencia Rápida

## Comandos Rápidos

### Docker

```bash
# Iniciar el servidor
docker-compose up -d

# Ver logs
docker-compose logs -f

# Detener el servidor
docker-compose down

# Reconstruir después de cambios
docker-compose up -d --build

# Ver estado
docker-compose ps
```

### Sin Docker

```bash
# Iniciar Login Server
cd server/LoginServer
./LoginServer

# Iniciar Game Server (en otra terminal)
cd server/HGServer
./HGServer
```

## Estructura de Archivos

```
hb-apocalypse.com/
├── config/
│   ├── GServer.cfg          # Configuración del servidor de juego
│   └── LServer.cfg          # Configuración del servidor de login
├── docs/
│   ├── CONFIGURATION.md     # Guía de configuración detallada
│   └── INSTALLATION.md      # Guía de instalación completa
├── server/
│   ├── HGServer/           # Ejecutables y datos del servidor de juego
│   └── LoginServer/        # Ejecutables del servidor de login
├── Dockerfile              # Configuración de Docker
├── docker-compose.yml      # Orquestación de servicios
├── start.sh               # Script de inicio
└── README.md              # Este archivo
```

## Puertos

| Servicio        | Puerto | Uso                           |
|-----------------|--------|-------------------------------|
| Login Server    | 2848   | Autenticación de jugadores    |
| Game Server     | 1234   | Juego principal               |

## Archivos de Configuración Importantes

### config/GServer.cfg
- `MaxPlayers`: Jugadores simultáneos
- `ExperienceRate`: Multiplicador de experiencia
- `DropRate`: Multiplicador de drop
- `GoldRate`: Multiplicador de oro

### config/LServer.cfg
- `MaxConnections`: Conexiones simultáneas
- `ConnectionTimeout`: Tiempo de espera
- `GameServers`: Lista de servidores de juego

## Logs

Los logs se encuentran en:
- `logs/gameserver.log` - Servidor de juego
- `logs/loginserver.log` - Servidor de login
- `logs/access.log` - Accesos

## Solución Rápida de Problemas

### No se puede conectar
```bash
# Verificar puertos
netstat -tulpn | grep -E '1234|2848'

# Verificar firewall
sudo ufw status
```

### Servidor no inicia
```bash
# Ver logs
docker-compose logs

# Verificar permisos
ls -la server/*/
```

### Resetear configuración
```bash
# Restaurar archivos de configuración por defecto
git checkout config/
```

## Recursos Adicionales

- [Documentación completa](docs/)
- [Guía de instalación](docs/INSTALLATION.md)
- [Configuración avanzada](docs/CONFIGURATION.md)
- [Cómo contribuir](CONTRIBUTING.md)

## Soporte

- Abrir un issue en GitHub
- Consultar la documentación en `docs/`
- Revisar los logs en `logs/`
