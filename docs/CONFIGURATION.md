# Configuración del Servidor Helbreath Apocalypse

## Archivos de Configuración

### GServer.cfg - Servidor de Juego

Ubicación: `config/GServer.cfg`

#### Configuraciones Principales

**[Server]**
- `ServerName`: Nombre del servidor que aparecerá en el listado
- `ServerPort`: Puerto TCP para conexiones de clientes (predeterminado: 1234)
- `ServerIP`: IP de escucha (usar 0.0.0.0 para todas las interfaces)
- `MaxPlayers`: Número máximo de jugadores simultáneos

**[LoginServer]**
- `LoginServerIP`: IP del servidor de login (127.0.0.1 si está en la misma máquina)
- `LoginServerPort`: Puerto del servidor de login (predeterminado: 2848)

**[Game]**
- `ExperienceRate`: Multiplicador de experiencia (1.0 = normal, 2.0 = doble)
- `DropRate`: Multiplicador de drop de items (1.0 = normal)
- `GoldRate`: Multiplicador de oro (1.0 = normal)
- `PvPEnabled`: Habilitar/deshabilitar PvP (true/false)
- `GameMode`: Modo de juego (Normal, PvP, PvE, etc.)

**[Database]**
- `DBHost`: Host de la base de datos
- `DBPort`: Puerto de la base de datos (MySQL: 3306)
- `DBName`: Nombre de la base de datos del juego
- `DBUser`: Usuario de la base de datos
- `DBPassword`: Contraseña de la base de datos

### LServer.cfg - Servidor de Login

Ubicación: `config/LServer.cfg`

#### Configuraciones Principales

**[Server]**
- `ServerName`: Nombre del servidor de login
- `ServerPort`: Puerto de escucha (predeterminado: 2848)
- `ServerIP`: IP de escucha (0.0.0.0 para todas las interfaces)
- `ConnectionTimeout`: Tiempo máximo de inactividad (segundos)
- `MaxConnections`: Número máximo de conexiones simultáneas

**[GameServers]**
Lista de servidores de juego disponibles:
```
Server1=Nombre|IP|Puerto|MaxPlayers
Server2=Nombre|IP|Puerto|MaxPlayers
```

**[Database]**
- `DBHost`: Host de la base de datos
- `DBPort`: Puerto de la base de datos
- `DBName`: Nombre de la base de datos de login
- `DBUser`: Usuario de la base de datos
- `DBPassword`: Contraseña de la base de datos

**[Security]**
- `EnableEncryption`: Habilitar encriptación de comunicación
- `AllowMultipleLogins`: Permitir múltiples logins simultáneos
- `BannedIPList`: Ruta al archivo de IPs bloqueadas

## Ajustes Recomendados

### Para Servidor de Pruebas
```ini
MaxPlayers=50
ExperienceRate=5.0
DropRate=3.0
GoldRate=2.0
```

### Para Servidor de Producción
```ini
MaxPlayers=500
ExperienceRate=1.0
DropRate=1.0
GoldRate=1.0
```

### Para Servidor PvP
```ini
PvPEnabled=true
GameMode=PvP
ExperienceRate=2.0
DropRate=1.5
```

## Variables de Entorno Docker

Al usar Docker Compose, puede sobrescribir configuraciones usando variables de entorno:

```yaml
environment:
  - SERVER_NAME=Mi Servidor
  - MAX_PLAYERS=500
  - EXP_RATE=2.0
```

## Seguridad

### Recomendaciones
1. Cambiar contraseñas predeterminadas de la base de datos
2. Usar encriptación para comunicación cliente-servidor
3. Mantener actualizada la lista de IPs bloqueadas
4. Configurar firewall para limitar acceso a puertos administrativos
5. Realizar backups regulares de la base de datos

### Firewall (UFW en Ubuntu)
```bash
sudo ufw allow 1234/tcp  # Game Server
sudo ufw allow 2848/tcp  # Login Server
sudo ufw enable
```

## Optimización de Rendimiento

### Base de Datos
- Usar índices en tablas de jugadores y cuentas
- Configurar pool de conexiones adecuado
- Realizar mantenimiento regular (OPTIMIZE TABLE)

### Servidor
- Ajustar `MaxPlayers` según recursos del servidor
- Monitorear uso de CPU y memoria
- Configurar logs rotativos para evitar llenar disco

## Logging

Los logs se almacenan en:
- `logs/gameserver.log` - Logs del servidor de juego
- `logs/loginserver.log` - Logs del servidor de login
- `logs/access.log` - Log de accesos

### Niveles de Log
- ERROR: Solo errores críticos
- WARN: Advertencias y errores
- INFO: Información general (recomendado)
- DEBUG: Información detallada para depuración

## Copias de Seguridad

### Script de Backup Automático

```bash
#!/bin/bash
# backup.sh
DATE=$(date +%Y%m%d_%H%M%S)
mysqldump -u hbuser -p helbreath > backups/helbreath_$DATE.sql
mysqldump -u hblogin -p helbreath_login > backups/helbreath_login_$DATE.sql
```

Ejecutar diariamente con cron:
```bash
0 2 * * * /path/to/backup.sh
```

## Monitoreo

### Verificar Estado del Servidor
```bash
# Con Docker
docker-compose ps
docker-compose logs -f

# Sin Docker
ps aux | grep -E 'HGServer|LoginServer'
netstat -tulpn | grep -E '1234|2848'
```

### Métricas Importantes
- Número de jugadores conectados
- Uso de CPU y memoria
- Latencia de base de datos
- Tasa de errores en logs
