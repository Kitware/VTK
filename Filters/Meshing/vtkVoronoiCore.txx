// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkVoronoiCore.h"

#ifndef vtkVoronoiCore_txx
#define vtkVoronoiCore_txx

VTK_ABI_NAMESPACE_BEGIN

//----------------------------------------------------------------------------
inline void vtkVoronoiAdjacencyGraph::Initialize(vtkIdType numWheels, vtkIdType numSpokes)
{
  // Cap off the wheels offsets array.
  this->Wheels[numWheels] = numSpokes;

  // Now copy the spokes from local thread data to the global
  // spokes array.
  this->Spokes.resize(numSpokes);
}

//----------------------------------------------------------------------------
// Given a pair of point ids (pt0,pt1), determine if the spoke (pt0,pt1)
// emanates from the wheel at pt0.
inline bool vtkVoronoiAdjacencyGraph::IsSpoke(vtkIdType pt0, vtkIdType pt1)
{
  vtkIdType numSpokes = this->GetNumberOfSpokes(pt0);
  vtkVoronoiSpoke* spokes = &(this->Spokes[this->Wheels[pt0]]);

  for (vtkIdType i = 0; i < numSpokes; ++i, ++spokes)
  {
    if (spokes->NeiId == pt1)
    {
      return true;
    }
  } // for all spokes in this wheel
  return false;
}

//----------------------------------------------------------------------------
inline vtkIdType vtkVoronoiAdjacencyGraph::GetNumberOfSpokes(vtkIdType ptId)
{
  return (this->Wheels[ptId + 1] - this->Wheels[ptId]);
}

//----------------------------------------------------------------------------
inline vtkVoronoiSpoke* vtkVoronoiAdjacencyGraph::GetSpokes(vtkIdType ptId, vtkIdType& numSpokes)
{
  numSpokes = (this->Wheels[ptId + 1] - this->Wheels[ptId]);
  return (this->Spokes.data() + this->Wheels[ptId]);
}

//----------------------------------------------------------------------------
inline void vtkVoronoiAdjacencyGraph::CountFaces(const vtkVoronoiSpoke* spokes, int numSpokes,
  int& numDomainBoundaryFaces, int& numRegionBoundaryFaces, int& numForwardFaces)
{
  numDomainBoundaryFaces = 0;
  numRegionBoundaryFaces = 0;
  numForwardFaces = 0;
  for (int i = 0; i < numSpokes; ++i)
  {
    if ((spokes[i].Classification & vtkSpokeClassification::DOMAIN_BOUNDARY))
    {
      numDomainBoundaryFaces++;
    }
    else if ((spokes[i].Classification & vtkSpokeClassification::FORWARD_SPOKE))
    {
      if ((spokes[i].Classification & vtkSpokeClassification::REGION_BOUNDARY))
      {
        numRegionBoundaryFaces++;
      }
      else
      {
        numForwardFaces++;
      }
    } // if forward spoke
  }   // for all spokes. Note backward spokes and invalid spokes ommitted
} // CountFaces

//----------------------------------------------------------------------------
// vtkSMPTools threaded interface
inline void vtkVoronoiAdjacencyGraph::ValidateAdjacencyGraph::Initialize()
{
  this->ThreadNumInvalid.Local() = 0;
}

//----------------------------------------------------------------------------
inline void vtkVoronoiAdjacencyGraph::ValidateAdjacencyGraph::operator()(
  vtkIdType wheelId, vtkIdType endWheelId)
{
  vtkVoronoiWheel wheel(this->Graph.Wheels, this->Graph.Spokes);
  int spokeNum, numSpokes;
  vtkVoronoiSpoke* spoke;
  vtkIdType& numInvalid = this->ThreadNumInvalid.Local();

  for (; wheelId < endWheelId; ++wheelId)
  {
    spoke = wheel.Initialize(wheelId, numSpokes);
    for (spokeNum = 0; spokeNum < numSpokes; ++spokeNum, ++spoke)
    {
      // Skip domain boundary spokes as there will be only one of these.
      if ((spoke->Classification & vtkSpokeClassification::DOMAIN_BOUNDARY))
      {
        continue;
      }

      // See if the spoke is valid (in the reverse order).
      if (!this->Graph.IsSpoke(spoke->NeiId, wheelId))
      {
        numInvalid++;
      } // if return edge does not exist
    }   // over all spokes for this wheel
  }     // for all wheels
}

//----------------------------------------------------------------------------
inline void vtkVoronoiAdjacencyGraph::ValidateAdjacencyGraph::Reduce()
{
  this->NumInvalid = 0;
  for (auto& localInvalid : this->ThreadNumInvalid)
  {
    this->NumInvalid += localInvalid;
  }
}

//----------------------------------------------------------------------------
// Note that this method simply determines whether the necessary conditions on
// spokes are satified (valid==true). The necessary conditions are basically:
// each spoke is used twice (i.e., alternatively, each face is used twice). If
// not valid, additonal steps can be taken to delete invalid spokes etc.
inline bool vtkVoronoiAdjacencyGraph::Validate()
{
  // Make sure there is something to validate.
  bool isValid;
  if (this->GetNumberOfSpokes() <= 0)
  {
    isValid = true;
  }
  else
  {
    vtkVoronoiAdjacencyGraph::ValidateAdjacencyGraph validate(*this);
    vtkSMPTools::For(0, this->GetNumberOfWheels(), validate);
    isValid = (validate.NumInvalid <= 0);
  }

  vtkGenericWarningMacro("All Valid: " << (isValid ? "True\n" : "False\n"));
  return isValid;
}

VTK_ABI_NAMESPACE_END
#endif
