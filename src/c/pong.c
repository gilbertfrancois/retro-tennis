//    Retro Tennis
//    Copyright (C) 2022  Gilbert Francois Duivesteijn
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "pong.h"
#include "SDL2/SDL_render.h"
#include "SDL2/SDL_video.h"

void handle_events(SDL_Window *window, GameState *g, bool *running) {
    SDL_Event event;
    const Uint8 *keystates = SDL_GetKeyboardState(NULL);
    // Handle keyboard events
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_KEYUP) {
            if (!keystates[SDL_SCANCODE_UP] && g->left_paddle.kb_vy < 0) {
                g->left_paddle.kb_vy = 0;
                g->mouse.needs_warp = true;
            }
            if (!keystates[SDL_SCANCODE_DOWN] && g->left_paddle.kb_vy > 0) {
                g->left_paddle.kb_vy = 0;
                g->mouse.needs_warp = true;
            }
        }
        if (event.type == SDL_KEYDOWN) {
            if (keystates[SDL_SCANCODE_UP]) {
                g->left_paddle.kb_vy = -g->paddle_speed;
                g->mouse.needs_warp = true;
            }
            if (keystates[SDL_SCANCODE_DOWN]) {
                g->left_paddle.kb_vy = g->paddle_speed;
                g->mouse.needs_warp = true;
            }
        }
        if (event.type == SDL_QUIT)
            *running = false;
        if (keystates[SDL_SCANCODE_ESCAPE])
            *running = false;
        if (keystates[SDL_SCANCODE_Q])
            *running = false;
    }
    // Allow mouse and keyboard input at the same time. Keep the mouse position
    // in sync.
    if (g->mouse.needs_warp) {
        SDL_WarpMouseInWindow(window, g->display_w / 2,
                              g->left_paddle.rect.y +
                                  g->left_paddle.rect.h / 2);
    }
    int x = 0;
    int y = 0;
    SDL_GetMouseState(&x, &y);
    if (!g->mouse.needs_warp && g->left_paddle.kb_vy == 0) {
        g->left_paddle.ms_vy = (y - g->mouse.y);
    } else {
        g->left_paddle.ms_vy = 0;
    }
    if (g->mouse.needs_warp) {
        g->mouse.needs_warp = false;
    }
    g->mouse.y = y;
}

// Update display size in case the user has changed it.
void update_display_size(SDL_Window *window, GameState *g) {
    SDL_GetWindowSize(window, &g->display_w, &g->display_h);
    g->pixel_w = g->display_w / g->retro_disp_w;
    g->pixel_h = g->display_h / g->retro_disp_h;
    g->ball_speed = g->pixel_w;
    g->paddle_speed = g->pixel_w * 2;
}

void update(SDL_Window *window, GameState *g) {
    // Update paddle position
    g->left_paddle.rect.y += g->left_paddle.kb_vy + g->left_paddle.ms_vy;
    // Update ball position
    g->ball.rect.x += g->ball.vx;
    g->ball.rect.y += g->ball.vy;
    // Robot player
    if (g->ball.rect.y >
        g->right_paddle.rect.y + (g->right_paddle.rect.h / 2)) {
        g->right_paddle.rect.y += g->ball_speed;
    }
    if (g->ball.rect.y <
        g->right_paddle.rect.y + (g->right_paddle.rect.h / 2)) {
        g->right_paddle.rect.y -= g->ball_speed;
    }
    // Prevent the paddles from going off-screen
    if (g->left_paddle.rect.y > (g->display_h - g->left_paddle.rect.h)) {
        g->left_paddle.rect.y = (g->display_h - g->left_paddle.rect.h);
    }
    if (g->left_paddle.rect.y < 0) {
        g->left_paddle.rect.y = 0;
    }
    if (g->right_paddle.rect.y > (g->display_h - g->right_paddle.rect.h)) {
        g->right_paddle.rect.y = (g->display_h - g->right_paddle.rect.h);
    }
    if (g->right_paddle.rect.y < 0) {
        g->right_paddle.rect.y = 0;
    }
    // Bounce agaist horizontal walls.
    if (g->ball.rect.y <= 0) {
        g->ball.vy = -g->ball.vy;
        g->ball.rect.y = 0;
    }
    if (g->ball.rect.y >= (g->display_h - g->ball.rect.h)) {
        g->ball.vy = -g->ball.vy;
        g->ball.rect.y = g->display_h - g->ball.rect.h;
    }
    // Point scored by one of the players.
    if (g->ball.rect.x <= 0) {
        g->right_score++;
        g->ball.rect.x = g->display_w / 2 - g->ball.rect.w / 2;
        g->ball.rect.y = g->right_paddle.rect.y - g->right_paddle.rect.h / 2 +
                         g->ball.rect.h / 2;
        g->ball.vy = ((rand() % 2) - 1) * g->ball_speed;
        g->ball.vx = -g->ball_speed;
    }
    if (g->ball.rect.x >= (g->display_w - g->ball.rect.w)) {
        g->left_score++;
        g->ball.rect.x = g->display_w / 2 - g->ball.rect.w / 2;
        g->ball.rect.y = g->left_paddle.rect.y - g->left_paddle.rect.h / 2 +
                         g->ball.rect.h / 2;
        g->ball.vy = ((rand() % 2) - 1) * g->ball_speed;
        g->ball.vx = g->ball_speed;
    }
    // Collision detection
    if (SDL_HasIntersection(&g->ball.rect, &g->left_paddle.rect)) {
        int rel = (g->left_paddle.rect.y + g->left_paddle.rect.h / 2) -
                  (g->ball.rect.y + g->ball.rect.h / 2);
        g->ball.vy = -rel / g->pixel_h * g->ball_speed;
        g->ball.vx = -g->ball.vx;
        g->ball.rect.x = g->left_paddle.rect.x + g->left_paddle.rect.w;
    }
    if (SDL_HasIntersection(&g->ball.rect, &g->right_paddle.rect)) {
        int rel = (g->right_paddle.rect.y + g->right_paddle.rect.h / 2) -
                  (g->ball.rect.y + g->ball.rect.h / 2);
        g->ball.vy = -rel / g->pixel_h * g->ball_speed;
        g->ball.vx = -g->ball.vx;
        g->ball.rect.x = g->right_paddle.rect.x - g->ball.rect.w;
    }
}

void draw(SDL_Renderer *renderer, GameState *g) {
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xff);
    SDL_RenderClear(renderer);
    /* g->right_paddle.rect.y += 1; */

    // Set the current render color.
    SDL_SetRenderDrawColor(renderer, g->color.r, g->color.g, g->color.b, 255);
    SDL_RenderFillRect(renderer, &g->left_paddle.rect);
    SDL_RenderFillRect(renderer, &g->right_paddle.rect);
    SDL_RenderFillRect(renderer, &g->ball.rect);
    // Render net
    SDL_Rect block;
    block.w = g->pixel_w / 4;
    block.h = g->pixel_h;
    block.x = g->display_w / 2 - block.w / 2;
    for (int i = 0; i < g->display_h; i += 3 * g->pixel_h) {
        block.y = i;
        SDL_RenderFillRect(renderer, &block);
    }
    SDL_RenderPresent(renderer);
}

/* void drawLabel(SDL_Renderer *renderer, TTF_Font *font, SDL_Color color, */
/*                char *text, int x, int y) { */
/*     SDL_Surface *surface; */
/*     SDL_Texture *texture; */
/*     surface = TTF_RenderText_Solid(font, text, color); */
/*     texture = SDL_CreateTextureFromSurface(renderer, surface); */
/*     score_board.w = surface->w; */
/*     score_board.h = surface->h; */
/*     score_board.x = x - surface->w / 2; */
/*     score_board.y = y; */
/*     SDL_FreeSurface(surface); */
/*     SDL_RenderCopy(renderer, texture, NULL, &score_board); */
/*     SDL_DestroyTexture(texture); */
/* } */

void init_game_state(GameState *g) {
    g->retro_disp_w = 100;
    g->retro_disp_h = 80;
    g->fullscreen = true;
    g->display_w = 500;
    g->display_h = 400;
    g->pixel_w = g->display_w / g->retro_disp_w;
    g->pixel_h = g->display_h / g->retro_disp_h;
    g->ball_speed = g->pixel_w;
    g->paddle_speed = g->pixel_w * 2;
    g->font_size = 32;
    g->color.r = 255;
    g->color.g = 255;
    g->color.b = 255;
    g->left_paddle.rect.w = g->pixel_w;
    g->left_paddle.rect.h = g->pixel_h * 5;
    g->left_paddle.rect.x = 9 * g->pixel_w;
    g->left_paddle.rect.y = g->display_h / 2 - g->left_paddle.rect.h / 2;
    g->left_paddle.kb_vy = 0;
    g->right_paddle.rect.w = g->left_paddle.rect.w;
    g->right_paddle.rect.h = g->left_paddle.rect.h;
    g->right_paddle.rect.x =
        g->display_w - g->left_paddle.rect.x - g->right_paddle.rect.w;
    g->right_paddle.rect.y = g->left_paddle.rect.y;
    g->right_paddle.kb_vy = 0;
    g->ball.rect.w = g->pixel_w;
    g->ball.rect.h = g->pixel_h;
    g->ball.rect.x = g->display_w / 2 - g->ball.rect.w / 2;
    g->ball.rect.y = g->display_h / 2 - g->ball.rect.h / 2;
    g->ball.vx = -g->ball_speed;
    g->ball.vy = rand() % 2 - 1;
}

int init_mouse(SDL_Window *window, GameState *g) {
    SDL_WarpMouseInWindow(window, g->display_w / 2, g->display_h / 2);
    g->mouse.needs_warp = false;
    SDL_GetMouseState(&g->mouse.x, &g->mouse.y);
    return 0;
}

int main(int argc, char *argv[]) {
    // Init variables
    SDL_Renderer *renderer;
    SDL_Window *window;
    GameState g;
    init_game_state(&g);

    // time sync variables
    int frame_count = 0;
    int timer_fps = 0;
    int prev_time = 0;
    int time = 0;
    int fps = 0;
    bool running = false;

    // Init Audio and video. Bail out with an error if it fails.
    int status = SDL_Init(SDL_INIT_VIDEO);
    if (status < 0) {
        printf("Failed at SDL_Init()");
        exit(status);
    }
    status = SDL_CreateWindowAndRenderer(g.display_w, g.display_h, 0, &window, &renderer);
    if (g.fullscreen) {
        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
    }
    update_display_size(window, &g);
    SDL_DisplayMode current;
    SDL_GetCurrentDisplayMode(0, &current);
    printf("Resolution: (%d, %d)\n", current.w, current.h);
    printf("Pixel format: %d\n", current.format);
    printf("Refresh rate: %d\n", current.refresh_rate);
    if (status < 0) {
        printf("Failed at SDL_CreateWindowAndRenderer()\n");
        exit(status);
    }

    // Init font
    TTF_Init();
    g.font = TTF_OpenFont("atari.ttf", g.font_size);
    // Init mouse
    init_mouse(window, &g);
    // Begin event loop
    running = true;
    while (running) {
        time = SDL_GetTicks();
        if (time >= (prev_time + 1000)) {
            prev_time = time;
            fps = frame_count;
            frame_count = 0;
            printf("FPS: %d\n", fps);
        }
        update_display_size(window, &g);
        handle_events(window, &g, &running);
        update(window, &g);
        draw(renderer, &g);

        frame_count++;
        timer_fps = SDL_GetTicks() - time;
        if (timer_fps < (1000 / 60)) {
            SDL_Delay((1000 / 60) - timer_fps);
        }
    }
    // Exit gracefully.
    TTF_CloseFont(g.font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
