////////////////////////////////////////////////////////////////////////
/// \file    GCNGraphNode.cxx
/// \brief   Node for GCN
/// \author  Leigh H. Whitehead - leigh.howard.whitehead@cern.ch
///////////////////////////////////////////////////////////////////////

#include <cassert>
#include <iostream>
#include <ostream>
#include "dune/CVN/func/GCNGraphNode.h"

namespace cvn
{

  GCNGraphNode::GCNGraphNode()
  {}

  GCNGraphNode::GCNGraphNode(std::vector<float> position,std::vector<float> features):
  fPosition(position),
  fFeatures(features)
  {

  }

  /// Get the node position
  const std::vector<float> GCNGraphNode::GetPosition() const
  {
    return fPosition;
  }

  // Get the node features
  const std::vector<float> GCNGraphNode::GetFeatures() const
  {
    return fFeatures;
  }

  /// Add a node position coordinate
  void GCNGraphNode::AddPositionCoordinate(float pos){
    fPosition.push_back(pos);
  }

  /// Add a node feature
  void GCNGraphNode::AddFeature(float feature){
    fFeatures.push_back(feature);
  }

  /// Get the number of features
  const unsigned int GCNGraphNode::GetNumberOfFeatures() const
  {
    return fFeatures.size();
  }

  /// Get the number of features
  const unsigned int GCNGraphNode::GetNumberOfCoordinates() const
  {
    return fPosition.size();
  }
  
  /// Get feature
  const float GCNGraphNode::GetFeature(const unsigned int feature) const
  {
    if(feature >= fFeatures.size()){
      return fFeatures.at(feature);
    }
    else{
      return -999.;
    }
  }

}

