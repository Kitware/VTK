/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointPlacer.cxx,v

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPointPlacer.h"

#include "vtkObjectFactory.h"
#include "vtkRenderer.h"

vtkCxxRevisionMacro(vtkPointPlacer, "$Revision 1.3");
vtkStandardNewMacro(vtkPointPlacer);

//----------------------------------------------------------------------
vtkPointPlacer::vtkPointPlacer()
{
  this->PixelTolerance           = 5;
  this->WorldTolerance           = 0.001;
}

//----------------------------------------------------------------------
vtkPointPlacer::~vtkPointPlacer()
{
}

//----------------------------------------------------------------------
int vtkPointPlacer::UpdateWorldPosition( vtkRenderer *vtkNotUsed(ren),
                                         double *vtkNotUsed(worldPos),
                                         double *vtkNotUsed(worldOrient) )
{
  return 1;
}

//----------------------------------------------------------------------
int vtkPointPlacer::ComputeWorldPosition( vtkRenderer *,
                                  double vtkNotUsed(displayPos)[2], 
                                  double vtkNotUsed(worldPos)[3],
                                  double vtkNotUsed(worldOrient)[9] )
{  
  return 1;
}

//----------------------------------------------------------------------
int vtkPointPlacer::ComputeWorldPosition( vtkRenderer *,
                                  double vtkNotUsed(displayPos)[2], 
                                  double vtkNotUsed(refWorldPos)[3],
                                  double vtkNotUsed(worldPos)[3],
                                  double vtkNotUsed(worldOrient)[9] )
{
  return 1;
}

//----------------------------------------------------------------------
int vtkPointPlacer::ValidateWorldPosition( double vtkNotUsed(worldPos)[3] )
{
  return 1;
}
  
//----------------------------------------------------------------------
int vtkPointPlacer::ValidateWorldPosition( double vtkNotUsed(worldPos)[3],
                                   double vtkNotUsed(worldOrient)[9] )
{
  return 1;
}

//----------------------------------------------------------------------
int vtkPointPlacer::ValidateDisplayPosition( vtkRenderer *, 
                                             double vtkNotUsed(displayPos)[2] )
{
  return 1;
}

//----------------------------------------------------------------------
void vtkPointPlacer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);  
}

