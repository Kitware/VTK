/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMRCutPlane.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkAMRCutPlane.h"
#include "vtkObjectFactory.h"

#include <cassert>
#include <algorithm>

vtkStandardNewMacro(vtkAMRCutPlane);

//------------------------------------------------------------------------------
vtkAMRCutPlane::vtkAMRCutPlane()
{
  this->LevelOfResolution = 0;
  for( int i=0; i < 3; ++i )
    {
      this->Center[i] = 0.0;
      this->Normal[i] = 0.0;
    }
}

//------------------------------------------------------------------------------
vtkAMRCutPlane::~vtkAMRCutPlane()
{
  // TODO Auto-generated destructor stub
}

//------------------------------------------------------------------------------
void vtkAMRCutPlane::PrintSelf( std::ostream &oss, vtkIndent indent )
{
  this->Superclass::PrintSelf( oss, indent );
}

//------------------------------------------------------------------------------
int vtkAMRCutPlane::FillInputPortInformation(
    int vtkNotUsed(port), vtkInformation *info )
{
  assert( "pre: information object is NULL!" && (info != NULL) );
  return 1;
}

//------------------------------------------------------------------------------
int vtkAMRCutPlane::FillOutputPortInformation(
    int vtkNotUsed(port), vtkInformation *info )
{
  assert( "pre: information object is NULL!" && (info != NULL) );
  return 1;
}

//------------------------------------------------------------------------------
int vtkAMRCutPlane::RequestInformation(
    vtkInformation* vtkNotUsed(rqst), vtkInformationVector** inputVector,
    vtkInformationVector* outputVector )
{
  // TODO: imlpement this
  return 1;
}

//------------------------------------------------------------------------------
int vtkAMRCutPlane::RequestUpdateExtent(
    vtkInformation* vtkNotUsed(rqst), vtkInformationVector** input,
    vtkInformationVector* output)
{
  // TODO: implement this
  return 1;
}

//------------------------------------------------------------------------------
int vtkAMRCutPlane::RequestData(
    vtkInformation* vtkNotUsed(rqst), vtkInformationVector** input,
    vtkInformationVector* output )
{
  // TODO: implement this
  return 1;
}
