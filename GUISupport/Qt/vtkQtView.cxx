/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtView.cxx

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkQtView.h"

#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkQtView, "1.3");

// Implementing the VTK static New method, in this case
// instead of fancy object factory stuff, just doing
// the vanilla return of the created class.
vtkQtView* vtkQtView::New()
{
  return new vtkQtView;
}

//----------------------------------------------------------------------------
void vtkQtView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

