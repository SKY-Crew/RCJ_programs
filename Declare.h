#ifndef CECLARE_ORIG
#define CECLARE_ORIG

const bool IS_SKY = true;
bool canRun;
bool prvCanRun;

bool prvChangeRole = false;
bool isFW = IS_SKY;
bool prvIsFW;
Count cBecomeFW(100, false);

Count cCorrectRot(3);

Count cEnemyStandsFront(8, false);

bool carryingBall = false;
bool willCarryBall = false;
uint32_t timeStartCB;

Count cCatchFreely;

#endif