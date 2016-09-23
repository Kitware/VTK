/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSpatialRepresentationFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSpatialRepresentationFilter.h"

#include "vtkLocator.h"
#include "vtkInformation.h"
#include "vtkGarbageCollector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"

#include <set>

class vtkSpatialRepresentationFilterInternal
{
public:
  std::set<int> Levels;
};

vtkStandardNewMacro(vtkSpatialRepresentationFilter);
vtkCxxSetObjectMacro(vtkSpatialRepresentationFilter,
                     SpatialRepresentation,vtkLocator);

vtkSpatialRepresentationFilter::vtkSpatialRepresentationFilter()
{
  this->SetNumberOfInputPorts(1);
  this->SpatialRepresentation = NULL;
  this->MaximumLevel = 0;
  this->GenerateLeaves = false;
  this->Internal = new vtkSpatialRepresentationFilterInternal;
}

vtkSpatialRepresentationFilter::~vtkSpatialRepresentationFilter()
{
  if ( this->SpatialRepresentation )
  {
    this->SpatialRepresentation->UnRegister(this);
    this->SpatialRepresentation = NULL;
  }
  delete this->Internal;
}

void vtkSpatialRepresentationFilter::AddLevel(int level)
{
  this->Internal->Levels.insert(level);
}

void vtkSpatialRepresentationFilter::ResetLevels()
{
  this->Internal->Levels.clear();
}

int vtkSpatialRepresentationFilter::RequestData(
  vtkInformation*,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkDataSet* input = vtkDataSet::GetData(inputVector[0]);
  vtkMultiBlockDataSet* output = vtkMultiBlockDataSet::GetData(outputVector);

  if (this->SpatialRepresentation == NULL)
  {
    vtkErrorMacro(<< "SpatialRepresentation is NULL.");
    return 0;
  }

  this->SpatialRepresentation->SetDataSet(input);
  this->SpatialRepresentation->Update();
  this->MaximumLevel = this->SpatialRepresentation->GetLevel();

  //
  // Loop over all requested levels generating new levels as necessary
  //
  std::set<int>::iterator it;
  for ( it = this->Internal->Levels.begin();
        it != this->Internal->Levels.end();
        it++ )
  {
    if ( *it <= this->MaximumLevel )
    {
      vtkNew<vtkPolyData> level_representation;
      output->SetBlock(*it, level_representation.GetPointer());
      this->SpatialRepresentation->GenerateRepresentation(
        *it, level_representation.GetPointer());
    }
  }
  if (this->GenerateLeaves)
  {
    vtkNew<vtkPolyData> leaf_representation;
    output->SetBlock(this->MaximumLevel + 1, leaf_representation.GetPointer());
    this->SpatialRepresentation->GenerateRepresentation(
      -1, leaf_representation.GetPointer());
  }

  return 1;
}

void vtkSpatialRepresentationFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Maximum Level: " << this->MaximumLevel << "\n";
  os << indent << "GenerateLeaves: " << this->GenerateLeaves << "\n";

  if ( this->SpatialRepresentation )
  {
    os << indent << "Spatial Representation: " << this->SpatialRepresentation
       << "\n";
  }
  else
  {
    os << indent << "Spatial Representation: (none)\n";
  }
}


//----------------------------------------------------------------------------
void vtkSpatialRepresentationFilter::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  // This filter shares our input and is therefore involved in a
  // reference loop.
  vtkGarbageCollectorReport(collector, this->SpatialRepresentation,
                            "SpatialRepresentation");
}

//----------------------------------------------------------------------------
int vtkSpatialRepresentationFilter::FillInputPortInformation(int port, vtkInformation* info)
{
  if(!this->Superclass::FillInputPortInformation(port, info))
  {
    return 0;
  }
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}
