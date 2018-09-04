#include "dune/Protodune/Analysis/ProtoDUNETrackUtils.h"

#include "larsim/MCCheater/BackTrackerService.h"
#include "larsim/MCCheater/ParticleInventoryService.h"
#include "lardataobj/RecoBase/Hit.h"
#include "art/Framework/Principal/Event.h"
#include "canvas/Persistency/Common/FindManyP.h"

protoana::ProtoDUNETrackUtils::ProtoDUNETrackUtils(){

}

protoana::ProtoDUNETrackUtils::~ProtoDUNETrackUtils(){

}

std::vector<anab::CosmicTag> protoana::ProtoDUNETrackUtils::GetRecoTrackCosmicTag(const recob::Track &track, art::Event const &evt, std::string trackModule) const{

  auto recoTracks = evt.getValidHandle<std::vector<recob::Track> >(trackModule);

  unsigned int trackIndex = track.ID();

  // Convert to std::vector<anab::CosmicTag> from std::vector<art::Ptr<anab::CosmicTag>>
  std::vector<anab::CosmicTag> trackTags;

  try{
    const art::FindManyP<anab::CosmicTag> findCosmicTags(recoTracks,evt,trackModule);
    for(unsigned int t = 0; t < findCosmicTags.at(trackIndex).size(); ++t){
      trackTags.push_back((*(findCosmicTags.at(trackIndex)[t])));
    }
  }
  catch(...){
//    std::cerr << "Product not found - returning empty vector" << std::endl;
  }

  return trackTags;    
}

std::vector<anab::T0> protoana::ProtoDUNETrackUtils::GetRecoTrackT0(const recob::Track &track, art::Event const &evt, std::string trackModule) const{

  auto recoTracks = evt.getValidHandle<std::vector<recob::Track> >(trackModule);

  unsigned int trackIndex = track.ID();

  // Convert to std::vector<anab::T0> from std::vector<art::Ptr<anab::T0>>
  std::vector<anab::T0> trackT0s;
  
  try{
    const art::FindManyP<anab::T0> findTrackT0s(recoTracks,evt,trackModule);
    for(unsigned int t = 0; t < findTrackT0s.at(trackIndex).size(); ++t){
      trackT0s.push_back((*(findTrackT0s.at(trackIndex)[t])));
    }
  }
  catch(...){
//    std::cerr << "Product not found - returning empty vector" << std::endl;
  }
  
  return trackT0s;

}

