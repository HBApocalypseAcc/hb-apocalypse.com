# Helbreath Apocalypse Server

Servidor privado de Helbreath Apocalypse - Un servidor de juego MMORPG clásico.

## Descripción

Este es un servidor privado para el juego Helbreath, configurado para el servidor Apocalypse. Helbreath es un juego de rol multijugador masivo en línea (MMORPG) clásico.

## Características

- Servidor de juego Helbreath
- Servidor de login
- Configuración mediante Docker para fácil despliegue
- Configuraciones personalizables

## Requisitos

- Docker y Docker Compose (recomendado)
- O compilación manual con las dependencias necesarias

## Instalación Rápida con Docker

```bash
# Clonar el repositorio
git clone https://github.com/HBApocalypseAcc/hb-apocalypse.com.git
cd hb-apocalypse.com

# Construir y ejecutar con Docker Compose
docker-compose up -d
```

## Estructura del Proyecto

```
hb-apocalypse.com/
├── server/              # Archivos del servidor de juego
│   ├── HGServer/        # Servidor de juego principal
│   └── LoginServer/     # Servidor de autenticación
├── config/              # Archivos de configuración
├── docker/              # Configuración de Docker
├── docs/                # Documentación
└── README.md
```

## Configuración

Los archivos de configuración principales se encuentran en el directorio `config/`:
- `GServer.cfg` - Configuración del servidor de juego
- `LServer.cfg` - Configuración del servidor de login

## Puertos

- Puerto 2848: Servidor de Login
- Puerto 1234: Servidor de Juego

## Documentación

Para más información, consulte la carpeta `docs/`.

## Contribuir

Las contribuciones son bienvenidas. Por favor, cree un issue o pull request.

## Licencia

Este proyecto es para uso educativo y de comunidad.

## Contacto

Para preguntas o soporte, abra un issue en GitHub.