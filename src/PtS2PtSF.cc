/*=========================================================================

  Program:   Visualization Library
  Module:    PtS2PtSF.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Description:
---------------------------------------------------------------------------
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

void vlPointSetToPointSetFilter::Update()
{
  vlPointData *pd;

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

vlMapper *vlPointSetToPointSetFilter::MakeMapper()
{
//
// A little tricky because mappers must be of concrete type, but this class 
// deals at abstract level of PointSet.  Depending upon Input member of this 
// filter, mapper may change.  Hence need to anticipate change in Input and 
// create new mappers as necessary.
//
  vlMapper *mapper;

  vlPointSetToPointSetFilter::Update(); // compiler bug, had to hard code call
  mapper = this->PointSet->MakeMapper();
  if ( !this->Mapper || mapper != this->Mapper )
    {
    if (this->Mapper) this->Mapper->UnRegister(this);
    this->Mapper = mapper;
    this->Mapper->Register(this);
    }
  return this->Mapper;
}

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
