#ifndef CECLARE_ORIG
#define CECLARE_ORIG

const bool IS_SKY = true;
bool canRun;
bool prvCanRun;

bool prvChangeRole = false;
bool isFW = IS_SKY;
bool prvIsFW;
Count cBecomeFW(100, false);

uint16_t BORDER_IF;
uint16_t BORDER_IC;

Count cCorrectRot(3);
uint16_t BORDER_INCREASE_CCR;
uint16_t BORDER_DECREASE_CCR;

Count cEnemyStandsFront(8, false);

bool carryingBall = false;
bool willCarryBall = false;
const bool BORDER_CONTINUE_CARRY = 0.3;
uint32_t timeStartCB;

Count cCatchFreely;

#endif