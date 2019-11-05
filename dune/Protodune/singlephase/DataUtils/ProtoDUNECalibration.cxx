#include "ProtoDUNECalibration.h"


protoana::ProtoDUNECalibration::ProtoDUNECalibration(const fhicl::ParameterSet & pset) : 
  planeID( pset.get< unsigned int >( "PlaneID" ) ),
  betap( pset.get< double >( "betap" ) ),
  Rho( pset.get< double >( "Rho" ) ),
  Wion( pset.get< double >( "Wion" ) ),
  norm_factor( pset.get< double >( "norm_factor" ) ),
  calib_factor( pset.get< double >( "calib_factor" ) ),
  X_correction_name( pset.get< std::string >( "X_correction" ) ),
  YZ_correction_name( pset.get< std::string >( "YZ_correction" ) ),
  E_field_correction_name( pset.get< std::string >("E_field_correction") )

{
  X_correction_file  = new TFile( X_correction_name.c_str(), "OPEN" );
  YZ_correction_file = new TFile( YZ_correction_name.c_str(), "OPEN" );
  E_field_file       = new TFile( E_field_correction_name.c_str(), "OPEN" );
  ///planeID = pset.get< unsigned int >( "PlaneID" );
  /// 
  ///betap   = pset.get< double >( "betap"  );
  ///Rho     = pset.get< double >( "Rho"    );
  /////Efield  = pset.get< double >( "Efield" );
  ///Wion    = pset.get< double >( "Wion"   );
  ///alpha   = pset.get< double >( "alpha"  );

  ///norm_factor  = pset.get< double >( "norm_factor" );
  ///calib_factor = pset.get< double >( "calib_factor" );

  ///X_correction_name = pset.get< std::string >( "X_correction" );
  ///YZ_correction_name = pset.get< std::string >( "YZ_correction" );

  ///X_correction_file = TFile( X_correction_name.c_str(), "OPEN" );
  //X_correction_hist = NULL;

  //UseNewVersion = pset.get< bool >( "UseNewVersion", false );
  //if( UseNewVersion ){
    std::string hist_name = "dqdx_X_correction_hist_" + std::to_string(planeID);
    X_correction_hist = (TH1F*)X_correction_file->Get( hist_name.c_str() );
  //  
  //}
  //else{
  //  X_correction_hist = (TH1F*)X_correction_file.Get( "dqdx_X_correction_hist" );
//  }

  ///YZ_correction_file = TFile( YZ_correction_name.c_str(), "OPEN" );
  YZ_neg = (TH2F*)YZ_correction_file->Get("correction_dqdx_ZvsY_negativeX_hist_2");
  YZ_pos = (TH2F*)YZ_correction_file->Get("correction_dqdx_ZvsY_positiveX_hist_2");

  ///E_field_correction_name = pset.get< std::string >("E_field_correction");
  
 ///E_field_file = TFile( E_field_correction_name.c_str(), "OPEN" );
 ex_neg = (TH3F*)E_field_file->Get("Reco_ElecField_X_Neg");
 ey_neg = (TH3F*)E_field_file->Get("Reco_ElecField_Y_Neg");
 ez_neg = (TH3F*)E_field_file->Get("Reco_ElecField_Z_Neg");
 ex_pos = (TH3F*)E_field_file->Get("Reco_ElecField_X_Pos");
 ey_pos = (TH3F*)E_field_file->Get("Reco_ElecField_Y_Pos");
 ez_pos = (TH3F*)E_field_file->Get("Reco_ElecField_Z_Pos");
 
}

std::vector< float >  protoana::ProtoDUNECalibration::GetCalibratedCalorimetry(  const recob::Track &track, art::Event const &evt, const std::string trackModule, const std::string caloModule ) {


  std::vector< float > calibrated_dEdx;

  //Get the Calorimetry vector from the track
  std::vector< anab::Calorimetry > caloVector = trackUtil.GetRecoTrackCalorimetry( track, evt, trackModule, caloModule ); 
  
  size_t calo_position;
  bool found_plane = false;
  for( size_t i = 0; i < caloVector.size(); ++i ){
     unsigned int thePlane = caloVector.at(i).PlaneID().Plane;
     if( thePlane == planeID ){
       calo_position = i;
       found_plane = true;
       break;
     }
  }

  if( !found_plane ){
    std::cout << "Could not find the correct plane in the calorimetry vector" << std::endl;
    return calibrated_dEdx;
  }

  std::vector< float > dQdX = caloVector.at( calo_position).dQdx();
  auto theXYZPoints = caloVector.at( calo_position).XYZ();
  std::vector< float > pitch = caloVector.at( calo_position ).TrkPitchVec();
  std::vector< float > resRange = caloVector.at( calo_position ).ResidualRange();

  //Get the hits from the track from a specific plane
  const std::vector< const recob::Hit* > hits = trackUtil.GetRecoTrackHitsFromPlane( track, evt, trackModule, planeID ); 
  if( hits.size() == 0 ){
    std::cout << "Got empty hits vector" << std::endl;
    return calibrated_dEdx;
  }

  //Do Ajib's correction 
  for( size_t i = 0; i < dQdX.size(); ++i ){ 
    float hit_x = theXYZPoints[i].X();
    float hit_y = theXYZPoints[i].Y();
    float hit_z = theXYZPoints[i].Z();

    if( hit_y < 0. || hit_y > 600. ) continue;
    if( hit_z < 0. || hit_z > 695. ) continue;
    if( pitch[i] < .5 || pitch[i] > .8 ) continue;

    if( (int(resRange[i])/5 ) >= 40 ) continue; 


    int X_bin = X_correction_hist->FindBin( hit_x );
    float X_correction = X_correction_hist->GetBinContent(X_bin);


    double YZ_correction = (
      hit_x < 0  
      ? YZ_neg->GetBinContent( YZ_neg->FindBin( hit_z, hit_y ) ) 
      : YZ_pos->GetBinContent( YZ_pos->FindBin( hit_z, hit_y ) )  
    );



    float corrected_dq_dx = dQdX[i] * X_correction * YZ_correction * norm_factor;
    float scaled_corrected_dq_dx = corrected_dq_dx / calib_factor;

    double Efield = tot_Ef( hit_x, hit_y, hit_z );


    float cal_de_dx = calc_dEdX( scaled_corrected_dq_dx,  betap,  Rho,  Efield,  Wion,  alpha );
 
    calibrated_dEdx.push_back( cal_de_dx );
  }


  return calibrated_dEdx;
}

float protoana::ProtoDUNECalibration::calc_dEdX(double dqdx, double betap, double Rho, double Efield, double Wion, double alpha){
  return ( exp( dqdx * ( betap / ( Rho * Efield ) * Wion ) ) -alpha ) / ( betap / ( Rho*Efield ) );  
}

double protoana::ProtoDUNECalibration::tot_Ef( double x, double y, double z ){

  if( x >= 0 ){
    double ex = 0.5 + 0.5 * ex_pos->GetBinContent( ex_pos->FindBin( x, y, z ) );
    double ey = 0.5 * ey_pos->GetBinContent( ey_pos->FindBin( x, y, z ) );
    double ez = 0.5 * ez_pos->GetBinContent( ez_pos->FindBin( x, y, z ) );
    return sqrt( (ex*ex) + (ey*ey) + (ez*ez) );
  }
  else if( x < 0 ){
    double ex= 0.5 + 0.5 * ex_neg->GetBinContent( ex_neg->FindBin( x, y, z ) );
    double ey= 0.5 * ey_neg->GetBinContent( ey_neg->FindBin( x, y, z ) );
    double ez= 0.5 * ez_neg->GetBinContent( ez_neg->FindBin( x, y, z ) );
    return sqrt( (ex*ex) + (ey*ey) + (ez*ez) );
  }
  else return 0.5;
}
