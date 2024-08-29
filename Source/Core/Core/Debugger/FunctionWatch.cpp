#include "Core/Debugger/FunctionWatch.h"
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <numeric>
#include "Common/Logging/Log.h"
#include "Core/Core.h"
#include "Core/PowerPC/JitInterface.h"
#include "Core/PowerPC/PPCSymbolDB.h"

// #include <iostream>
#include <fstream>

#include "VideoCommon/Present.h"
#include "VideoCommon/VideoEvents.h"

namespace Core
{

size_t FunctionWatch::n_tracing = 0;

void FunctionWatch::Enable()
{
  NOTICE_LOG_FMT(POWERPC, "FunctionWatch::Enable");
  m_VI_end_field_event = AfterFrameEvent::Register(
      [this](Core::System& system) { OnFrameEnd(system); }, "FunctionWatch");
}

void FunctionWatch::Disable()
{
  NOTICE_LOG_FMT(POWERPC, "FunctionWatch::Disable");
  m_VI_end_field_event.reset();
}

void FunctionWatch::Dump(const Core::System& system)
{

  // this is probably a terrible way to serialize :/
  // this code can break and segfault easily, however, i do not care! :)
  // btw u are assumed to be using little endian
  // this should prolly use some vecctor or other stl dynalloc shiz 
  // but idc i just wanna get this data serialized and outta here so i dont have to use cpp anymore
  size_t buffer_size = std::accumulate(m_heatmap.begin(), m_heatmap.end(), 0, [](const size_t& acc, const auto& it) {
    const auto& frameMap = it.second;
    return acc + (frameMap.size() * (sizeof(framenum_t) + sizeof(hit_count_t)) + sizeof(addr_t) + sizeof(u32));
  });

  buffer_size += 4 * sizeof(u32);
  constexpr const char header[32] = "idk how to serialize things";
  buffer_size += sizeof(header);

  u32* buffer = (u32*) malloc(buffer_size);
  size_t write_head = 0;
  memcpy(buffer, header, sizeof(header));
  write_head += 8;
  buffer[write_head] = (u32) m_heatmap.size(); write_head++;
  for (const auto& [addr, frameMap] : m_heatmap) {
    buffer[write_head] = addr; write_head++;
    buffer[write_head] = (u32) frameMap.size(); write_head++;
    for (const auto& [framenum, heat] : frameMap) {
      buffer[write_head] = framenum; write_head++;
      buffer[write_head] = heat; write_head++;
    }
  }

  // ~~12 bytes short!~~ scratch that LOTS of KILObytes short LOL
  NOTICE_LOG_FMT(POWERPC, "Exporting FWFW data. buffer_size: {}, final write_head: {}", buffer_size, write_head);

  std::ofstream outFile("funcs.bin", std::ios::binary);
  if (!outFile) {
    ERROR_LOG_FMT(POWERPC, "Error opening file for writing.\n");
  }
  outFile.write((char*) buffer, buffer_size);
  outFile.close();
  NOTICE_LOG_FMT(POWERPC, "Data written successfully.\n");


  auto symbol_db = system.GetPPCSymbolDB();
  auto* file = std::fopen("funcs.tsv", "w");
  fmt::println(file, "addr\tfunc_name\tn_frames\ttotal_heat");
  for (auto& [addr, frameMap] : m_heatmap)
  {
    auto symbol = symbol_db.GetSymbolFromAddr(addr);
    fmt::println(file, "0x{:X}\t{}\t{}\t{}", addr, symbol->function_name, frameMap.size(), symbol->num_calls);
  }
  std::fclose(file);
}

bool FunctionWatch::IsMagma(u32 addr) {
  return m_magma_addrs.count(addr) != 0;
}

void FunctionWatch::OnFrameEnd(const Core::System& system)
{
  auto& symDB = system.GetPPCSymbolDB();
  size_t i = 0;
  size_t magmas_ignored = 0;
  for (auto& [addr, symbol] : symDB.AccessSymbols()) {
    if (symbol.num_calls_this_frame != 0) {
      m_heatmap[addr][g_presenter->FrameCount()] = symbol.num_calls_this_frame;
      symbol.num_calls += symbol.num_calls_this_frame;

      // cancel tracing for magma functions
      // wehre a magma function is a function that has a high heat
      // where heat is amount of hits
      // where a hit is an execution
      // so, a magma funciton is a function that is executed often
      // so, this block cancels tracing fro functions that are executed often.
      // TODO: make this condition customizable in the to-be-created Function Watch Dialog UI (like Branch Watch Dialog)
      // TODO: keep track of magma fns and show the user in the FWFW dialog so they can choose to trace them anyway,
      // or manually mark other fns as magma
      if (symbol.num_calls > 1'000'000 || symbol.num_calls_this_frame > 1'000) {
        m_magma_addrs.insert(addr);
        magmas_ignored++;
      }

      i++;
    }
  }
  NOTICE_LOG_FMT(POWERPC, "{}/{} fns ({} magma) hit frame {}", i, n_tracing, magmas_ignored, g_presenter->FrameCount());
}

size_t FunctionWatch::MagmaCount() {
  return m_magma_addrs.size();
}

}  // namespace Core