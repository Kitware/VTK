/*=========================================================================
      </ProxyProperty>

  Program:   Visualization Toolkit
  Module:    vtkGhostCellsGenerator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Hide VTK_DEPRECATED_IN_9_1_0() warning for this class
#define VTK_DEPRECATION_LEVEL 0

#include "vtkGhostCellsGenerator.h"

#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDIYGhostUtilities.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkRectilinearGrid.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGrid.h"

#include <vector>

vtkStandardNewMacro(vtkGhostCellsGenerator);
vtkCxxSetObjectMacro(vtkGhostCellsGenerator, Controller, vtkMultiProcessController);

//----------------------------------------------------------------------------
vtkGhostCellsGenerator::vtkGhostCellsGenerator()
  : Controller(nullptr)
  , NumberOfGhostLayers(1)
{
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
vtkGhostCellsGenerator::~vtkGhostCellsGenerator()
{
  this->SetController(nullptr);
}

//------------------------------------------------------------------------------
int vtkGhostCellsGenerator::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
int vtkGhostCellsGenerator::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataObject* inputDO = vtkDataObject::GetData(inputVector[0], 0);
  vtkDataObject* outputDO = vtkDataObject::GetData(outputVector, 0);

  bool error = false;

  if (auto outputCDS = vtkCompositeDataSet::SafeDownCast(outputDO))
  {
    if (auto inputCDS = vtkCompositeDataSet::SafeDownCast(inputDO))
    {
      outputCDS->CopyStructure(inputCDS);
      auto iter = inputCDS->NewIterator();
      for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
        auto subInputDO = iter->GetCurrentDataObject();
        outputCDS->SetDataSet(
          iter, vtkSmartPointer<vtkDataObject>::Take(subInputDO->NewInstance()));
      }
      iter->Delete();
    }
    else
    {
      error = true;
    }
  }
  else if (!vtkDataSet::SafeDownCast(outputDO) || !vtkDataSet::SafeDownCast(inputDO))
  {
    error = true;
  }

  if (error)
  {
    vtkErrorMacro(<< "Could not generate output");
    return 0;
  }

  std::vector<vtkImageData*> inputsID = vtkCompositeDataSet::GetDataSets<vtkImageData>(inputDO);
  std::vector<vtkImageData*> outputsID = vtkCompositeDataSet::GetDataSets<vtkImageData>(outputDO);

  std::vector<vtkRectilinearGrid*> inputsRG =
    vtkCompositeDataSet::GetDataSets<vtkRectilinearGrid>(inputDO);
  std::vector<vtkRectilinearGrid*> outputsRG =
    vtkCompositeDataSet::GetDataSets<vtkRectilinearGrid>(outputDO);

  std::vector<vtkStructuredGrid*> inputsSG =
    vtkCompositeDataSet::GetDataSets<vtkStructuredGrid>(inputDO);
  std::vector<vtkStructuredGrid*> outputsSG =
    vtkCompositeDataSet::GetDataSets<vtkStructuredGrid>(outputDO);

  if (!inputsID.empty() && !inputsRG.empty() && !inputsSG.empty())
  {
    vtkWarningMacro(<< "Ghost cell generator called with mixed types."
                    << "Ghosts are not exchanged between data sets of different types.");
  }

  return vtkDIYGhostUtilities::GenerateGhostCells(
           inputsID, outputsID, this->NumberOfGhostLayers, this->Controller) &&
    vtkDIYGhostUtilities::GenerateGhostCells(
      inputsRG, outputsRG, this->NumberOfGhostLayers, this->Controller) &&
    vtkDIYGhostUtilities::GenerateGhostCells(
      inputsSG, outputsSG, this->NumberOfGhostLayers, this->Controller);
}

//----------------------------------------------------------------------------
int vtkGhostCellsGenerator::RequestUpdateExtent(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  outputVector->GetInformationObject(0)->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), this->NumberOfGhostLayers);
  return 1;
}

//----------------------------------------------------------------------------
void vtkGhostCellsGenerator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
}
