/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractPiece.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtractPiece.h"

#include "vtkExtentTranslator.h"
#include "vtkImageClip.h"
#include "vtkExtractPolyDataPiece.h"
#include "vtkExtractRectilinearGrid.h"
#include "vtkExtractGrid.h"
#include "vtkExtractUnstructuredGridPiece.h"
#include "vtkCompositeDataSet.h"
#include "vtkCompositeDataIterator.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGrid.h"
#include "vtkUnstructuredGrid.h"

vtkStandardNewMacro(vtkExtractPiece);

//=============================================================================
int vtkExtractPiece::RequestUpdateExtent(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *vtkNotUsed(outputVector))
{
  // get the info object
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);

  return 1;
}

//=============================================================================
int vtkExtractPiece::RequestDataObject(
  vtkInformation*,
  vtkInformationVector** inputVector ,
  vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
    {
    return 0;
    }
  vtkDataObject *input = inInfo->Get(vtkDataObject::DATA_OBJECT());


  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkDataObject *output = outInfo->Get(vtkDataObject::DATA_OBJECT());

  if (input)
    {
    if (!output || !output->IsA(input->GetClassName()))
      {
      vtkDataObject* outData = input->NewInstance();
      outInfo->Set(vtkDataObject::DATA_OBJECT(), outData);
      outData->Delete();
      }
    return 1;
    }
  return 0;
}

//=============================================================================
int vtkExtractPiece::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkCompositeDataSet *input = vtkCompositeDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (!input)
    {
    return 0;
    }
  vtkCompositeDataSet *output = vtkCompositeDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (!output)
    {
    return 0;
    }

  // Copy structure and meta-data.
  output->CopyStructure(input);

  int updateNumPieces =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  int updatePiece =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int updateGhostLevel =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());


  vtkCompositeDataIterator* iter = input->NewIterator();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal();
     iter->GoToNextItem())
    {
    vtkDataObject *tmpDS  = iter->GetCurrentDataObject();
    switch (tmpDS->GetDataObjectType())
      {
      case VTK_IMAGE_DATA:
        this->ExtractImageData(
          (vtkImageData*)(tmpDS), output,
          updatePiece, updateNumPieces, updateGhostLevel, iter);
        break;
      case VTK_POLY_DATA:
        this->ExtractPolyData(
          (vtkPolyData*)(tmpDS), output,
          updatePiece, updateNumPieces, updateGhostLevel, iter);
        break;
      case VTK_RECTILINEAR_GRID:
        this->ExtractRectilinearGrid(
          (vtkRectilinearGrid*)(tmpDS), output,
          updatePiece, updateNumPieces, updateGhostLevel, iter);
        break;
      case VTK_STRUCTURED_GRID:
        this->ExtractStructuredGrid(
          (vtkStructuredGrid*)(tmpDS), output,
          updatePiece, updateNumPieces, updateGhostLevel, iter);
        break;
      case VTK_UNSTRUCTURED_GRID:
        this->ExtractUnstructuredGrid(
          (vtkUnstructuredGrid*)(tmpDS), output,
          updatePiece, updateNumPieces, updateGhostLevel, iter);
        break;
      default:
        vtkErrorMacro("Cannot extract data of type "
                      << tmpDS->GetClassName());
        break;
      }
    }
  iter->Delete();

  return 1;
}

//=============================================================================
void vtkExtractPiece::ExtractImageData(
  vtkImageData *imageData, vtkCompositeDataSet *output,
  int piece, int numberOfPieces, int ghostLevel,
  vtkCompositeDataIterator* iter)
{
  vtkStreamingDemandDrivenPipeline *extractExecutive;
  vtkInformation *extractInfo;
  int ext[6];

  vtkImageClip *extractID = vtkImageClip::New();
  extractID->ClipDataOn();
  imageData->GetExtent(ext);

  vtkExtentTranslator *translate = vtkExtentTranslator::New();
  translate->SetPiece(piece);
  translate->SetNumberOfPieces(numberOfPieces);
  translate->SetGhostLevel(ghostLevel);
  translate->SetWholeExtent(ext);
  translate->PieceToExtent();
  translate->GetExtent(ext);

  extractID->SetInputData(imageData);
  extractID->SetOutputWholeExtent(ext);
  extractExecutive = vtkStreamingDemandDrivenPipeline::SafeDownCast(
    extractID->GetExecutive());
  extractInfo = extractExecutive->GetOutputInformation(0);
  extractExecutive->UpdateDataObject();
  extractInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
                   ext, 6);
  extractInfo->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT_INITIALIZED(), 1);
  extractID->Update();
  vtkImageData *extractOutput = vtkImageData::New();
  extractOutput->ShallowCopy(extractID->GetOutput());
  output->SetDataSet(iter, extractOutput);
  extractID->Delete();
  translate->Delete();
  extractOutput->Delete();
}

//=============================================================================
void vtkExtractPiece::ExtractPolyData(
  vtkPolyData *polyData, vtkCompositeDataSet *output,
  int piece, int numberOfPieces, int ghostLevel,
  vtkCompositeDataIterator* iter)
{
  vtkStreamingDemandDrivenPipeline *extractExecutive;
  vtkInformation *extractInfo;

  vtkExtractPolyDataPiece *extractPD = vtkExtractPolyDataPiece::New();
  extractPD->SetInputData(polyData);
  extractExecutive = vtkStreamingDemandDrivenPipeline::SafeDownCast(
    extractPD->GetExecutive());
  extractInfo = extractExecutive->GetOutputInformation(0);
  extractExecutive->UpdateDataObject();
  extractInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
                   numberOfPieces);
  extractInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
                   piece);
  extractInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
                   ghostLevel);
  extractInfo->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT_INITIALIZED(), 1);
  extractPD->Update();
  vtkPolyData *extractOutput = vtkPolyData::New();
  extractOutput->ShallowCopy(extractPD->GetOutput());
  output->SetDataSet(iter, extractOutput);
  extractPD->Delete();
  extractOutput->Delete();
}

void vtkExtractPiece::ExtractRectilinearGrid(
  vtkRectilinearGrid *rGrid, vtkCompositeDataSet *output,
  int piece, int numberOfPieces, int ghostLevel,
  vtkCompositeDataIterator* iter)
{
  vtkStreamingDemandDrivenPipeline *extractExecutive;
  vtkInformation *extractInfo;
  int ext[6];

  vtkExtractRectilinearGrid *extractRG =
    vtkExtractRectilinearGrid::New();
  rGrid->GetExtent(ext);

  vtkExtentTranslator *translate = vtkExtentTranslator::New();
  translate->SetPiece(piece);
  translate->SetNumberOfPieces(numberOfPieces);
  translate->SetGhostLevel(ghostLevel);
  translate->SetWholeExtent(ext);
  translate->PieceToExtent();
  translate->GetExtent(ext);

  extractRG->SetInputData(rGrid);
  extractExecutive = vtkStreamingDemandDrivenPipeline::SafeDownCast(
    extractRG->GetExecutive());
  extractInfo = extractExecutive->GetOutputInformation(0);
  extractExecutive->UpdateDataObject();
  extractInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
                   ext, 6);
  extractInfo->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT_INITIALIZED(), 1);
  extractRG->Update();
  vtkRectilinearGrid *extractOutput = vtkRectilinearGrid::New();
  extractOutput->ShallowCopy(extractRG->GetOutput());
  output->SetDataSet(iter, extractOutput);
  extractRG->Delete();
  translate->Delete();
  extractOutput->Delete();
}

//=============================================================================
void vtkExtractPiece::ExtractStructuredGrid(
  vtkStructuredGrid *sGrid, vtkCompositeDataSet *output,
  int piece, int numberOfPieces, int ghostLevel,
  vtkCompositeDataIterator* iter)
{
  vtkStreamingDemandDrivenPipeline *extractExecutive;
  vtkInformation *extractInfo;
  int ext[6];

  vtkExtractGrid *extractSG = vtkExtractGrid::New();
  sGrid->GetExtent(ext);

  vtkExtentTranslator *translate = vtkExtentTranslator::New();
  translate->SetPiece(piece);
  translate->SetNumberOfPieces(numberOfPieces);
  translate->SetGhostLevel(ghostLevel);
  translate->SetWholeExtent(ext);
  translate->PieceToExtent();
  translate->GetExtent(ext);

  extractSG->SetInputData(sGrid);
  extractExecutive = vtkStreamingDemandDrivenPipeline::SafeDownCast(
    extractSG->GetExecutive());
  extractInfo = extractExecutive->GetOutputInformation(0);
  extractExecutive->UpdateDataObject();
  extractInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
                   ext, 6);
  extractInfo->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);
  extractInfo->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT_INITIALIZED(), 1);
  extractSG->Update();
  vtkStructuredGrid *extractOutput = vtkStructuredGrid::New();
  extractOutput->ShallowCopy(extractSG->GetOutput());
  output->SetDataSet(iter, extractOutput);
  extractSG->Delete();
  translate->Delete();
  extractOutput->Delete();
}

//=============================================================================
void vtkExtractPiece::ExtractUnstructuredGrid(
  vtkUnstructuredGrid *uGrid, vtkCompositeDataSet *output,
  int piece, int numberOfPieces, int ghostLevel,
  vtkCompositeDataIterator* iter)
{
  vtkStreamingDemandDrivenPipeline *extractExecutive;
  vtkInformation *extractInfo;

  vtkExtractUnstructuredGridPiece *extractUG =
    vtkExtractUnstructuredGridPiece::New();
  extractUG->SetInputData(uGrid);
  extractExecutive = vtkStreamingDemandDrivenPipeline::SafeDownCast(
    extractUG->GetExecutive());
  extractInfo = extractExecutive->GetOutputInformation(0);
  extractExecutive->UpdateDataObject();
  extractInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
                   numberOfPieces);
  extractInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
                   piece);
  extractInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
                   ghostLevel);
  extractInfo->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT_INITIALIZED(), 1);
  extractUG->Update();
  vtkUnstructuredGrid *extractOutput = vtkUnstructuredGrid::New();
  extractOutput->ShallowCopy(extractUG->GetOutput());
  output->SetDataSet(iter, extractOutput);
  extractUG->Delete();
  extractOutput->Delete();
}

//=============================================================================
void vtkExtractPiece::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
