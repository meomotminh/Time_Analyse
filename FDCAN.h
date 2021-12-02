#ifndef FDCAN_H
#define FDCAN_H

#include "mbed.h"


void Set_Freq(can_t , const can_pinmap_t *, int);

void Set_CAN_Pin(can_t *, PinName , PinName , int );

void Trd_internal_init(can_t *);


int my_can_mode(can_t *, CanMode );


#endif