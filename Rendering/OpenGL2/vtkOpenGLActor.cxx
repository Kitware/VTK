// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOpenGLActor.h"

#include "vtkDepthPeelingPass.h"
#include "vtkDualDepthPeelingPass.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkMapper.h"
#include "vtkMatrix3x3.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLState.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkTransform.h"

#include <cmath>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkOpenGLActor);

vtkInformationKeyMacro(vtkOpenGLActor, GLDepthMaskOverride, Integer);

vtkOpenGLActor::vtkOpenGLActor()
{
  this->MCWCMatrix = vtkMatrix4x4::New();
  this->NormalMatrix = vtkMatrix3x3::New();
  this->NormalTransform = vtkTransform::New();
}

vtkOpenGLActor::~vtkOpenGLActor()
{
  this->MCWCMatrix->Delete();
  this->NormalMatrix->Delete();
  this->NormalTransform->Delete();
}

// Actual actor render method.
void vtkOpenGLActor::Render(vtkRenderer* ren, vtkMapper* mapper)
{
  vtkOpenGLClearErrorMacro();

  vtkOpenGLState* ostate = static_cast<vtkOpenGLRenderer*>(ren)->GetState();
  vtkOpenGLState::ScopedglDepthMask dmsaver(ostate);

  // get opacity
  bool opaque = !this->IsRenderingTranslucentPolygonalGeometry();
  if (opaque)
  {
    ostate->vtkglDepthMask(GL_TRUE);
  }
  else
  {
    vtkHardwareSelector* selector = ren->GetSelector();
    bool picking = (selector != nullptr);
    if (picking)
    {
      ostate->vtkglDepthMask(GL_TRUE);
    }
    else
    {
      // check for depth peeling
      vtkInformation* info = this->GetPropertyKeys();
      if (info && info->Has(vtkOpenGLActor::GLDepthMaskOverride()))
      {
        int maskoverride = info->Get(vtkOpenGLActor::GLDepthMaskOverride());
        switch (maskoverride)
        {
          case 0:
            ostate->vtkglDepthMask(GL_FALSE);
            break;
          case 1:
            ostate->vtkglDepthMask(GL_TRUE);
            break;
          default:
            // Do nothing.
            break;
        }
      }
      else
      {
        ostate->vtkglDepthMask(GL_FALSE); // transparency with alpha blending
      }
    }
  }

  // send a render to the mapper; update pipeline
  mapper->Render(ren, this);

  if (!opaque)
  {
    ostate->vtkglDepthMask(GL_TRUE);
  }

  vtkOpenGLCheckErrorMacro("failed after Render");
}

//------------------------------------------------------------------------------
void vtkOpenGLActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void vtkOpenGLActor::GetKeyMatrices(vtkMatrix4x4*& mcwc, vtkMatrix3x3*& normMat)
{
  vtkMTimeType rwTime = 0;
  if (this->CoordinateSystem != WORLD && this->CoordinateSystemRenderer)
  {
    rwTime = this->CoordinateSystemRenderer->GetVTKWindow()->GetMTime();
  }

  // has the actor changed or is in device coords?
  if (this->GetMTime() > this->KeyMatrixTime || rwTime > this->KeyMatrixTime ||
    this->CoordinateSystem == DEVICE)
  {
    this->GetModelToWorldMatrix(this->MCWCMatrix);

    this->MCWCMatrix->Transpose();

    if (this->GetIsIdentity())
    {
      this->NormalMatrix->Identity();
    }
    else
    {
      this->NormalTransform->SetMatrix(this->Matrix);
      vtkMatrix4x4* mat4 = this->NormalTransform->GetMatrix();
      for (int i = 0; i < 3; ++i)
      {
        for (int j = 0; j < 3; ++j)
        {
          this->NormalMatrix->SetElement(i, j, mat4->GetElement(i, j));
        }
      }
    }
    this->NormalMatrix->Invert();
    this->KeyMatrixTime.Modified();
  }

  mcwc = this->MCWCMatrix;
  normMat = this->NormalMatrix;
}
VTK_ABI_NAMESPACE_END
