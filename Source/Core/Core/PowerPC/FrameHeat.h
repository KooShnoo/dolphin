// frame heat is a cringe thing i made, enjoy :) the name is stupid, basically it means its a map
// from a frame number, like the index of a frame, to another number, called heat or hits or
// executions, which is the amount of times the frameheatmap's target was run. so you can have a
// frameheatmap for a funciton, and itll tell you how many times the funciton executed on each
// frame. or a block, to see how many times it was run on a given frame. you can also (eventuall,
// once i write the code for it) have a frameheatmap for a memory region, to see hoe many times that
// memory region was "hit", where hit can mean accessed, i.e. read or written to, or i t can just
// mean read or written. i want to eventually store data on both, so you can see how many times the
// memory region was written to, AND how many times it was read from.

// TODO holy shit refactor this please

#pragma once

#include <map>
#include <optional>
#include <vector>
#include "Common/CommonTypes.h"

// I love c plus plus :)
// forward declarations are such a fun language feature!!1! 
// template<typename Frame = u16, typename Heat = u16, typename MapType = std::unordered_map<Frame,
// Heat>> class FrameHeatMap; using FrameHeatMapPtr = std::shared_ptr<FrameHeatMap<>>;
class FrameHeatMap;
using FrameHeatMapPtr = std::shared_ptr<FrameHeatMap>;
struct FrameHeat
{
  u16 frame;
  u16 heat;

  FrameHeat operator+(const FrameHeat& other) const { return FrameHeat(frame, heat + other.heat); }
  FrameHeat(u16 frame, u16 heat) : frame(frame), heat(heat) {}
};

// // Maps a frame number to a "heat", where heat is an amount of "hits", where a
// // hit is a function execution. Basically, it counts how many times <value> a
// // function was executed on frame <key>.
// template<typename Frame_t, typename Heat_t, typename MapType>
// class FrameHeatMap : public MapType
class FrameHeatMap : public std::vector<FrameHeat>
{
public:
  FrameHeatMap();
  static std::optional<u16> Heat(const u32 addr, const u16 framenum);
  static std::optional<u32> TotalHeat(const u32 addr);
  static std::optional<FrameHeatMapPtr> GetFrameHeatMap(const u32 addr);
  // // This also resets heat for this frame!
  // static void AddNewFrame(u16 current_frame);
  // colors the function a shade of red based on its heat
  static constexpr u32 HeatToColor(const u16 heat, const u32 defaultColor = 0x000000)
  {
    if (!heat)
      return defaultColor;
    if (heat == 1)
      return 0xFFEEEE;  // lighter pink
    else if (heat < 10)
      return 0xFFBBBB;  // light pink
    else if (heat < 50)
      return 0xFF8888;  // pink
    else if (heat < 100)
      return 0xFF4444;  // red
    else
      return 0xFF0000;  // deep red
  }
  static std::map<u32, FrameHeatMap> MakeRealMaps();
  static std::optional<std::map<u32, FrameHeatMap>*> GetFuncFHMap()
  {
    return m_funcFHMap.empty() ? std::nullopt : std::make_optional(&m_funcFHMap);
  }

  u32 TotalHeat();
  static inline std::map<u32, FrameHeatMap> m_funcFHMap;
  // static inline const std::set<std::string> magama_fns = {
  //     "memcpy    Global",        "__fill_mem    Global",
  //     "memset    Global",        "ColorFader_getStatus    Global",
  //     "Color_destroy    Global", "FUN_80008d18    Global"};

  std::optional<u32> m_totalHeat;
};
