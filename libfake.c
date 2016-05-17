
#define _GNU_SOURCE
#include "libfake.h"

#if defined(_WIN32)
#define LIBSDL2_FILENAME "SDL2.dll"
#define DEFAULT_OPENGL "OPENGL32.DLL"
#define CAPSULE_WINDOWS

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

#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>
#include <stdint.h>
#include <string.h>

static void assert (const char *msg, int cond) {
  if (cond) {
    return;
  }
  fprintf(stderr, "[main] Assertion failed: %s\n", msg);
  /* const char *err = SDL_GetError(); */
  /* fprintf(stderr, "[main] Last SDL GetError: %s\n", err); */
  exit(1);
}

#ifdef CAPSULE_LINUX
void glXSwapBuffers (void *a, void *b);

typedef int (*glXQueryExtensionType)(void*, void*, void*);
glXQueryExtensionType _realglXQueryExtension;

typedef void (*glXSwapBuffersType)(void*, void*);
glXSwapBuffersType _realglXSwapBuffers;

typedef void* (*glXGetProcAddressARBType)(const char*);
glXGetProcAddressARBType _realglXGetProcAddressARB;
#endif

#define FRAME_WIDTH 512
#define FRAME_HEIGHT 512
char *frameData;
int frameNumber = 0;

typedef void* (*dlopen_type)(const char*, int);
dlopen_type real_dlopen;
void *gl_handle;

void ensure_real_dlopen() {
#ifdef CAPSULE_LINUX
  if (!real_dlopen) {
    fprintf(stderr, "[libfake] Getting real dlopen\n");
    real_dlopen = dlsym(RTLD_NEXT, "dlopen");
  }
#endif
}

void load_opengl (const char *openglPath) {
  fprintf(stderr, "[libfake] Loading real opengl from %s\n", openglPath);
#ifdef CAPSULE_LINUX
  ensure_real_dlopen();
  gl_handle = real_dlopen(openglPath, (RTLD_NOW|RTLD_LOCAL));
#else
  gl_handle = dlopen(openglPath, (RTLD_NOW|RTLD_LOCAL));
#endif
  assert("Loaded real OpenGL lib", !!gl_handle);
  fprintf(stderr, "[libfake] Loaded opengl!\n");

#ifdef CAPSULE_LINUX
  fprintf(stderr, "[libfake] Getting glXQueryExtension adress\n");
  _realglXQueryExtension = dlsym(gl_handle, "glXQueryExtension");
  assert("Got glXQueryExtension", !!_realglXQueryExtension);
  fprintf(stderr, "[libfake] Got glXQueryExtension adress: %p\n", _realglXQueryExtension);

  fprintf(stderr, "[libfake] Getting glXSwapBuffers adress\n");
  _realglXSwapBuffers = dlsym(gl_handle, "glXSwapBuffers");
  assert("Got glXSwapBuffers", !!_realglXSwapBuffers);
  fprintf(stderr, "[libfake] Got glXSwapBuffers adress: %p\n", _realglXSwapBuffers);

  fprintf(stderr, "[libfake] Getting glXGetProcAddressARB address\n");
  _realglXGetProcAddressARB = dlsym(gl_handle, "glXGetProcAddressARB");
  assert("Got glXGetProcAddressARB", !!_realglXGetProcAddressARB);
  fprintf(stderr, "[libfake] Got glXGetProcAddressARB adress: %p\n", _realglXGetProcAddressARB);
#endif
}

void ensure_opengl () {
  if (!gl_handle) {
    load_opengl(DEFAULT_OPENGL);
  }
}

void* glXGetProcAddressARB (const char *name) {
#ifdef CAPSULE_LINUX
  if (strcmp(name, "glXSwapBuffers") == 0) {
    fprintf(stderr, "[libfake] In glXGetProcAddressARB: %s\n", name);
    fprintf(stderr, "[libfake] Returning fake glXSwapBuffers\n");
    return &glXSwapBuffers;
  }

  /* fprintf(stderr, "[libfake] In glXGetProcAddressARB: %s\n", name); */
#endif

  ensure_opengl();
  return _realglXGetProcAddressARB(name);
}

#ifdef CAPSULE_LINUX
void* dlopen (const char * filename, int flag) {
  ensure_real_dlopen();

  if (filename != NULL && strstr(filename, "libGL.so.1")) {
    load_opengl(filename);

    if (!strcmp(filename, "libGL.so.1")) {
      fprintf(stderr, "[libfake] Faking libGL for %s\n", filename);
      return real_dlopen(NULL, RTLD_NOW|RTLD_LOCAL);
    } else {
      fprintf(stderr, "[libfake] Looks like a real libGL? %s\n", filename);
      return real_dlopen(filename, flag);
    }
  } else {
    pid_t pid = getpid();
    fprintf(stderr, "[libfake] pid %d, dlopen(%s, %d)\n", pid, filename, flag);
    void *res = real_dlopen(filename, flag);
    fprintf(stderr, "[libfake] pid %d, dlopen(%s, %d): %p\n", pid, filename, flag, res);
    return res;
  }
}
#endif

#define GL_RGB 6407
#define GL_UNSIGNED_BYTE 5121
#define GL_VIEWPORT 2978

typedef void (*glReadPixelsType)(int, int, int, int, int, int, void*);
glReadPixelsType _realglReadPixels;

typedef void* (*glGetIntegervType)(int, int*);
glGetIntegervType _realglGetIntegerv;

FILE *outFile;

void libfake_captureFrame () {
  int width = FRAME_WIDTH;
  int height = FRAME_HEIGHT;
  int components = 3;
  int format = GL_RGB;

  if (!_realglGetIntegerv) {
    ensure_opengl();
    _realglGetIntegerv = dlsym(gl_handle, "glGetIntegerv");
    assert("Got glGetIntegerv address", !!_realglGetIntegerv);
  }

  int viewport[4];
  _realglGetIntegerv(GL_VIEWPORT, viewport);

  if (viewport[2] > 0 && viewport[3] > 0) {
    width = viewport[2];
    height = viewport[3];
  }

  if (frameNumber % 60 == 0) {
    fprintf(stderr, "[libfake] Saved %d frames. Current resolution = %dx%d\n", frameNumber, width, height);
  }

  size_t frameDataSize = width * height * components;
  if (!frameData) {
    frameData = malloc(frameDataSize);
  }

  if (!_realglReadPixels) {
    ensure_opengl();
    _realglReadPixels = dlsym(gl_handle, "glReadPixels");
    assert("Got glReadPixels address", !!_realglReadPixels);
  }
  _realglReadPixels(0, 0, width, height, format, GL_UNSIGNED_BYTE, frameData);

  if (!outFile) {
    outFile = fopen("capsule.rawvideo", "wb");
    assert("Opened output file", !!outFile);
  }

  fwrite(frameData, 1, frameDataSize, outFile);
  fflush(outFile);

  frameNumber++;
}

#ifdef CAPSULE_LINUX
void glXSwapBuffers (void *a, void *b) {
  fprintf(stderr, "[libfake] About to capture frame..\n");
  libfake_captureFrame();
  fprintf(stderr, "[libfake] About to call real swap buffers..\n");
  return _realglXSwapBuffers(a, b);
}

int glXQueryExtension (void *a, void *b, void *c) {
  return _realglXQueryExtension(a, b, c);
}
#endif

void __attribute__((constructor)) libfake_load() {
  pid_t pid = getpid();
  fprintf(stderr, "[libfake] Initializing (pid %d)...\n", pid);

#ifdef CAPSULE_LINUX
  fprintf(stderr, "[libfake] LD_LIBRARY_PATH: %s\n", getenv("LD_LIBRARY_PATH"));
#endif
}

void __attribute__((destructor)) libfake_unload() {
  fprintf(stderr, "[libfake] Winding down...\n");
}

