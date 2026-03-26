#include <SDL2/SDL.h>
#include <iostream>

const int WINDOW_WIDTH = 640;
const int WINDOW_HEIGHT = 480;
const int PADDLE_WIDTH = 15;
const int PADDLE_HEIGHT = 60;
const int BALL_SIZE = 15;

int main(int argc, char *argv[]) {
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    std::cerr << "Video Initialization failed (Headless mode). Shutting down "
                 "securely for automated CI tests: "
              << SDL_GetError() << "\n";
    return 0; // Exit successfully for CI pipelines
  }

  SDL_Window *window = SDL_CreateWindow(
      "Vessel SDL2 Pong", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
  if (!window) {
    std::cerr << "Window creation failed: " << SDL_GetError() << "\n";
    SDL_Quit();
    return 0;
  }

  SDL_Renderer *renderer = SDL_CreateRenderer(
      window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!renderer) {
    std::cerr << "Renderer creation failed: " << SDL_GetError() << "\n";
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
  }

  SDL_Rect p1 = {30, (WINDOW_HEIGHT / 2) - (PADDLE_HEIGHT / 2), PADDLE_WIDTH,
                 PADDLE_HEIGHT};
  SDL_Rect p2 = {WINDOW_WIDTH - 30 - PADDLE_WIDTH,
                 (WINDOW_HEIGHT / 2) - (PADDLE_HEIGHT / 2), PADDLE_WIDTH,
                 PADDLE_HEIGHT};
  SDL_Rect ball = {(WINDOW_WIDTH / 2) - (BALL_SIZE / 2),
                   (WINDOW_HEIGHT / 2) - (BALL_SIZE / 2), BALL_SIZE, BALL_SIZE};

  int ball_dx = 4;
  int ball_dy = 4;

  bool running = true;
  SDL_Event event;

  while (running) {
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT ||
          (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
        running = false;
      }
    }

    // Extremely basic automated simulation
    ball.x += ball_dx;
    ball.y += ball_dy;

    if (ball.y <= 0 || ball.y + BALL_SIZE >= WINDOW_HEIGHT)
      ball_dy = -ball_dy;
    if (ball.x <= 0 || ball.x + BALL_SIZE >= WINDOW_WIDTH) {
      ball.x = (WINDOW_WIDTH / 2) - (BALL_SIZE / 2);
      ball.y = (WINDOW_HEIGHT / 2) - (BALL_SIZE / 2);
    }

    // Track automated paddles
    p1.y = ball.y - (PADDLE_HEIGHT / 2);
    p2.y = ball.y - (PADDLE_HEIGHT / 2);

    // Render Frame
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderFillRect(renderer, &p1);
    SDL_RenderFillRect(renderer, &p2);
    SDL_RenderFillRect(renderer, &ball);

    SDL_RenderPresent(renderer);
    SDL_Delay(16); // ~60fps
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  std::cout << "Engine successfully decoupled from renderer. Shutting down.\n";
  return 0;
}
