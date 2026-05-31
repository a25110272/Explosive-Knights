#include "raylib.h"
#include <string>

enum Direction {
    DOWN,
    UP,
    RIGHT,
    LEFT
};

struct Player {
    Vector2 position;
    Direction direction;
    bool moving;

    int currentFrame;
    float frameTimer;
    float frameSpeed;

    float speed;
};

std::string GetAssetPath(const std::string& path) {
    if (FileExists(path.c_str())) {
        return path;
    }

    std::string altPath = "../" + path;
    if (FileExists(altPath.c_str())) {
        return altPath;
    }

    return path;
}

int main() {
    const int screenWidth = 1000;
    const int screenHeight = 700;

    InitWindow(screenWidth, screenHeight, "Knight Blast Arena - Movimiento");
    SetTargetFPS(60);

    Texture2D walkDown = LoadTexture(GetAssetPath("assets/images/rojo_frente.png").c_str());
    Texture2D walkUp = LoadTexture(GetAssetPath("assets/images/rojo_espalda.png").c_str());
    Texture2D walkRight = LoadTexture(GetAssetPath("assets/images/rojo_derecha.png").c_str());
    Texture2D walkLeft = LoadTexture(GetAssetPath("assets/images/rojo_izquierda.png").c_str());

    if (walkDown.id == 0) {
        TraceLog(LOG_ERROR, "No se pudo cargar assets/images/rojo_frente.png");
    }

    if (walkUp.id == 0) {
        TraceLog(LOG_ERROR, "No se pudo cargar assets/images/rojo_espalda.png");
    }

    if (walkRight.id == 0) {
        TraceLog(LOG_ERROR, "No se pudo cargar assets/images/rojo_derecha.png");
    }

    if (walkLeft.id == 0) {
        TraceLog(LOG_ERROR, "No se pudo cargar assets/images/rojo_izquierda.png");
    }

    Player player;
    player.position = { 400, 300 };
    player.direction = DOWN;
    player.moving = false;
    player.currentFrame = 0;
    player.frameTimer = 0.0f;
    player.frameSpeed = 0.15f;
    player.speed = 180.0f;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        player.moving = false;

        if (IsKeyDown(KEY_W)) {
            player.position.y -= player.speed * dt;
            player.direction = UP;
            player.moving = true;
        }

        if (IsKeyDown(KEY_S)) {
            player.position.y += player.speed * dt;
            player.direction = DOWN;
            player.moving = true;
        }

        if (IsKeyDown(KEY_A)) {
            player.position.x -= player.speed * dt;
            player.direction = LEFT;
            player.moving = true;
        }

        if (IsKeyDown(KEY_D)) {
            player.position.x += player.speed * dt;
            player.direction = RIGHT;
            player.moving = true;
        }

        if (player.moving) {
            player.frameTimer += dt;

            if (player.frameTimer >= player.frameSpeed) {
                player.frameTimer = 0.0f;
                player.currentFrame++;

                if (player.currentFrame > 3) {
                    player.currentFrame = 0;
                }
            }
        } else {
            player.currentFrame = 0;
            player.frameTimer = 0.0f;
        }

        Texture2D currentTexture = walkDown;

        if (player.direction == DOWN) {
            currentTexture = walkDown;
        } else if (player.direction == UP) {
            currentTexture = walkUp;
        } else if (player.direction == RIGHT) {
            currentTexture = walkRight;
        } else if (player.direction == LEFT) {
            currentTexture = walkLeft;
        }

        int frameCount = 4;
        float frameWidth = (float)currentTexture.width / frameCount;
        float frameHeight = (float)currentTexture.height;

        Rectangle sourceRec = {
            frameWidth * player.currentFrame,
            0,
            frameWidth,
            frameHeight
        };

        Rectangle destRec = {
            player.position.x,
            player.position.y,
            frameWidth * 2.0f,
            frameHeight * 2.0f
        };

        Vector2 origin = {
            destRec.width / 2.0f,
            destRec.height / 2.0f
        };

        BeginDrawing();

        ClearBackground(Color{ 35, 35, 45, 255 });

        DrawText("Mueve al caballero rojo con W A S D", 20, 20, 24, WHITE);

        DrawTexturePro(
            currentTexture,
            sourceRec,
            destRec,
            origin,
            0.0f,
            WHITE
        );

        EndDrawing();
    }

    UnloadTexture(walkDown);
    UnloadTexture(walkUp);
    UnloadTexture(walkRight);
    UnloadTexture(walkLeft);

    CloseWindow();

    return 0;
}