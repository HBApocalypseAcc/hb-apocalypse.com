// legacy_stdio_fix.cpp
// Fix para compatibilidad con librerías compiladas con versiones antiguas del CRT de MSVC
// Resuelve el símbolo __iob que fue removido en Visual Studio 2015+

#include <stdio.h>

#if _MSC_VER >= 1900  // Visual Studio 2015 y posteriores

#pragma warning(disable: 4996)

extern "C" {

// La estructura FILE cambió en VS2015+. Las librerías antiguas buscan __iob o __iob_func
// Creamos un array estático que simula la estructura antigua

#if defined(_M_IX86)  // Solo para 32-bit

// Esta es la estructura FILE de las versiones antiguas de MSVC (pre-VS2015)
// Tamaño: 32 bytes en x86
struct _iobuf_legacy {
    char *_ptr;
    int   _cnt;
    char *_base;
    int   _flag;
    int   _file;
    int   _charbuf;
    int   _bufsiz;
    char *_tmpfname;
};

// Array de 3 elementos para stdin, stdout, stderr
// Este es el símbolo que las librerías antiguas buscan como __imp___iob
__declspec(dllexport) struct _iobuf_legacy _iob[3];

// Función que inicializa el array _iob
// Nota: Esta función debe ser llamada antes de usar las funciones de stdio
static int _init_iob_done = 0;

void __cdecl _init_legacy_iob(void) {
    if (!_init_iob_done) {
        // Inicializar con valores básicos - las librerías antiguas normalmente
        // solo usan _file para identificar stdin(0), stdout(1), stderr(2)
        _iob[0]._file = 0;  // stdin
        _iob[1]._file = 1;  // stdout
        _iob[2]._file = 2;  // stderr
        _init_iob_done = 1;
    }
}

// Proporciona __iob_func para librerías que lo buscan como función
FILE* __cdecl __iob_func(void) {
    return (FILE*)_iob;
}

// Crear el símbolo __imp___iob que apunte a _iob
// Usamos un pragma para crear un alias del símbolo
#pragma comment(linker, "/alternatename:__imp___iob=__iob")

// Otra variante del símbolo que algunas librerías pueden buscar
extern FILE* __cdecl __p__iob(void) {
    return (FILE*)_iob;
}

#endif  // _M_IX86

}  // extern "C"

#endif  // _MSC_VER >= 1900
