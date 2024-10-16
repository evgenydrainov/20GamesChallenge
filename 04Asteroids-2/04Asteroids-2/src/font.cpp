#include "font.h"

#include "package.h"
#include "batch_renderer.h"

Font load_bmfont_file(const char* fnt_filepath, const char* png_filepath) {
	Font font = {};

	string text = get_file_str(fnt_filepath);
	if (text.count == 0) {
		log_error("Couldn't load font \"%s\"", fnt_filepath);
		return font;
	}

	string line;
	string word;

	line = eat_line(&text); // info

	// 
	// TODO: Parse 'face="MS Gothic"' somehow
	// 

	while (line.count > 0) {
		eat_whitespace(&line);
		word = eat_non_whitespace(&line);

		string prefix = "size=";
		if (starts_with(word, prefix)) {
			advance(&word, prefix.count);

			bool done;
			int size = string_to_int(word, &done);
			Assert(done);

			font.size = size;
		}
	}

	line = eat_line(&text); // common

	while (line.count > 0) {
		eat_whitespace(&line);
		word = eat_non_whitespace(&line);

		string prefix = "lineHeight=";
		if (starts_with(word, prefix)) {
			advance(&word, prefix.count);

			bool done;
			int line_height = string_to_int(word, &done);
			Assert(done);

			font.line_height = line_height;
		}
	}

	line = eat_line(&text); // page
	line = eat_line(&text); // chars

	font.glyphs.count = 95;
	font.glyphs.data  = (Glyph*) malloc(font.glyphs.count * sizeof(font.glyphs[0]));

	for (int i = 0; i < 95; i++) {
		line = eat_line(&text);

		eat_whitespace(&line);
		word = eat_non_whitespace(&line);
		Assert(word == "char");

		auto eat_value_int = [&](string prefix) -> int {
			Assert(prefix.count >= 2);

			eat_whitespace(&line);
			word = eat_non_whitespace(&line);

			Assert(starts_with(word, prefix));

			advance(&word, prefix.count);

			bool done;
			int value = string_to_int(word, &done);
			if (!done) {
				prefix.count--;
				log_warn("Couldn't parse value " Str_Fmt " for char %d in font %s", Str_Arg(prefix), i + 32, fnt_filepath);
			}

			return value;
		};

		int id = eat_value_int("id=");
		Assert(id == i + 32);

		Glyph glyph = {};

		glyph.u        = eat_value_int("x=");
		glyph.v        = eat_value_int("y=");
		glyph.width    = eat_value_int("width=");
		glyph.height   = eat_value_int("height=");
		glyph.xoffset  = eat_value_int("xoffset=");
		glyph.yoffset  = eat_value_int("yoffset=");
		glyph.xadvance = eat_value_int("xadvance=");

		font.glyphs[i] = glyph;
	}

	font.atlas = load_texture_from_file(png_filepath);

	return font;
}

vec2 draw_text(Font font, string text, vec2 text_pos,
			   HAlign halign, VAlign valign, vec4 color) {
	if (font.glyphs.count == 0) return text_pos;

	if (valign == VALIGN_MIDDLE) {
		text_pos.y -= measure_text(font, text).y / 2.0f;
	} else if (valign == VALIGN_BOTTOM) {
		text_pos.y -= measure_text(font, text).y;
	}

	float ch_x = text_pos.x;
	float ch_y = text_pos.y;

	if (halign == HALIGN_CENTER) {
		ch_x -= measure_text(font, text, true).x / 2.0f;
	} else if (halign == HALIGN_RIGHT) {
		ch_x -= measure_text(font, text, true).x;
	}

	for (size_t i = 0; i < text.count; i++) {
		u8 ch = (u8) text[i];

		// If char isn't valid, set it to '?'
		if (!((ch >= 32 && ch <= 127) || ch == '\n')) {
			ch = '?';
		}

		if (ch == '\n') {
			ch_x = text_pos.x;
			ch_y += font.line_height;

			if (halign == HALIGN_CENTER) {
				string str = text;
				advance(&str, i + 1);
				ch_x -= measure_text(font, str, true).x / 2.0f;
			} else if (halign == HALIGN_RIGHT) {
				string str = text;
				advance(&str, i + 1);
				ch_x -= measure_text(font, str, true).x;
			}
		} else {
			Assert(font.glyphs.count == 95);
			Glyph glyph = font.glyphs[ch - 32];

			// If char isn't whitespace, draw it
			if (ch != ' ') {
				Rect src = {glyph.u, glyph.v, glyph.width, glyph.height};

				vec2 pos;
				pos.x = ch_x + glyph.xoffset;
				pos.y = ch_y + glyph.yoffset;

				pos = glm::floor(pos);

				draw_texture(font.atlas, src, pos, {1, 1}, {}, 0, color);
			}

			ch_x += glyph.xadvance;
		}
	}

	return {ch_x, ch_y};
}

vec2 measure_text(Font font, string text, bool only_one_line) {
	if (font.glyphs.count == 0) return {};

	float w = 0;
	float h = (float) font.size;

	float ch_x = 0;
	float ch_y = 0;

	for (size_t i = 0; i < text.count; i++) {
		u8 ch = (u8) text[i];

		// If char isn't valid, set it to '?'
		if (!((ch >= 32 && ch <= 127) || ch == '\n')) {
			ch = '?';
		}

		if (ch == '\n') {
			if (only_one_line) return {w, h};

			ch_x = 0;
			ch_y += font.line_height;

			h = max(h, ch_y + font.size);
		} else {
			Assert(font.glyphs.count == 95);
			Glyph glyph = font.glyphs[ch - 32];

			// If char isn't whitespace, draw it
			if (ch != ' ') {
				vec2 pos;
				pos.x = ch_x + glyph.xoffset;
				pos.y = ch_y + glyph.yoffset;

				pos = glm::floor(pos);

				w = max(w, pos.x + glyph.width);
				h = max(h, ch_y + font.size);
			}

			ch_x += glyph.xadvance;
		}
	}

	return {w, h};
}
