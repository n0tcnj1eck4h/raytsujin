#include "raylib.h"
#include "osuProcessing.h"
#include "mainMenu.h"
#include "config.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

void DrawElementsPlaying();
void DrawPlayfield();
void SendNote(Note taikoNote);
void UpdateGamePlaying();
void RetryButton();

float songTimeElapsed;

int wasPressedLastFrame = 0;
int lastNoteTiming = 0; // Used for hit feedback images (miss, hit)
int isMiss = 0;
int isHit = 0; // TODO: 100s and 300s
int currentNote = 0;
float deltaPress = 0;

int comboCounter = 0;
int hitCounter = 0;
int missCounter = 0;

Texture2D mapBackground;
Texture2D taikoMiss;
Texture2D taikoHit;

float scrollFieldHeight = 100.0f; // Support for numbers different than 100 doesn't really work yet

const float hitWindow = 100.0f;

const int screenWidth = 1600;
const int screenHeight = 900;

const char* songsDirectory = "resources/Songs/";
char* pathToMap = "1610611 100 gecs - sympathy 4 the grinch/";
char* pathToDifficulty = "100 gecs - sympathy 4 the grinch (clockbite) [Muzukashii].osu";
char* fullMapPath;

Sound redSound;
Sound blueSound;
Sound comboBreak;
Music mapAudio;

Image icon;

int gameStateSwitch = 0;
int lastGameState = 0;

int main() {

    InitAudioDevice();

    InitWindow(screenWidth, screenHeight, "raytsujin");
    icon = LoadImage("resources/teri.png"); // TODO: Change this icon to something more fitting
    SetWindowIcon(icon);

    taikoMiss = LoadTexture("resources/taiko-hit0.png");
    taikoHit = LoadTexture("resources/taiko-hit300.png");

    redSound = LoadSound("resources/drum-hitfinish.wav");
    blueSound = LoadSound("resources/drum-hitclap.wav");
    comboBreak = LoadSound("resources/combobreak.wav");

    char* mapPathBuffer = malloc(strlen(songsDirectory) + strlen(pathToMap) + strlen(pathToDifficulty));
    strcpy(mapPathBuffer, songsDirectory);
    strcat(mapPathBuffer, pathToMap);
    strcat(mapPathBuffer, pathToDifficulty);
    fullMapPath = mapPathBuffer;

    StartOsuFileProcessing(fullMapPath);

    char* mapAudioBuffer = malloc(strlen(songsDirectory) + strlen(pathToMap) + strlen(beatmap.audioFileName));
    strcpy(mapAudioBuffer, songsDirectory);
    strcat(mapAudioBuffer, pathToMap);
    strcat(mapAudioBuffer, beatmap.audioFileName);

    mapAudio = LoadMusicStream(mapAudioBuffer);

    char* mapBackgroundBuffer = malloc(strlen(songsDirectory) + strlen(pathToMap) + strlen(beatmap.backgroundFileName) + 1);
    strcpy(mapBackgroundBuffer, songsDirectory);
    strcat(mapBackgroundBuffer, pathToMap);
    strcat(mapBackgroundBuffer, beatmap.backgroundFileName);
    mapBackground = LoadTexture(mapBackgroundBuffer);

    while(!WindowShouldClose()) {
        if(gameStateSwitch == Menu) {
            DrawMainMenu();
            UpdateMainMenu();
            lastGameState = Menu;
        }
        else if(gameStateSwitch == Playing) {
            if(lastGameState != Playing) {
                PlayMusicStream(mapAudio);
            }
            songTimeElapsed = GetMusicTimePlayed(mapAudio) * 1000;
            lastGameState = Playing;
            UpdateMusicStream(mapAudio);
            UpdateGamePlaying();
            DrawElementsPlaying();
        }
    }

    CloseAudioDevice();

    UnloadTexture(mapBackground);
    UnloadTexture(taikoHit);
    UnloadTexture(taikoMiss);

    free(beatmap.audioFileName);
    free(beatmap.backgroundFileName);
    free(mapPathBuffer);
    free(mapBackgroundBuffer);
    free(mapAudioBuffer);

    CloseWindow();

    return 0;
}

void DrawElementsPlaying() {
    BeginDrawing();
    
    ClearBackground(RAYWHITE);
    DrawPlayfield();

    for (int i = 0; i < noteCounter; i++)
    {
        SendNote(Notes[i]);

        Notes[i].position.x = 100 + Notes[i].timing - songTimeElapsed;
    }    

    if(isMiss && songTimeElapsed - lastNoteTiming < 300) {
        DrawTexture(taikoMiss, -70, -50, WHITE); // TODO: Animate this (fade in fade out or scale)
    }
    if(isHit && songTimeElapsed - lastNoteTiming < 300) {
        DrawTexture(taikoHit, -90, -40, WHITE); // TODO: Animate this (fade in fade out or scale)
    }


    DrawFPS(2, 0);
    DrawText(TextFormat("Misses: %i", missCounter), 2, screenHeight - 80, 16, BLACK);
    DrawText(TextFormat("Hits: %i", hitCounter), 2, screenHeight - 60, 16, BLACK);
    DrawText(TextFormat("%ix", comboCounter), 2, screenHeight - 40, 48, BLACK);

    EndDrawing();
}

void DrawPlayfield() {
    DrawTexture(mapBackground, 0, 0, WHITE);
    DrawRectangleGradientH(0, 0, screenWidth, scrollFieldHeight, LIGHTGRAY, BLACK);
    DrawRectangleGradientV(0, screenHeight - 100, 100, 100, LIGHTGRAY, GRAY);
    DrawCircle(50, scrollFieldHeight / 2, scrollFieldHeight / 2, BLACK);

}

void SendNote(Note taikoNote) {
    DrawCircleV(taikoNote.position, scrollFieldHeight / 2, taikoNote.noteColor);
}

void UpdateGamePlaying() {
    int timingProper = Notes[currentNote].timing + 100;

    deltaPress = fabsf(timingProper - songTimeElapsed);

    if((timingProper < songTimeElapsed - hitWindow)) {
        // If no note pressed in time
        comboCounter = 0;
        PlaySound(comboBreak);
        currentNote++;
        missCounter++;
    }
    // Blue note logic
    else if(Notes[currentNote].isBlue) {
        if((IsKeyPressed(KEY_D) || IsKeyPressed(KEY_K)) && (deltaPress < hitWindow) && Notes[currentNote].isPressed == 0 && !wasPressedLastFrame) {
            // If note is hit
            Notes[currentNote].isPressed = 1;
            Notes[currentNote].noteColor = Fade(BLUE, 0.1f);

            lastNoteTiming = timingProper;
            isMiss = 0;
            isHit = 1;
            comboCounter++;
            wasPressedLastFrame = 1;
            currentNote++;
            hitCounter++;
        } else if((IsKeyPressed(KEY_F) || IsKeyPressed(KEY_J)) && (deltaPress < hitWindow) && Notes[currentNote].isPressed == 0 && !wasPressedLastFrame) {
            // If wrong note is pressed

            PlaySound(comboBreak);

            Notes[currentNote].isPressed = 1;
            Notes[currentNote].noteColor = BLANK;

            lastNoteTiming = timingProper;
            isMiss = 1;
            isHit = 0;
            comboCounter = 0;
            wasPressedLastFrame = 1;
            currentNote++;
            missCounter++;
        }
    // Red note logic
    } else {
        if((IsKeyPressed(KEY_F) || IsKeyPressed(KEY_J)) && (deltaPress < hitWindow) && Notes[currentNote].isPressed == 0 && !wasPressedLastFrame) {
            // If note is hit
            Notes[currentNote].isPressed = 1;
            Notes[currentNote].noteColor = Fade(RED, 0.1f);

            lastNoteTiming = timingProper;
            isMiss = 0;
            isHit = 1;
            comboCounter++;
            wasPressedLastFrame = 1;
            currentNote++;
            hitCounter++;
        } else if((IsKeyPressed(KEY_D) || IsKeyPressed(KEY_K)) && (deltaPress < hitWindow) && Notes[currentNote].isPressed == 0 && !wasPressedLastFrame) {
            // If wrong note is pressed

            PlaySound(comboBreak);

            Notes[currentNote].isPressed = 1;
            Notes[currentNote].noteColor = BLANK;

            lastNoteTiming = timingProper;
            isMiss = 1;
            isHit = 0;
            comboCounter = 0;
            wasPressedLastFrame = 1;
            currentNote++;
            missCounter++;
        }
    }
    if(GetKeyPressed() == 0) {
        wasPressedLastFrame = 0;
    }
    if(IsKeyDown(KEY_D) || IsKeyDown(KEY_K)) {
        DrawText("BLUE PRESSED", 150, 200, 16, BLUE);
    }
    if(IsKeyDown(KEY_F) || IsKeyDown(KEY_J)) {
        DrawText("RED PRESSED", 150, 220, 16, RED);
    }
    if(IsKeyPressed(KEY_D) || IsKeyPressed(KEY_K)) {
        PlaySound(blueSound);
    }
    if(IsKeyPressed(KEY_F) || IsKeyPressed(KEY_J)) {
        PlaySound(redSound);
    }
    if(IsKeyPressed(KEY_GRAVE)) {
        RetryButton();
    }
}

void RetryButton() {
    if(IsMusicStreamPlaying(mapAudio)) {
        StopMusicStream(mapAudio);
        PlayMusicStream(mapAudio);

        for(int i = 0; i < noteCounter; i++) {
            Notes[i].isPressed = 0;
            Notes[i].noteColor = Notes[i].isBlue?BLUE:RED;
        }

        comboCounter = 0;
        missCounter = 0;
        hitCounter = 0;
        isMiss = 0;
        isHit = 0;
        lastNoteTiming = 0;
        currentNote = 0;
    }
}