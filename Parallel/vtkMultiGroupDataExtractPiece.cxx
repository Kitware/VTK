/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiGroupDataExtractPiece.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMultiGroupDataExtractPiece.h"

#include "vtkCellData.h"
#include "vtkExtentTranslator.h"
#include "vtkExtractPolyDataPiece.h"
#include "vtkExtractRectilinearGrid.h"
#include "vtkExtractGrid.h"
#include "vtkExtractUnstructuredGridPiece.h"
#include "vtkMultiGroupDataSet.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGrid.h"
#include "vtkUnstructuredGrid.h"

vtkCxxRevisionMacro(vtkMultiGroupDataExtractPiece, "1.3");
vtkStandardNewMacro(vtkMultiGroupDataExtractPiece);

int vtkMultiGroupDataExtractPiece::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and ouptut
  vtkMultiGroupDataSet *input = vtkMultiGroupDataSet::SafeDownCast(
    inInfo->Get(vtkCompositeDataSet::COMPOSITE_DATA_SET()));
  if (!input)
    {
    return 0;
    }
  vtkMultiGroupDataSet *output = vtkMultiGroupDataSet::SafeDownCast(
    outInfo->Get(vtkCompositeDataSet::COMPOSITE_DATA_SET()));
  if (!output)
    {
    return 0;
    }

  int updateNumPieces =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  int updatePiece =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int updateGhostLevel =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());

  vtkDataObject *tmpDS;
  unsigned int i;

  for (i = 0; i < input->GetNumberOfGroups(); i++)
    {
    tmpDS = input->GetDataSet(i, 0);
    switch (tmpDS->GetDataObjectType())
      {
      case VTK_IMAGE_DATA:
        this->ExtractImageData(
          (vtkImageData*)(tmpDS), output,
          updatePiece, updateNumPieces, updateGhostLevel, i);
        break;
      case VTK_POLY_DATA:
        this->ExtractPolyData(
          (vtkPolyData*)(tmpDS), output,
          updatePiece, updateNumPieces, updateGhostLevel, i);
        break;
      case VTK_RECTILINEAR_GRID:
        this->ExtractRectilinearGrid(
          (vtkRectilinearGrid*)(tmpDS), output,
          updatePiece, updateNumPieces, updateGhostLevel, i);
        break;
      case VTK_STRUCTURED_GRID:
        this->ExtractStructuredGrid(
          (vtkStructuredGrid*)(tmpDS), output,
          updatePiece, updateNumPieces, updateGhostLevel, i);
        break;
      case VTK_UNSTRUCTURED_GRID:
        this->ExtractUnstructuredGrid(
          (vtkUnstructuredGrid*)(tmpDS), output,
          updatePiece, updateNumPieces, updateGhostLevel, i);
        break;
      default:
        vtkErrorMacro("Cannot extract data of type "
                      << tmpDS->GetClassName());
        break;
      }
    }

  return 1;
}

void vtkMultiGroupDataExtractPiece::ExtractImageData(
  vtkImageData *imageData, vtkMultiGroupDataSet *output,
  int piece, int numberOfPieces, int ghostLevel, unsigned int group)
{
  int ext[6];

  // not using vtkImageClip because it doesn't work properly if you set
  // the update extent on the filter's information object manually
  imageData->GetExtent(ext);

  vtkImageData *extractOutput = vtkImageData::New();
  extractOutput->SetScalarType(imageData->GetScalarType());
  extractOutput->SetNumberOfScalarComponents(
    imageData->GetNumberOfScalarComponents());
  extractOutput->SetExtent(ext);
  extractOutput->SetWholeExtent(ext);
  extractOutput->GetPointData()->CopyAllocate(imageData->GetPointData(),
                                              imageData->GetNumberOfPoints());
  extractOutput->GetCellData()->CopyAllocate(imageData->GetCellData(),
                                             imageData->GetNumberOfCells());
  extractOutput->SetOrigin(imageData->GetOrigin());
  extractOutput->SetSpacing(imageData->GetSpacing());

  vtkExtentTranslator *translate = vtkExtentTranslator::New();
  translate->SetPiece(piece);
  translate->SetNumberOfPieces(numberOfPieces);
  translate->SetGhostLevel(ghostLevel);
  translate->SetWholeExtent(ext);
  translate->PieceToExtent();
  translate->GetExtent(ext);

  extractOutput->SetUpdateExtent(ext);
  extractOutput->GetPointData()->PassData(imageData->GetPointData());
  extractOutput->GetCellData()->PassData(imageData->GetCellData());
  extractOutput->Crop();

  output->SetDataSet(group, piece, extractOutput);
  translate->Delete();
  extractOutput->Delete();
}

void vtkMultiGroupDataExtractPiece::ExtractPolyData(
  vtkPolyData *polyData, vtkMultiGroupDataSet *output,
  int piece, int numberOfPieces, int ghostLevel, unsigned int group)
{
  vtkStreamingDemandDrivenPipeline *extractExecutive;
  vtkInformation *extractInfo;
  
  vtkExtractPolyDataPiece *extractPD = vtkExtractPolyDataPiece::New();
  extractPD->SetInput(polyData);
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
  output->SetDataSet(group, piece, extractOutput);
  extractPD->Delete();
  extractOutput->Delete();
}

void vtkMultiGroupDataExtractPiece::ExtractRectilinearGrid(
  vtkRectilinearGrid *rGrid, vtkMultiGroupDataSet *output,
  int piece, int numberOfPieces, int ghostLevel, unsigned int group)
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

  extractRG->SetInput(rGrid);
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
  output->SetDataSet(group, piece, extractOutput);
  extractRG->Delete();
  translate->Delete();
  extractOutput->Delete();
}

void vtkMultiGroupDataExtractPiece::ExtractStructuredGrid(
  vtkStructuredGrid *sGrid, vtkMultiGroupDataSet *output,
  int piece, int numberOfPieces, int ghostLevel, unsigned int group)
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

  extractSG->SetInput(sGrid);
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
  output->SetDataSet(group, piece, extractOutput);
  extractSG->Delete();
  translate->Delete();
  extractOutput->Delete();
}

void vtkMultiGroupDataExtractPiece::ExtractUnstructuredGrid(
  vtkUnstructuredGrid *uGrid, vtkMultiGroupDataSet *output,
  int piece, int numberOfPieces, int ghostLevel, unsigned int group)
{
  vtkStreamingDemandDrivenPipeline *extractExecutive;
  vtkInformation *extractInfo;
  
  vtkExtractUnstructuredGridPiece *extractUG =
    vtkExtractUnstructuredGridPiece::New();
  extractUG->SetInput(uGrid);
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
  output->SetDataSet(group, piece, extractOutput);
  extractUG->Delete();
  extractOutput->Delete();
}

void vtkMultiGroupDataExtractPiece::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
