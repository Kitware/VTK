/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSphereTreeFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSphereTreeFilter.h"
#include "vtkSphereTree.h"
#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkStructuredGrid.h"
#include "vtkDoubleArray.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkObjectFactory.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkAbstractArray.h"
#include "vtkDataArray.h"

vtkStandardNewMacro(vtkSphereTreeFilter);
vtkCxxSetObjectMacro(vtkSphereTreeFilter,SphereTree,vtkSphereTree);

// Construct object.
//----------------------------------------------------------------------------
vtkSphereTreeFilter::vtkSphereTreeFilter()
{
  this->SphereTree = NULL;
  this->TreeHierarchy = 1;
}

//----------------------------------------------------------------------------
vtkSphereTreeFilter::~vtkSphereTreeFilter()
{
  if ( this->SphereTree )
  {
    this->SphereTree->Delete();
    this->SphereTree = NULL;
    this->Level = (-1);
  }
}

//----------------------------------------------------------------------------
// Overload standard modified time function. If contour values are modified,
// then this object is modified as well.
vtkMTimeType vtkSphereTreeFilter::GetMTime()
{
  unsigned long mTime=this->Superclass::GetMTime();
  unsigned long time;

  if (this->SphereTree)
  {
    time = this->SphereTree->GetMTime();
    mTime = ( time > mTime ? time : mTime );
  }

  return mTime;
}

//----------------------------------------------------------------------------
// Produce the sphere tree as requested
int vtkSphereTreeFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and ouptut
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Make sure there is data
  vtkIdType numCells;
  if ( ((numCells=input->GetNumberOfCells()) < 1) )
  {
    vtkDebugMacro(<< "No input!");
    return 1;
  }

  vtkDebugMacro(<<"Generating spheres");

  // If no sphere tree, create one
  if ( ! this->SphereTree )
  {
    this->SphereTree = vtkSphereTree::New();
    this->SphereTree->SetBuildHierarchy(this->TreeHierarchy);
    this->SphereTree->Build(input);
  }
  const double *cellSpheres=this->SphereTree->GetCellSpheres();
  int numLevels=this->SphereTree->GetNumberOfLevels();

  // See if hierarchy was created
  int buildHierarchy = this->SphereTree->GetBuildHierarchy() &&
    this->TreeHierarchy;

  // Allocate: points (center of spheres), radii, level in tree
  vtkPoints *newPts = vtkPoints::New();
  newPts->SetDataTypeToDouble();
  newPts->Allocate(numCells);

  vtkDoubleArray *radii = vtkDoubleArray::New();
  radii->Allocate(numCells);

  vtkIntArray *levels = vtkIntArray::New();
  levels->Allocate(numCells);

  // Create a point per cell. Create a scalar per cell (the radius).
  if ( this->Level < 0 || this->Level == (numLevels - 1) )
  {
    vtkIdType cellId;
    const double *sphere=cellSpheres;
    for (cellId=0; cellId < numCells; cellId++, sphere+=4)
    {
      newPts->InsertPoint(cellId,sphere);
      radii->InsertValue(cellId,sphere[3]);
      levels->InsertValue(cellId,numLevels);
    }
  }

  // If the hierarchy is requested, generate these points too.
  if ( buildHierarchy )
  {
    int i, level;
    vtkIdType numSpheres;
    const double *lSphere;
    for (level=0; level<numLevels; ++level)
    {
      if ( this->Level < 0 || this->Level == level )
      {
        lSphere = this->SphereTree->GetTreeSpheres(level,numSpheres);
        for (i=0; i<numSpheres; ++i, lSphere+=4)
        {
          newPts->InsertNextPoint(lSphere);
          radii->InsertNextValue(lSphere[3]);
          levels->InsertNextValue(level);
        }
      }
    }
  }

  // Produce output
  output->SetPoints(newPts);

  radii->SetName("SphereTree");
  output->GetPointData()->SetScalars(radii);

  levels->SetName("SphereLevels");
  output->GetPointData()->AddArray(levels);

  return 1;
}

//----------------------------------------------------------------------------
int vtkSphereTreeFilter::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 0);

  return 1;
}

//----------------------------------------------------------------------------
void vtkSphereTreeFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Sphere Tree: " << this->SphereTree << "\n";
  os << indent << "Build Tree Hierarchy: "
     << (this->TreeHierarchy ? "On\n" : "Off\n");
}
