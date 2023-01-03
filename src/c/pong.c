#include "pong.h"
#include "SDL2/SDL_render.h"
#include "SDL2/SDL_video.h"

void handle_events(SDL_Window *window, Mouse *mouse, Paddle *left_paddle,
                   Paddle *right_paddle, bool *running) {
    SDL_Event event;
    const Uint8 *keystates = SDL_GetKeyboardState(NULL);
    // Handle keyboard events
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_KEYUP) {
            if (!keystates[SDL_SCANCODE_UP] && left_paddle->kb_vy < 0) {
                left_paddle->kb_vy = 0;
                mouse->needs_warp = true;
            }
            if (!keystates[SDL_SCANCODE_DOWN] && left_paddle->kb_vy > 0) {
                left_paddle->kb_vy = 0;
                mouse->needs_warp = true;
            }
        }
        if (event.type == SDL_KEYDOWN) {
            if (keystates[SDL_SCANCODE_UP]) {
                left_paddle->kb_vy = -PADDLE_SPEED;
                mouse->needs_warp = true;
            }
            if (keystates[SDL_SCANCODE_DOWN]) {
                left_paddle->kb_vy = PADDLE_SPEED;
                mouse->needs_warp = true;
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
    if (mouse->needs_warp) {
        SDL_WarpMouseInWindow(window, display_w / 2,
                              left_paddle->rect.y + left_paddle->rect.h / 2);
    }
    int x = 0;
    int y = 0;
    SDL_GetMouseState(&x, &y);
    if (!mouse->needs_warp && left_paddle->kb_vy == 0) {
        left_paddle->ms_vy = (y - mouse->y);
    } else {
        left_paddle->ms_vy = 0;
    }
    if (mouse->needs_warp) {
        mouse->needs_warp = false;
    }
    mouse->y = y;
}

// Update display size in case the user has changed it.
void update_display_size(SDL_Window *window) {
    SDL_GetWindowSize(window, &display_w, &display_h);
    pixel_w = display_w / retro_disp_w;
    pixel_h = display_h / retro_disp_h;
    BALL_SPEED = pixel_w;
    PADDLE_SPEED = pixel_w * 2;
}

void update(SDL_Window *window, Mouse *mouse, Ball *ball, Paddle *left_paddle,
            Paddle *right_paddle) {

    char _score[] = "0  0";
    score = _score;
    // Update paddle position
    left_paddle->rect.y += left_paddle->kb_vy + left_paddle->ms_vy;
    // Update ball position
    ball->rect.x += ball->vx;
    ball->rect.y += ball->vy;
    // Robot player
    if (ball->rect.y > right_paddle->rect.y + (right_paddle->rect.h / 2)) {
        right_paddle->rect.y += BALL_SPEED;
    }
    if (ball->rect.y < right_paddle->rect.y + (right_paddle->rect.h / 2)) {
        right_paddle->rect.y -= BALL_SPEED;
    }
    // Prevent the paddles from going off-screen
    if (left_paddle->rect.y > (display_h - left_paddle->rect.h)) {
        left_paddle->rect.y = (display_h - left_paddle->rect.h);
    }
    if (left_paddle->rect.y < 0) {
        left_paddle->rect.y = 0;
    }
    if (right_paddle->rect.y > (display_h - right_paddle->rect.h)) {
        right_paddle->rect.y = (display_h - right_paddle->rect.h);
    }
    if (right_paddle->rect.y < 0) {
        right_paddle->rect.y = 0;
    }
    // Bounce agaist horizontal walls.
    if (ball->rect.y <= 0) {
        ball->vy = -ball->vy;
        ball->rect.y = 0;
    }
    if (ball->rect.y >= (display_h - ball->rect.h)) {
        ball->vy = -ball->vy;
        ball->rect.y = display_h - ball->rect.h;
    }
    // Point scored by one of the players.
    if (ball->rect.x <= 0) {
        right_score++;
        ball->rect.x = display_w / 2 - ball->rect.w / 2;
        ball->rect.y =
            right_paddle->rect.y - right_paddle->rect.h / 2 + ball->rect.h / 2;
        ball->vy = ((rand() % 2) - 1) * BALL_SPEED;
        ball->vx = -BALL_SPEED;
    }
    if (ball->rect.x >= (display_w - ball->rect.w)) {
        left_score++;
        ball->rect.x = display_w / 2 - ball->rect.w / 2;
        ball->rect.y =
            left_paddle->rect.y - left_paddle->rect.h / 2 + ball->rect.h / 2;
        ball->vy = ((rand() % 2) - 1) * BALL_SPEED;
        ball->vx = BALL_SPEED;
    }
    // Collision detection
    if (SDL_HasIntersection(&ball->rect, &left_paddle->rect)) {
        int rel = (left_paddle->rect.y + left_paddle->rect.h / 2) -
                  (ball->rect.y + ball->rect.h / 2);
        ball->vy = -rel / pixel_h * BALL_SPEED;
        ball->vx = -ball->vx;
        ball->rect.x = left_paddle->rect.x + left_paddle->rect.w;
    }
    if (SDL_HasIntersection(&ball->rect, &right_paddle->rect)) {
        int rel = (right_paddle->rect.y + right_paddle->rect.h / 2) -
                  (ball->rect.y + ball->rect.h / 2);
        ball->vy = -rel / pixel_h * BALL_SPEED;
        ball->vx = -ball->vx;
        ball->rect.x = right_paddle->rect.x - ball->rect.w;
    }
}

void draw(SDL_Renderer *renderer, TTF_Font *font, SDL_Color color, Ball *ball,
          Paddle *left_paddle, Paddle *right_paddle) {
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xff);
    SDL_RenderClear(renderer);
    /* right_paddle->rect.y += 1; */

    // Set the current render color.
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);
    // Draw the scores
    /* drawLabel(renderer, font, color, score, WIDTH / 2, 2 * pixel_h); */
    SDL_RenderFillRect(renderer, &left_paddle->rect);
    SDL_RenderFillRect(renderer, &right_paddle->rect);
    SDL_RenderFillRect(renderer, &ball->rect);
    // Render net
    SDL_Rect block;
    block.w = pixel_w / 4;
    block.h = pixel_h;
    block.x = display_w / 2 - block.w / 2;
    for (int i = 0; i < display_h; i += 3 * pixel_h) {
        block.y = i;
        SDL_RenderFillRect(renderer, &block);
    }
    SDL_RenderPresent(renderer);
}

void drawLabel(SDL_Renderer *renderer, TTF_Font *font, SDL_Color color,
               char *text, int x, int y) {
    SDL_Surface *surface;
    SDL_Texture *texture;
    surface = TTF_RenderText_Solid(font, text, color);
    texture = SDL_CreateTextureFromSurface(renderer, surface);
    score_board.w = surface->w;
    score_board.h = surface->h;
    score_board.x = x - surface->w / 2;
    score_board.y = y;
    SDL_FreeSurface(surface);
    SDL_RenderCopy(renderer, texture, NULL, &score_board);
    SDL_DestroyTexture(texture);
}

int init_mouse(SDL_Window *window, Mouse *mouse) {
    SDL_WarpMouseInWindow(window, display_w / 2, display_h / 2);
    mouse->needs_warp = false;
    SDL_GetMouseState(&mouse->x, &mouse->y);
    return 0;
}

int main(int argc, char *argv[]) {
    // Init variables
    SDL_Renderer *renderer;
    SDL_Window *window;
    TTF_Font *font;
    SDL_Color color;
    color.r = 255;
    color.g = 255;
    color.b = 255;
    Mouse mouse;
    Paddle left_paddle;
    left_paddle.rect.w = pixel_w;
    left_paddle.rect.h = pixel_h * 5;
    left_paddle.rect.x = 9 * pixel_w;
    left_paddle.rect.y = display_h / 2 - left_paddle.rect.h / 2;
    left_paddle.kb_vy = 0;
    Paddle right_paddle;
    right_paddle.rect.w = left_paddle.rect.w;
    right_paddle.rect.h = left_paddle.rect.h;
    right_paddle.rect.x = display_w - left_paddle.rect.x - right_paddle.rect.w;
    right_paddle.rect.y = left_paddle.rect.y;
    right_paddle.kb_vy = 0;
    Ball ball;
    ball.rect.w = pixel_w;
    ball.rect.h = pixel_h;
    ball.rect.x = display_w / 2 - ball.rect.w / 2;
    ball.rect.y = display_h / 2 - ball.rect.h / 2;
    ball.vx = -BALL_SPEED;
    ball.vy = rand() % 2 - 1;
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
    status = SDL_CreateWindowAndRenderer(display_w, display_h, 0, &window, &renderer);
    if (WITH_FULLSCREEN) {
        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
        SDL_DisplayMode current;
        SDL_GetCurrentDisplayMode(0, &current);
        printf("Resolution: (%d, %d)\n", current.w, current.h);
        printf("Pixel format: %d\n", current.format);
        printf("Refresh rate: %d\n", current.refresh_rate);
    }
    update_display_size(window);
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
    font = TTF_OpenFont("atari.ttf", FONT_SIZE);
    // Init mouse
    init_mouse(window, &mouse);
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
        update_display_size(window);
        handle_events(window, &mouse, &left_paddle, &right_paddle, &running);
        update(window, &mouse, &ball, &left_paddle, &right_paddle);
        draw(renderer, font, color, &ball, &left_paddle, &right_paddle);

        frame_count++;
        timer_fps = SDL_GetTicks() - time;
        if (timer_fps < (1000 / 60)) {
            SDL_Delay((1000 / 60) - timer_fps);
        }
    }
    // Exit gracefully.
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}