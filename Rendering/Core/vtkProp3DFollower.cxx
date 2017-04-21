/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProp3DFollower.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkProp3DFollower.h"

#include "vtkCamera.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkTexture.h"
#include "vtkTransform.h"
#include "vtkAssemblyPaths.h"

#include <cmath>

vtkStandardNewMacro(vtkProp3DFollower);

vtkCxxSetObjectMacro(vtkProp3DFollower,Camera,vtkCamera);

//----------------------------------------------------------------------
// Creates a follower with no camera set
vtkProp3DFollower::vtkProp3DFollower()
{
  this->Camera = NULL;
  this->Device = NULL;

  this->InternalMatrix = vtkMatrix4x4::New();
}

//----------------------------------------------------------------------
vtkProp3DFollower::~vtkProp3DFollower()
{
  if (this->Camera)
  {
    this->Camera->UnRegister(this);
  }

  if (this->Device)
  {
    this->Device->Delete();
  }

  this->InternalMatrix->Delete();
}

//----------------------------------------------------------------------------
void vtkProp3DFollower::SetProp3D(vtkProp3D *prop)
{
  if (this->Device != prop)
  {
    if ( this->Device != NULL )
    {
      this->Device->Delete();
    }
    this->Device = prop;
    if ( this->Device != NULL )
    {
      this->Device->Register(this);
    }
    this->Modified();
  }
}


//----------------------------------------------------------------------------
vtkProp3D *vtkProp3DFollower::GetProp3D()
{
  return this->Device;
}


//----------------------------------------------------------------------------
void vtkProp3DFollower::ComputeMatrix()
{
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

//-----------------------------------------------------------------------------
double *vtkProp3DFollower::GetBounds()
{
  if ( this->Device )
  {
    this->ComputeMatrix();
    this->Device->SetUserMatrix(this->Matrix);
    return this->Device->GetBounds();
  }
  else
  {
    return NULL;
  }
}


//-----------------------------------------------------------------------------
void vtkProp3DFollower::ReleaseGraphicsResources(vtkWindow *w)
{
  if ( this->Device )
  {
    this->Device->ReleaseGraphicsResources(w);
  }
}

//-----------------------------------------------------------------------------
// Description:
// Does this prop have some translucent polygonal geometry?
int vtkProp3DFollower::HasTranslucentPolygonalGeometry()
{
  if ( this->Device )
  {
    return this->Device->HasTranslucentPolygonalGeometry();
  }
  else
  {
    return 0;
  }
}

//----------------------------------------------------------------------
int vtkProp3DFollower::RenderOpaqueGeometry(vtkViewport *vp)
{
  if ( this->Device )
  {
    this->ComputeMatrix();
    this->Device->SetUserMatrix(this->Matrix);
    if (this->GetPropertyKeys())
    {
      this->Device->SetPropertyKeys(this->GetPropertyKeys());
    }
    if (this->GetVisibility())
    {
      return this->Device->RenderOpaqueGeometry(vp);
    }
  }
  return 0;
}

//-----------------------------------------------------------------------------
int vtkProp3DFollower::RenderTranslucentPolygonalGeometry(vtkViewport *vp)
{
  if ( this->Device )
  {
    this->ComputeMatrix();
    this->Device->SetUserMatrix(this->Matrix);
    if (this->GetPropertyKeys())
    {
      this->Device->SetPropertyKeys(this->GetPropertyKeys());
    }
    if (this->GetVisibility())
    {
      return this->Device->RenderTranslucentPolygonalGeometry(vp);
    }
  }
  return 0;
}

//----------------------------------------------------------------------
int vtkProp3DFollower::RenderVolumetricGeometry(vtkViewport *vp)
{
  if ( this->Device )
  {
    this->ComputeMatrix();
    this->Device->SetUserMatrix(this->Matrix);
    if (this->GetPropertyKeys())
    {
      this->Device->SetPropertyKeys(this->GetPropertyKeys());
    }
    if (this->GetVisibility())
    {
      return this->Device->RenderVolumetricGeometry(vp);
    }
  }
  return 0;
}

//----------------------------------------------------------------------
void vtkProp3DFollower::ShallowCopy(vtkProp *prop)
{
  vtkProp3DFollower *f = vtkProp3DFollower::SafeDownCast(prop);
  if ( f != NULL )
  {
    this->SetCamera(f->GetCamera());
  }

  // Now do superclass
  this->vtkProp3D::ShallowCopy(prop);
}

//----------------------------------------------------------------------------
void vtkProp3DFollower::InitPathTraversal()
{
  if ( this->Device )
  {
    this->Device->InitPathTraversal();
  }
}

//----------------------------------------------------------------------------
vtkAssemblyPath *vtkProp3DFollower::GetNextPath()
{
  if ( this->Device )
  {
    return this->Device->GetNextPath();
  }
  else
  {
    return NULL;
  }
}

//----------------------------------------------------------------------
void vtkProp3DFollower::PrintSelf(ostream& os, vtkIndent indent)
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
