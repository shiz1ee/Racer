#include <stdio.h>
#include <raylib.h>
#include <math.h>
#include <stdlib.h>

#define CAR_COLOR BLACK
#define ROATION_SPEED 10
#define RIVER_HALF 200
#define RAMP_LEN 100000.0f
#define BR_SLOT 6000.0f

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

float river_p1, river_p2, river_f1, river_f2, river_p3, river_f3;



void InitRiver(unsigned int seed) {
    river_p1 = (HashCell(seed, 1) % 6283) / 1000.0f;
    river_p2 = (HashCell(seed, 2) % 6283) / 1000.0f;
    river_f1 = 0.0010f + (HashCell(seed, 3) % 100) * 0.000004f;
    river_f2 = 0.0029f + (HashCell(seed, 4) % 100) * 0.000006f;
    river_p3 = (HashCell(seed, 5) % 6283) / 1000.0f;
    river_f3 = 0.0060f + (HashCell(seed, 6) % 100) * 0.00001f;
}

float RiverDifficulty(float y) {
    float t = (300.0f - y) / RAMP_LEN;
    if ( t < 0) t = 0;
    if (t > 1) t = 1;
    return t * t * (3.0f - 2.0f * t);
}

float RiverHalfAt(float y) {
    return RIVER_HALF - 110.0f * RiverDifficulty(y);
}

float RiverCenterX(float y) {
    float d = 300.0f - y;
    if (d < 0) d = 0;
    float blend = d / 3000.0f;
    if (blend > 1) blend = 1;
    float diff = RiverDifficulty(y);
    float amp = 1.0f + 0.6f * diff;
    return blend * amp * (350.0f * sinf(y * river_f1 + river_p1) + 100.0f * sinf(y * river_f2 + river_p2) + 50.0f * diff * sinf(y * river_f3 + river_p3));
}

typedef struct {
    float y_bot, len, side, dmax;
    int type;
} Branch;

int GetBranch(int slot, Branch *b) {
    unsigned int h = HashCell(slot, 4242);
    unsigned int h2 = HashCell(slot, 9191);
    float y_bot = slot * BR_SLOT + (float)(h % 600);
    if (y_bot > -2000.0f) return 0;
    if ((int)((h >> 8) % 100) >= 45 + (int)(RiverDifficulty(y_bot) * 35)) return 0;
    int t = (int)((h >> 16) % 100);
    b->type = t < 45 ? 0 : (t < 75 ? 1 : 2);
    b->side = ((h >> 20) & 1) ? 1.0f : -1.0f;
    b->y_bot = y_bot;
    b->len = (b->type == 0 ? 3500.0f : 3000.0f) + (float)(h2 % 1400);
    b->dmax = 600.0f + (float)((h2 >> 16) % 200) + (b->type == 2 ? 300.0f : 0.0f);
    return 1;
}

int BranchChannel(Branch *b, float y, float *cx, float *half) {
    float u = (b->y_bot - y) / b->len;
    if (u < 0 || u > 1) return 0;
    float w = RiverHalfAt(y) * 0.7f;
    float d;
    if (b->type == 0) {
        d = b->dmax * sinf(3.14159f * u);
    } else {
        float t = fminf(u / 0.45f, 1.0f);
        d = b->dmax * t * t * (3.0f - 2.0f * t);
        float e = fminf(fmaxf((u - 0.85f) / 0.15f, 0.0f), 1.0f);
        if (b->type == 2) {
            float g = fminf(fmaxf((u - 0.55f) / 0.3f, 0.0f), 1.0f);
            w *= 1.0f + 0.9f * g * g * (3.0f - 2.0f * g);
        }
        w *= 1.0f - 0.9f * e;
    }
    *cx = RiverCenterX(y) + b->side * d;
    *half = w;
    return 1;
}

int GetChannels(float y, float *cx, float *half) {
    cx[0] = RiverCenterX(y);
    half[0] = RiverHalfAt(y);
    int q = (int)floorf(y / BR_SLOT);
    for (int s = q; s <= q + 1; s++) {
        Branch b;
        if (GetBranch(s, &b) && BranchChannel(&b, y, &cx[1], &half[1])) return 2;
    }
    return 1;
}

float WaterEdgeDist(float x, float y) {
    float cx[2], half[2];
    int n = GetChannels(y, cx, half);
    float best = 1e9f;
    for (int i = 0; i < n; i++) {
        float e = fabsf(x - cx[i]) - half[i];
        if (e < best) best = e;
    }
    return best;
}

int NearWater(float x, float y, float margin) {
    return WaterEdgeDist(x, y) < margin;
}

void ClampToWater(float *x, float *y, float old_x, float old_y, float *speed) {
    float cx[2], half[2];
    int n = GetChannels(*y, cx, half);
    float lo = cx[0] - half[0], hi = cx[0] + half[0];
    if (n == 2) {
        float lo2 = cx[1] - half[1], hi2 = cx[1] + half[1];
        if (lo2 <= hi && hi2 >= lo) {
            lo = fminf(lo, lo2);
            hi = fmaxf(hi, hi2);
        } else {
            float d0 = fmaxf(fmaxf(lo - *x, *x - hi), 0.0f);
            float d1 = fmaxf(fmaxf(lo2 - *x, *x - hi2), 0.0f);
            if (d1 < d0) { lo = lo2; hi = hi2; }
        }
    }
    if (*x < lo - 40.0f || *x > hi + 40.0f) {
        *x = old_x;
        *y = old_y;
        *speed *= 0.8f;
        return;
    }
    float a = lo + 40.0f, b = hi - 40.0f;
    if (a > b ) { a = (lo + hi) / 2.0f; b = a;}
    if (*x < a) { *x = a; *speed *= 0.95f; }
    else if (*x > b) { *x = b; *speed *= 0.95f;}
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

int GetIsland(int slot, float *ix, float *iy, float *ir) {
    unsigned int h = HashCell(slot, 777);
    float y = slot * 360.0f + (h % 200);
    float diff = RiverDifficulty(y);
    if ((int)((h >> 8) % 100) >= (int)(diff * 70)) return 0;
    float room = RiverHalfAt(y) - 100.0f;
    float off = ((float)((h >> 16) % 1000) / 500.0f - 1.0f) * room;
    *ix = RiverCenterX(y) + off;
    *iy = y;
    int kind = ((h >> 4) & 1) ? 2 : 1;
    *ir = (kind == 2) ? 36.0f : 22.0f;
    return kind;
}

void DrawIslands(Camera2D cam, int screen_h, Texture2D tex) {
    float top = cam.target.y - cam.offset.y;
    int first = (int)floorf(top / 360.0f) - 1;
    int last = (int)floorf((top + screen_h) / 360.0f) + 1;
    for (int s = first; s <= last; s++) {
        float ix, iy, ir;
        int kind = GetIsland(s, &ix, &iy, &ir);
        if (kind == 0) continue;
        Rectangle src = (kind == 2) ? (Rectangle){82, 6, 44, 41} : (Rectangle){130, 8, 28, 23};
        Rectangle dst = { ix, iy, src.width * 2, src.height * 2};
        DrawTexturePro(tex, src, dst, (Vector2){src.width, src.height }, 00.0f, WHITE);
    }
}

void DrawGrass(Camera2D cam, int screen_w, int screen_h, Texture2D tex) {
    Rectangle peices[4] = {
        { 6, 6, 36, 36 }, { 55, 23, 18, 19 }, { 87, 6, 34, 20 }, { 87, 39, 50, 51 }
    };
    int cell = 64;
    float left = cam.target.x - cam.offset.x;
    float top = cam.target.y - cam.offset.y;
    int first_x = (int)floorf(left / cell) -3;
    int last_x = (int)floorf((left + screen_w) / cell) + 3;
    int first_y = (int)floorf(top / cell) - 3;
    int last_y = (int)floorf((top + screen_h) / cell) + 3;

    for (int cy = first_y; cy <= last_y; cy++) {
        for (int cx = first_x; cx <= last_x; cx++) {
            unsigned int h = HashCell(cx + 9000, cy + 9000);
            float wx = cx * cell + (float)(h % 40);
            float wy = cy * cell + (float)((h >> 8) % 40);
            float edge = WaterEdgeDist(wx, wy);
            if (edge < -20.0f || edge > 260.0f) continue;
            if ((h >> 16) % 100 >= 35) continue;
            Rectangle src = peices[(h >> 24) % 4];
            Rectangle dst = { wx, wy, src.width * 2, src.height * 2};
            if ((h >> 20) & 1) src.width = -src.width;
            DrawTexturePro(tex, src, dst, (Vector2){ dst.width / 2, dst.height / 2}, 0.0f, WHITE);
        }
    }
}

void DrawRiver(Camera2D cam, int screen_h, Texture2D tex) {
    float top = cam.target.y - cam.offset.y;
    int first_row = (int)floorf(top / 32) - 1;
    int last_row = (int)floorf((top + screen_h) / 32) + 1;

    for (int row = first_row; row <= last_row; row++) {
        float chx[2], chh[2];
        int nch = GetChannels(row * 32 + 16, chx, chh);
        for (int c = 0; c < nch; c++) {
            float cx = chx[c];
            float half = chh[c];
            int first_col = (int)floorf((cx - half) / 32);
            int last_col = (int)floorf((cx + half) / 32);
            for (int col = first_col; col <= last_col; col++) {
                unsigned int h = HashCell(col, row);
                float flip_x = (h & 1) ? -16.0f : 16.0f;
                float flip_y = (h & 2) ? -16.0f : 16.0f;
                int tx = 0, ty = (h >> 4) % 3;
                float dist = fabsf(col * 32 + 16 - cx) / half;
                if (PickSwampTile(HashCell(col + 5000, row + 5000), dist, &tx, &ty)) {
                    flip_y = 16.0f;
                }
                Rectangle src = { 112 + tx * 16, 48 + ty * 16, flip_x, flip_y };
                Rectangle dst = { col * 32.0f, row * 32.0f, 32, 32 };
                DrawTexturePro(tex, src, dst, (Vector2){ 0, 0 }, 0.0f, WHITE);
            }
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
            if (NearWater(world_x, (float)(cy * cell), 40.0f)) continue;
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
    int s = (int)size;
    s -= s % 2;
    float l = (float)(w / 2 - s / 2), r = (float)(w / 2 + s / 2);
    float t = (float)(h / 2 - s / 2), b = (float)(w / 2 + s / 2);
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

    Image island_img = LoadImage("assets/riverbank_02.png");
    ImageColorReplace(&island_img, (Color){51, 97, 81, 255 }, BLANK);
    Texture2D island_tex = LoadTextureFromImage(island_img);
    UnloadImage(island_img);

    Texture2D grass_tex = LoadTexture("assets/Grass_transition_02.png");

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
        ClearBackground( (Color){53, 55, 36, 255 } );
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

        float old_x = car_x, old_y = car_y;
        car_x -= x_move;
        car_y -= y_move;
        ClampToWater(&car_x, &car_y, old_x, old_y, &car_speed);

        for (int s = (int)floorf(car_y / 360.0f) - 1; s <= (int)floorf(car_y / 360.0f) + 1; s++) {
            float ix, iy, ir;
            if (GetIsland(s, &ix, &iy, &ir)) {
                float dx = car_x - ix, dy = car_y - iy;
                float dist = sqrtf(dx * dx + dy * dy);
                float min_dist = ir + 25.0f;
                if (dist < min_dist && dist> 0.001f) {
                    car_x = ix + dx / dist * min_dist;
                    car_y = iy + dy / dist * min_dist;
                    car_speed *= 0.9f;
                }
            }
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
        DrawGrass(camera, width, height, grass_tex);
        DrawRiver(camera, height, water_tex);
        DrawIslands(camera, height, island_tex);
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
        DrawText(TextFormat("Speed: %.1f", car_speed), 10, 10, 20, WHITE);
        DrawText(TextFormat("Heading: %.1f", car_rotation), 10, 35, 20, WHITE);
        DrawText(TextFormat("Frame: %d", GetShipFrame(car_rotation)), 10, 60, 20, WHITE);
        DrawText(TextFormat("Difficulty: %.0f%%", RiverDifficulty(car_y) * 100), 10, 85, 20, WHITE);
                DrawText(TextFormat("Edge: %.0f", WaterEdgeDist(car_x, car_y)), 10, 110, 20, WHITE);
        EndDrawing();
    }

    UnloadTexture(ship_tex);
    UnloadTexture(water_tex);
    UnloadTexture(light_tex);
    UnloadTexture(island_tex);
    UnloadTexture(grass_tex);
    CloseWindow();
    printf("hello world\n");
    return 0;
}