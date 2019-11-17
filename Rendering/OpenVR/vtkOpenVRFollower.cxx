/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenVRFollower.h"

#include "vtkCamera.h"
#include "vtkObjectFactory.h"
#include "vtkOpenVRRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkTransform.h"

vtkStandardNewMacro(vtkOpenVRFollower);

vtkOpenVRFollower::vtkOpenVRFollower() {}

vtkOpenVRFollower::~vtkOpenVRFollower() {}

//-----------------------------------------------------------------------------
// This causes the actor to be rendered. It, in turn, will render the actor's
// property and then mapper.
void vtkOpenVRFollower::Render(vtkRenderer* ren)
{
  vtkOpenVRRenderWindow* renWin = static_cast<vtkOpenVRRenderWindow*>(ren->GetVTKWindow());

  renWin->GetPhysicalViewUp(this->LastViewUp);
  this->Superclass::Render(ren);
}

//----------------------------------------------------------------------------
void vtkOpenVRFollower::ComputeMatrix()
{
  // check whether or not need to rebuild the matrix
  // only rebuild on left eye otherwise we get two different
  // poses for two eyes
  if (this->Camera->GetLeftEye() &&
    (this->GetMTime() > this->MatrixMTime ||
      (this->Camera && this->Camera->GetMTime() > this->MatrixMTime)))
  {
    this->GetOrientation();
    this->Transform->Push();
    this->Transform->Identity();
    this->Transform->PostMultiply();

    this->Transform->Translate(-this->Origin[0], -this->Origin[1], -this->Origin[2]);
    // scale
    this->Transform->Scale(this->Scale[0], this->Scale[1], this->Scale[2]);

    // rotate
    this->Transform->RotateY(this->Orientation[1]);
    this->Transform->RotateX(this->Orientation[0]);
    this->Transform->RotateZ(this->Orientation[2]);

    if (this->Camera)
    {
      double *pos, *cvup, *vup, distance;
      double Rx[3], Ry[3], Rz[3];

      vtkMatrix4x4* matrix = this->InternalMatrix;
      matrix->Identity();

      // do the rotation
      // first rotate y
      pos = this->Camera->GetPosition();
      cvup = this->Camera->GetViewUp();
      vup = this->LastViewUp;

      if (this->Camera->GetParallelProjection())
      {
        this->Camera->GetDirectionOfProjection(Rz);
        Rz[0] = -Rz[0];
        Rz[1] = -Rz[1];
        Rz[2] = -Rz[2];
      }
      else
      {
        distance = sqrt((pos[0] - this->Position[0]) * (pos[0] - this->Position[0]) +
          (pos[1] - this->Position[1]) * (pos[1] - this->Position[1]) +
          (pos[2] - this->Position[2]) * (pos[2] - this->Position[2]));
        for (int i = 0; i < 3; i++)
        {
          Rz[i] = (pos[i] - this->Position[i]) / distance;
        }
      }

      // We cannot directly use the vup angle since it can be aligned with Rz:

      // instead use the view right angle:
      double dop[3], vur[3];
      this->Camera->GetDirectionOfProjection(dop);

      // if vup is close to Rz then use cvup instead
      // aka if we are looking mostly up or down
      // then use the headsets view up
      // if (fabs(vtkMath::Dot(vup,Rz)) > 0.9)
      // {
      //   vup = cvup;
      // }

      vtkMath::Cross(vup, Rz, vur);
      vtkMath::Normalize(vur);

      // vtkMath::Cross(vup,Rz,Rx);
      // vtkMath::Normalize(Rx);
      // vtkMath::Cross(vup,Rx,Rz);

      vtkMath::Cross(Rz, vur, Ry);
      vtkMath::Normalize(Ry);
      vtkMath::Cross(Ry, Rz, Rx);

      matrix->Element[0][0] = Rx[0];
      matrix->Element[1][0] = Rx[1];
      matrix->Element[2][0] = Rx[2];
      matrix->Element[0][1] = Ry[0];
      matrix->Element[1][1] = Ry[1];
      matrix->Element[2][1] = Ry[2];
      matrix->Element[0][2] = Rz[0];
      matrix->Element[1][2] = Rz[1];
      matrix->Element[2][2] = Rz[2];

      this->Transform->Concatenate(matrix);
    }

    // translate to projection reference point PRP
    // this is the camera's position blasted through
    // the current matrix
    this->Transform->Translate(this->Origin[0] + this->Position[0],
      this->Origin[1] + this->Position[1], this->Origin[2] + this->Position[2]);

    // apply user defined matrix last if there is one
    if (this->UserMatrix)
    {
      this->Transform->Concatenate(this->UserMatrix);
    }

    this->Transform->PreMultiply();
    this->Transform->GetMatrix(this->Matrix);
    this->MatrixMTime.Modified();
    this->Transform->Pop();
  }
}

void vtkOpenVRFollower::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
