#ifndef DECLARE_ORIG
#define DECLARE_ORIG

const bool IS_SKY = true;
bool canRun;
bool prvCanRun;

bool prvChangeRole = false;
bool isFW = IS_SKY;
bool prvIsFW;
Count cBecomeFW(100, false);

Count cCorrectRot(3);

bool carryingBall = false;
bool willCarryBall = false;
uint64_t timeStartCB;

Count cLineForward(60);

#endif