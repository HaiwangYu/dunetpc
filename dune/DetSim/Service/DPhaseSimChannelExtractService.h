// DPhaseSimChannelExtractService.h

//
// David Adams
// December 2015
//
// Interface for a service that extracts charge from
// a SimChannel object and assigns it to ticks.
//
// vgalymov@ipnl.in2p3.fr
// Simulate signals from dual-phase detector:
//      - charge amplification in gas
//      - charge collection in two views
// 

#ifndef _DPhaseSimChannelExtractService_H_
#define _DPhaseSimChannelExtractService_H_

#include <vector>
#include "dune/DuneInterface/SimChannelExtractService.h"
#include "art/Framework/Services/Registry/ServiceMacros.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "Utilities/LArFFT.h"
#include "dune/Utilities/SignalShapingServiceDUNEDPhase.h"

//namespace CLHEP {
//class HepRandomEngine;
//class RandGaussQ;
//}

namespace sim {
class SimChannel;
}

class DPhaseSimChannelExtractService : public SimChannelExtractService {

public:

  DPhaseSimChannelExtractService(fhicl::ParameterSet const& pset, art::ActivityRegistry&);

  int extract(const sim::SimChannel* psc, AdcSignalVector& sig) const;

private:
  
  // standard larsoft FFT service
  art::ServiceHandle<util::LArFFT> m_pfft;

  // dual-phase signal response service
  art::ServiceHandle<util::SignalShapingServiceDUNEDPhase> m_psss;
  
  unsigned int m_ntick;
  //CLHEP::HepRandomEngine* m_pran;
  
  float fDPGainPerView; // gain in dual-phase
  float fRedENC;       // ENC noise, set to 0 to disable
};

DECLARE_ART_SERVICE_INTERFACE_IMPL(DPhaseSimChannelExtractService, SimChannelExtractService, LEGACY)

#endif

