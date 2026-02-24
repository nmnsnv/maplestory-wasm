#ifndef __WGLEW_H__
#define __WGLEW_H__

#ifdef MS_PLATFORM_WASM
#define WGLEW_OK 0
#define wglewIsSupported(x) 0
#define wglewInit() 0
#endif

#endif
