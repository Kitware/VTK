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
#include "vtkDataObject.h"
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
  this->SphereTree = nullptr;
  this->TreeHierarchy = 1;

  this->ExtractionMode = VTK_SPHERE_TREE_LEVELS;
  this->Level = (-1);
  this->Point[0] = this->Point[1] = this->Point[2] = 0.0;
  this->Ray[0] = 1.0;
  this->Ray[1] = this->Ray[2] = 0.0;
  this->Normal[0] = this->Normal[1] = 0.0;
  this->Normal[2] = 1.0;
}

//----------------------------------------------------------------------------
vtkSphereTreeFilter::~vtkSphereTreeFilter()
{
  this->SetSphereTree(nullptr);
}

//----------------------------------------------------------------------------
// Overload standard modified time function. If the sphere tree is modified,
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
int vtkSphereTreeFilter::RequestData(vtkInformation *vtkNotUsed(request),
                                     vtkInformationVector **inputVector,
                                     vtkInformationVector *outputVector)
{
  vtkDebugMacro(<<"Generating spheres");

  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet *input = nullptr;
  if ( inInfo != nullptr )
  {
    input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  }
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Make sure there is data
  vtkIdType numCells;
  int numLevels=0;
  if ( this->SphereTree ) //first choice
  {
    // Get number of spheres/cells from root level
    numLevels = this->SphereTree->GetNumberOfLevels();
    this->SphereTree->GetTreeSpheres(numLevels-1,numCells);
  }
  else if ( input ) //next choice
  {
    numCells = input->GetNumberOfCells();
  }
  else //oh oh no input
  {
    vtkWarningMacro(<< "No input!");
    return 1;
  }

  // If no sphere tree, create one from the input
  if ( ! this->SphereTree )
  {
    this->SphereTree = vtkSphereTree::New();
    this->SphereTree->SetBuildHierarchy(this->TreeHierarchy);
    this->SphereTree->Build(input);
    numLevels = this->SphereTree->GetNumberOfLevels();
    this->SphereTree->GetTreeSpheres(numLevels-1,numCells);
  }

  // See if hierarchy was created
  int builtHierarchy = this->SphereTree->GetBuildHierarchy() &&
    this->TreeHierarchy;

  // Allocate: points (center of spheres), radii, level in tree
  vtkPoints *newPts = vtkPoints::New();
  newPts->SetDataTypeToDouble();

  vtkDoubleArray *radii = vtkDoubleArray::New();
  radii->Allocate(numCells);

  vtkIntArray *levels=nullptr; //in case they are needed

  const double *cellSpheres = this->SphereTree->GetCellSpheres();
  if ( this->ExtractionMode == VTK_SPHERE_TREE_LEVELS )
  {
    levels = vtkIntArray::New();
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
    if ( builtHierarchy )
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
  } //extract levels

  else //perform geometric query
  {
    // Use the slower API because it tests the code better
    vtkIdList *cellIds = vtkIdList::New();
    vtkIdType cellId, numSelectedCells;
    const double *sphere=cellSpheres;

    if ( this->ExtractionMode == VTK_SPHERE_TREE_POINT )
    {
      this->SphereTree->SelectPoint(this->Point,cellIds);
    }
    else if ( this->ExtractionMode == VTK_SPHERE_TREE_LINE )
    {
      this->SphereTree->SelectLine(this->Point,this->Ray,cellIds);
    }
    else if ( this->ExtractionMode == VTK_SPHERE_TREE_PLANE )
    {
      this->SphereTree->SelectPlane(this->Point,this->Normal,cellIds);
    }

    numSelectedCells = cellIds->GetNumberOfIds();
    for (vtkIdType i=0; i < numSelectedCells; ++i)
    {
      cellId = cellIds->GetId(i);
      sphere = cellSpheres + 4*cellId;
      newPts->InsertPoint(i,sphere);
      radii->InsertValue(i,sphere[3]);
    }
    cellIds->Delete();
  }//geometric queries

  // Produce output
  output->SetPoints(newPts);
  newPts->Delete();

  radii->SetName("SphereTree");
  output->GetPointData()->SetScalars(radii);
  radii->Delete();

  if ( levels != nullptr )
  {
    levels->SetName("SphereLevels");
    output->GetPointData()->AddArray(levels);
    levels->Delete();
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkSphereTreeFilter::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);

  return 1;
}

//----------------------------------------------------------------------------
const char *vtkSphereTreeFilter::GetExtractionModeAsString(void)
{
  if ( this->ExtractionMode == VTK_SPHERE_TREE_LEVELS )
  {
    return "Levels";
  }
  else if ( this->ExtractionMode == VTK_SPHERE_TREE_POINT )
  {
    return "Point";
  }
  else if ( this->ExtractionMode == VTK_SPHERE_TREE_LINE )
  {
    return "Line";
  }
  else
  {
    return "Plane";
  }
}

//----------------------------------------------------------------------------
void vtkSphereTreeFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Sphere Tree: " << this->SphereTree << "\n";
  os << indent << "Build Tree Hierarchy: "
     << (this->TreeHierarchy ? "On\n" : "Off\n");

  os << indent << "Extraction Mode: "
     << this->GetExtractionModeAsString() << endl;

  os << indent << "Level: " << this->Level << "\n";

  os << indent << "Point: (" << this->Point[0] << ", "
    << this->Point[1] << ", " << this->Point[2] << ")\n";

  os << indent << "Ray: (" << this->Ray[0] << ", "
    << this->Ray[1] << ", " << this->Ray[2] << ")\n";

  os << indent << "Normal: (" << this->Normal[0] << ", "
    << this->Normal[1] << ", " << this->Normal[2] << ")\n";
}
