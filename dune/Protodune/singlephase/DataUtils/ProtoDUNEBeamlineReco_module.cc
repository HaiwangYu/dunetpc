////////////////////////////////////////////////////////////////////////
//
// ProtoDUNEBeamlineReco does tracking and reconstruction from the beamline
// monitors
//
// 2018 Jake Calcutt, calcuttj@msu.edu 
//
////////////////////////////////////////////////////////////////////////

// Framework includes
#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "art_root_io/TFileService.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "fhiclcpp/ParameterSet.h"
#include "dune/Protodune/singlephase/DataUtils/ProtoDUNEBeamlineUtils.h"

#include "lardataobj/RecoBase/Track.h"

#include "TTree.h"

namespace protoana{
  class ProtoDUNEBeamlineReco;
}

class protoana::ProtoDUNEBeamlineReco : public art::EDAnalyzer{
public:
  
  explicit ProtoDUNEBeamlineReco(fhicl::ParameterSet const& pset);
  virtual ~ProtoDUNEBeamlineReco();
  
  virtual void beginJob() override;
  virtual void endJob() override;

  void analyze(art::Event const & evt) override;
  void reconfigure(fhicl::ParameterSet const& pset);
  void reset();
  
private:
  
  protoana::ProtoDUNEBeamlineUtils fBeamlineUtils;

  TTree * fOutTree;

  double tof;
  int chan;
  double momentum;
  int c0, c1;
  int nMomenta;
  std::vector<double> momenta;
  int nTracks;


  std::vector<short> fibers_h_upstream,   fibers_v_upstream, 
                     fibers_h_downstream, fibers_v_downstream,
                     fibers_p1,           fibers_p2,
                     fibers_p3;

  unsigned long long GenTrigTS;
};
  
//-----------------------------------------------------------------------
protoana::ProtoDUNEBeamlineReco::ProtoDUNEBeamlineReco(fhicl::ParameterSet const& pset):
  EDAnalyzer(pset),
  fBeamlineUtils(pset.get<fhicl::ParameterSet>("BeamlineUtils"))
{
  this->reconfigure(pset);
}

//-----------------------------------------------------------------------
protoana::ProtoDUNEBeamlineReco::~ProtoDUNEBeamlineReco(){}

//-----------------------------------------------------------------------
void protoana::ProtoDUNEBeamlineReco::beginJob() {
  
  art::ServiceHandle<art::TFileService> tfs;

  fOutTree = tfs->make<TTree>("tree", "");
  fOutTree->Branch("TOF", &tof);
  fOutTree->Branch("Chan", &chan);
  fOutTree->Branch("Momentum", &momentum);
  fOutTree->Branch("C0",&c0);
  fOutTree->Branch("C1",&c1);
  fOutTree->Branch("nMomenta", &nMomenta);
  fOutTree->Branch("Momenta", &momenta);
  fOutTree->Branch("nTracks", &nTracks);

  //Tracking Monitors
  fOutTree->Branch("fibers_h_upstream", &fibers_h_upstream);
  fOutTree->Branch("fibers_v_upstream", &fibers_v_upstream);
  fOutTree->Branch("fibers_h_downstream", &fibers_h_downstream);
  fOutTree->Branch("fibers_v_downstream", &fibers_v_downstream);

  //Momentum Monitors
  fOutTree->Branch("fibers_p1", &fibers_p1);
  fOutTree->Branch("fibers_p2", &fibers_p2);
  fOutTree->Branch("fibers_p3", &fibers_p3);

  //Timestamp
  fOutTree->Branch("GenTrigTS", &GenTrigTS);
}

//-----------------------------------------------------------------------
void protoana::ProtoDUNEBeamlineReco::reconfigure(fhicl::ParameterSet const& pset){
}

//-----------------------------------------------------------------------
void protoana::ProtoDUNEBeamlineReco::analyze(art::Event const & evt){

  reset();

  //std::cout << "Computed TOF "      << fBeamlineUtils.ComputeTOF( kProton, 2.0 ) << std::endl;
  //std::cout << "Computed Momentum " << fBeamlineUtils.ComputeMomentum( kProton, 130. ) << std::endl;

  c0 = -1; 
  c1 = -1;
  momentum = -1.;
  tof = -1.;
  chan = -1;
  GenTrigTS = 0;

  //Access the Beam Event
  auto beamHandle = evt.getValidHandle<std::vector<beam::ProtoDUNEBeamEvent>>("beamevent");
  
  std::vector<art::Ptr<beam::ProtoDUNEBeamEvent>> beamVec;
  if( beamHandle.isValid()){
    art::fill_ptr_vector(beamVec, beamHandle);
  }

  const beam::ProtoDUNEBeamEvent & beamEvent = *(beamVec.at(0)); //Should just have one
  /////////////////////////////////////////////////////////////
  
  
  //Check the quality of the event
  std::cout << "Timing Trigger: " << beamEvent.GetTimingTrigger() << std::endl; 
  std::cout << "Is Matched: "     << beamEvent.CheckIsMatched() << std::endl << std::endl;

  if( !fBeamlineUtils.IsGoodBeamlineTrigger( evt ) ){
    std::cout << "Failed quality check" << std::endl;
    return;
  }

  std::cout << "Passed quality check!" << std::endl << std::endl;  
  /////////////////////////////////////////////////////////////
  

  //Access momentum
  const std::vector< double > & the_momenta = beamEvent.GetRecoBeamMomenta();
  std::cout << "Number of reconstructed momenta: " << the_momenta.size() << std::endl;

  if( the_momenta.size() > 0 ) 
    std::cout << "Measured Momentum: " << the_momenta[0] << std::endl;

  if( the_momenta.size()  == 1)
    momentum = the_momenta[0];

  momenta.insert( momenta.end(), the_momenta.begin(), the_momenta.end() );
  /*
  for( size_t i = 0; i < the_momenta.size(); ++i ){
    momenta.push_back( the_momenta[i] );
  }
  */
  nMomenta = momenta.size();
  ///////////////////////////////////////////////////////////// 


  std::cout << "Current: " << beamEvent.GetMagnetCurrent() << std::endl;

  //Access time of flight
  const std::vector< double > & the_tofs  = beamEvent.GetTOFs();
  const std::vector< int    > & the_chans = beamEvent.GetTOFChans();

  std::cout << "Number of measured TOF: " << the_tofs.size() << std::endl;
  std::cout << "First TOF: "              << beamEvent.GetTOF()         << std::endl;
  std::cout << "First TOF Channel: "      << beamEvent.GetTOFChan()     << std::endl << std::endl;

  std::cout << "All (TOF, Channels): " << std::endl;
  for( size_t i = 0; i < the_tofs.size(); ++i ){
    std::cout << "\t(" << the_tofs[i] << ", " << the_chans[i] << ")" << std::endl;
  }
  std::cout << std::endl;

  if( the_tofs.size() > 0){
    tof = the_tofs[0];
    chan = the_chans[0];
  }
  /////////////////////////////////////////////////////////////
  

  //Access Cerenkov info
  std::cout << "Cerenkov status, pressure:" << std::endl;
  std::cout << "C0: " << beamEvent.GetCKov0Status() << ", " << beamEvent.GetCKov0Pressure() << std::endl;
  std::cout << "C1: " << beamEvent.GetCKov1Status() << ", " << beamEvent.GetCKov1Pressure() << std::endl << std::endl;
  c0 = beamEvent.GetCKov0Status();
  c1 = beamEvent.GetCKov1Status();
  ///////////////////////////////////////////////////////////// 



  //Access PID
  std::vector< int > pids = fBeamlineUtils.GetPID( beamEvent, 1. );

  std::cout << "Possible particles" << std::endl;

  for( size_t i = 0; i < pids.size(); ++i ){ 
    std::cout << pids[i] << std::endl;
  }
  std::cout << std::endl;

  PossibleParticleCands candidates = fBeamlineUtils.GetPIDCandidates( beamEvent, 1. );
  std::cout << std::left << std::setw(10) << "electron " << candidates.electron << std::endl;
  std::cout << std::left << std::setw(10) << "muon "     << candidates.muon     << std::endl;
  std::cout << std::left << std::setw(10) << "pion "     << candidates.pion     << std::endl;
  std::cout << std::left << std::setw(10) << "kaon "     << candidates.kaon     << std::endl;
  std::cout << std::left << std::setw(10) << "proton "   << candidates.proton   << std::endl << std::endl;

  std::string candidates_string = fBeamlineUtils.GetPIDCandidates( beamEvent, 1. );
  std::cout << candidates_string << std::endl;
  ///////////////////////////////////////////////////////////// 
  
  //Tracking info
  nTracks = beamEvent.GetBeamTracks().size();
  /////////////////////////////////////////////////////////////

  //Fibers
  std::cout << beamEvent.GetFiberTime( "XBPF022697" ) << std::endl;
  std::cout.precision(20);
  unsigned long test = (unsigned long)(beamEvent.GetT0().first*1e6) + (unsigned long)(beamEvent.GetT0().second/1.e3);
  std::cout << beamEvent.GetT0().first << " " << beamEvent.GetT0().second << std::endl;
  std::cout << test << std::endl;
  
  GenTrigTS = (unsigned long)(beamEvent.GetT0().first*1e6) + (unsigned long)(beamEvent.GetT0().second/1.e3);

  fibers_p1 = beamEvent.GetActiveFibers( "XBPF022697" );  
  fibers_p2 = beamEvent.GetActiveFibers( "XBPF022701" );  
  fibers_p3 = beamEvent.GetActiveFibers( "XBPF022702" );  

  fibers_h_upstream = beamEvent.GetActiveFibers( "XBPF022707" );  
  fibers_v_upstream = beamEvent.GetActiveFibers( "XBPF022708" );  
  fibers_h_downstream = beamEvent.GetActiveFibers( "XBPF022716" );  
  fibers_v_downstream = beamEvent.GetActiveFibers( "XBPF022717" );  
  /////////////////////////////////////////////////////////////

  fOutTree->Fill();
}

void protoana::ProtoDUNEBeamlineReco::endJob() {}

void protoana::ProtoDUNEBeamlineReco::reset(){
  momenta.clear();

  fibers_p1.clear();
  fibers_p2.clear();
  fibers_p3.clear();

  fibers_h_upstream.clear();
  fibers_v_upstream.clear();
  fibers_h_downstream.clear();
  fibers_v_downstream.clear();
}
 
DEFINE_ART_MODULE(protoana::ProtoDUNEBeamlineReco)
