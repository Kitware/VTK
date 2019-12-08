/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGLCamera.h"

#include "vtkMatrix3x3.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLState.h"
#include "vtkOutputWindow.h"
#include "vtkRenderer.h"

#include <cmath>

vtkStandardNewMacro(vtkOpenGLCamera);

vtkOpenGLCamera::vtkOpenGLCamera()
{
  this->WCDCMatrix = vtkMatrix4x4::New();
  this->WCVCMatrix = vtkMatrix4x4::New();
  this->NormalMatrix = vtkMatrix3x3::New();
  this->VCDCMatrix = vtkMatrix4x4::New();
  this->LastRenderer = nullptr;
}

vtkOpenGLCamera::~vtkOpenGLCamera()
{
  this->WCDCMatrix->Delete();
  this->WCVCMatrix->Delete();
  this->NormalMatrix->Delete();
  this->VCDCMatrix->Delete();
}

// Implement base class method.
void vtkOpenGLCamera::Render(vtkRenderer* ren)
{
  vtkOpenGLClearErrorMacro();

  int lowerLeft[2];
  int usize, vsize;

  vtkOpenGLRenderWindow* win = vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());
  vtkOpenGLState* ostate = win->GetState();

  // find out if we should stereo render
  this->Stereo = (ren->GetRenderWindow())->GetStereoRender();
  ren->GetTiledSizeAndOrigin(&usize, &vsize, lowerLeft, lowerLeft + 1);

  ostate->vtkglViewport(lowerLeft[0], lowerLeft[1], usize, vsize);
  ostate->vtkglEnable(GL_SCISSOR_TEST);
  if (this->UseScissor)
  {
    ostate->vtkglScissor(this->ScissorRect.GetX(), this->ScissorRect.GetY(),
      this->ScissorRect.GetWidth(), this->ScissorRect.GetHeight());
    this->UseScissor = false;
  }
  else
  {
    ostate->vtkglScissor(lowerLeft[0], lowerLeft[1], usize, vsize);
  }

  if ((ren->GetRenderWindow())->GetErase() && ren->GetErase())
  {
    ren->Clear();
  }

  vtkOpenGLCheckErrorMacro("failed after Render");
}

//----------------------------------------------------------------------------
void vtkOpenGLCamera::UpdateViewport(vtkRenderer* ren)
{
  vtkOpenGLClearErrorMacro();
  vtkOpenGLRenderWindow* win = vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());
  vtkOpenGLState* ostate = win->GetState();

  int lowerLeft[2];
  int usize, vsize;
  ren->GetTiledSizeAndOrigin(&usize, &vsize, lowerLeft, lowerLeft + 1);

  ostate->vtkglViewport(lowerLeft[0], lowerLeft[1], usize, vsize);
  ostate->vtkglEnable(GL_SCISSOR_TEST);
  if (this->UseScissor)
  {
    ostate->vtkglScissor(this->ScissorRect.GetX(), this->ScissorRect.GetY(),
      this->ScissorRect.GetWidth(), this->ScissorRect.GetHeight());
    this->UseScissor = false;
  }
  else
  {
    ostate->vtkglScissor(lowerLeft[0], lowerLeft[1], usize, vsize);
  }

  vtkOpenGLCheckErrorMacro("failed after UpdateViewport");
}

//----------------------------------------------------------------------------
void vtkOpenGLCamera::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void vtkOpenGLCamera::GetKeyMatrices(vtkRenderer* ren, vtkMatrix4x4*& wcvc, vtkMatrix3x3*& normMat,
  vtkMatrix4x4*& vcdc, vtkMatrix4x4*& wcdc)
{
  // has the camera changed?
  if (ren != this->LastRenderer || this->MTime > this->KeyMatrixTime ||
    ren->GetMTime() > this->KeyMatrixTime)
  {
    this->WCVCMatrix->DeepCopy(this->GetModelViewTransformMatrix());

    for (int i = 0; i < 3; ++i)
    {
      for (int j = 0; j < 3; ++j)
      {
        this->NormalMatrix->SetElement(i, j, this->WCVCMatrix->GetElement(i, j));
      }
    }
    this->NormalMatrix->Invert();

    this->WCVCMatrix->Transpose();

    this->VCDCMatrix->DeepCopy(
      this->GetProjectionTransformMatrix(ren->GetTiledAspectRatio(), -1, 1));
    this->VCDCMatrix->Transpose();

    vtkMatrix4x4::Multiply4x4(this->WCVCMatrix, this->VCDCMatrix, this->WCDCMatrix);

    this->KeyMatrixTime.Modified();
    this->LastRenderer = ren;
  }

  wcvc = this->WCVCMatrix;
  normMat = this->NormalMatrix;
  vcdc = this->VCDCMatrix;
  wcdc = this->WCDCMatrix;
}
