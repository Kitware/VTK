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
#include "vtkClipPlanesPainter.h"

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

  // Now insert the SurfaceLICPainter before the display-list painter.
  vtkPainter* prevPainter = this->GetClipPlanesPainter();
  this->SurfaceLICPainter->SetDelegatePainter(prevPainter->GetDelegatePainter());
  prevPainter->SetDelegatePainter(this->SurfaceLICPainter);
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
