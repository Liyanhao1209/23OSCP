#ifndef REPLACE_H
#define REPLACE_H

#include "system.h"
#include "list.h"

// push/pop new/old refs in the refStk
void refPush(int ref);

void refPop(int ref);

// calculate which page is currently in main memory to replace
int calVictim();

#endif //REPLACE_H
