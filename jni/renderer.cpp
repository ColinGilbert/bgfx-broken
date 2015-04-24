//
// Copyright 2011 Tero Saarni
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#include <bgfx.h>
#include <bgfxplatform.h>

#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include <android/native_window.h> 
#include <EGL/egl.h>



#include "logger.h"
#include "renderer.h"

#define LOG_TAG "EglSample"

Renderer::Renderer() : _msg(MSG_NONE), _display(0), _surface(0), _context(0)
{
	LOG_INFO("Renderer instance created");
	pthread_mutex_init(&_mutex, 0);    
	return;
}

Renderer::~Renderer()
{
	LOG_INFO("Renderer instance destroyed");
	pthread_mutex_destroy(&_mutex);
	return;
}

void Renderer::start()
{
	LOG_INFO("Creating renderer thread");
	pthread_create(&_threadId, 0, threadStartCallback, this);
	return;
}

void Renderer::stop()
{
	LOG_INFO("Stopping renderer thread");

	// send message to render thread to stop rendering
	pthread_mutex_lock(&_mutex);
	_msg = MSG_RENDER_LOOP_EXIT;
	pthread_mutex_unlock(&_mutex);

	pthread_join(_threadId, 0);
	LOG_INFO("Renderer thread stopped");

	return;
}

void Renderer::setWindow(ANativeWindow *window)
{
	LOG_INFO("Setting window");
	// notify render thread that window has changed
	pthread_mutex_lock(&_mutex);
	_msg = MSG_WINDOW_SET;
	_window = window;

	pthread_mutex_unlock(&_mutex);

	return;
}



void Renderer::renderLoop()
{
	bool renderingEnabled = true;

	LOG_INFO("renderLoop()");

	while (renderingEnabled)
	{
		pthread_mutex_lock(&_mutex);

		// process incoming messages
		switch (_msg)
		{
			case MSG_WINDOW_SET:
				initialize();
				break;

			case MSG_RENDER_LOOP_EXIT:
				renderingEnabled = false;
				destroy();
				break;

			default:
				break;
		}
		_msg = MSG_NONE;
		if (_display)
		{
			drawFrame();
			if (!eglSwapBuffers(_display, _surface))
			{
				LOG_ERROR("eglSwapBuffers() returned error %d", eglGetError());
			}
		}
		pthread_mutex_unlock(&_mutex);
	}
	LOG_INFO("Render loop exits");
	return;
}

bool Renderer::initialize()
{
	const EGLint attribs[] =
	{
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_BLUE_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_RED_SIZE, 8,
		EGL_NONE
	};

	EGLDisplay display;
	EGLConfig config;    
	EGLint numConfigs;
	EGLint format;
	EGLSurface surface;
	EGLContext context;
	EGLint width;
	EGLint height;

	LOG_INFO("Initializing context");

	if ((display = eglGetDisplay(EGL_DEFAULT_DISPLAY)) == EGL_NO_DISPLAY)
	{
		LOG_ERROR("eglGetDisplay() returned error %d", eglGetError());
		return false;
	}
	if (!eglInitialize(display, 0, 0)) 
	{
		LOG_ERROR("eglInitialize() returned error %d", eglGetError());
		return false;
	}

	if (!eglChooseConfig(display, attribs, &config, 1, &numConfigs))
	{
		LOG_ERROR("eglChooseConfig() returned error %d", eglGetError());
		destroy();
		return false;
	}

	if (!eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format))
	{
		LOG_ERROR("eglGetConfigAttrib() returned error %d", eglGetError());
		destroy();
		return false;
	}

	ANativeWindow_setBuffersGeometry(_window, 0, 0, format);

	if (!(surface = eglCreateWindowSurface(display, config, _window, 0))) 
	{
		LOG_ERROR("eglCreateWindowSurface() returned error %d", eglGetError());
		destroy();
		return false;
	}

	if (!(context = eglCreateContext(display, config, 0, 0)))
	{
		LOG_ERROR("eglCreateContext() returned error %d", eglGetError());
		destroy();
		return false;
	}

	if (!eglMakeCurrent(display, surface, surface, context))
	{
		LOG_ERROR("eglMakeCurrent() returned error %d", eglGetError());
		destroy();
		return false;
	}

	if (!eglQuerySurface(display, surface, EGL_WIDTH, &width) || !eglQuerySurface(display, surface, EGL_HEIGHT, &height))
	{
		LOG_ERROR("eglQuerySurface() returned error %d", eglGetError());
		destroy();
		return false;
	}

	_display = display;
	_surface = surface;
	_context = context;

	_width = width;
	_height = height;

	bgfx::PlatformData pd;
	
	pd.ndt = _display;
	pd.nwh = _window;
	pd.context = _context;
	pd.backbuffer = 0;
	
	bgfx::setPlatformData(pd);


	uint32_t debug = BGFX_DEBUG_TEXT;
	uint32_t reset = BGFX_RESET_VSYNC;

	bgfx::init();
	bgfx::reset(width, height, reset);

	// Enable debug text.
	bgfx::setDebug(debug);

	// Set view 0 clear state.
	bgfx::setViewClear(0, BGFX_CLEAR_COLOR|BGFX_CLEAR_DEPTH	, 0x703070ff, 1.0f, 0);

	return true;
}

void Renderer::destroy()
{
	LOG_INFO("Destroying context");

	bgfx::shutdown();

	eglMakeCurrent(_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

	eglDestroyContext(_display, _context);
	eglDestroySurface(_display, _surface);
	eglTerminate(_display);

	_display = EGL_NO_DISPLAY;
	_surface = EGL_NO_SURFACE;
	_context = EGL_NO_CONTEXT;

	return;
}

void Renderer::drawFrame()
{
	bgfx::setViewRect(0, 0 ,0, _width, _height);
	bgfx::submit(0);

	// Use debug font to print information about this example.
	bgfx::dbgTextClear();
	bgfx::dbgTextPrintf(0, 1, 0x4f, "bgfx/examples/00-helloworld");
	bgfx::dbgTextPrintf(0, 2, 0x6f, "Description: Initialization and debug text.");

	// Advance to next frame. Rendering thread will be kicked to
	// process submitted rendering primitives.

	bgfx::frame();
}

void* Renderer::threadStartCallback(void *myself)
{
	Renderer *renderer = (Renderer*)myself;

	renderer->renderLoop();
	pthread_exit(0);

	return 0;
}

