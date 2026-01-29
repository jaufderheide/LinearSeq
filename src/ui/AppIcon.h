#pragma once

#include <FL/Fl_RGB_Image.H>

#include <array>

// Simple 32x32 icon: dark block with grey bar and orange line.
inline Fl_RGB_Image* linearseq_app_icon() {
	static std::array<unsigned char, 32 * 32 * 3> pixels{};
	static Fl_RGB_Image icon(pixels.data(), 32, 32, 3);
	static bool initialized = false;
	if (initialized) {
		return &icon;
	}
	initialized = true;

	auto setPixel = [&](int x, int y, unsigned char r, unsigned char g, unsigned char b) {
		const int index = (y * 32 + x) * 3;
		pixels[index] = r;
		pixels[index + 1] = g;
		pixels[index + 2] = b;
	};

	for (int y = 0; y < 32; ++y) {
		for (int x = 0; x < 32; ++x) {
			setPixel(x, y, 0x2b, 0x2b, 0x2b);
		}
	}

	for (int y = 12; y <= 17; ++y) {
		for (int x = 4; x < 28; ++x) {
			setPixel(x, y, 0x5c, 0x5c, 0x5c);
		}
	}

	for (int y = 14; y <= 15; ++y) {
		for (int x = 4; x < 28; ++x) {
			setPixel(x, y, 0xf2, 0x8c, 0x1a);
		}
	}

	return &icon;
}
