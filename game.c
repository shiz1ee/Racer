#include <stdio.h>
#include <raylib.h>

#define CAR_COLOR BLACK
int main() {
    int width = 1250;
    int height = 720;
    InitWindow(width, height, "Racer");

    // car shape
    int car_x = width/2-30;
    int car_y = height/2-50;
    int car_W = 60;
    int car_H = 100;
    float car_speed = 0;
    float car_max_speed = 1000;

    while( ! WindowShouldClose() ) {
        float dt = GetFrameTime();


        BeginDrawing();
        ClearBackground( PINK );
        if (IsKeyDown(KEY_UP) ) {
            car_speed  += 100 * dt;
            if (car_speed > car_max_speed) {
                car_speed = car_max_speed;
            }
        } else {
            car_speed -= 30 * dt;
            if (car_speed < 0) {
                car_speed = 0;
            }
        }

        car_y -= car_speed;


        DrawRectangle(car_x, car_y, car_W, car_H, CAR_COLOR);
        EndDrawing();
    }

    CloseWindow();
    printf("hello world\n");
    return 0;
}