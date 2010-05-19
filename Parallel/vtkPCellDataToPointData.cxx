/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPCellDataToPointData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPCellDataToPointData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkPCellDataToPointData);

//----------------------------------------------------------------------------
vtkPCellDataToPointData::vtkPCellDataToPointData()
{
  this->PieceInvariant = 1;
}

//----------------------------------------------------------------------------
int vtkPCellDataToPointData::RequestData(
  vtkInformation* request,
  vtkInformationVector** inputVector ,
  vtkInformationVector* outputVector)
{
  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkDataSet *output = vtkDataSet::SafeDownCast(
    info->Get(vtkDataObject::DATA_OBJECT()));
  if (!output) {return 0;}

  if ( !this->Superclass::RequestData(request, inputVector, outputVector) )
    {
    return 0;
    }

  // Remove the extra (now ivalid) ghost cells.
  // This is only necessary fro unstructured data.  
  if (this->PieceInvariant)
    {
    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    int ghostLevel = outInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());
    vtkPolyData *pd = vtkPolyData::SafeDownCast(output);
    vtkUnstructuredGrid *ug = vtkUnstructuredGrid::SafeDownCast(output);
    if (pd)
      {
      pd->RemoveGhostCells(ghostLevel+1);
      }
    if (ug)
      {
      ug->RemoveGhostCells(ghostLevel+1);
      }
    }
  return 1;
}

//--------------------------------------------------------------------------
int vtkPCellDataToPointData::RequestUpdateExtent(
  vtkInformation*,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  if (this->PieceInvariant == 0)
    { 
    // I believe the default input update extent 
    // is set to the input update extent.
    return 1;
    }

  vtkInformation* opInfo = this->GetOutputPortInformation(0);
  int extentType = opInfo->Get(vtkDataObject::DATA_EXTENT_TYPE());
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  
  int piece, numPieces, ghostLevel;
  int* wholeExt;
  int* upExt;
  int ext[6];

  int isInputPiecesExtent = 1;
  if (extentType == VTK_3D_EXTENT && 
      inInfo->Has(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()))
    {
    isInputPiecesExtent = 0;
    }
  if (isInputPiecesExtent)
    {
    piece = outInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
    numPieces = outInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
    ghostLevel = outInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()) + 1;
    inInfo->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), piece);
    inInfo->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), numPieces);
    inInfo->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 
      ghostLevel);
    }
  else
    {
    wholeExt = inInfo->Get(
      vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
    upExt = outInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
    memcpy(ext, upExt, 6*sizeof(int));
    for (int i = 0; i < 3; ++i)
      {
      --ext[i*2];
      if (ext[i*2] < wholeExt[i*2])
        {
        ext[i*2] = wholeExt[i*2];
        }
      ++ext[i*2+1];
      if (ext[i*2+1] > wholeExt[i*2+1])
        {
        ext[i*2+1] = wholeExt[i*2+1];
        }
      }
    inInfo->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), ext, 6);
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkPCellDataToPointData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "PieceInvariant: "
     << this->PieceInvariant << "\n";
}
