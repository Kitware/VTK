/*=========================================================================

  Program:   Visualization Library
  Module:    PtS2PtSF.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "PtS2PtSF.hh"
#include "PolyData.hh"

vlPointSetToPointSetFilter::vlPointSetToPointSetFilter()
{
  // prevents dangling reference to PointSet
  this->PointSet = new vlPolyData;
  this->PointSet->Register(this);
}

vlPointSetToPointSetFilter::~vlPointSetToPointSetFilter()
{
  this->PointSet->UnRegister(this);
}

vlDataSet* vlPointSetToPointSetFilter::MakeObject()
{
  vlPointSetToPointSetFilter *o = new vlPointSetToPointSetFilter();
  o->PointSet = this->PointSet;
  o->PointSet->Register(o);
  o->SetPoints(this->GetPoints());
  return o;
}

void vlPointSetToPointSetFilter::Update()
{
  vlPointSetFilter::Update();
}

void vlPointSetToPointSetFilter::Initialize()
{
  if ( this->Input )
    {
    this->PointSet->UnRegister(this);
    // copies input geometry to internal data set
    this->PointSet = this->Input->MakeObject(); 
    this->PointSet->Register(this);
    }
  else
    {
    return;
    }
}

void vlPointSetToPointSetFilter::ComputeBounds()
{
  if ( this->Points != NULL )
    {
    this->Points->ComputeBounds();
    float *bounds=this->Points->GetBounds();
    for (int i=0; i < 6; i++) this->Bounds[i] = bounds[i];
    }
};

void vlPointSetToPointSetFilter::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlPointSetToPointSetFilter::GetClassName()))
    {
    this->PrintWatchOn(); // watch for multiple inheritance

    vlPointSet::PrintSelf(os,indent);
    vlPointSetFilter::PrintSelf(os,indent);

    if ( this->PointSet )
      {
      os << indent << "PointSet: (" << this->PointSet << ")\n";
      os << indent << "PointSet type: " << this->PointSet->GetClassName() << "\n";
      }
    else
      {
      os << indent << "PointSet: (none)\n";
      }

    this->PrintWatchOff(); // stop worrying about it now
   }
}
