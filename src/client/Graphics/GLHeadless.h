//////////////////////////////////////////////////////////////////////////////
// Minimal OpenGL type shim for the headless (MS_HEADLESS) test build.        //
//                                                                            //
// The headless GraphicsGL path references GL *types* in struct fields and    //
// function signatures but makes no GL calls (all are compiled out), so the   //
// host test binary needs neither GLEW nor a real GL/driver -- just these     //
// typedefs. The real client build includes <GL/glew.h> instead.             //
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <cstdint>

typedef short GLshort;
typedef unsigned short GLushort;
typedef int GLint;
typedef unsigned int GLuint;
typedef int GLsizei;
typedef float GLfloat;
typedef unsigned char GLubyte;
typedef unsigned char GLboolean;
typedef unsigned int GLenum;
