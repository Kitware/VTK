/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSurfaceLICDefaultPainter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSurfaceLICDefaultPainter.h"

#include "vtkGarbageCollector.h"
#include "vtkSurfaceLICPainter.h"
#include "vtkObjectFactory.h"
#include "vtkScalarsToColorsPainter.h"

vtkStandardNewMacro(vtkSurfaceLICDefaultPainter);
vtkCxxSetObjectMacro(vtkSurfaceLICDefaultPainter, SurfaceLICPainter, vtkSurfaceLICPainter);
//----------------------------------------------------------------------------
vtkSurfaceLICDefaultPainter::vtkSurfaceLICDefaultPainter()
{
  this->SurfaceLICPainter = vtkSurfaceLICPainter::New();
}

//----------------------------------------------------------------------------
vtkSurfaceLICDefaultPainter::~vtkSurfaceLICDefaultPainter()
{
  this->SetSurfaceLICPainter(0);
}

//----------------------------------------------------------------------------
void vtkSurfaceLICDefaultPainter::BuildPainterChain()
{
  this->Superclass::BuildPainterChain();
  
  // Now insert the SurfaceLICPainter after the scalar to colors painter.
  vtkPainter* stc = this->GetScalarsToColorsPainter();
  
  this->SurfaceLICPainter->SetDelegatePainter(stc->GetDelegatePainter());
  stc->SetDelegatePainter(this->SurfaceLICPainter);
}

//----------------------------------------------------------------------------
void vtkSurfaceLICDefaultPainter::ReportReferences(vtkGarbageCollector *collector)
{
  this->Superclass::ReportReferences(collector);
  vtkGarbageCollectorReport(collector, this->SurfaceLICPainter,
    "SurfaceLICPainter");
}

//----------------------------------------------------------------------------
void vtkSurfaceLICDefaultPainter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SurfaceLICPainter: " << this->SurfaceLICPainter << endl;
}
