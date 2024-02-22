// Copyright 2008 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "Common/CommonTypes.h"
#include "Core/PowerPC/FrameHeat.h"

namespace Profiler
{
struct BlockStat
{
  BlockStat(u32 _addr, u64 c, u64 ticks, u64 run, u32 size, FrameHeatMapPtr hm)
      : addr(_addr), cost(c), tick_counter(ticks), run_count(run), block_size(size), frameHeatMap(hm)
  {
  }
  u32 addr;
  u64 cost;
  u64 tick_counter;
  u64 run_count;
  u32 block_size;
  FrameHeatMapPtr frameHeatMap;

  bool operator<(const BlockStat& other) const { return cost > other.cost; }
};
struct ProfileStats
{
  std::vector<BlockStat> block_stats;
  u64 cost_sum = 0;
  u64 timecost_sum = 0;
  u64 countsPerSec = 0;
};

}  // namespace Profiler
