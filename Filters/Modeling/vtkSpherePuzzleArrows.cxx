/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSpherePuzzleArrows.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSpherePuzzleArrows.h"

#include "vtkCellArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkSpherePuzzle.h"

#include <cmath>

vtkStandardNewMacro(vtkSpherePuzzleArrows);

//----------------------------------------------------------------------------
// Construct a new puzzle.
vtkSpherePuzzleArrows::vtkSpherePuzzleArrows()
{
  int idx;

  for (idx = 0; idx < 32; ++idx)
  {
    this->Permutation[idx] = idx;
  }

  this->Radius = 0.51;

  this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
// Destruct the puzzle.
vtkSpherePuzzleArrows::~vtkSpherePuzzleArrows()
{
}

//----------------------------------------------------------------------------
void vtkSpherePuzzleArrows::SetPermutationComponent(int comp, int val)
{
  if (this->Permutation[comp] == val)
  {
    return;
  }

  this->Permutation[comp] = val;
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkSpherePuzzleArrows::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the ouptut
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkPoints *pts = vtkPoints::New();
  vtkCellArray *polys = vtkCellArray::New();
  int idx;

  for (idx = 0; idx < 32; ++idx)
  {
    if (this->Permutation[idx] != idx)
    {
      //this->AppendArrow(idx, this->Permutation[idx], pts, polys);
      this->AppendArrow(this->Permutation[idx], idx, pts, polys);
    }
  }
  output->SetPoints(pts);
  output->SetPolys(polys);
  pts->Delete();
  polys->Delete();

  return 1;
}

//----------------------------------------------------------------------------
// Draw an arrow for piece with id1 to piece with id2.
void vtkSpherePuzzleArrows::AppendArrow(int id1, int id2,
                                        vtkPoints *pts, vtkCellArray *polys)
{
  double phi1, phi2, dPhi;
  double theta1, theta2, dTheta;
  double phi=0, theta=0, phiOff, thetaOff;
  double length;
  double x, y, z;
  int num, idx;
  vtkIdType ptId1, ptId2, ptId3, ptId4, ptId5;

  // Convert the start and end piece ids into sphere coordinates.
  phi1 = vtkMath::Pi() * ((id1 / 8)+0.5) / 4.0;
  theta1 = vtkMath::Pi() * ((id1 % 8)+0.5) / 4.0;
  phi2 = vtkMath::Pi() * ((id2 / 8)+0.5) / 4.0;
  theta2 = vtkMath::Pi() * ((id2 % 8)+0.5) / 4.0;
  dPhi = phi2 - phi1;
  dTheta = theta2 - theta1;
  // Take the short way around.
  while (dPhi > vtkMath::Pi())
  {
    dPhi -= 2*vtkMath::Pi();
  }
  while (dPhi < -vtkMath::Pi())
  {
    dPhi += 2*vtkMath::Pi();
  }
  while (dTheta > vtkMath::Pi())
  {
    dTheta -= 2*vtkMath::Pi();
  }
  while (dTheta < -vtkMath::Pi())
  {
    dTheta += 2*vtkMath::Pi();
  }
  theta2 = theta1 + dTheta;
  phi2 = phi1 + dPhi;

  // Compute the length (world coords).
  length = dTheta * sin(0.5*(phi1+phi2));
  length = sqrt(length*length + dPhi*dPhi);
  // How many division do we need.
  num = (int)(length / 0.1);

  // Compute the perpendicular phi theta step.
  thetaOff = dPhi;
  phiOff = -dTheta;
  // Normalize (sphere coords).
  length = sqrt(thetaOff*thetaOff + phiOff*phiOff);
  phiOff = 0.08 * phiOff / length;
  thetaOff = 0.08 * thetaOff / length;

  // First point.
  x = cos(theta1+thetaOff)*sin(phi1+phiOff);
  y = sin(theta1+thetaOff)*sin(phi1+phiOff);
  z = cos(phi1+phiOff);
  ptId1 = pts->InsertNextPoint(this->Radius*x, this->Radius*y, this->Radius*z);
  x = cos(theta1+2*thetaOff)*sin(phi1+2*phiOff);
  y = sin(theta1+2*thetaOff)*sin(phi1+2*phiOff);
  z = cos(phi1+2*phiOff);
  ptId2 = pts->InsertNextPoint(this->Radius*x, this->Radius*y, this->Radius*z);
  for (idx = 1; idx < num; ++idx)
  {
    // Interpolate angles.
    theta = theta1 + ((double)(idx)/(double)(num)) * dTheta;
    phi = phi1 + ((double)(idx)/(double)(num)) * (phi2-phi1);
    x = cos(theta+thetaOff)*sin(phi+phiOff);
    y = sin(theta+thetaOff)*sin(phi+phiOff);
    z = cos(phi+phiOff);
    ptId3 =
      pts->InsertNextPoint(this->Radius*x, this->Radius*y, this->Radius*z);
    x = cos(theta+2*thetaOff)*sin(phi+2*phiOff);
    y = sin(theta+2*thetaOff)*sin(phi+2*phiOff);
    z = cos(phi+2*phiOff);
    ptId4 =
      pts->InsertNextPoint(this->Radius*x, this->Radius*y, this->Radius*z);
    // Create the rectangle.
    polys->InsertNextCell(4);
    polys->InsertCellPoint(ptId1);
    polys->InsertCellPoint(ptId2);
    polys->InsertCellPoint(ptId4);
    polys->InsertCellPoint(ptId3);
    // Initialize the next step.
    ptId1 = ptId3;
    ptId2 = ptId4;
  }
  // Now create the arrow.
  x = cos(theta)*sin(phi);
  y = sin(theta)*sin(phi);
  z = cos(phi);
  ptId3 = pts->InsertNextPoint(this->Radius*x, this->Radius*y, this->Radius*z);
  x = cos(theta+3*thetaOff)*sin(phi+3*phiOff);
  y = sin(theta+3*thetaOff)*sin(phi+3*phiOff);
  z = cos(phi+3*phiOff);
  ptId4 = pts->InsertNextPoint(this->Radius*x, this->Radius*y, this->Radius*z);
  x = cos(theta2+1.5*thetaOff)*sin(phi2+1.5*phiOff);
  y = sin(theta2+1.5*thetaOff)*sin(phi2+1.5*phiOff);
  z = cos(phi2+1.5*phiOff);
  ptId5 = pts->InsertNextPoint(this->Radius*x, this->Radius*y, this->Radius*z);
  polys->InsertNextCell(5);
  polys->InsertCellPoint(ptId5);
  polys->InsertCellPoint(ptId4);
  polys->InsertCellPoint(ptId2);
  polys->InsertCellPoint(ptId1);
  polys->InsertCellPoint(ptId3);
}

//----------------------------------------------------------------------------
void vtkSpherePuzzleArrows::SetPermutation(vtkSpherePuzzle *puz)
{
  this->SetPermutation(puz->GetState());
}

//----------------------------------------------------------------------------
void vtkSpherePuzzleArrows::PrintSelf(ostream& os, vtkIndent indent)
{
  int i;
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Permutation: ";
  for (i = 0; i < 32; ++i)
  {
    os << this->Permutation[i] << " ";
  }
  os << endl;
}
