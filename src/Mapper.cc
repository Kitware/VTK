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
#include "Mapper.hh"

// Description:
// Construct with initial range (0,1).
vlMapper::vlMapper()
{
  this->StartRender = NULL;
  this->StartRenderArg = NULL;
  this->EndRender = NULL;
  this->EndRenderArg = NULL;

  this->LookupTable = NULL;

  this->ScalarsVisible = 1;
  this->ScalarRange[0] = 0.0; this->ScalarRange[1] = 1.0;
}

vlMapper::~vlMapper()
{
  if (this->LookupTable)
    {
    this->LookupTable->UnRegister(this);
    }
}

// Description:
// Overload standard modified time function. If cut functions is modified,
// then we are modified as well.
unsigned long vlMapper::GetMTime()
{
  unsigned long mTime=this->MTime.GetMTime();
  unsigned long lutMTime;

  if ( this->LookupTable != NULL )
    {
    lutMTime = this->LookupTable->GetMTime();
    mTime = ( lutMTime > mTime ? lutMTime : mTime );
    }

  return mTime;
}

void vlMapper::operator=(const vlMapper& m)
{
  this->SetLookupTable(m.LookupTable);

  this->SetScalarsVisible(m.ScalarsVisible);
  this->SetScalarRange(m.ScalarRange[0], m.ScalarRange[1]);

  this->SetStartRender(m.StartRender,m.StartRenderArg);
  this->SetEndRender(m.EndRender,m.EndRenderArg);
}

// Description:
// Specify a function to be called before rendering process begins.
// Function will be called with argument provided.
void vlMapper::SetStartRender(void (*f)(void *), void *arg)
{
  if ( f != this->StartRender || arg != this->StartRenderArg )
    {
    this->StartRender = f;
    this->StartRenderArg = arg;
    this->Modified();
    }
}

// Description:
// Specify a function to be called when rendering process completes.
// Function will be called with argument provided.
void vlMapper::SetEndRender(void (*f)(void *), void *arg)
{
  if ( f != this->EndRender || arg != EndRenderArg )
    {
    this->EndRender = f;
    this->EndRenderArg = arg;
    this->Modified();
    }
}

void vlMapper::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlMapper::GetClassName()))
    {
    vlObject::PrintSelf(os,indent);

    os << indent << "Build Time: " <<this->BuildTime.GetMTime() << "\n";
    if ( this->StartRender )
      {
      os << indent << "Start Render method defined.\n";
      }
    else
      {
      os << indent << "No Start Render method.\n";
      }

    if ( this->EndRender )
      {
      os << indent << "End Render method defined.\n";
      }
    else
      {
      os << indent << "No End Render method.\n";
      }

    if ( this->LookupTable )
      {
      os << indent << "Lookup Table:\n";
      this->LookupTable->PrintSelf(os,indent.GetNextIndent());
      }
    else
      {
      os << indent << "Lookup Table: (none)\n";
      }
    os << indent << "Scalars Visible: " 
      << (this->ScalarsVisible ? "On\n" : "Off\n");

    float *range = this->GetScalarRange();
    os << indent << "Scalar Range: (" << range[0] << ", " << range[1] << ")\n";
   }
}
