/*=========================================================================
      </ProxyProperty>

  Program:   Visualization Toolkit
  Module:    vtkCompositeDataSetGhostCellsGenerator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCompositeDataSetGhostCellsGenerator.h"

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

vtkStandardNewMacro(vtkCompositeDataSetGhostCellsGenerator);
vtkCxxSetObjectMacro(vtkCompositeDataSetGhostCellsGenerator, Controller, vtkMultiProcessController);

//----------------------------------------------------------------------------
vtkCompositeDataSetGhostCellsGenerator::vtkCompositeDataSetGhostCellsGenerator()
  : Controller(nullptr)
  , NumberOfGhostLayers(2)
{
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
vtkCompositeDataSetGhostCellsGenerator::~vtkCompositeDataSetGhostCellsGenerator()
{
  this->SetController(nullptr);
}

//------------------------------------------------------------------------------
int vtkCompositeDataSetGhostCellsGenerator::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  return 1;
}

//----------------------------------------------------------------------------
int vtkCompositeDataSetGhostCellsGenerator::RequestData(
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
        outputCDS->SetDataSet(iter, subInputDO->NewInstance());
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

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  // FIXME This should be rethought.
  // See https://gitlab.kitware.com/vtk/vtk/-/merge_requests/7507#note_886095
  int inputGhostLevels =
    inInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());

  std::vector<vtkImageData*> inputsID = vtkDIYUtilities::GetDataSets<vtkImageData>(inputDO);
  std::vector<vtkImageData*> outputsID = vtkDIYUtilities::GetDataSets<vtkImageData>(outputDO);
  std::vector<vtkRectilinearGrid*> inputsRG =
    vtkDIYUtilities::GetDataSets<vtkRectilinearGrid>(inputDO);
  std::vector<vtkRectilinearGrid*> outputsRG =
    vtkDIYUtilities::GetDataSets<vtkRectilinearGrid>(outputDO);
  std::vector<vtkStructuredGrid*> inputsSG =
    vtkDIYUtilities::GetDataSets<vtkStructuredGrid>(inputDO);
  std::vector<vtkStructuredGrid*> outputsSG =
    vtkDIYUtilities::GetDataSets<vtkStructuredGrid>(outputDO);

  if (!inputsID.empty() && !inputsRG.empty())
  {
    vtkWarningMacro(<< "Ghost cell generator called with mixed types."
                    << "Ghosts are not exchanged between data sets of different types.");
  }

  return vtkDIYGhostUtilities::GenerateGhostCells(
           inputsID, outputsID, inputGhostLevels, this->NumberOfGhostLayers, this->Controller) &&
    vtkDIYGhostUtilities::GenerateGhostCells(
      inputsRG, outputsRG, inputGhostLevels, this->NumberOfGhostLayers, this->Controller) &&
    vtkDIYGhostUtilities::GenerateGhostCells(
      inputsSG, outputsSG, inputGhostLevels, this->NumberOfGhostLayers, this->Controller);
}

//----------------------------------------------------------------------------
int vtkCompositeDataSetGhostCellsGenerator::RequestUpdateExtent(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  outputVector->GetInformationObject(0)->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), this->NumberOfGhostLayers);
  return 1;
}

//----------------------------------------------------------------------------
void vtkCompositeDataSetGhostCellsGenerator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
}
