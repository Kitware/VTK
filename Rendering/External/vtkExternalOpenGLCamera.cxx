// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkExternalOpenGLCamera.h"

#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLState.h"
#include "vtkPerspectiveTransform.h"
#include "vtkRenderer.h"
#include "vtkTransform.h"

#include <cmath>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkExternalOpenGLCamera);

//------------------------------------------------------------------------------
vtkExternalOpenGLCamera::vtkExternalOpenGLCamera()
{
  this->UserProvidedViewTransform = false;
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
void vtkExternalOpenGLCamera::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
