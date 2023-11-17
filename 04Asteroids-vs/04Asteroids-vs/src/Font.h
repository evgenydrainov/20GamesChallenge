#pragma once

#include <SDL.h>

struct GlyphData {
	SDL_Rect src;
	int xoffset;
	int yoffset;
	int advance;
};

struct Font {
	SDL_Texture* texture;
	int ptsize;
	int height;
	int ascent;
	int descent;
	int lineskip;
	GlyphData* glyphs; // 32..126
};

enum {
	HALIGN_LEFT,
	HALIGN_CENTER,
	HALIGN_RIGHT
};

enum {
	VALIGN_TOP,
	VALIGN_MIDDLE,
	VALIGN_BOTTOM
};

enum {
	FONT_STYLE_NORMAL        = 0x00,
	FONT_STYLE_BOLD          = 0x01,
	FONT_STYLE_ITALIC        = 0x02,
	FONT_STYLE_UNDERLINE     = 0x04,
	FONT_STYLE_STRIKETHROUGH = 0x08
};

// Expects SDL_ttf (and SDL) to be initialized.
bool LoadFontFromFileTTF(SDL_Renderer* renderer, Font* font, const char* fname, int ptsize, int style = 0);
void DestroyFont(Font* font);

SDL_Point DrawText(SDL_Renderer* renderer, Font* font, const char* text,
				   int x, int y,
				   int halign = 0, int valign = 0,
				   SDL_Color color = {255, 255, 255, 255},
				   float xscale = 1.0f, float yscale = 1.0f);

SDL_Point DrawTextShadow(SDL_Renderer* renderer, Font* font, const char* text,
						 int x, int y,
						 int halign = 0, int valign = 0,
						 SDL_Color color = {255, 255, 255, 255},
						 float xscale = 1.0f, float yscale = 1.0f);

SDL_Point MeasureText(Font* font, const char* text,
					  float xscale = 1.0f, float yscale = 1.0f,
					  bool only_one_line = false);
