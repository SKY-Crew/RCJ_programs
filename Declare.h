#ifndef DECLARE_ORIG
#define DECLARE_ORIG

bool canRun;
bool prvCanRun;

bool prvChangeRole = false;
bool prvIsFW = isFW;
Count cBecomeFW(100, false);

// Count cCorrectRot(3);

bool carryingBall = false;
bool willCarryBall = false;
uint64_t timeStartCB;
uint16_t powerCB;

Count cLineForward(90);
Side sideLF = CENTER;

Count cLineBackward(45);

#endif