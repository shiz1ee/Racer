#include <stdio.h>
#include <raylib.h>
#include <math.h>
#include <stdlib.h>

#define CAR_COLOR BLACK
#define ROATION_SPEED 10
#define RIVER_HALF 200

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

int GetShipFrame(float rotation) {
    int frame = (int)roundf((90.0f - rotation) / 22.5f);
    frame = ((frame % 16 ) + 16) % 16;
    return frame;
}

unsigned int HashCell(int cx, int cy) {
    unsigned int h = (unsigned int) cx * 374761393u + (unsigned int)cy * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

float river_p1, river_p2, river_f1, river_f2;



void InitRiver(unsigned int seed) {
    river_p1 = (HashCell(seed, 1) % 6283) / 1000.0f;
    river_p2 = (HashCell(seed, 2) % 6283) / 1000.0f;
    river_f1 = 0.0010f + (HashCell(seed, 3) % 100) * 0.000004f;
    river_f2 = 0.0029f + (HashCell(seed, 4) % 100) * 0.000006f;
}

float RiverCenterX(float y) {
    float d = 300.0f - y;
    if (d < 0) d = 0;
    float blend = d / 3000.0f;
    if (blend > 1) blend = 1;
    return blend * (350.0f * sinf(y * river_f1 + river_p1) + 100.0f * sinf(y * river_f2 + river_p2));
}

int PickSwampTile(unsigned int r, float dist, int *tx, int *ty) {
    if (r % 100 >= 18) return 0;
    unsigned int k = (r >> 8) % 100;
    if (dist > 0.7f) {
        if (k < 30) { *tx = 4; *ty = (r >> 16) % 2; }
        else if (k < 60) { *tx = 2; *ty = 1 + (r >> 16) % 2; }
        else if (k < 90) { *tx = 3; *ty = (r >> 16) % 2; }
        else if (k < 95) { *tx = 3; *ty = 2; }
    } else {
        if (k < 40) { *tx = 1; *ty = 1 + (r >> 16) % 2; }
        else if (k < 70) { *tx = 2; *ty = 0; }
        else if (k < 90) { *tx = 1; *ty = 0; }
        else { *tx = 3; *ty = ( r >> 16) % 2; }
    }
    return 1;
}

void DrawRiver(Camera2D cam, int screen_h, Texture2D tex) {
    float top = cam.target.y - cam.offset.y;
    int first_row = (int)floorf(top / 32) - 1;
    int last_row = (int)floorf((top + screen_h) / 32) + 1;

    for (int row = first_row; row <= last_row; row++) {
        float cx = RiverCenterX(row * 32 + 16);
        int first_col = (int)floorf((cx - RIVER_HALF) / 32);
        int last_col = (int)floorf((cx + RIVER_HALF) / 32);
        for (int col = first_col; col <= last_col; col++) {
            unsigned int h = HashCell(col, row);
            float flip_x = (h & 1) ? -16.0f : 16.0f;
            float flip_y = (h & 2) ? -16.0f : 16.0f;
            int tx = 0, ty  = (h >> 4) % 3;
            float dist = fabsf(col * 32 + 16 - cx) / RIVER_HALF;
            if (PickSwampTile(HashCell(col + 5000, row + 5000), dist, &tx, &ty)) {
                flip_y = 16.0f;
            }
            Rectangle src = { 112 + tx * 16, 48 + ty * 16, flip_x, flip_y};
            Rectangle dst = { col * 32.0f, row * 32.0f, 32, 32 };
            DrawTexturePro(tex, src, dst, (Vector2){ 0, 0 }, 0.0f, WHITE);
        }
    }
    
}

void DrawForest(Camera2D cam, int screen_w, int screen_h, Texture2D tex) {
    int cell = 60;
    float left = cam.target.x - cam.offset.x;
    float top = cam.target.y - cam.offset.y;
    int first_x = (int)floorf(left / cell ) -2;
    int last_x = (int)floorf((left + screen_w) / cell) + 2;
    int first_y = (int)floorf(top / cell) - 2;
    int last_y = (int)floorf((top + screen_h) / cell) + 2;

    for (int cy = first_y; cy <= last_y; cy++) {
        for (int cx = first_x; cx <= last_x; cx++) {
            float world_x = cx * cell;
            if (fabsf(world_x - RiverCenterX(cy * cell)) < RIVER_HALF + 40) continue;
            unsigned int h = HashCell(cx, cy);
            //if ((h >> 4) % 10 < 2) continue;
            int variant = h % 4;
            float off_x = (float)((h >> 8) % 40) -20;
            float off_y = (float)((h >> 16) % 40) -20;
            float flip = ((h >> 24) & 1) ? -48.0f : 48.0f;
            Rectangle src = { variant * 48.0f, 0, flip, 48 };
            Rectangle dst = { world_x + off_x, cy * cell + off_y, 96, 96 };
            DrawTexturePro(tex, src, dst, (Vector2){48, 48 }, 0.0f, WHITE);
        }
    }
}

void DrawDarkness(Texture2D light_tex, int w, int h, float size) {
    Color dark = { 0, 0, 0, 235};
    float cx =  w / 2.0f, cy = h / 2.0f;
    float l = cx - size / 2, r = cx + size / 2;
    float t = cy - size / 2, b = cy + size / 2;
    float ty = t < 0 ? 0 : t;
    float by = b > h ? h : b;

    Rectangle src = { 0, 0, (float)light_tex.width, (float)light_tex.height };
    Rectangle dst = { l, t, size, size };
    DrawTexturePro(light_tex, src, dst, (Vector2){0, 0 }, 0.0f, WHITE);
    
    if (t > 0) DrawRectangle(0, 0, w, (int)t, dark);
    if (b < h) DrawRectangle(0, (int)b, w, h - (int)b, dark);
    if (l > 0) DrawRectangle(0, (int)ty, (int)l, (int)(by - ty), dark);
    if (r < w) DrawRectangle((int)r, (int)ty, w - (int)r, (int)(by - ty), dark);
}

int main() {
    int width = 1250;
    int height = 720;
    InitWindow(width, height, "Racer");
    SetTargetFPS(60);
    Texture2D ship_tex = LoadTexture("assets/sprPlayer_strip16.png");
    Texture2D tree_tex = LoadTexture("assets/48x48_trees.png");
    Texture2D water_tex = LoadTexture("assets/Water+.png");

    Image light_img = GenImageGradientRadial(512, 512, 0.3f, BLANK, (Color){0, 0, 0, 235});
    Texture2D light_tex = LoadTextureFromImage(light_img);
    UnloadImage(light_img);
    SetTextureFilter(light_tex, TEXTURE_FILTER_BILINEAR);

    InitRiver(2026);


    //shape
    float car_x = 0;
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
        ClearBackground( (Color){18, 30, 22, 255 } );
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

        if (IsKeyPressed(KEY_LEFT) && fabsf(car_speed) > 0.5f) {
            car_rotation -= 22.5f;
            } else if (IsKeyPressed(KEY_RIGHT) && fabsf(car_speed) > 0.5f ) {
                car_rotation += 22.5f;
            }

        if ( car_rotation < 0) {
            car_rotation += 360;
        }


        float radians = PI * (90.0f - GetShipFrame(car_rotation) * 22.5f) / 180;
        float x_move = car_speed * cosf(radians);
        float y_move = car_speed * sinf(radians);

        car_x -= x_move;
        car_y -= y_move;
        float river_cx = RiverCenterX(car_y);
        float limit = RIVER_HALF - 40;
        float off = car_x - river_cx;
        if (off > limit) {
            car_x = river_cx + limit;
            car_speed *= 0.95f;
        } else if (off < -limit) {
            car_x = river_cx - limit;
            car_speed *= 0.95f;
        }
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
        //DrawWorldGrid(camera, width, height); 
        DrawRiver(camera, height, water_tex);
        DrawForest(camera, width, height, tree_tex);
        //DrawRectanglePro(car_rec, car_origin, car_rotation, CAR_COLOR); i no need u 
        int frame = GetShipFrame(car_rotation);
        Rectangle ship_src = { frame * 64.0f, 0, 64, 64 };
        Rectangle ship_dst = { car_x, car_y, 128, 128 };
        Vector2 ship_origin = { 64, 64 };
        DrawTexturePro(ship_tex, ship_src, ship_dst, ship_origin, 0.0f, WHITE); 
        EndMode2D();
        DrawDarkness(light_tex, width, height, 900 + 25 * sinf(GetTime() * 2));
        // remove ts
        DrawText(TextFormat("Speed: %.1f", car_speed), 10, 10, 20, BLACK);
        DrawText(TextFormat("Heading: %.1f", car_rotation), 10, 35, 20, BLACK);
        DrawText(TextFormat("Frame: %d", GetShipFrame(car_rotation)), 10, 60, 20, BLACK);
        EndDrawing();
    }

    UnloadTexture(ship_tex);
    UnloadTexture(water_tex);
    UnloadTexture(light_tex);
    CloseWindow();
    printf("hello world\n");
    return 0;
}