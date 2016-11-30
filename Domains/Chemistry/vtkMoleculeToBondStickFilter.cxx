/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMoleculeToBondStickFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMoleculeToBondStickFilter.h"

#include "vtkCellArray.h"
#include "vtkCylinderSource.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"
#include "vtkMolecule.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkTransform.h"
#include "vtkUnsignedShortArray.h"

vtkStandardNewMacro(vtkMoleculeToBondStickFilter);

//----------------------------------------------------------------------------
vtkMoleculeToBondStickFilter::vtkMoleculeToBondStickFilter()
{
}

//----------------------------------------------------------------------------
vtkMoleculeToBondStickFilter::~vtkMoleculeToBondStickFilter()
{
}

//----------------------------------------------------------------------------
int vtkMoleculeToBondStickFilter::RequestData(
  vtkInformation *,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkMolecule *input = vtkMolecule::SafeDownCast
    (vtkDataObject::GetData(inputVector[0]));
  vtkPolyData *output = vtkPolyData::SafeDownCast
    (vtkDataObject::GetData(outputVector));

  // Extract data from input molecule
  vtkIdType numBonds = input->GetNumberOfBonds();

  // Prep the output
  output->Initialize();
  vtkCellArray *polys = vtkCellArray::New();
  vtkPoints *points = vtkPoints::New();
  vtkUnsignedShortArray *bondOrders = vtkUnsignedShortArray::New();

  // Initialize a CylinderSource
  vtkCylinderSource *cylSource = vtkCylinderSource::New();
  cylSource->SetResolution(20);
  cylSource->SetHeight(1.0);
  cylSource->Update();

  // Preallocate memory
  points->Allocate(3 * numBonds * cylSource->GetOutput()->GetPoints()->
                   GetNumberOfPoints());
  polys->Allocate(3 * numBonds * cylSource->GetOutput()->GetPolys()->
                  GetNumberOfCells());
  bondOrders->Allocate(points->GetNumberOfPoints());

  // Create a transform object to map the cylinder source to the bond
  vtkTransform *xform = vtkTransform::New();
  xform->PostMultiply();

  // Declare some variables for later
  vtkIdType numCellPoints, *cellPoints;
  unsigned short bondOrder;
  double bondLength;
  double radius;
  double delta[3];
  double initialDisp[3];
  double rotAngle;
  double rotAxis[3];
  double bondCenter[3];
  double pos1[3], pos2[3];
  // Normalized vector pointing along the cylinder (y axis);
  static const double cylVec[3] = {0.0, 1.0, 0.0};
  // Normalized vector pointing along bond
  double bondVec[3];
  // Unit z vector
  static const double unitZ[3] = {0.0, 0.0, 1.0};

  // Build a sphere for each atom and append it's data to the output
  // arrays.
  for (vtkIdType bondInd = 0; bondInd < numBonds; ++bondInd)
  {
    // Extract bond info
    vtkBond bond = input->GetBond(bondInd);
    bondOrder = bond.GetOrder();
    bond.GetBeginAtom().GetPosition(pos1);
    bond.GetEndAtom().GetPosition(pos2);

    // Compute additional bond info
    // - Normalized vector in direction of bond
    vtkMath::Subtract(pos2, pos1, bondVec);
    bondLength = vtkMath::Normalize(bondVec);
    // - Axis for cylinder rotation, bondVec [cross] cylVec
    vtkMath::Cross(bondVec, cylVec, rotAxis);
    // Rotation angle
    rotAngle = -vtkMath::DegreesFromRadians
      (acos(vtkMath::Dot(bondVec, cylVec)));
    // - Center of bond for translation
    vtkMath::Add(pos2, pos1, bondCenter);
    vtkMath::MultiplyScalar(bondCenter, 0.5);

    // Set up delta step vector and bond radius from bond order:
    switch (bondOrder)
    {
      case 1:
      default:
        radius = 0.1;
        delta[0] = 0.0; delta[1] = 0.0; delta[2] = 0.0;
        initialDisp[0] = 0.0; initialDisp[1] = 0.0; initialDisp[2] = 0.0;
        break;
      case 2:
        radius = 0.1;
        vtkMath::Cross(bondVec, unitZ, delta);
        //vtkMath::Normalize(delta);
        vtkMath::MultiplyScalar
          (delta, radius + radius);
        initialDisp[0] = delta[0] * -0.5;
        initialDisp[1] = delta[1] * -0.5;
        initialDisp[2] = delta[2] * -0.5;
        break;
      case 3:
        radius = 0.1;
        vtkMath::Cross(bondVec, unitZ, delta);
        //vtkMath::Normalize(delta);
        vtkMath::MultiplyScalar
          (delta, radius + radius);
        initialDisp[0] = -delta[0];
        initialDisp[1] = -delta[1];
        initialDisp[2] = -delta[2];
        break;
    }

    // Construct transform
    xform->Identity();
    xform->Scale(radius, bondLength, radius);
    xform->RotateWXYZ(rotAngle, rotAxis);
    xform->Translate(bondCenter[0], bondCenter[1], bondCenter[2]);
    xform->Translate(initialDisp);

    // For each bond order, add a cylinder to output, translate by
    // delta, and repeat.
    for (unsigned short iter = 0; iter < bondOrder; ++iter)
    {
      vtkPolyData *cylinder = cylSource->GetOutput();
      vtkPoints *cylPoints = cylinder->GetPoints();
      vtkCellArray *cylPolys = cylinder->GetPolys();
      xform->TransformPoints(cylPoints, points);

      // Get offset for the new point IDs that will be added to points
      vtkIdType pointOffset = points->GetNumberOfPoints();
      // Total number of new points
      vtkIdType numPoints = cylPoints->GetNumberOfPoints();

      // Use bond order for point scalar data.
      for (vtkIdType i = 0; i < numPoints; ++i)
      {
        bondOrders->InsertNextValue(bondOrder);
      }

      // Add new cells (polygons) that represent the cylinder
      cylPolys->InitTraversal();
      while (cylPolys->GetNextCell(numCellPoints, cellPoints) != 0)
      {
        vtkIdType *newCellPoints = new vtkIdType[numCellPoints];
        for (vtkIdType i = 0; i < numCellPoints; ++i)
        {
          // The new point ids should be offset by the pointOffset above
          newCellPoints[i] = cellPoints[i] + pointOffset;
        }
        polys->InsertNextCell(numCellPoints, newCellPoints);
        delete [] newCellPoints;
      }

      // Setup for the next cylinder in a multi-bond
      xform->Translate(delta);
    }
  }

  // Release extra memory
  points->Squeeze();
  bondOrders->Squeeze();
  polys->Squeeze();

  // update output
  output->SetPoints(points);
  output->GetPointData()->SetScalars(bondOrders);
  output->SetPolys(polys);

  // Clean up
  xform->Delete();
  polys->Delete();
  points->Delete();
  bondOrders->Delete();
  cylSource->Delete();

  return 1;
}

//----------------------------------------------------------------------------
void vtkMoleculeToBondStickFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
