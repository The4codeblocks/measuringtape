
#include "raylib.h" // depends on raylib framework (www.raylib.com)
#include <stdlib.h>

#include "resource_dir.h"

#define dragging_body 1
#define dragging_tape 2

#define textcol (Color){ 0xa6, 0x9a, 0x0d, 0xff }

#define rotateInputMaxdist_sq 51 * 51
#define rotateInputMindist_sq 42 * 42

#define right 0
#define down  1
#define left  2
#define up    3

int squareDist(int x0, int y0, int x1, int y1) {
	int dx = x0 - x1;
	int dy = y0 - y1;
	return dx * dx + dy * dy;
}

Vector2 rotateSelect(char rotation, Vector2 a, Vector2 b, Vector2 c, Vector2 d) {
	switch (rotation) {
	case 0: return a;
	case 1: return b;
	case 2: return c;
	case 3: return d;
	}
}

Vector2 winRotateYD(Vector2 v, char rotation, int winW, int winH) {
	return rotateSelect(rotation,
		v,
		(Vector2) {v.y, winW - v.x},
		(Vector2) {winW - v.x, winH - v.y},
		(Vector2) {winH - v.y, v.x}
	);
}

Vector2 winRotateYU(Vector2 v, char rotation, int winW, int winH) {
	return rotateSelect(rotation,
		v,
		(Vector2) {	winH - v.y, v.x	},
		(Vector2) {	winW - v.x, winH - v.y },
		(Vector2) {	v.y, winW - v.x	}
	);
}

Vector2 add(Vector2 a, Vector2 b) {
	return (Vector2) { a.x + b.x, a.y + b.y };
}

char reverseLookup[4] = { 0,3,2,1 };

char reverseRotation(char rotation) {
	return reverseLookup[rotation];
}

void swap(int* a, int* b) {
	int intermediate = *b;
	*b = *a;
	*a = intermediate;
}

int main() {
	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI | FLAG_WINDOW_TRANSPARENT | FLAG_WINDOW_UNDECORATED);

	int windowW = 128 + 4 + 8;
	int windowH = 128;

	// Create the window and OpenGL context
	InitWindow(windowW, windowH, "Measuring Tape");

	BeginBlendMode(BLEND_ALPHA);

	// Utility function from resource_dir.h to find the resources folder and set it as the current working directory so we can load from it
	SearchAndSetResourceDir("resources");

	// Load a texture from the resources directory
	Texture body = LoadTexture("measuringTapeBody.png");
	Texture bodyLoose = LoadTexture("measuringTapeBodyLoose.png");
	Texture tape = LoadTexture("tapeSegment.png");
	Texture head = LoadTexture("measuringTapeHead.png");

	Font font = GetFontDefault();

	Vector2 initmousepos;
	Vector2 initwindowpos;
	Vector2 windowpos;
	Vector2 relativemousepos;
	Vector2 mousepos;

	char isDragging = false;

	char isLoose = false;

	int tapeLength = 0;
	int initialTapeLength = 0;

	char rotation = right;

	char buffer[8];

	// main loop
	while (!WindowShouldClose()) // run the loop untill the user presses ESCAPE or presses the Close button on the window
	{
		// drawing
		BeginDrawing();

		// Setup the back buffer for drawing (clear color and depth buffers)
		ClearBackground(BLANK);

		windowpos = GetWindowPosition();
		relativemousepos = GetMousePosition();
		mousepos = (Vector2){ relativemousepos.x + windowpos.x, relativemousepos.y + windowpos.y };
		relativemousepos = winRotateYD(relativemousepos, rotation, windowW, windowH);

		if (IsMouseButtonPressed(0) ) {
			int sqd = squareDist(relativemousepos.x, relativemousepos.y, 64, 64);
			if ((relativemousepos.x < 128) & ((relativemousepos.x + relativemousepos.y) > (128 + 92 - 2))) isLoose = !isLoose;
			else if ((sqd < rotateInputMaxdist_sq) & (sqd > rotateInputMindist_sq)) {
				rotation = (rotation + 1) & 3;
				swap(&windowW, &windowH);
				SetWindowSize(windowW, windowH);
				int totalTape = tapeLength + 12;
				if (rotation == left) {
					windowpos = add(windowpos, (Vector2) { -totalTape, 0 });
				} else if (rotation == up) {
					windowpos = add(windowpos, (Vector2) { totalTape, -totalTape });
				} else if (rotation == right) {
					windowpos = add(windowpos, (Vector2) { 0, totalTape });
				}
				SetWindowPosition(windowpos.x, windowpos.y);
			} else {
				isDragging = ((relativemousepos.x > 128) | ((relativemousepos.x > (120 - 8)) & (relativemousepos.y > ((128 - 40) - 8)))) ? dragging_tape : dragging_body;
				initmousepos = mousepos;
				initwindowpos = windowpos;
				initialTapeLength = tapeLength;
			}
		}

		if (IsMouseButtonReleased(0)) {
			isDragging = false;
			if (isLoose) {
				tapeLength = 0;
				windowW = tapeLength + 128 + 4 + 8;
				windowH = 128;
				if (rotation & 1) swap(&windowW, &windowH);
				SetWindowSize(windowW, windowH);
			}
		}

		switch(isDragging) {
			case dragging_body:
				SetWindowPosition(initwindowpos.x + mousepos.x - initmousepos.x, initwindowpos.y + mousepos.y - initmousepos.y);
				break;
			case dragging_tape:
				tapeLength = initialTapeLength + ((rotation & 1) ? (mousepos.y - initmousepos.y) : (mousepos.x - initmousepos.x));
				tapeLength = tapeLength > 0    ? tapeLength : 0;
				tapeLength = tapeLength < 8192 ? tapeLength : 8192;
				windowW = tapeLength + 128 + 4 + 8;
				windowH = 128;
				if (rotation & 1) swap(&windowW, &windowH);
				SetWindowSize(windowW, windowH);
				break;
		}

		Vector2 rotatedPos = winRotateYU((Vector2) { 128, 0 }, rotation, windowW, windowH);
		Vector2 bodyOrigin = rotateSelect(rotation,
			(Vector2) {0,0},
			(Vector2) {128,0},
			(Vector2) {windowW,128},
			(Vector2) {0,windowH}
		);
		Vector2 tapeOrigin = rotateSelect(rotation,
			(Vector2) {128,0},
			(Vector2) {128,128},
			(Vector2) {windowW - 128,128},
			(Vector2) {0,windowH - 128}
		);
		DrawTexturePro(tape, (Rectangle) { -tapeLength, 0, tapeLength - 1, 128 - 1 }, (Rectangle) { tapeOrigin.x, tapeOrigin.y, tapeLength, 128 }, (Vector2) { .5, .5 }, 90 * rotation, WHITE);
		int measure = 0;
		for (int x = tapeLength + 128 - 32; x > 96; x -= 32) {
			measure += 32;
			sprintf(buffer, "%d", measure);
			DrawTextPro(font, buffer, rotateSelect(rotation,
				(Vector2) {x + 4, 12},
				(Vector2) {128 - 12, x + 4 },
				(Vector2) {windowW - (x + 4), 128 - 12},
				(Vector2) {12, windowH - (x + 4)}
			), (Vector2) { .5, .5 }, 90 * rotation, 12, 1, textcol);
		}
		sprintf(buffer, "%d", tapeLength);

		rotatedPos = rotateSelect(rotation,
			(Vector2) {windowW - (8 + 4 + 32), 0 },
			(Vector2) {128, windowH - (12 + 1 + 32)},
			(Vector2) {(8 + 4 + 1 + 32), 128},
			(Vector2) {0, 12 + 32}
		);

		DrawTexturePro(head, (Rectangle) { 0, 0, 32 + 4 - 1, 128 - 1 }, (Rectangle) { rotatedPos.x - 1, rotatedPos.y, 32 + 4, 128 }, (Vector2) { .5, .5 }, 90 * rotation, WHITE);
		DrawTexturePro(isLoose ? bodyLoose : body, (Rectangle) { 0, 0, 128 - 1, 128 - 1 }, (Rectangle) { bodyOrigin.x, bodyOrigin.y, 128, 128 }, (Vector2) { .5, .5 }, 90 * rotation, WHITE);
		DrawTextPro(font, buffer, rotateSelect(rotation,
			(Vector2) {42,58},
			(Vector2) {windowW - 58,42},
			(Vector2) {windowW + (42 - 128),58},
			(Vector2) {windowW - 58,windowH + (42 - 128)}
		), (Vector2) { .5, .5 }, 90 * (rotation & 1), 16, 2, WHITE);
		
		// end the frame and get ready for the next one  (display frame, poll input, etc...)
		EndDrawing();
	}

	// cleanup
	UnloadTexture(body);
	UnloadTexture(bodyLoose);
	UnloadTexture(tape);
	UnloadTexture(head);

	UnloadFont(font);

	// destroy the window and cleanup the OpenGL context
	CloseWindow();
	return 0;
}
