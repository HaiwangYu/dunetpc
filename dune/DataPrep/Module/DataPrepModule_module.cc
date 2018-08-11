// DataPrepModule_module.cc

// David Adams
// July 2016
// November 2016 - Add option to group channels.
//
// Module that reads RawData and writes Wire and their associations.
// It uses RawDigitPrepService to build the wires.
//
// It is possible to also write intermediate states. List the states in the fcl
// vector IntermediateStates. Supported values are:
//   extracted - After pedestal subtraction
//   mitigated - After mitigation (e.g. stuck bit interpolation)
//   noiseRemoved - After noise removal
// The states are written to the containers with the same name. To save space
// and time, e.g. in standard production, this vector should be empty.
//
// If the flag is set set, channels are processed in groups specified by
// the ChannelGroupService. This can save memory because the transient ADC
// channel data is deleted after each group is processed.
//
// Configuration parameters:
//             LogLevel - Usual logging level.
//           DigitLabel - Full label for the input digit container, e.g. daq
//             WireName - Name for the output wire container.
//   IntermediateStates - Names of intermediate states to record.
//             DoGroups - Process channels in groups.

#include "art/Framework/Core/ModuleMacros.h" 
#include "art/Framework/Core/EDProducer.h"
#include "art/Framework/Principal/Event.h" 
#include "art/Framework/Services/Registry/ServiceHandle.h" 
#include "lardataobj/RawData/RawDigit.h"
#include "lardataobj/RecoBase/Wire.h"
#include "lardata/Utilities/AssociationUtil.h"
#include "dune/DuneInterface/RawDigitPrepService.h"
#include "dune/DuneInterface/ChannelGroupService.h"
#include "dune/DuneInterface/Tool/IndexMapTool.h"
#include "dune/DuneCommon/DuneTimeConverter.h"
#include "dune/ArtSupport/DuneToolManager.h"
#include "TDatime.h"

using std::cout;
using std::endl;
using std::string;
using std::vector;
using std::move;
using art::ServiceHandle;
using art::Timestamp;
using recob::Wire;

//**********************************************************************

class DataPrepModule : public art::EDProducer {

public:
    
  // Ctor.
  explicit DataPrepModule(fhicl::ParameterSet const& pset); 

  // Dtor.
  ~DataPrepModule();
    
  // Producer methods.
  void produce(art::Event& evt);
  void beginJob();
  void endJob();
  void reconfigure(fhicl::ParameterSet const& p);
    
private:
    
  // Configuration parameters.
  int m_LogLevel;
  std::string m_DigitLabel;  ///< Full label for the input digit container, e.g. daq:
  std::string m_WireName;    ///< Second field in full label for the output wire container.
  std::vector<std::string> m_IntermediateStates;
  bool m_DoAssns = false;
  bool m_DoGroups = false;
  AdcChannel m_KeepChannelBegin =0;
  AdcChannel m_KeepChannelEnd =0;
  AdcChannelVector m_SkipChannels;

  // Split label into producer and name: PRODUCER or PRODUCER:NAME
  std::string m_DigitProducer;
  std::string m_DigitName;

  // Accessed services.
  RawDigitPrepService* m_pRawDigitPrepService = nullptr;
  ChannelGroupService* m_pChannelGroupService = nullptr;

  // Tools.
  std::string m_OnlineChannelMapTool;
  std::unique_ptr<IndexMapTool> m_onlineChannelMapTool;

};

DEFINE_ART_MODULE(DataPrepModule)
  
//**********************************************************************

DataPrepModule::DataPrepModule(fhicl::ParameterSet const& pset) {
  this->reconfigure(pset);
  produces<std::vector<recob::Wire>>(m_WireName);
  if ( m_DoAssns ) {
    produces<art::Assns<raw::RawDigit, recob::Wire>>(m_WireName);
  }
  for ( string sname : m_IntermediateStates ) {
    produces<std::vector<recob::Wire>>(sname);
  }
}
  
//**********************************************************************

DataPrepModule::~DataPrepModule() { }

//**********************************************************************

void DataPrepModule::reconfigure(fhicl::ParameterSet const& pset) {
  const string myname = "DataPrepModule::reconfigure: ";
  m_LogLevel   = pset.get<int>("LogLevel");
  m_DigitLabel = pset.get<std::string>("DigitLabel", "daq");
  m_WireName   = pset.get<std::string>("WireName", "");
  m_DoAssns    = pset.get<bool>("DoAssns");
  m_DoGroups   = pset.get<bool>("DoGroups");
  m_IntermediateStates = pset.get<vector<string>>("IntermediateStates");
  pset.get_if_present<AdcChannel>("KeepChannelBegin", m_KeepChannelBegin);
  pset.get_if_present<AdcChannel>("KeepChannelEnd", m_KeepChannelEnd);
  pset.get_if_present<AdcChannelVector>("SkipChannels", m_SkipChannels);
  pset.get_if_present<std::string>("OnlineChannelMapTool", m_OnlineChannelMapTool);

  size_t ipos = m_DigitLabel.find(":");
  if ( ipos == std::string::npos ) {
    m_DigitProducer = m_DigitLabel;
  } else {
    m_DigitProducer = m_DigitLabel.substr(0, ipos);
    m_DigitName = m_DigitLabel.substr(ipos + 1);
  }

  m_pRawDigitPrepService = &*ServiceHandle<RawDigitPrepService>();
  if ( m_DoGroups ) m_pChannelGroupService = &*ServiceHandle<ChannelGroupService>();

  if ( m_OnlineChannelMapTool.size() ) {
    DuneToolManager* ptm = DuneToolManager::instance();
    m_onlineChannelMapTool = ptm->getPrivate<IndexMapTool>(m_OnlineChannelMapTool);
  }

  if ( m_LogLevel >= 1 ) {
    cout << myname << "             LogLevel: " << m_LogLevel << endl;
    cout << myname << "           DigitLabel: " << m_DigitLabel << " (" << m_DigitProducer
                   << ", " << m_DigitName << ")" << endl;
    cout << myname << "             WireName: " << m_WireName << endl;
    cout << myname << "              DoAssns: " << m_DoAssns << endl;
    cout << myname << "             DoGroups: " << m_DoGroups << endl;
    cout << myname << "   IntermediateStates: [";
    int count = 0;
    for ( string sname : m_IntermediateStates ) cout << (count++ == 0 ? "" : " ") << sname;
    cout << "]" << endl;
    cout << myname << "  OnlineChannelMapTool: " << m_OnlineChannelMapTool << endl;
    cout << myname << "      KeepChannelBegin: " << m_KeepChannelBegin << endl;
    cout << myname << "        KeepChannelEnd: " << m_KeepChannelEnd << endl;
    cout << myname << "          SkipChannels: " << "{";
    bool first = true;
    for ( AdcChannel ich : m_SkipChannels ) {
      if ( first ) first = false;
      else cout << ", ";
      cout << ich;
    }
    cout << "}" << endl;
  }
}

//**********************************************************************

void DataPrepModule::beginJob() { }

//**********************************************************************

void DataPrepModule::endJob() { }
  
//**********************************************************************

void DataPrepModule::produce(art::Event& evt) {      
  const string myname = "DataPrepModule::produce: ";

  // Fetch the event time.
  Timestamp beginTime = evt.time();

  // Read in the digits. 
  if ( m_LogLevel >= 2 ) {
    cout << myname << "Reading raw digits for producer, name: " << m_DigitProducer << ", " << m_DigitName << endl;
    // July 2018. ProtoDUNE real data has zero in high field and unix time in low field.
    if ( beginTime.timeHigh() == 0 ) {
      unsigned int itim = beginTime.timeLow();
      TDatime rtim(itim);
      string stim = rtim.AsString();
      cout << myname << "Real data event time: " << itim << " (" << stim << ")" << endl;
    } else {
      cout << myname << "Sim data event time: " << DuneTimeConverter::toString(beginTime) << endl;
    }
    cout << myname << "Run " << evt.run();
    if ( evt.subRun() ) cout << "-" << evt.subRun();
    cout << " event " << evt.event() << endl;
  }
  if ( m_LogLevel >= 3 ) {
    cout << myname << "Event time high, low: " << beginTime.timeHigh() << ", " << beginTime.timeLow() << endl;
  }
  art::Handle< std::vector<raw::RawDigit> > hdigits;
  evt.getByLabel(m_DigitProducer, m_DigitName, hdigits);
  if ( m_LogLevel > 1 ) {
    cout << myname << "# digits read: " << hdigits->size() << endl;
  }
  if ( hdigits->size() == 0 ) mf::LogWarning("DataPrepModule") << "Input digit container is empty";

  // Create the container to hold the output wires.
  std::unique_ptr<std::vector<recob::Wire>> pwires(new std::vector<recob::Wire>);
  pwires->reserve(hdigits->size());

  // Create the association container.
  std::unique_ptr<art::Assns<raw::RawDigit,recob::Wire>> passns(new art::Assns<raw::RawDigit,recob::Wire>);

  // Prepare the intermediate state cache.
  // Note that transient data is retained between groups and so most of the memory saving
  // of groups is lost if  intermediate states are recorded.
  WiredAdcChannelDataMap* pintStates = nullptr;
  if ( m_IntermediateStates.size() ) {
    pintStates = new WiredAdcChannelDataMap(m_IntermediateStates, hdigits->size());
  }

  // Create the transient data map and copy the digits there.
  AdcChannelDataMap fulldatamap;
  bool checkKeep = m_KeepChannelEnd > m_KeepChannelBegin;
  unsigned int nkeep = 0;
  unsigned int nskip = 0;
  for ( unsigned int idig=0; idig<hdigits->size(); ++idig ) {
    const raw::RawDigit& dig = (*hdigits)[idig];
    AdcChannel chan = dig.Channel();
    if ( checkKeep ) {
      if ( chan < m_KeepChannelBegin || chan >= m_KeepChannelEnd ) {
        ++nskip;
        continue;
      }
    }
    if ( std::find(m_SkipChannels.begin(), m_SkipChannels.end(), chan) != m_SkipChannels.end() ) {
      ++nskip;
      continue;
    }
    ++nkeep;
    if ( fulldatamap.find(chan) != fulldatamap.end() ) {
      mf::LogWarning("DataPrepModule") << "Skipping duplicate channel " << chan << "." << endl;
      ++nskip;
      continue;
    }
    AdcChannelData& acd = fulldatamap[chan];
    acd.run = evt.run();
    acd.subRun = evt.subRun();
    acd.event = evt.event();
    acd.channel = chan;
    acd.digitIndex = idig;
    acd.digit = &dig;
    if ( m_onlineChannelMapTool ) {
      unsigned int ichOn = m_onlineChannelMapTool->get(chan);
      if ( ichOn != IndexMapTool::badIndex() ) {
        acd.fembID = ichOn/128;
        acd.fembChannel = ichOn % 128;
      }
    }
  }

  // Create a vector of data maps with an entry for each group.
  unsigned int ncgrp = 0;
  vector<AdcChannelDataMap> datamaps;
  if ( m_DoGroups ) {
    if ( m_pChannelGroupService == nullptr ) {
      mf::LogError("DataPrepModule") << "Channel group service not found." << endl;
      return;
    }
    unsigned int ngrp = m_pChannelGroupService->size();
    for ( unsigned int igrp=0; igrp<ngrp; ++igrp ) {
      datamaps.emplace_back();
      AdcChannelDataMap& datamap = datamaps.back();
      for ( AdcChannel chan : m_pChannelGroupService->channels(igrp) ) {
        if ( fulldatamap.find(chan) != fulldatamap.end() ) {
          datamap.emplace(chan, move(fulldatamap[chan]));
          ++ncgrp;
        }
      }
    }
  } else {
    datamaps.emplace_back(move(fulldatamap));
  }
  if ( m_LogLevel >= 2 ) {
    cout << myname << "         # channels selected: " << ncgrp << endl;
    cout << myname << "          # channels skipped: " << ncgrp << endl;
    cout << myname << "  # channels to be processed: " << ncgrp << endl;
  }

  for ( AdcChannelDataMap& datamap : datamaps ) {

    if ( datamap.size() == 0 ) continue;

    // Use the data preparation service to build the wires and intermediate states.
    int rstat = m_pRawDigitPrepService->prepare(datamap, pwires.get(), pintStates);
    if ( rstat != 0 ) mf::LogWarning("DataPrepModule") << "Data preparation service returned error " << rstat;

    // Build associations between wires and digits.
    if ( m_DoAssns ) {
      for ( const AdcChannelDataMap::value_type& iacd : datamap ) {
        const AdcChannelData& acd = iacd.second;
        AdcIndex idig = acd.digitIndex;
        if ( idig == AdcChannelData::badIndex )
          throw art::Exception(art::errors::ProductRegistrationFailure) << "Digit index is not set.";
        AdcIndex iwir = acd.wireIndex;
        if ( iwir == AdcChannelData::badIndex ) continue;
        art::Ptr<raw::RawDigit> pdig(hdigits, idig);
        bool success = util::CreateAssn(*this, evt, *pwires, pdig, *passns, m_WireName, iwir);
        if ( !success ) throw art::Exception(art::errors::ProductRegistrationFailure)
                              << "Can't associate wire " << iwir << " with raw digit " << idig;
      }
    }

    // Delete the entries from the current channel map.
    // This is an easy way to clear the transient data.
    datamap.erase(datamap.begin(), datamap.end());

  }  // end loop over groups

  if ( m_LogLevel >= 2 ) {
    cout << myname << "Created wire count: " << pwires->size() << endl;
  }
  if ( pwires->size() == 0 ) mf::LogWarning("DataPrepModule") << "No wires made for this event.";

  // Record wires and associations in the event.
  evt.put(std::move(pwires), m_WireName);
  if ( m_DoAssns ) {
    evt.put(std::move(passns), m_WireName);
  }

  // Record intermediate state wires.
  for ( string sname : m_IntermediateStates ) {
    vector<Wire>* pintWires = pintStates->wires[sname];
    if ( pintWires == nullptr ) {
      cout << myname << "WARNING: Wires not found for state " << sname << "." << endl;
      continue;
    }
    if ( m_LogLevel >=2 ) {
      cout << myname << "Recording intermediate state " << sname << "  with "
                     << pintWires->size() << " channels." << endl;
    }
    if ( m_LogLevel >=3 ) {
      for ( const Wire& wire : *pintWires ) {
        cout << myname << "   Channel " << wire.Channel() << " has "
                       << wire.SignalROI().n_ranges() << " ROI and "
                       << wire.SignalROI().count() << "/" << wire.SignalROI().size()
                       << " ticks." << endl;
      }
    }
    std::unique_ptr<std::vector<recob::Wire>> pintWiresWrapped(pintWires);
    evt.put(std::move(pintWiresWrapped), sname);
  }

  return;
}
