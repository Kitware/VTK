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
  this->DataSet->Register(this);
}

vlDataSetToDataSetFilter::~vlDataSetToDataSetFilter()
{
  this->DataSet->UnRegister(this);
}

void vlDataSetToDataSetFilter::Update()
{
  vlPointData *pd;

  vlDataSetFilter::Update();
  // Following copies data from this filter to internal dataset
  pd = this->DataSet->GetPointData();
  *pd = this->PointData;
}

void vlDataSetToDataSetFilter::Initialize()
{
  if ( this->Input )
    {
    this->DataSet->UnRegister(this);
    // copies input geometry to internal data set
    this->DataSet = this->Input->MakeObject(); 
    this->DataSet->Register(this);
    }
  else
    {
    return;
    }
}

void vlDataSetToDataSetFilter::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlDataSetToDataSetFilter::GetClassName()))
    {
    this->PrintWatchOn(); // watch for multiple inheritance

    vlDataSet::PrintSelf(os,indent);
    vlDataSetFilter::PrintSelf(os,indent);

    if ( this->DataSet )
      {
      os << indent << "DataSet: (" << this->DataSet << ")\n";
      os << indent << "DataSet type: " << this->DataSet->GetClassName() << "\n";
      }
    else
      {
      os << indent << "DataSet: (none)\n";
      }

    this->PrintWatchOff(); // stop worrying about it now
   }
}
