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

#include "vtkObjectFactory.h"
#include "vtkGarbageCollector.h"
#include "vtkSurfaceLICPainter.h"
#include "vtkCoincidentTopologyResolutionPainter.h"
#include "vtkClipPlanesPainter.h"

//#define vtkSurfaceLICDefaultPainterDEBUG

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSurfaceLICDefaultPainter);

//----------------------------------------------------------------------------
vtkCxxSetObjectMacro(
      vtkSurfaceLICDefaultPainter,
      SurfaceLICPainter,
      vtkSurfaceLICPainter);

//----------------------------------------------------------------------------
vtkSurfaceLICDefaultPainter::vtkSurfaceLICDefaultPainter()
{
  this->SurfaceLICPainter = vtkSurfaceLICPainter::New();
}

//----------------------------------------------------------------------------
vtkSurfaceLICDefaultPainter::~vtkSurfaceLICDefaultPainter()
{
  this->SetSurfaceLICPainter(NULL);
}

//----------------------------------------------------------------------------
void vtkSurfaceLICDefaultPainter::BuildPainterChain()
{
  this->Superclass::BuildPainterChain();

  vtkPainter *prevPainter = this->GetClipPlanesPainter();
  vtkPainter *nextPainter = prevPainter->GetDelegatePainter();

  prevPainter->SetDelegatePainter(this->SurfaceLICPainter);
  this->SurfaceLICPainter->SetDelegatePainter(nextPainter);

  #if defined (vtkSurfaceLICDefaultPainterDEBUG)
  cerr << "SurfaceLIC Default Painter Chain:" << endl;
  vtkPainter *painter = this->vtkPainter::GetDelegatePainter();;
  while (painter)
    {
    cerr << painter->GetClassName() << "->";
    painter = painter->GetDelegatePainter();
    }
  cerr << "NULL" << endl;
  #endif
}

//----------------------------------------------------------------------------
void vtkSurfaceLICDefaultPainter::ReportReferences(
        vtkGarbageCollector *collector)
{
  this->Superclass::ReportReferences(collector);
  vtkGarbageCollectorReport(
        collector,
        this->SurfaceLICPainter,
        "SurfaceLICPainter");
}

//-------------------------------------------------------------------------
void vtkSurfaceLICDefaultPainter::UpdateBounds(double bounds[6])
{
  // need the superclass to start with the first painter in the chain
  vtkPainter *painter = this->Superclass::GetDelegatePainter();
  if (painter)
    {
    // delegate the task of updating the bounds
    painter->UpdateBounds(bounds);
    }
  else
    {
    // no painter in the chain. let's build the chain if needed.
    if (this->ChainBuildTime < this->MTime)
      {
      // build the chain of painters
      this->BuildPainterChain();
      this->ChainBuildTime.Modified();
      }
    // try again to get the first painter in the chain
    painter = this->Superclass::GetDelegatePainter();
    if (painter)
      {
      //delegate the task of updating the bounds
      painter->UpdateBounds(bounds);
      }
    }
}

//----------------------------------------------------------------------------
void vtkSurfaceLICDefaultPainter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SurfaceLICPainter: " << this->SurfaceLICPainter << endl;
}
