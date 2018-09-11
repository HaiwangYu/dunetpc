/*
 * \file: CalibrationTreeBuilder.h
 * \author: Jason Stock (jason.stock@mines.sdsmt.edu
 * \brief: This is a small Tree made for use in calibration analysis.
 *
 */

#ifndef CALIBRATIONTREEBUILDER_H
#define CALIBRATIONTREEBUILDER_H

//Includes
#include "dune/DuneObj/CalibTreeRecord.h"

//LArSoft Includes
#include "lardataobj/RecoBase/Track.h"
#include "lardataobj/RecoBase/Hit.h"
#include "lardataobj/RecoBase/OpHit.h"
#include "larcore/CoreUtils/ServiceUtil.h"
#include "larsim/MCCheater/ParticleInventoryService.h"
#include "larsim/MCCheater/BackTrackerService.h"
#include "larsim/MCCheater/PhotonBackTrackerService.h"

//FrameworkIncludes
#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Services/Optional/TFileService.h"
#include "art/Framework/Services/Optional/TFileDirectory.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "canvas/Persistency/Common/FindManyP.h"
#include "canvas/Persistency/Common/Ptr.h"
#include "canvas/Persistency/Common/PtrVector.h"
#include "canvas/Utilities/Exception.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

//ROOT includes
#include "TTree.h"
#include "TFile.h"

//CPP includes
#include <vector>
#include <map>


namespace{}//

namespace CalibrationTreeBuilder {

  struct flatfiller{ //ordered for descending size.
    Double_t eve_x ;
    Double_t eve_y ;
    Double_t eve_z ;
    Double_t eve_t ;
    Double_t part_x ;
    Double_t part_y ;
    Double_t part_z ;
    Double_t part_t ;
    Double_t hit_charge;
    Double_t hit_energy;
    Double_t hit_time ;
    Double_t hit_width ;
    Double_t hit_split ;
    Double_t ophit_pes ;
    Double_t ophit_energy;
    Double_t ophit_time ;
    Double_t ophit_width ;
    Double_t ophit_split ;
    int64_t hit_index ;
    int64_t ophit_index ;
    UInt_t run ;
    UInt_t subrun ;
    UInt_t event_n ;
    UInt_t hit_wire ;
    UInt_t ophit_opchan ;
    UInt_t eve_index;
    UInt_t part_index;
    Int_t eve_trackid;
    Int_t eve_pdgid ;
    Int_t part_trackid ;
    Int_t part_pdgid;
    bool part_iseve ;
  };


  class CalibrationTreeBuilder : public art::EDAnalyzer
  {
    public:

      //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
      CalibrationTreeBuilder(fhicl::ParameterSet const& pSet);

      virtual void beginJob();
      virtual void analyze(const art::Event& evt) override;
      virtual void endJob();


    private:

      bool AddHit(const art::Ptr<recob::Hit> hit, unsigned int& counter);
      bool AddHit(const art::Ptr<recob::OpHit> hit, unsigned int& counter);
      std::pair<std::vector<CalibTreeRecord::EveRecord>::iterator, bool> EmplaceEve(const simb::MCParticle* new_eve);
      std::pair<std::vector<CalibTreeRecord::ParticleRecord>::iterator, bool> EmplaceParticle(const simb::MCParticle* new_part);

      art::ServiceHandle<cheat::ParticleInventoryService> PIS;
      art::ServiceHandle<cheat::BackTrackerService> BTS;
      art::ServiceHandle<cheat::PhotonBackTrackerService> PBS;
      art::ServiceHandle<geo::Geometry> GS;

      art::InputTag private_HitLabel;
      art::InputTag private_OpHitLabel;

      art::ServiceHandle<art::TFileService> private_service_tfs;

      TTree* private_CalibrationTree;
      TTree* private_FlatCalibrationTree;
      TBranch* private_CalibrationRecord;
      TBranch* private_FlatCalibrationRecord;

      CalibTreeRecord::CalibTreeRecord private_eventBuffer;
      CalibTreeRecord::CalibTreeRecord private_eventPrep;
      flatfiller fl;



      //Also need buffers

  };// end class CalibrationTreeBuilder

}

#endif//endif CALIBRATIONTREEBUILDER_H
