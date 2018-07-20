// ProtoDuneChannelRanges.h
//
// David Adams
// July 2018
//
// Tool to return channel ranges for protoDUNE. There are many
// hardwired ranges apa2, apa4u, etc. and teh option to override
// or extend these with ranges specified in another index range tool.
//
// Parameters:
//   LogLevel - Message logging level (0=none, 1=ctor, 2=each call, ...)
//   ExtraRange - Name of tool with additional ranges. Blank for none.

#ifndef ProtoDuneChannelRanges_H
#define ProtoDuneChannelRanges_H

#include "art/Utilities/ToolMacros.h"
#include "fhiclcpp/ParameterSet.h"
#include "dune/DuneInterface/Tool/IndexRangeTool.h"
#include <map>

class ProtoDuneChannelRanges : public IndexRangeTool {

public:

  using Name = std::string;
  using Index = IndexRange::Index;
  using IndexRangeMap = std::map<Name, IndexRange>;

  // Ctor.
  ProtoDuneChannelRanges(fhicl::ParameterSet const& ps);

  // Dtor.
  ~ProtoDuneChannelRanges() override =default;

  // Return a range.
  IndexRange get(Name nam) const override;

private:

  // Configuration parameters.
  Index m_LogLevel;
  Name m_ExtraRanges;

  IndexRangeMap m_Ranges;
  const IndexRangeTool* m_pExtraRanges =nullptr;

  // Add an entry to the range map.
  void insertLen(Name nam, Index begin, Index len, Name lab);

};

DEFINE_ART_CLASS_TOOL(ProtoDuneChannelRanges)

#endif
