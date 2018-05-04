// AdcPedestalFitter.h

// David Adams
// August 2017
//
// Tool to fit ADC data and extract a pedestal.
//
// If FitRmsMin < FitRmsMax, the the RMS is constrained to the range
// (FitRmsMin, FitRmsMax) in the fit.
//
// Configuration:
//   LogLevel - 0=silent, 1=init, 2=each event, >2=more
//   FitRmsMin: Lower limit for RMS fit range.
//   FitRmsMax: Upper limit for RMS fit range.
//   HistName:  Name for the histogram.
//   HistTitle: Title for the histogram.
//   HistManager: Name of the tool that manages the output histogram.
//                This is obsolete.
//   PlotFileName: If nonblank, histograms are displayed in this file.
//   RootFileName: If nonblank, histogram is copied to this file.
//
// Tools:
//   adcNameManipulator is used to make the following substitioutions:
//      %RUN% - run number
//      %SUBRUN% - event number
//      %EVENT% - event number
//      %CHAN% - channel number


#ifndef AdcPedestalFitter_H
#define AdcPedestalFitter_H

#include "art/Utilities/ToolMacros.h"
#include "fhiclcpp/ParameterSet.h"
#include "dune/DuneInterface/Tool/AdcChannelTool.h"
#include <string>
#include <vector>

class HistogramManager;
class AdcChannelStringTool;
class TH1;

class AdcPedestalFitter
: public AdcChannelTool {

public:

  AdcPedestalFitter(fhicl::ParameterSet const& ps);

  DataMap view(const AdcChannelData& acd) const override;

  DataMap update(AdcChannelData& acd) const override;

  //DataMap updateMap(AdcChannelDataMap& acds) const override;

private:

  using Name = std::string;
  using NameVector = std::vector<Name>;

  // Configuration data.
  int m_LogLevel;
  float m_FitRmsMin;
  float m_FitRmsMax;
  Name m_HistName;
  Name m_HistTitle;
  Name m_HistManager;
  Name m_PlotFileName;
  Name m_RootFileName;

  // ADC string tool.
  const AdcChannelStringTool* m_adcChannelStringTool;

  // Histogram manager.
  HistogramManager* m_phm;

  // Make replacements in a name.
  Name nameReplace(Name name, const AdcChannelData& acd) const;

  // Find and return pedestal.
  DataMap getPedestal(const AdcChannelData& acd) const;

};

DEFINE_ART_CLASS_TOOL(AdcPedestalFitter)

#endif
