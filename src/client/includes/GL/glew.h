#ifndef __GLEW_H__
#define __GLEW_H__

#ifdef MS_PLATFORM_WASM
#include <GLES3/gl3.h>
#define GLEW_OK 0
#define GLEW_NO_ERROR 0
#define GLEW_VERSION 1
#define GLEW_VERSION_MAJOR 2
#define GLEW_VERSION_MINOR 1
#define GLEW_VERSION_MICRO 0

static int glewInit() { return 0; }
static int glewIsSupported(const char* name) { return 0; }
#define glewGetErrorString(x) (const GLubyte*)"(WASM: No GLEW Error)"
#define glewGetString(x) (const GLubyte*)"(WASM: No GLEW String)"

static int glewExperimental = 0;

typedef GLuint GLhandleARB; // Compatibility
#endif

#endif
