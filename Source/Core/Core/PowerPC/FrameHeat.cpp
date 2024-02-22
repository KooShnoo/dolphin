#include "Core/PowerPC/FrameHeat.h"
#include <algorithm>
#include <numeric>
#include <optional>
#include <utility>
#include "Common/CommonTypes.h"
#include "Common/Logging/Log.h"
#include "Core/PowerPC/JitCommon/JitBase.h"
#include "Core/PowerPC/JitCommon/JitCache.h"
#include "Core/PowerPC/JitInterface.h"
#include "Core/PowerPC/MMU.h"
#include "Core/System.h"
#include "VideoCommon/Present.h"

FrameHeatMap::FrameHeatMap()
{
  // this is an arbitrary number. id idn;t thnik before typing it too much.
  this->reserve(0x400 - 4);
  this->push_back(FrameHeat(g_presenter->FrameCount(), 0));
}

std::optional<u16> FrameHeatMap::Heat(const u32 addr, const u16 framenum)
{
  auto pFrameHeat = GetFrameHeatMap(addr);
  if (!pFrameHeat)
    return std::nullopt;
  auto& frameHeat = *pFrameHeat.value().get();
  return std::partition_point(frameHeat.begin(), frameHeat.end(),
                              [framenum](const auto& l) { return l.heat != framenum; })
      ->heat;
  // return std::make_optional(frameHeat(framenum));
}

std::optional<u32> FrameHeatMap::TotalHeat(const u32 addr)
{
  auto frameHeat = GetFrameHeatMap(addr);
  return frameHeat ? std::make_optional(frameHeat.value().get()->TotalHeat()) : std::nullopt;
}

std::optional<FrameHeatMapPtr> FrameHeatMap::GetFrameHeatMap(const u32 addr)
{
  return Core::System::GetInstance().GetJitInterface().GetFrameHeat(addr);
}

// void FrameHeatMap::AddNewFrame(u16 current_frame)
// {
//   auto soga = Core::System::GetInstance().GetJitInterface().m_jit->GetBlockCache();
//   soga->RunOnBlocks([current_frame](const JitBlock& b) {
//     if (!b.profile_data.frameHeatMap)
//     {
//       NOTICE_LOG_FMT(POWERPC, "not caching block with no symbol data @{:#x}", b.effectiveAddress);
//       return;
//     }
//     auto& frameHeatMap = *b.profile_data.frameHeatMap;
//     frameHeatMap.push_back(FrameHeat(current_frame, 0));
//     frameHeatMap.m_curent_frame_hit = &frameHeatMap.back().heat;
//   });
// }

u32 FrameHeatMap::TotalHeat()
{
  if (!m_totalHeat.has_value())
    m_totalHeat = std::accumulate(begin(), end(), 0, [](const auto sum, const auto frameHeat) {
      return sum + frameHeat.heat;
    });
  return m_totalHeat.value();
}

// makes a funcitonHeatMap from a blocHeatMap
// TODO make it work! :)
std::map<u32, FrameHeatMap> FrameHeatMap::MakeRealMaps()
{
  std::map<u32, FrameHeatMap> funcFHMap;

  // fill up funcFHMap
  auto soga = Core::System::GetInstance().GetJitInterface().m_jit->GetBlockCache();
  soga->RunOnBlocks([&funcFHMap](const JitBlock& b) {
    if (!b.profile_data.funPtr)
    {
      NOTICE_LOG_FMT(POWERPC, "ignoring block with no symbol data @{:#x}", b.effectiveAddress);
      return;
    }
    u32 funcAddr = b.profile_data.funPtr->address;
    auto blockFrameHeat = *b.profile_data.frameHeatMap;
    // try insert if we don't have that func's FH data yet
    auto [_, res] = funcFHMap.insert(std::make_pair(funcAddr, blockFrameHeat));
    if (res)
      return;
    NOTICE_LOG_FMT(POWERPC, "yikes! we got a two blocks for function ...");
    // we have two blocks in a function, merge the two blocks' data by summing their heats
    std::transform(funcFHMap[funcAddr].begin(), funcFHMap[funcAddr].end(), blockFrameHeat.begin(),
                   funcFHMap[funcAddr].begin(), std::plus<FrameHeat>());
  });
  m_funcFHMap = funcFHMap;
  return funcFHMap;
}
