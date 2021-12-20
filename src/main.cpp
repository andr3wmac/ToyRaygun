#include "Renderers/D3D12.h"

#include <SDL.h>
#undef main

void main(int argc, char *args[]) {
	SDL_Window* window = SDL_CreateWindow("Toy Raygun", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, 0);

	Renderer* renderer = new D3D12();
	renderer->Init();

	while (true)
	{
		SDL_Event windowEvent;
		if (SDL_PollEvent(&windowEvent)) {
			if (windowEvent.type == SDL_QUIT) break;
		}

		renderer->RenderFrame();
	}

	renderer->Destroy();
	delete renderer;
}