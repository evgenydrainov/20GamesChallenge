#include "window_creation.h"


SDL_Window* window = nullptr;
SDL_GLContext gl_context = nullptr;
bool should_quit = false;

constexpr int NUM_KEYS = SDL_SCANCODE_UP + 1;
static u32 key_pressed[(NUM_KEYS + 31) / 32];
static u32 key_repeat [(NUM_KEYS + 31) / 32];


#ifdef _DEBUG
static void GLAPIENTRY gl_debug_callback(GLenum source,
										 GLenum type,
										 unsigned int id,
										 GLenum severity,
										 GLsizei length,
										 const char *message,
										 const void *userParam) {
	// ignore non-significant error/warning codes
	// if (id == 131169 || id == 131185 || id == 131218 || id == 131204) {
	// 	return;
	// }

	log_info("---------------");
	log_info("Debug message (%u): %s", id, message);

	switch (source)
	{
		case GL_DEBUG_SOURCE_API:             log_info("Source: API"); break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   log_info("Source: Window System"); break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER: log_info("Source: Shader Compiler"); break;
		case GL_DEBUG_SOURCE_THIRD_PARTY:     log_info("Source: Third Party"); break;
		case GL_DEBUG_SOURCE_APPLICATION:     log_info("Source: Application"); break;
		case GL_DEBUG_SOURCE_OTHER:           log_info("Source: Other"); break;
	}

	switch (type)
	{
		case GL_DEBUG_TYPE_ERROR:               log_info("Type: Error"); break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: log_info("Type: Deprecated Behaviour"); break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  log_info("Type: Undefined Behaviour"); break;
		case GL_DEBUG_TYPE_PORTABILITY:         log_info("Type: Portability"); break;
		case GL_DEBUG_TYPE_PERFORMANCE:         log_info("Type: Performance"); break;
		case GL_DEBUG_TYPE_MARKER:              log_info("Type: Marker"); break;
		case GL_DEBUG_TYPE_PUSH_GROUP:          log_info("Type: Push Group"); break;
		case GL_DEBUG_TYPE_POP_GROUP:           log_info("Type: Pop Group"); break;
		case GL_DEBUG_TYPE_OTHER:               log_info("Type: Other"); break;
	}

	switch (severity)
	{
		case GL_DEBUG_SEVERITY_HIGH:         log_info("Severity: high"); break;
		case GL_DEBUG_SEVERITY_MEDIUM:       log_info("Severity: medium"); break;
		case GL_DEBUG_SEVERITY_LOW:          log_info("Severity: low"); break;
		case GL_DEBUG_SEVERITY_NOTIFICATION: log_info("Severity: notification"); break;
	}

	// SDL_Window* win = SDL_GL_GetCurrentWindow();
	// SDL_ShowSimpleMessageBox(0, "", message, win);

	SDL_TriggerBreakpoint();
}
#endif




void init_window_and_opengl(const char* title,
							int width, int height, int init_scale,
							bool vsync) {
	// SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);

	SDL_SetHint("SDL_WINDOWS_DPI_AWARENESS", "system");

	if (SDL_Init(SDL_INIT_VIDEO
				 | SDL_INIT_AUDIO) != 0) {
		panic_and_abort("Couldn't initialize SDL: %s", SDL_GetError());
	}

	window = SDL_CreateWindow(title,
							  SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
							  width * init_scale, height * init_scale,
							  SDL_WINDOW_OPENGL
							  | SDL_WINDOW_RESIZABLE);

	if (!window) {
		panic_and_abort("Couldn't create window: %s", SDL_GetError());
	}

	// 
	// A little workaround for Linux Mint Cinnamon.
	// 
#if defined(_DEBUG) && defined(__unix__)
	SDL_RaiseWindow(window);
#endif


	SDL_SetWindowMinimumSize(window, width, height);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

#ifdef _DEBUG
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif

	gl_context = SDL_GL_CreateContext(window);
	SDL_GL_MakeCurrent(window, gl_context);

	{
		int version = gladLoadGL([](const char* name) {
			return (GLADapiproc) SDL_GL_GetProcAddress(name);
		});

		if (version == 0) {
			panic_and_abort("Couldn't load OpenGL.");
		}
	}


#ifdef _DEBUG
	if (GLAD_GL_KHR_debug) {
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(gl_debug_callback, nullptr);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
	}
#endif

	if (vsync) {
		SDL_GL_SetSwapInterval(1);
	} else {
		SDL_GL_SetSwapInterval(0);
	}

	glDisable(GL_CULL_FACE);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glDisable(GL_DEPTH_TEST);
}

void deinit_window_and_opengl() {
	SDL_GL_DeleteContext(gl_context);
	gl_context = nullptr;

	SDL_DestroyWindow(window);
	window = nullptr;

	SDL_Quit();
}

void handle_event(SDL_Event* ev) {
	switch (ev->type) {
		case SDL_QUIT: {
			should_quit = true;
			break;
		}

		case SDL_KEYDOWN: {
			SDL_Scancode scancode = ev->key.keysym.scancode;

			if (scancode >= 0 && scancode < NUM_KEYS) {
				if (ev->key.repeat) {
					key_repeat[scancode / 32] |= 1 << (scancode % 32);
				} else {
					key_pressed[scancode / 32] |= 1 << (scancode % 32);
				}
			}
			break;
		}
	}
}

void swap_buffers() {
	SDL_GL_SwapWindow(window);
}


bool is_key_pressed(SDL_Scancode key, bool repeat) {
	bool result = false;

	if (!(key >= 0 && key < NUM_KEYS)) return result;

	/*if (!g->console.show)*/ {
		result |= (key_pressed[key / 32] & (1 << (key % 32))) != 0;

		if (repeat) {
			result |= (key_repeat[key / 32] & (1 << (key % 32))) != 0;
		}
	}

	return result;
}

bool is_key_held(SDL_Scancode key) {
	bool result = false;

	if (!(key >= 0 && key < NUM_KEYS)) return result;

	/*if (!g->console.show)*/ {
		const u8* state = SDL_GetKeyboardState(nullptr);

		result |= (state[key] != 0);
	}

	return result;
}
