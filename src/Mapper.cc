/*=========================================================================

  Program:   Visualization Library
  Module:    Mapper.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
//
// Methods for abstract class mapper
//
#include "Mapper.hh"

vlMapper::vlMapper()
{
  this->StartRender = 0;
  this->EndRender = 0;
  this->LookupTable = 0;
  this->ScalarsVisible = 1;
  this->ScalarRange[0] = 0.0; this->ScalarRange[1] = 1.0;
}

vlMapper::~vlMapper()
{
  if (this->LookupTable)
    {
    this->LookupTable->UnRegister((void *)this);
    }
}

void vlMapper::operator=(const vlMapper& m)
{
  if (this->LookupTable) this->LookupTable->UnRegister((void *)this);
  this->LookupTable = m.LookupTable;
  if (this->LookupTable) this->LookupTable->Register((void *)this);

  this->ScalarsVisible = m.ScalarsVisible;
  this->ScalarRange[0] = m.ScalarRange[0];
  this->ScalarRange[1] = m.ScalarRange[1];

  this->StartRender = m.StartRender;
  this->EndRender = m.EndRender;

  this->Modified();
}

void vlMapper::SetStartRender(void (*f)())
{
  if ( f != this->StartRender )
    {
    this->StartRender = f;
    this->Modified();
    }
}

void vlMapper::SetEndRender(void (*f)())
{
  if ( f != this->EndRender )
    {
    this->EndRender = f;
    this->Modified();
    }
}

void vlMapper::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlMapper::GetClassName()))
    {
    vlObject::PrintSelf(os,indent);

    if ( this->StartRender )
      {
      os << indent << "StartRender method defined.\n";
      }
    else
      {
      os << indent << "No StartRender method.\n";
      }

    if ( this->EndRender )
      {
      os << indent << "EndRender method defined.\n";
      }
    else
      {
      os << indent << "No EndRender method.\n";
      }

    if ( this->LookupTable )
      {
      os << indent << "Lookup table:\n";
      this->LookupTable->PrintSelf(os,indent.GetNextIndent());
      }
    else
      {
      os << indent << "LookupTable: (none)\n";
      }

    if ( this->ScalarsVisible )
      {
      os << indent << "Scalar data is visible.\n";
      }
    else
      {
      os << indent << "Scalar data is not visible.\n";
      }

    float *range = this->GetScalarRange();
    os << indent << "Scalar range: (" << range[0] << ", " << range[1] << ")\n";
    os << indent << "Build time: " <<this->BuildTime.GetMtime() << "\n";
   }
}
