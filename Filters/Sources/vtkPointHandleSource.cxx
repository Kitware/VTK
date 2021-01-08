/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointHandleSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPointHandleSource.h"

#include "vtkConeSource.h"
#include "vtkSphereSource.h"

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkPointHandleSource);

//------------------------------------------------------------------------------
void vtkPointHandleSource::SetPosition(double xPos, double yPos, double zPos)
{
  if ((this->Position[0] != xPos) || (this->Position[1] != yPos) || (this->Position[2] != zPos))
  {
    this->Position[0] = xPos;
    this->Position[1] = yPos;
    this->Position[2] = zPos;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
double* vtkPointHandleSource::GetPosition()
{
  return this->Position;
}

//------------------------------------------------------------------------------
void vtkPointHandleSource::SetDirection(double xDir, double yDir, double zDir)
{
  if ((this->Direction[0] != xDir) || (this->Direction[1] != yDir) || (this->Direction[2] != zDir))
  {
    this->Direction[0] = xDir;
    this->Direction[1] = yDir;
    this->Direction[2] = zDir;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
double* vtkPointHandleSource::GetDirection()
{
  return this->Direction;
}

//------------------------------------------------------------------------------
int vtkPointHandleSource::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  auto output = vtkPolyData::GetData(outputVector);
  if (!this->Directional)
  {
    this->RecomputeSphere();
    output->ShallowCopy(this->PositionSphere->GetOutput(0));
  }
  else
  {
    this->RecomputeCone();
    output->ShallowCopy(this->PositionCone->GetOutput(0));
  }
  return 1;
}

//------------------------------------------------------------------------------
void vtkPointHandleSource::RecomputeSphere()
{
  this->PositionSphere->SetRadius(this->Size);
  this->PositionSphere->SetCenter(this->Position);
  this->PositionSphere->SetThetaResolution(16);
  this->PositionSphere->SetPhiResolution(8);
  this->PositionSphere->Update();
}

//------------------------------------------------------------------------------
void vtkPointHandleSource::RecomputeCone()
{
  this->PositionCone->SetRadius(this->Size);
  this->PositionCone->SetCenter(this->Position);
  this->PositionCone->SetHeight(2.8 * this->Size);
  this->PositionCone->SetResolution(16);
  this->PositionCone->SetDirection(this->Direction);
  this->PositionCone->Update();
}

//------------------------------------------------------------------------------
void vtkPointHandleSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->Directional)
  {
    os << indent << "PositionCone: (" << this->PositionCone << "\n";
    if (this->PositionCone)
    {
      this->PositionCone->PrintSelf(os, indent.GetNextIndent());
      os << indent << ")\n";
    }
    else
    {
      os << "none)\n";
    }
  }
  else
  {
    os << indent << "PositionSphere: (" << this->PositionSphere << "\n";
    if (this->PositionSphere)
    {
      this->PositionSphere->PrintSelf(os, indent.GetNextIndent());
      os << indent << ")\n";
    }
    else
    {
      os << "none)\n";
    }
  }
}
