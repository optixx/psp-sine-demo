#ifndef FC_H
#define FC_H

#include "mytypes.h"
#include "myendian.h"
#include "lamepaula.h"

extern bool FC_init(void *, udword, int, int);
extern void FC_play();
extern void FC_off();

extern bool FC_songEnd;

#endif // FC_H
