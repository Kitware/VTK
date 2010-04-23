/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDisplayListPainter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkDisplayListPainter.h"

#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkGraphicsFactory.h"
#include "vtkObjectFactory.h"

// Needed when we don't use the vtkStandardNewMacro.
vtkInstantiatorNewMacro(vtkDisplayListPainter);
vtkInformationKeyMacro(vtkDisplayListPainter, IMMEDIATE_MODE_RENDERING, Integer);

//----------------------------------------------------------------------------
vtkDisplayListPainter::vtkDisplayListPainter()
{
  this->ImmediateModeRendering = 0;
}

//----------------------------------------------------------------------------
vtkDisplayListPainter::~vtkDisplayListPainter()
{
}

//----------------------------------------------------------------------------
vtkDisplayListPainter* vtkDisplayListPainter::New()
{
  vtkObject* o = vtkGraphicsFactory::CreateInstance("vtkDisplayListPainter");
  return static_cast<vtkDisplayListPainter *>(o);
}

//----------------------------------------------------------------------------
void vtkDisplayListPainter::ProcessInformation(vtkInformation* info)
{
  if (info->Has(IMMEDIATE_MODE_RENDERING()))
    {
    this->SetImmediateModeRendering(info->Get(IMMEDIATE_MODE_RENDERING()));
    }

  this->Superclass::ProcessInformation(info);
}

//----------------------------------------------------------------------------
double vtkDisplayListPainter::GetTimeToDraw()
{
  if (this->ImmediateModeRendering)
    {
    return this->DelegatePainter->GetTimeToDraw();
    }
  return this->TimeToDraw;
}


//----------------------------------------------------------------------------
void vtkDisplayListPainter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ImmediateModeRendering: " << this->ImmediateModeRendering
    << endl;
}
