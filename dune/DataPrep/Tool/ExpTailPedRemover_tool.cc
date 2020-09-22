// ExpTailPedRemover_tool.cc

#include "ExpTailPedRemover.h"
#include <iostream>
#include <iomanip>
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "dune/DuneCommon/SampleTailer.h"
#include "dune/ArtSupport/DuneToolManager.h"
#include "dune/DuneInterface/Tool/IndexRangeTool.h"
#include "dune-raw-data/Services/ChannelMap/PdspChannelMapService.h"
#include "TVectorD.h"
#include "TMatrixDSym.h"
#include <iomanip>

using std::string;
using std::cout;
using std::endl;
using std::setw;
using std::copy;
using std::setw;

using Index = unsigned int;
using DoubleVector = std::vector<double>;

//**********************************************************************
// Class methods.
//**********************************************************************

ExpTailPedRemover::ExpTailPedRemover(fhicl::ParameterSet const& ps)
: m_LogLevel(ps.get<int>("LogLevel")),
  m_SignalFlag(ps.get<Index>("SignalFlag")),
  m_SignalIterationLimit(ps.get<Index>("SignalIterationLimit")),
  m_SignalTool(ps.get<string>("SignalTool")),
  m_DecayTime(ps.get<double>("DecayTime")) ,
  m_MaxTick(ps.get<Index>("MaxTick")),
  m_PedDegree(ps.get<Index>("PedDegree")),
  m_PedTick0(ps.get<Index>("PedTick0")),
  m_PedFreqs(ps.get<FloatVector>("PedFreqs")) ,
  m_IncludeChannelRanges(ps.get<NameVector>("IncludeChannelRanges")),
  m_ExcludeChannelRanges(ps.get<NameVector>("ExcludeChannelRanges")),
  m_useChannelRanges(false),
  m_pSignalTool(nullptr) {
  const string myname = "ExpTailPedRemover::ctor: ";
  if ( m_SignalFlag > 3 ) {
    Index signalFlag = 2;
    cout << myname << "WARNING: Invalid SignalFlag value " << m_SignalFlag
         << " reset to " << signalFlag << "." << endl;
    m_SignalFlag = signalFlag;
  }
  if ( m_SignalFlag >= 2  ) {
    DuneToolManager* ptm = DuneToolManager::instance();
    if ( ptm == nullptr ) {
      cout << myname << "ERROR: Unable to retrieve tool manager." << endl;
    } else {
      m_pSignalTool = ptm->getShared<AdcChannelTool>(m_SignalTool);
      if ( m_pSignalTool == nullptr ) {
        cout << myname << "ERROR: Signal finding tool not found: " << m_SignalTool << endl;
      }
    }
  }
  Index nchaCheck = 0;
  if ( m_IncludeChannelRanges.size() ) {
    DuneToolManager* ptm = DuneToolManager::instance();
    const IndexRangeTool* pcrt = ptm->getShared<IndexRangeTool>("channelRanges");
    if ( pcrt == nullptr ) {
      cout << myname << "ERROR: IndexRangeTool not found: channelRanges" << endl;
    } else {
      for ( Name crn : m_IncludeChannelRanges ) {
        IndexRange ran = pcrt->get(crn);
        if ( ran.isValid() ) {
          if ( ran.end > m_checkChannels.size() ) m_checkChannels.resize(ran.end, false);
          for ( Index icha=ran.begin; icha<ran.end; ++icha ) m_checkChannels[icha] = true;
        } else {
          cout << myname << "WARNING: Ignoring invalid include channel range " << crn << endl;
        }
      }
      if ( m_ExcludeChannelRanges.size() ) {
        for ( Name crn : m_ExcludeChannelRanges ) {
          if ( crn == "all" ) {
            m_checkChannels.clear();
            break;
          }
          IndexRange ran = pcrt->get(crn);
          if ( ran.isValid() ) {
            Index end = ran.end < m_checkChannels.size() ? ran.end : m_checkChannels.size();
            for ( Index icha=ran.begin; icha<end; ++icha ) m_checkChannels[icha] = false;
          } else {
            cout << myname << "WARNING: Ignoring invalid exclude channel range " << crn << endl;
          }
        }
      }
      m_useChannelRanges = true;
      for ( bool keep : m_checkChannels ) if ( keep ) ++nchaCheck;
    }
  }
  // Build the pedestal vectors.
  if ( m_PedDegree > 2 ) {
    cout << myname << "WARNING: Pedestal degree reduced from m_PedDegree to 2." << endl;
    m_PedDegree = 2;
  }
  Index nvec = 1 + m_PedDegree+ 2*m_PedFreqs.size();
  m_fitNames.resize(nvec+1);
  Index ifit = 0;
  m_fitNames[ifit++] = "Tail";
  m_fitNames[ifit++] = "Pedestal";
  Index nsam = m_MaxTick;
  m_pedVectors.resize(nvec, FloatVector(nsam, 1.0));
  FloatVectorVector::iterator ivec = m_pedVectors.begin();
  if ( m_PedDegree > 0 ) {
    FloatVector& vec = *(++ivec);
    for ( Index isam=0; isam<nsam; ++isam ) {
      vec[isam] = isam - m_PedTick0;
    }
    m_fitNames[ifit++] = "Slope";
  }
  if ( m_PedDegree > 1 ) {
    FloatVector& vec = *(++ivec);
    for ( Index isam=0; isam<nsam; ++isam ) {
      float dsam = isam - m_PedTick0;
      vec[isam] = dsam*dsam;
    }
    m_fitNames[ifit++] = "Curvature";
  }
  float twopi = 2.0*acos(-1.0);
  for ( float frq : m_PedFreqs ) {
    FloatVector& cvec = *(++ivec);
    FloatVector& svec = *(++ivec);
    for ( Index isam=0; isam<nsam; ++isam ) {
      cvec[isam] = cos(twopi*frq*isam);
      svec[isam] = sin(twopi*frq*isam);
    }
    m_fitNames[ifit++] = "Cos";
    m_fitNames[ifit++] = "Sin";
  }
  if ( ivec != m_pedVectors.end() ) {
    cout << myname << "ERROR: Unexpected pedestal vector size: " << m_pedVectors.size() << endl;
  }
  if ( m_LogLevel >= 1 ) {
    cout << myname << "Parameters:" << endl;
    cout << myname << "              LogLevel: " << m_LogLevel << endl;
    cout << myname << "            SignalFlag: " << m_SignalFlag << endl;
    cout << myname << "  SignalIterationLimit: " << m_SignalIterationLimit << endl;
    cout << myname << "            SignalTool: " << m_SignalTool << endl;
    cout << myname << "             DecayTime: " << m_DecayTime << endl;
    cout << myname << "               MaxTick: " << m_MaxTick << endl;
    cout << myname << "             PedDegree: " << m_PedDegree << endl;
    cout << myname << "              PedTick0: " << m_PedTick0 << endl;
    cout << myname << "              PedFreqs: [";
    bool first = true;
    for ( float frq : m_PedFreqs ) {
      if ( first ) first = false;
      else cout << ", ";
      cout << frq;
    }
    cout << "]" << endl;
    cout << myname << "  IncludeChannelRanges: [";
    first = true;
    for ( Name crn : m_IncludeChannelRanges ) {
      if ( first ) first = false;
      else cout << ", ";
      cout  << crn;
    }
    cout << "]" << endl;
    cout << myname << "  ExcludeChannelRanges: [";
    first = true;
    for ( Name crn : m_ExcludeChannelRanges ) {
      if ( first ) first = false;
      else cout << ", ";
      cout  << crn;
    }
    cout << "]" << endl;
    if ( m_useChannelRanges ) {
      cout << myname << "Channel checking enabled for " << nchaCheck << " channel"
           << ( nchaCheck == 1 ? "" : "s") << "." << endl;
    } else {
      cout << myname << "Channel checking disabled." << endl;
    }
  }
}

//**********************************************************************

DataMap ExpTailPedRemover::update(AdcChannelData& acd) const {
  const string myname = "ExpTailPedRemover::update: ";
  DataMap ret;

  // Save the data before tail removal.
  AdcSignalVector samples = acd.samples;
  Index nsam = samples.size();
  string sampleUnit = acd.sampleUnit;

  // Check the channel.
  if ( m_useChannelRanges ) {
    if ( acd.channel >= m_checkChannels.size() || ! m_checkChannels[acd.channel] ) {
      if ( m_LogLevel >= 2 ) cout << myname << "Skipping channel " << acd.channel << endl;
      return ret;
    }
  }

  // Check input data size.
  if ( nsam < 10 ) {
    cout << myname << "WARNING: Data for channel " << acd.channel << " has "
         << ( nsam==0 ? "no" : "too few" ) << " ticks." << endl;
    return ret.setStatus(1);
  }

  if ( m_LogLevel >= 2 ) cout << myname << "Correcting run " << acd.run << " event " << acd.event
                              << " channel " << acd.channel << endl;

  // Build the initial signal selection.
  bool checkSignal = true;   // Whether to use only non-signal in fit.
  bool findSignal = false;   // Whether to find signals each iteration.
  if ( m_SignalFlag == 0 ) {
    checkSignal = false;
  } else if ( m_SignalFlag == 1  ) {
    if ( acd.signal.size() < nsam ) {
      cout << myname << "WARNING: Data is missing signal flags--padding from " << acd.signal.size()
           << " to " << nsam << " samples." << endl;
    }
  } else if ( m_SignalFlag >= 2  ) {
    if ( m_pSignalTool == nullptr ) {
      cout << myname << "WARNING: Signal-finding tool is missing. Using all signals." << endl;
      checkSignal = false;
    } else {
      findSignal = true;
    }
  }

  Index niter = 0;  // # fit iterations
  Index ncof = 1 + m_pedVectors.size();
  FloatVector cofs(ncof, 0);    // {tau0, lam1, lam2, ...} (ped = lam1)
  Index nsamFit = 0;
  Index maxiter = findSignal ? m_SignalIterationLimit : 1;
  double noise = 0.0;
  while ( niter < maxiter ) {
    // Do signal finding.
    if ( findSignal ) {
      AdcFilterVector signalLast = acd.signal;
      DataMap fret = m_pSignalTool->update(acd);
      if ( fret ) {
        cout << myname << "WARNING: Signal-finding failed for event " << acd.event
             << " channel " << acd.channel << endl;
        break;
      }
      if ( acd.signal == signalLast ) {
        if ( m_LogLevel >=3 ) cout << myname << "Signal is unchanged. Exiting loop." << endl;
        break;
      }
    }
    //
    // Evaluate the signal coefficients.
    using DoubleVector = std::vector<double>;
    using DoubleVectorVector = std::vector<DoubleVector>;
    DoubleVectorVector cofvecs(ncof, DoubleVector(nsam, 0.0));    // Ctau, Clam1, Clam2, ...
    SampleTailer sta(m_DecayTime);
    sta.setTail0(1.0);
    sta.setDataZero(nsam);
    DoubleVector& ctau = cofvecs[0];
    copy(sta.signal().begin(), sta.signal().end(), ctau.begin());
    for ( Index icof=1; icof<ncof; ++icof ) {
      sta.setTail0(0.0);
      sta.setPedestalVector(&m_pedVectors[icof-1]);
      sta.setDataZero(nsam);
      DoubleVector& cped = cofvecs[icof];
      copy(sta.signal().begin(), sta.signal().end(), cped.begin());
    }
    sta.setPedestal(0.0);
    sta.setData(samples);
    DoubleVector cdat(nsam);
    copy(sta.signal().begin(), sta.signal().end(), cdat.begin());
    // Evaluate the K-params.
    TMatrixDSym kmat(ncof);
    TVectorD kvec(ncof);
    nsamFit = 0;
    for ( Index  isam=0; isam<nsam; ++isam ) {
      if ( checkSignal &&  isam < acd.signal.size() && acd.signal[isam] ) continue;
      double cd = cdat[isam];
      for ( Index icof=0; icof<ncof; ++icof ) {
        double ci = cofvecs[icof][isam];
        kvec[icof] += cd*ci;
        for ( Index jcof=0; jcof<ncof; ++jcof ) {
          double cj = cofvecs[jcof][isam];
          kmat[icof][jcof] += ci*cj;
        }
      }
      ++nsamFit;
    }
    // Invert matrix and solve for (ped, tau).
    double det = 0.0;
    kmat.Invert(&det);
    if ( ! kmat.IsValid() || det == 0.0 ) {
      if ( acd.channelStatus == 0 || m_LogLevel >= 2 ) {
        cout << myname << "WARNING: Unable to invert K-matrix with "
             << nsamFit << " of " << nsam << " samples--stopping iteration for channel "
             << acd.channel << " with status " << acd.channelStatus << "." << endl;
        for ( Index icof=0; icof<ncof; ++icof ) {
          cout << myname;
          for ( Index jcof=0; jcof<ncof; ++jcof ) {
            cout << setw(20) << kmat[icof][jcof];
          }
          cout << endl;
        }
      }
      break;
    }
    kvec *= kmat;
    kvec *= -1.0;   // kvec now holds the fitted coefficients {tau, lam1, lam2, ...}
    for ( Index icof=0; icof<ncof; ++icof ) {
      cofs[icof] = kvec[icof];
    }
    float tau = cofs[0];
    FloatVector peds(nsam, 0.0);
    for ( Index icof=1; icof<ncof; ++icof ) {
      for ( Index isam=0; isam<nsam; ++isam ) {
        peds[isam] += cofs[icof]*m_pedVectors[icof-1][isam];
      }
    }
    // Use the parameters to remove tail from the original data and store
    // these tail-subtracted samples in the channel data.
    sta.setTail0(tau);
    sta.setPedestalVector(&peds);
    sta.setData(samples);
    acd.samples = sta.signal();
    // Evaluate the non-signal rms.
    double rms = 0.0;
    for ( Index  isam=0; isam<nsam; ++isam ) {
      if ( checkSignal &&  isam < acd.signal.size() && acd.signal[isam] ) continue;
      double sig = sta.signal()[isam];
      rms += sig*sig;
    }
    Index ndof = nsamFit > ncof ? nsamFit - ncof : 1;
    noise = sqrt(rms)/ndof;
    // Update iteration count.
    if ( m_LogLevel >= 3 ) {
      cout << myname << "Iteration " << niter << ": noise=" << noise << "; tau, {ped} : {";
      bool first = true;
      for ( float cof : cofs ) {
        if ( first ) first = false;
        else cout << ", ";
        cout << cof;
      }
      cout << "}" << endl;
    }
    ++niter;
  }

  // Log result of interation.
  if ( m_LogLevel >= 2 ) {
    cout << myname << "Iteration count: " << niter << endl;
    cout << myname << "Noise: " << noise;
    if ( sampleUnit.size() ) cout << " " << sampleUnit;
    cout << " from " << nsamFit << "/" << nsam << " channels" << endl;
    cout << myname << "Final tau0, ped params: " << " {";
    bool first = false;
    for ( float cof : cofs ) {
      if ( first ) first = false;
      else cout << ", ";
      cout << cof;
    }
    cout << "}" << endl;
  }

  // Use the fitted
  acd.metadata["uscPedestal"] = cofs[1];
  acd.metadata["uscTail"] = cofs[0];
  acd.metadata["uscNoise"] = noise;
  for ( Index icof=0; icof<cofs.size(); ++icof ) {
    Name name = "usc" + m_fitNames[icof];
    ret.setFloat(name, cofs[icof]);
  }
  ret.setFloat("uscNoise", noise);
  ret.setInt("uscNsamFit", nsamFit);
  ret.setInt("uscNiteration", niter);

  return ret;
}

//**********************************************************************
