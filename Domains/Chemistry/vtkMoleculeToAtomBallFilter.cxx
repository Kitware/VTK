/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMoleculeToAtomBallFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMoleculeToAtomBallFilter.h"

#include "vtkCellArray.h"
#include "vtkObjectFactory.h"
#include "vtkMolecule.h"
#include "vtkPeriodicTable.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkSphereSource.h"
#include "vtkUnsignedShortArray.h"

vtkStandardNewMacro(vtkMoleculeToAtomBallFilter);

//----------------------------------------------------------------------------
vtkMoleculeToAtomBallFilter::vtkMoleculeToAtomBallFilter()
  : Resolution(50),
    RadiusScale(0.8),
    RadiusSource(CovalentRadius)
{
}

//----------------------------------------------------------------------------
vtkMoleculeToAtomBallFilter::~vtkMoleculeToAtomBallFilter()
{
}

//----------------------------------------------------------------------------
int vtkMoleculeToAtomBallFilter::RequestData(
  vtkInformation *,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkMolecule *input = vtkMolecule::SafeDownCast
    (vtkDataObject::GetData(inputVector[0]));
  vtkPolyData *output = vtkPolyData::SafeDownCast
    (vtkDataObject::GetData(outputVector));

  // Need this for radius / color lookups
  vtkPeriodicTable *pTab = vtkPeriodicTable::New();

  // Extract data from input molecule
  vtkIdType numAtoms = input->GetNumberOfAtoms();

  // Prep the output
  output->Initialize();
  vtkCellArray *polys = vtkCellArray::New();
  vtkPoints *points = vtkPoints::New();
  vtkUnsignedShortArray *atomicNums = vtkUnsignedShortArray::New();

  // Initialize a SphereSource
  vtkSphereSource *sphereSource = vtkSphereSource::New();
  sphereSource->SetThetaResolution(this->Resolution);
  sphereSource->SetPhiResolution(this->Resolution);
  sphereSource->Update();

  // Preallocate memory
  points->Allocate(numAtoms * sphereSource->GetOutput()->GetPoints()->
                   GetNumberOfPoints());
  polys->Allocate(numAtoms * sphereSource->GetOutput()->GetPolys()->
                  GetNumberOfCells());
  atomicNums->Allocate(points->GetNumberOfPoints());

  // Initialize some variables for later
  vtkIdType numCellPoints, *cellPoints;
  double scaledRadius;
  unsigned short atomicNum;
  vtkVector3f pos;

  // Build a sphere for each atom and append it's data to the output
  // arrays.
  for (vtkIdType atomInd = 0; atomInd < numAtoms; ++atomInd)
  {
    // Extract atomic number, position
    vtkAtom atom = input->GetAtom(atomInd);
    atomicNum = atom.GetAtomicNumber();
    pos = atom.GetPosition();

    // Get scaled radius:
    switch (this->RadiusSource)
    {
      default:
      case CovalentRadius:
        scaledRadius = this->RadiusScale * pTab->GetCovalentRadius(atomicNum);
        break;
      case VDWRadius:
        scaledRadius = this->RadiusScale * pTab->GetVDWRadius(atomicNum);
        break;
      case UnitRadius:
        scaledRadius = this->RadiusScale /* * 1.0 */;
        break;
    }

    // Make hydrogens slightly larger
    if (atomicNum == 1 && RadiusSource == CovalentRadius)
    {
      scaledRadius *= 1.1;
    }


    // Update sphere source
    sphereSource->SetRadius(scaledRadius);
    sphereSource->SetCenter(pos.Cast<double>().GetData());
    sphereSource->Update();

    // Extract polydata from sphere
    vtkPolyData *sphere = sphereSource->GetOutput();
    vtkPoints *spherePoints = sphere->GetPoints();
    vtkCellArray *spherePolys = sphere->GetPolys();

    // Get offset for the new point IDs that will be added to points
    vtkIdType pointOffset = points->GetNumberOfPoints();
    // Total number of new points
    vtkIdType numPoints = spherePoints->GetNumberOfPoints();

    // Add new points, use atomic number for point scalar data.
    for (vtkIdType i = 0; i < numPoints; ++i)
    {
      points->InsertNextPoint(spherePoints->GetPoint(i));
      atomicNums->InsertNextValue(atomicNum);
    }

    // Add new cells (polygons) that represent the sphere
    spherePolys->InitTraversal();
    while (spherePolys->GetNextCell(numCellPoints, cellPoints) != 0)
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
  }

  // update output
  output->SetPoints(points);
  output->GetPointData()->SetScalars(atomicNums);
  output->SetPolys(polys);

  // Clean up:
  pTab->Delete();
  polys->Delete();
  points->Delete();
  atomicNums->Delete();
  sphereSource->Delete();

  return 1;
}

//----------------------------------------------------------------------------
void vtkMoleculeToAtomBallFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "RadiusSource: ";
  switch (RadiusSource)
  {
    case CovalentRadius:
      os << "CovalentRadius\n";
      break;
    case VDWRadius:
      os << "CovalentRadius\n";
      break;
    case UnitRadius:
      os << "CovalentRadius\n";
      break;
    default:
      os << "Unknown\n";
      break;
  }
  os << indent << "Resolution: " << Resolution << "\n";
  os << indent << "RadiusScale: " << RadiusScale << "\n";
}
