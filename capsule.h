
#pragma once

#ifdef _WIN32
#define CAPSULE_STDCALL __stdcall
#else
#define CAPSULE_STDCALL
#endif

#if defined(_WIN32)
#define LIBSDL2_FILENAME "SDL2.dll"
#define DEFAULT_OPENGL "OPENGL32.DLL"
#define CAPSULE_WINDOWS

#define getpid(a) 0
#define pid_t int

#elif defined(__APPLE__)
#define LIBSDL2_FILENAME "libSDL2.dylib"
#define DEFAULT_OPENGL "/System/Library/Frameworks/OpenGL.framework/Libraries/libGL.dylib"
#define CAPSULE_OSX

#elif defined(__linux__) || defined(__unix__)
#define LIBSDL2_FILENAME "libSDL2.so"
#define DEFAULT_OPENGL "libGL.so.1"
#define CAPSULE_LINUX
#include <sys/types.h>
#include <unistd.h>

#else
#error Unsupported platform
#endif

#ifdef CAPSULE_WINDOWS
#ifdef BUILD_CAPSULE_DLL
#define CAPSULE_DLL __declspec(dllexport)
#else
#define CAPSULE_DLL __declspec(dllimport)
#endif
#else
#define CAPSULE_DLL
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CAPSULE_WINDOWS
CAPSULE_DLL void libfake_hello ();
#endif

void CAPSULE_STDCALL libfake_captureFrame ();

void* glXGetProcAddressARB (const char*);
void glXSwapBuffers (void *a, void *b);
int glXQueryExtension (void *a, void *b, void *c);

#ifdef __cplusplus
}
#endif
