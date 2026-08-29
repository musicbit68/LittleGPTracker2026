#ifndef _SEND_BUSES_H_
#define _SEND_BUSES_H_

#include "T_SendBus.h"
#include "DelayEffect.h"
#include "ChorusEffect.h"
#include "ReverbEffect.h"

// Three independent, parallel send buses - one per effect. Each is summed
// into master on its own (see PlayerMixer::Init), so they behave like three
// separate aux sends on a mixing console rather than one serial FX chain.
typedef T_SendBus<ChorusEffect> ChorusBus;
typedef T_SendBus<DelayEffect> DelayBus;
typedef T_SendBus<ReverbEffect> ReverbBus;

#endif
