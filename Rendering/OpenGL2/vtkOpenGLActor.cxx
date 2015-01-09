/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGLActor.h"

#include "vtkMapper.h"
#include "vtkMatrix3x3.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkOpenGLRenderer.h"
#include "vtkProperty.h"
#include "vtkOpenGLError.h"
#include "vtkRenderWindow.h"
#include "vtkTransform.h"

#include <math.h>

vtkStandardNewMacro(vtkOpenGLActor);

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
void vtkOpenGLActor::Render(vtkRenderer *ren, vtkMapper *mapper)
{
  vtkOpenGLClearErrorMacro();

  // get opacity
  double opacity = this->GetProperty()->GetOpacity();
  if (opacity == 1.0)
    {
    glDepthMask(GL_TRUE);
    }
  else
    {
    vtkHardwareSelector* selector = ren->GetSelector();
    bool picking = (ren->GetRenderWindow()->GetIsPicking() || selector != NULL);
    if (picking)
      {
      glDepthMask(GL_TRUE);
      }
    else
      {
      if (ren->GetLastRenderingUsedDepthPeeling())
        {
        glDepthMask(GL_TRUE); // transparency with depth peeling
        }
      else
        {
        glDepthMask(GL_FALSE); // transparency with alpha blending
        }
      }
    }

  // send a render to the mapper; update pipeline
  mapper->Render(ren, this);

  if (opacity != 1.0)
    {
    glDepthMask(GL_TRUE);
    }

  vtkOpenGLCheckErrorMacro("failed after Render");
}

//----------------------------------------------------------------------------
void vtkOpenGLActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

void vtkOpenGLActor::GetKeyMatrices(vtkMatrix4x4 *&mcwc, vtkMatrix3x3 *&normMat)
{
  // has the actor changed?
  if (this->GetMTime() > this->KeyMatrixTime)
    {
    this->ComputeMatrix();
    this->MCWCMatrix->DeepCopy(this->Matrix);
    this->MCWCMatrix->Transpose();

    if (this->GetIsIdentity())
      {
      this->NormalMatrix->Identity();
      }
    else
      {
      this->NormalTransform->SetMatrix(this->Matrix);
      vtkMatrix4x4 *mat4 = this->NormalTransform->GetMatrix();
      for(int i = 0; i < 3; ++i)
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

vtkIdType vtkOpenGLActor::GetConvertedPickValue(vtkIdType idIn, int fieldassociation)
{
  vtkOpenGLPolyDataMapper *pdm = vtkOpenGLPolyDataMapper::SafeDownCast(this->GetMapper());
  if (pdm)
    {
    return pdm->GetConvertedPickValue(idIn, fieldassociation,this);
    }
  return idIn;
}
