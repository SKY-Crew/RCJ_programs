#ifndef DECLARE_ORIG
#define DECLARE_ORIG

const bool IS_SKY = true;
bool canRun;
bool prvCanRun;

const int64_t WAIT = 8500;

bool prvChangeRole = false;
bool isFW = IS_SKY;
bool prvIsFW;
Count cBecomeFW(100, false);

Count cCorrectRot(3);

bool carryingBall = false;
bool willCarryBall = false;
uint32_t timeStartCB;

Count cCatchFreely;

Count cLineForward(60);

#endif