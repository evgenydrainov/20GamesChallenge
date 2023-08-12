#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>

#include "minicoro.h"

#define GAME_W 1066
#define GAME_H 800
#define GAME_FPS 60
// #define PLAYER_ACC 0.3f
#define PLAYER_ACC 0.5f
#define PLAYER_MAX_SPD 10.0f
#define PLAYER_FOCUS_SPD 3.0f
// #define PLAYER_TURN_SPD 4.0f
#define PLAYER_TURN_SPD 6.0f
#define MAX_ENEMIES 1000
#define MAX_BULLETS 1000
#define MAX_PLR_BULLETS 1000
#define ArrayLength(a) (sizeof(a) / sizeof(*a))
#define MAP_W 10'000.0f
#define MAP_H 10'000.0f
#define ASTEROID_RADIUS_3 50.0f
#define ASTEROID_RADIUS_2 25.0f
#define ASTEROID_RADIUS_1 12.0f

enum {
    INPUT_RIGHT = 1,
    INPUT_UP    = 1 << 1,
    INPUT_LEFT  = 1 << 2,
    INPUT_DOWN  = 1 << 3,
    INPUT_FIRE  = 1 << 4,
    INPUT_FOCUS = 1 << 5
};

struct Player {
    float x;
    float y;
    float hsp;
    float vsp;
    float dir;
    bool focus;
    float power;
};

struct Enemy {
    float x;
    float y;
    float hsp;
    float vsp;
    float radius = 10.0f;
    int type;
    float hp = 1.0f;
    SDL_Texture* texture;
    float power = 1.0f;
    float angle;
    mco_coro* co;

    union {
        struct { // used by type 10
            float catch_up_timer;
        };
    };
};

struct Bullet {
    float x;
    float y;
    float hsp;
    float vsp;
    float radius = 5.0f;
    float dmg = 1.0f;
    float lifespan = 2.0f * 60.0f;
    float lifetime;
};

struct Game {
    Player player;
    float camera_x;
    float camera_y;
    Enemy* enemies;
    int enemy_count;
    Bullet* bullets;
    int bullet_count;
    Bullet* p_bullets; // player bullets
    int p_bullet_count;
    unsigned int input;
    unsigned int input_press;
    unsigned int input_release;
    mco_coro* co;

    SDL_Texture* tex_player_ship;
    SDL_Texture* tex_bg;
    SDL_Texture* tex_bg1;
    SDL_Texture* tex_asteroid1;
    SDL_Texture* tex_asteroid2;
    SDL_Texture* tex_asteroid3;
    SDL_Texture* tex_moon;

    TTF_Font* fnt_mincho;

    SDL_Window* window;
    SDL_Renderer* renderer;
    double prev_time;
    bool quit;
    double fps_sum;
    double fps_timer;
    SDL_Texture* fps_texture;
    float interface_update_t;
    SDL_Texture* interface_texture;
    SDL_Texture* interface_map_texture;
    bool hide_interface;

    void Init();
    void Quit();
    void Run();
    void Frame();

    Enemy* CreateEnemy();
    Bullet* CreateBullet();
    Bullet* CreatePlrBullet();

    void DestroyEnemy(int enemy_idx);
    void DestroyBullet(int bullet_idx);
    void DestroyPlrBullet(int p_bullet_idx);
};

extern Game* game;
