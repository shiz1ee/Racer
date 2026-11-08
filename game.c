#include <stdio.h>
#include <raylib.h>
#include <math.h>
#include <stdlib.h>

#define CAR_COLOR BLACK
#define ROATION_SPEED 10

void DrawWorldGrid(Camera2D cam, int screen_w, int screen_h) {
    int cell = 100;
    float left = cam.target.x - cam.offset.x;
    float top = cam.target.y - cam.offset.y;
    int start_x = (int)floorf(left / cell) * cell;
    int start_y = (int)floorf(top / cell) * cell;

    for (int x = start_x; x <= left + screen_w; x += cell) {
        DrawLine(x, (int)top, x, (int)top + screen_h, WHITE);
    }
    for (int y = start_y; y <= top + screen_h; y += cell) {
        DrawLine((int)left, y, (int)left + screen_w, y, WHITE);
    }
}

int main() {
    int width = 1250;
    int height = 720;
    InitWindow(width, height, "Racer");
    SetTargetFPS(60);

    // car shape
    float car_x = width/2-30;
    float car_y = height/2-50;
    int car_W = 60;
    int car_H = 100;
    float car_speed = 0;
    float car_max_speed = 10;
    float car_max_downspeed = -10;
    int car_direction = -1;
    float car_rotation = 90;
    float car_speedup = 10;
    float car_slowdown = 10;

    //kamera
    Camera2D camera = { 0 };
    camera.offset = (Vector2) {width/2.0f, height/2.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    while( ! WindowShouldClose() ) {
        float dt = GetFrameTime();


        BeginDrawing();
        ClearBackground( PINK );
        if (IsKeyDown(KEY_UP) ) {
            car_direction = -1;
            car_speed  += car_speedup * dt;
            if (car_speed > car_max_speed) {
                car_speed = car_max_speed;
            }
        } else if (IsKeyDown(KEY_DOWN) ) {
            car_direction = 1;
            car_speed  -= car_speedup * dt;
            if (car_speed < car_max_downspeed ) {
                car_speed = car_max_downspeed;
            }
        } else {
            car_speed += car_slowdown * dt * car_direction;
            if (car_direction == -1 && car_speed < 0) {
                car_speed = 0;
            } else if ( car_direction == 1 && car_speed > 0) {
                car_speed = 0;
            }
        }

        if (IsKeyDown(KEY_LEFT) ) {
            car_rotation -= ROATION_SPEED * dt * abs( car_speed );
            } else if (IsKeyDown(KEY_RIGHT) ) {
                car_rotation += ROATION_SPEED * dt * abs( car_speed );
            }

        if ( car_rotation >= 360) {
            car_rotation = 0;
        }


        float radians = PI * car_rotation / 180;
        float x_move = car_speed * cosf(radians);
        float y_move = car_speed * sinf(radians);

        car_x -= x_move;
        car_y -= y_move;
        camera.target = (Vector2) { car_x, car_y };

        Rectangle car_rec = {
            .x = car_x,
            .y = car_y,
            .width = car_H,
            .height = car_W,
        };

        Vector2 car_origin = {
            .x = car_H/2,
            .y = car_W/2,
        };
        BeginMode2D(camera);
        DrawWorldGrid(camera, width, height);
        DrawRectanglePro(car_rec, car_origin, car_rotation, CAR_COLOR);
        EndMode2D();
        // remove ts
        DrawText(TextFormat("Speed: %.1f", car_speed), 10, 10, 20, BLACK);
        DrawText(TextFormat("Heading: %.0f", car_rotation), 10, 35, 20, BLACK);
        EndDrawing();
    }

    CloseWindow();
    printf("hello world\n");
    return 0;
}