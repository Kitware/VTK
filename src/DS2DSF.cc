/*=========================================================================

  Program:   Visualization Library
  Module:    DS2DSF.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "DS2DSF.hh"
#include "PolyData.hh"

vlDataSetToDataSetFilter::vlDataSetToDataSetFilter()
{
  // prevents dangling reference to DataSet
  this->DataSet = new vlPolyData;
}

vlDataSetToDataSetFilter::~vlDataSetToDataSetFilter()
{
  delete this->DataSet;
}

// Description:
// Initialize method is fancy: creates an internal dataset that holds
// geometry representation. All methods directed at geometry are 
// forwarded to internal dataset.
void vlDataSetToDataSetFilter::Initialize()
{
  if ( this->Input )
    {
    delete this->DataSet;
    // copies input geometry to internal data set
    this->DataSet = this->Input->MakeObject(); 
    }
  else
    {
    return;
    }
}

void vlDataSetToDataSetFilter::ComputeBounds()
{
  float *bounds = this->DataSet->GetBounds();
  for (int i=0; i<6; i++) this->Bounds[i] = bounds[i];
}

void vlDataSetToDataSetFilter::Modified()
{
  this->vlDataSet::Modified();
  this->vlDataSetFilter::_Modified();
}

void vlDataSetToDataSetFilter::DebugOn()
{
  vlDataSet::DebugOn();
  vlDataSetFilter::_DebugOn();
}

void vlDataSetToDataSetFilter::DebugOff()
{
  vlDataSet::DebugOff();
  vlDataSetFilter::_DebugOff();
}

unsigned long int vlDataSetToDataSetFilter::GetMTime()
{
  unsigned long dtime = this->vlDataSet::GetMTime();
  unsigned long ftime = this->vlDataSetFilter::_GetMTime();
  return (dtime > ftime ? dtime : ftime);
}

int vlDataSetToDataSetFilter::GetDataReleased()
{
  return this->DataReleased;
}

void vlDataSetToDataSetFilter::SetDataReleased(int flag)
{
  this->DataReleased = flag;
}

void vlDataSetToDataSetFilter::Update()
{
  this->UpdateFilter();
}

void vlDataSetToDataSetFilter::PrintSelf(ostream& os, vlIndent indent)
{
  vlDataSet::PrintSelf(os,indent);
  vlDataSetFilter::_PrintSelf(os,indent);

  if ( this->DataSet )
    {
    os << indent << "DataSet: (" << this->DataSet << ")\n";
    os << indent << "DataSet type: " << this->DataSet->GetClassName() << "\n";
    }
  else
    {
    os << indent << "DataSet: (none)\n";
    }
}
