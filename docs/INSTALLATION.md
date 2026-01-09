# Guía de Instalación - Helbreath Apocalypse Server

## Requisitos Previos

### Opción 1: Usando Docker (Recomendado)
- Docker Engine 20.10+
- Docker Compose 2.0+

### Opción 2: Instalación Manual
- Ubuntu 20.04+ o Windows Server
- Build tools (gcc, make)
- Git

## Instalación con Docker

### Paso 1: Clonar el Repositorio

```bash
git clone https://github.com/HBApocalypseAcc/hb-apocalypse.com.git
cd hb-apocalypse.com
```

### Paso 2: Configurar el Servidor

Edite los archivos de configuración en la carpeta `config/`:

```bash
nano config/GServer.cfg
nano config/LServer.cfg
```

Ajuste los siguientes parámetros según sus necesidades:
- `MaxPlayers`: Número máximo de jugadores
- `ServerPort`: Puerto del servidor
- `DBHost`, `DBUser`, `DBPassword`: Configuración de base de datos

### Paso 3: Obtener Archivos del Servidor

Necesitará los archivos binarios del servidor de Helbreath. Puede:

1. Compilarlos desde el código fuente:
   ```bash
   git clone https://github.com/play-helbreath/helbreath-v3.82.git
   cd helbreath-v3.82
   # Seguir instrucciones de compilación
   ```

2. O descargarlos desde la comunidad de Helbreath.

Coloque los archivos en:
- `server/HGServer/` - Servidor de juego
- `server/LoginServer/` - Servidor de login

### Paso 4: Construir y Ejecutar

```bash
docker-compose up -d
```

Para ver los logs:
```bash
docker-compose logs -f
```

### Paso 5: Verificar el Servidor

Verifique que los puertos estén abiertos:
```bash
netstat -tulpn | grep -E '1234|2848'
```

O con Docker:
```bash
docker-compose ps
```

## Instalación Manual (Linux)

### Paso 1: Instalar Dependencias

```bash
sudo apt-get update
sudo apt-get install -y build-essential git
```

### Paso 2: Compilar el Servidor

```bash
git clone https://github.com/play-helbreath/helbreath-v3.82.git
cd helbreath-v3.82/Server
make
```

### Paso 3: Configurar

Copie los archivos de configuración y ajústelos según sus necesidades.

### Paso 4: Ejecutar

```bash
./LoginServer &
./HGServer &
```

## Configuración de Base de Datos

### MySQL/MariaDB

1. Instalar MySQL:
   ```bash
   sudo apt-get install mysql-server
   ```

2. Crear base de datos:
   ```sql
   CREATE DATABASE helbreath;
   CREATE DATABASE helbreath_login;
   CREATE USER 'hbuser'@'localhost' IDENTIFIED BY 'your_password';
   CREATE USER 'hblogin'@'localhost' IDENTIFIED BY 'your_password';
   GRANT ALL PRIVILEGES ON helbreath.* TO 'hbuser'@'localhost';
   GRANT ALL PRIVILEGES ON helbreath_login.* TO 'hblogin'@'localhost';
   FLUSH PRIVILEGES;
   ```

3. Actualizar configuración en `config/GServer.cfg` y `config/LServer.cfg`

## Solución de Problemas

### El servidor no inicia
- Verifique los logs en `logs/`
- Asegúrese de que los puertos no estén en uso
- Verifique permisos de archivos

### No se puede conectar
- Verifique el firewall
- Asegúrese de que los puertos 1234 y 2848 estén abiertos
- Verifique la configuración de IP en los archivos .cfg

### Error de base de datos
- Verifique credenciales en archivos de configuración
- Asegúrese de que MySQL esté ejecutándose
- Verifique que las bases de datos existan

## Puertos del Servidor

| Servicio | Puerto | Protocolo |
|----------|--------|-----------|
| Login Server | 2848 | TCP |
| Game Server | 1234 | TCP |

## Siguientes Pasos

1. Configure el firewall para permitir los puertos necesarios
2. Configure backups automáticos de la base de datos
3. Ajuste las tasas de experiencia y drop según su preferencia
4. Personalice el contenido del juego

## Recursos Adicionales

- [Helbreath Hub](https://helbreathhub.com/)
- [Helbreath Xtreme Forums](https://forum.helbreathx.net/)
- [GitHub - Helbreath v3.82](https://github.com/play-helbreath/helbreath-v3.82)
