/*=========================================================================

  Program:   Visualization Library
  Module:    PtS2PtSF.cc
  Language:  C++
  Date:      11/6/94
  Version:   1.8

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
}

vlPointSetToPointSetFilter::~vlPointSetToPointSetFilter()
{
  delete this->PointSet;
}

vlDataSet* vlPointSetToPointSetFilter::MakeObject()
{
//  vlPointSetToPointSetFilter *o = new vlPointSetToPointSetFilter();
  vlPointSetToPointSetFilter *o;
  o->PointSet = this->PointSet;
  o->SetPoints(this->GetPoints());
  return o;
}

void vlPointSetToPointSetFilter::Modified()
{
  this->vlPointSet::Modified();
  this->vlPointSetFilter::_Modified();
}

unsigned long int vlPointSetToPointSetFilter::GetMTime()
{
  unsigned long dtime = this->vlPointSet::GetMTime();
  unsigned long ftime = this->vlPointSetFilter::_GetMTime();
  return (dtime > ftime ? dtime : ftime);
}

void vlPointSetToPointSetFilter::Update()
{
  this->UpdateFilter();
}

void vlPointSetToPointSetFilter::Initialize()
{
  if ( this->Input != NULL )
    {
    vlDataSet *ds=this->Input->MakeObject();
    delete this->PointSet;
    // copies input geometry to internal data set
    this->PointSet = ds;
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
  vlPointSet::PrintSelf(os,indent);
  vlPointSetFilter::_PrintSelf(os,indent);

  if ( this->PointSet )
    {
    os << indent << "PointSet: (" << this->PointSet << ")\n";
    os << indent << "PointSet type: " << this->PointSet->GetClassName() <<"\n";
    }
  else
    {
    os << indent << "PointSet: (none)\n";
    }
}
