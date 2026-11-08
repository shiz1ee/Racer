#include <stdio.h>
#include <raylib.h>
#include <math.h>
#include <stdlib.h>

#define CAR_COLOR BLACK
#define ROATION_SPEED 20
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
    int car_direction = -1;
    float car_rotation = 90;
    float car_speedup = 10;
    float car_slowdown = 20;

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
            if (car_speed > car_max_speed) {
                car_speed = car_max_speed;
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
                printf("Rotation: %f\n", car_rotation);
            }

        if ( car_rotation >= 360) {
            car_rotation = 0;
        }


        float radians = PI * car_rotation / 180;
        float x_move = car_speed * cosf(radians);
        float y_move = car_speed * sinf(radians);

        car_x -= x_move;
        car_y -= y_move;

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
        DrawRectanglePro(car_rec, car_origin, car_rotation, CAR_COLOR);
        EndDrawing();
    }

    CloseWindow();
    printf("hello world\n");
    return 0;
}