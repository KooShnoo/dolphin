// Copyright 2014 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#if !defined(_WIN32)
#include "Common/PerformanceCounter.h"

#include <cstdint>
#include <ctime>

#include <unistd.h>

#include "Common/CommonTypes.h"
#include "VideoCommon/Present.h"

#if defined(_POSIX_TIMERS) && _POSIX_TIMERS > 0
#if defined(_POSIX_MONOTONIC_CLOCK) && _POSIX_MONOTONIC_CLOCK > 0
#define DOLPHIN_CLOCK CLOCK_MONOTONIC
#else
#define DOLPHIN_CLOCK CLOCK_REALTIME
#endif
#endif

bool QueryPerformanceCounter(u64* out)
{
#if defined(_POSIX_TIMERS) && _POSIX_TIMERS > 0
  timespec tp;
  if (clock_gettime(DOLPHIN_CLOCK, &tp))
    return false;
  *out = (u64)tp.tv_nsec + (u64)1000000000 * (u64)tp.tv_sec;
  return true;
#else
  *out = 0;
  return false;
#endif
}

bool QueryPerformanceFrequency(u64* out)
{
#if defined(_POSIX_TIMERS) && _POSIX_TIMERS > 0
  *out = 1000000000;
  return true;
#else
  *out = 1;
  return false;
#endif
}

// JIT hook for recording the frames where a function is run. Used for the flamegraph feature
// Ya boi KooShnoo is here, at it again! this is my funciton i wrote so that whenever a block is run,
// it records when it was run. then, we store into a data structure. this is for a feautre
// where yyou can see on which frame a function was tun and how often. and also what funciotns
// were run on a agiven frame and how mcuh
// SUPER SLOWDOWN MOMENT SUPER SLOW LAG LAG !! I WROTE A LAG MACHINE!! 
// this should run literally on every JIY vlovk call
void AddPerformanceFrame(JitBlock* bloc)
{
  if (!bloc->profile_data.frameHeatMap)
    return;
  auto& frameHeatMap = *bloc->profile_data.frameHeatMap;

  if (frameHeatMap.back().frame != g_presenter->FrameCount())
    frameHeatMap.push_back(FrameHeat(g_presenter->FrameCount(), 0));
  frameHeatMap.back().heat++;
}

#endif
