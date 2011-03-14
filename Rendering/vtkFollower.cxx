/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFollower.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkFollower.h"

#include "vtkCamera.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkTexture.h"
#include "vtkTransform.h"

#include <math.h>

vtkStandardNewMacro(vtkFollower);

vtkCxxSetObjectMacro(vtkFollower,Camera,vtkCamera);

//----------------------------------------------------------------------
// Creates a follower with no camera set
vtkFollower::vtkFollower()
{
  this->Camera = NULL;
  this->Device = vtkActor::New();

  this->InternalMatrix = vtkMatrix4x4::New();
}

//----------------------------------------------------------------------
vtkFollower::~vtkFollower()
{
  if (this->Camera)
    {
    this->Camera->UnRegister(this);
    }

  this->Device->Delete();

  this->InternalMatrix->Delete();
}

//----------------------------------------------------------------------------
void vtkFollower::ComputeMatrix()
{
  // check whether or not need to rebuild the matrix
  if ( this->GetMTime() > this->MatrixMTime ||
       (this->Camera && this->Camera->GetMTime() > this->MatrixMTime) )
    {
    this->GetOrientation();
    this->Transform->Push();
    this->Transform->Identity();
    this->Transform->PostMultiply();

    this->Transform->Translate(-this->Origin[0],
                               -this->Origin[1],
                               -this->Origin[2]);
    // scale
    this->Transform->Scale(this->Scale[0],
                           this->Scale[1],
                           this->Scale[2]);

    // rotate
    this->Transform->RotateY(this->Orientation[1]);
    this->Transform->RotateX(this->Orientation[0]);
    this->Transform->RotateZ(this->Orientation[2]);

    if (this->Camera)
      {
      double *pos, *vup, distance;
      double Rx[3], Ry[3], Rz[3];

      vtkMatrix4x4 *matrix = this->InternalMatrix;
      matrix->Identity();

      // do the rotation
      // first rotate y
      pos = this->Camera->GetPosition();
      vup = this->Camera->GetViewUp();

      if (this->Camera->GetParallelProjection())
        {
        this->Camera->GetDirectionOfProjection(Rz);
        Rz[0] = -Rz[0];
        Rz[1] = -Rz[1];
        Rz[2] = -Rz[2];
        }
      else
        {
        distance = sqrt(
          (pos[0] - this->Position[0])*(pos[0] - this->Position[0]) +
          (pos[1] - this->Position[1])*(pos[1] - this->Position[1]) +
          (pos[2] - this->Position[2])*(pos[2] - this->Position[2]));
        for (int i = 0; i < 3; i++)
          {
          Rz[i] = (pos[i] - this->Position[i])/distance;
          }
        }

      // We cannot directly use the vup angle since it can be aligned with Rz:
      //vtkMath::Cross(vup,Rz,Rx);
      //vtkMath::Normalize(Rx);
      //vtkMath::Cross(Rz,Rx,Ry);

      //instead use the view right angle:
      double dop[3], vur[3];
      this->Camera->GetDirectionOfProjection(dop);

      vtkMath::Cross(dop,vup,vur);
      vtkMath::Normalize(vur);

      vtkMath::Cross(Rz, vur, Ry);
      vtkMath::Normalize(Ry);
      vtkMath::Cross(Ry,Rz,Rx);

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
                               this->Origin[1] + this->Position[1],
                               this->Origin[2] + this->Position[2]);

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

//----------------------------------------------------------------------
void vtkFollower::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if ( this->Camera )
    {
    os << indent << "Camera:\n";
    this->Camera->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Camera: (none)\n";
    }
}

//----------------------------------------------------------------------
int vtkFollower::RenderOpaqueGeometry(vtkViewport *vp)
{
  if ( ! this->Mapper )
    {
    return 0;
    }

  if (!this->Property)
    {
    // force creation of a property
    this->GetProperty();
    }

  if (this->GetIsOpaque())
    {
    vtkRenderer *ren = static_cast<vtkRenderer *>(vp);
    this->Render(ren);
    return 1;
    }
  return 0;
}

//-----------------------------------------------------------------------------
int vtkFollower::RenderTranslucentPolygonalGeometry(vtkViewport *vp)
{
  if ( ! this->Mapper )
    {
    return 0;
    }

  if (!this->Property)
    {
    // force creation of a property
    this->GetProperty();
    }

  if (!this->GetIsOpaque())
    {
    vtkRenderer *ren = static_cast<vtkRenderer *>(vp);
    this->Render(ren);
    return 1;
    }
  return 0;
}

//-----------------------------------------------------------------------------
void vtkFollower::ReleaseGraphicsResources(vtkWindow *w)
{
  this->Device->ReleaseGraphicsResources(w);
}

//-----------------------------------------------------------------------------
// Description:
// Does this prop have some translucent polygonal geometry?
int vtkFollower::HasTranslucentPolygonalGeometry()
{
  if ( ! this->Mapper )
    {
    return 0;
    }
  // make sure we have a property
  if (!this->Property)
    {
    // force creation of a property
    this->GetProperty();
    }

  // is this actor opaque ?
  return !this->GetIsOpaque();
}

//-----------------------------------------------------------------------------
// This causes the actor to be rendered. It, in turn, will render the actor's
// property and then mapper.
void vtkFollower::Render(vtkRenderer *ren)
{
  this->Property->Render(this, ren);

  this->Device->SetProperty (this->Property);
  this->Property->Render(this, ren);
  if (this->BackfaceProperty)
    {
    this->BackfaceProperty->BackfaceRender(this, ren);
    this->Device->SetBackfaceProperty(this->BackfaceProperty);
    }

  /* render the texture */
  if (this->Texture)
    {
    this->Texture->Render(ren);
    }

  // make sure the device has the same matrix
  this->ComputeMatrix();
  this->Device->SetUserMatrix(this->Matrix);

  this->Device->Render(ren,this->Mapper);
}

//----------------------------------------------------------------------
void vtkFollower::ShallowCopy(vtkProp *prop)
{
  vtkFollower *f = vtkFollower::SafeDownCast(prop);
  if ( f != NULL )
    {
    this->SetCamera(f->GetCamera());
    }

  // Now do superclass
  this->vtkActor::ShallowCopy(prop);
}



