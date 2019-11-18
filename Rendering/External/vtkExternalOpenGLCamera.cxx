/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExternalOpenGLCamera.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExternalOpenGLCamera.h"

#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGL.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLState.h"
#include "vtkOutputWindow.h"
#include "vtkPerspectiveTransform.h"
#include "vtkRenderer.h"
#include "vtkTransform.h"

#include <cmath>

vtkStandardNewMacro(vtkExternalOpenGLCamera);

//----------------------------------------------------------------------------
vtkExternalOpenGLCamera::vtkExternalOpenGLCamera()
{
  this->UserProvidedViewTransform = false;
}

//----------------------------------------------------------------------------
void vtkExternalOpenGLCamera::SetViewTransformMatrix(const double elements[16])
{
  if (!elements)
  {
    return;
  }
  // Transpose the matrix to undo the transpose that VTK does internally
  vtkMatrix4x4* matrix = vtkMatrix4x4::New();
  matrix->DeepCopy(elements);
  matrix->Transpose();
  this->ViewTransform->SetMatrix(matrix);
  this->ModelViewTransform->SetMatrix(matrix);
  this->UserProvidedViewTransform = true;
  matrix->Delete();
}

//----------------------------------------------------------------------------
void vtkExternalOpenGLCamera::SetProjectionTransformMatrix(const double elements[16])
{
  if (!elements)
  {
    return;
  }
  // Transpose the matrix to undo the transpose that VTK does internally
  vtkMatrix4x4* matrix = vtkMatrix4x4::New();
  matrix->DeepCopy(elements);
  matrix->Transpose();

  this->SetExplicitProjectionTransformMatrix(matrix);
  this->SetUseExplicitProjectionTransformMatrix(true);
  matrix->Delete();
}

//----------------------------------------------------------------------------
void vtkExternalOpenGLCamera::ComputeViewTransform()
{
  if (this->UserProvidedViewTransform)
  {
    // Do not do anything
    return;
  }
  else
  {
    this->Superclass::ComputeViewTransform();
  }
}

//----------------------------------------------------------------------------
void vtkExternalOpenGLCamera::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
