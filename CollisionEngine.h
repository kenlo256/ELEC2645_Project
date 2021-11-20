#ifndef COLLISIONENGINE_H
#define COLLISIONENGINE_H

#include <cstdint>
#include <vector>

#include "mbed.h"

typedef struct {
    uint8_t r;
    // position on the screen (84x48)
    float x;
    float y;
    float Vx;  // verticle velocity
    float Vy;  // horizontal velocity
    uint8_t id;
} Particle;

typedef struct {
    std::vector<Particle> pl;  // Box
    uint8_t n;                 // number of particles
} Box;

Box* getBox();
void refresh_param();
// void runSimulation();
void spawnParticle(const int i, const Particle mc);
void collisionHandler();
bool isColliding(const Particle& p1, const Particle& p2);
void mainCharDesPar(const Particle mc);
bool mainCharCollidePar(const Particle mc);
const uint32_t customRand(const uint8_t digits);
#endif