#include <algorithm>
#include <bitset>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <utility>

#include "CollisionEngine.h"

#define BOLTZMANNCONSTANT 1.3806503e-23
#define MAXPARTICLESIZE 5
#define WIDTH 84
#define HEIGHT 48
#define PI 3.14159

AnalogIn ldr(p15);
AnalogIn tmp36(p16);
AnalogIn joy_button(p17);
Box box;

std::vector<std::pair<Particle, Particle>> stillCollPar;

const uint32_t customRand(const uint8_t digits);
void initBoxEnv();
void SweepNPrune();
void destructWall(uint8_t x, uint8_t y);
const int randNeg() {
    int mod = customRand(1) % 2;
    // return a value that is either 1 or -1
    return (1 - mod * 2);
}

bool isOverlappedX(const Particle& p1, const Particle& p2) {
    uint8_t p2LowerBound = p2.x - p2.r;
    uint8_t p1UpperBound = p1.x + p1.r;

    if (p1UpperBound >= p2LowerBound) return true;

    return false;
}

bool isOverlappedXMainChar(const Particle& p1, const Particle& p2) {
    uint8_t p2LowerBound = p2.x - p2.r;
    uint8_t p1UpperBound = p1.x + p1.r;

    uint8_t p2UpperBound = p2.x + p2.r;
    uint8_t p1LowerBound = p1.x - p1.r;

    if (p1.x < p2.x) {
        if (p1UpperBound >= p2LowerBound) return true;
    } else {
        if (p2UpperBound >= p1LowerBound) return true;
    }

    return false;
}

bool isOverlappedY(const Particle& p1, const Particle& p2) {
    uint8_t p1UpperBound = p1.y - p1.r;
    uint8_t p2LowerBound = p2.y + p2.r;

    uint8_t p2UpperBound = p2.y - p2.r;
    uint8_t p1LowerBound = p1.y + p1.r;

    if (p1.y < p2.y) {
        if (p1LowerBound >= p2UpperBound) return true;
    } else {
        if (p2LowerBound >= p1UpperBound) return true;
    }

    return false;
}

void collisionBoundary(Particle& p) {
    if (p.x >= (WIDTH - 1) - p.r) {
        p.x = (WIDTH - 1) - p.r;
        p.Vx = -p.Vx;
    }

    if (p.x <= p.r) {
        p.x = p.r;
        p.Vx = -p.Vx;
    }

    if (p.y >= (HEIGHT - 1) - p.r) {
        p.y = (HEIGHT - 1) - p.r;
        p.Vy = -p.Vy;
    }

    if (p.y <= p.r) {
        p.y = p.r;
        p.Vy = -p.Vy;
    }
}

bool StillCollidingParticle(const Particle& p1, const Particle& p2) {
    if (stillCollPar.empty())
        return false;
    else
        return stillCollPar.end() !=
               std::find_if(
                   stillCollPar.begin(), stillCollPar.end(),
                   [&p1, &p2](const std::pair<Particle, Particle> p) {
                       return (p.first.id == p1.id && p.second.id == p2.id) ||
                              (p.second.id == p1.id && p.first.id == p2.id);
                   });
}

void P2PCollisonMovement(const uint8_t i, const uint8_t j) {
    std::swap(box.pl.at(i).Vx, box.pl.at(j).Vx);
    std::swap(box.pl.at(i).Vy, box.pl.at(j).Vy);
    stillCollPar.push_back(make_pair(box.pl.at(i), box.pl.at(j)));
}

bool isColliding(const Particle& p1, const Particle& p2) {
    return (isOverlappedX(p1, p2) && isOverlappedY(p1, p2));
}

bool main_character_isColliding(const Particle& p1, const Particle& p2) {
    return (isOverlappedXMainChar(p1, p2) && isOverlappedY(p1, p2));
}

void SweepNPrune() {
    std::sort(
        box.pl.begin(), box.pl.end(),
        [](const Particle& p1, const Particle& p2) { return p1.x < p2.x; });

    if (!stillCollPar.empty()) {
        for (uint8_t i = 0; i < box.n - 1; i++) {
            if (!isColliding(box.pl.at(i), box.pl.at(i + 1))) {
                // https://stackoverflow.com/questions/3385229/c-erase-vector-element-by-value-rather-than-by-position
                stillCollPar.erase(
                    std::remove_if(
                        stillCollPar.begin(), stillCollPar.end(),
                        [i](const std::pair<Particle, Particle> p) {
                            return (p.first.id == box.pl.at(i).id &&
                                    p.second.id == box.pl.at(i + 1).id) ||
                                   (p.second.id == box.pl.at(i).id &&
                                    p.first.id == box.pl.at(i + 1).id);
                        }),
                    stillCollPar.end());
            }
        }
        stillCollPar.shrink_to_fit();
    }

    for (uint8_t i = 0; i < box.n - 1; i++) {
        if (box.pl.at(i).x == box.pl.at(i + 1).x && i + 2 < box.n - 1) {
            if (isColliding(box.pl.at(i), box.pl.at(i + 2)) &&
                !StillCollidingParticle(box.pl.at(i), box.pl.at(i + 2)))
                P2PCollisonMovement(i, i + 2);
            if (isColliding(box.pl.at(i + 1), box.pl.at(i + 2)) &&
                !StillCollidingParticle(box.pl.at(i + 1), box.pl.at(i + 2)))
                P2PCollisonMovement(i + 1, i + 2);
            i = i + 2;
        }
        if (isColliding(box.pl.at(i), box.pl.at(i + 1)) &&
            !StillCollidingParticle(box.pl.at(i), box.pl.at(i + 1)))
            P2PCollisonMovement(i, i + 1);
    }
}

void spawnParticle(const int i, const Particle mc) {
    // reason for this 2 value is const because it only need to initialize once
    // for effiency and space for the ram
    const float voltage = 3.3f * tmp36.read();
    const float T = 100.0f * voltage - 50.0f + 273.15f;
    const float v = (2e-03f) * sqrt(2 * ((BOLTZMANNCONSTANT * T) / (4.8e-26)));
    Particle p;

    do {
        p.x = customRand(2);
    } while (p.x > mc.x + 12 && p.x < mc.x - 12 &&
             p.x < WIDTH - (MAXPARTICLESIZE - 1) &&
             p.x > (MAXPARTICLESIZE - 1) && p.x < 77);

    do {
        p.y = customRand(2);
    } while (p.y > mc.y + 12 && p.y < mc.y - 12 &&
             p.y < HEIGHT - (MAXPARTICLESIZE - 1) &&
             p.y > (MAXPARTICLESIZE - 1) && p.y < 41);

    p.id = i;

    srand(customRand(1));
    p.r = rand() % (MAXPARTICLESIZE - 2) + 2;

    srand(customRand(4));
    p.Vx = randNeg() * (rand() % 8192) / 1e+04;
    p.Vy = randNeg() * sqrt(v * v + (p.Vx) * (p.Vx));

    box.pl.push_back(p);
}

void refresh_param() {
    if (box.n > 1) SweepNPrune();
    std::for_each(box.pl.begin(), box.pl.end(), [](Particle& p) {
        p.x += p.Vx;
        p.y += p.Vy;
        collisionBoundary(p);
    });
}

Box* getBox() { return &box; }
// a customised random number genrator
const uint32_t customRand(const uint8_t digits) {
    float ldrVal = ldr.read();
    float tmpVal = tmp36.read();
    float joybutVal = joy_button.read();

    std::bitset<32> ldrBit(*reinterpret_cast<unsigned long*>(&ldrVal));
    std::bitset<32> tmpBit(*reinterpret_cast<unsigned long*>(&tmpVal));
    std::bitset<32> joybutBit(*reinterpret_cast<unsigned long*>(&joybutVal));
    std::bitset<32> processedBit = ldrBit << 24 ^ tmpBit << 32 & joybutBit
                                                                     << 28;
    processedBit = processedBit << 4;

    float adcMixVal = processedBit.to_ullong();
    adcMixVal = adcMixVal * ldr.read() * tmp36.read() * joy_button.read();
    srand(adcMixVal);
    adcMixVal = (rand() / pow(10, ceil((log10(adcMixVal))) - digits));
    return adcMixVal;
}

void mainCharDesPar(const Particle mc) {
    if (!stillCollPar.empty()) {
        stillCollPar.erase(
            std::remove_if(stillCollPar.begin(), stillCollPar.end(),
                           [mc](const std::pair<Particle, Particle> p) {
                               return main_character_isColliding(mc, p.first) ||
                                      main_character_isColliding(mc, p.second);
                           }),
            stillCollPar.end());
        stillCollPar.shrink_to_fit();
    }

    if (!box.pl.empty()) {
        box.pl.erase(std::remove_if(box.pl.begin(), box.pl.end(),
                                    [mc](Particle p) {
                                        if (main_character_isColliding(p, mc)) {
                                            box.n = box.n - 1;
                                        }
                                        return main_character_isColliding(p,
                                                                          mc);
                                    }),
                     box.pl.end());
        box.pl.shrink_to_fit();
    }
}

bool mainCharCollidePar(const Particle mc) {
    return box.pl.end() !=
           std::find_if(box.pl.begin(), box.pl.end(), [mc](const Particle p) {
               return main_character_isColliding(p, mc);
           });
}