// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkXMLPUnstructuredGridReader.h"

#include "vtkAbstractArray.h"
#include "vtkCellArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLUnstructuredGridReader.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkXMLPUnstructuredGridReader);

//------------------------------------------------------------------------------
vtkXMLPUnstructuredGridReader::vtkXMLPUnstructuredGridReader() = default;

//------------------------------------------------------------------------------
vtkXMLPUnstructuredGridReader::~vtkXMLPUnstructuredGridReader() = default;

//------------------------------------------------------------------------------
void vtkXMLPUnstructuredGridReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkUnstructuredGrid* vtkXMLPUnstructuredGridReader::GetOutput()
{
  return this->GetOutput(0);
}

//------------------------------------------------------------------------------
vtkUnstructuredGrid* vtkXMLPUnstructuredGridReader::GetOutput(int idx)
{
  return vtkUnstructuredGrid::SafeDownCast(this->GetOutputDataObject(idx));
}

//------------------------------------------------------------------------------
const char* vtkXMLPUnstructuredGridReader::GetDataSetName()
{
  return "PUnstructuredGrid";
}

//------------------------------------------------------------------------------
void vtkXMLPUnstructuredGridReader::GetOutputUpdateExtent(
  int& piece, int& numberOfPieces, int& ghostLevel)
{
  vtkInformation* outInfo = this->GetCurrentOutputInformation();
  piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  numberOfPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  ghostLevel = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());
}

//------------------------------------------------------------------------------
void vtkXMLPUnstructuredGridReader::SetupOutputTotals()
{
  this->Superclass::SetupOutputTotals();
  // Find the total size of the output.
  this->TotalNumberOfCells = 0;
  for (int i = this->StartPiece; i < this->EndPiece; ++i)
  {
    if (this->PieceReaders[i])
    {
      this->TotalNumberOfCells += this->PieceReaders[i]->GetNumberOfCells();
    }
  }

  // Data reading will start at the beginning of the output.
  this->StartCell = 0;
}

//------------------------------------------------------------------------------
void vtkXMLPUnstructuredGridReader::SetupOutputData()
{
  this->Superclass::SetupOutputData();

  vtkUnstructuredGrid* output = vtkUnstructuredGrid::SafeDownCast(this->GetCurrentOutput());

  // Setup the output's cell arrays.
  vtkUnsignedCharArray* cellTypes = vtkUnsignedCharArray::New();
  cellTypes->SetNumberOfTuples(this->GetNumberOfCells());
  vtkCellArray* outCells = vtkCellArray::New();

  output->SetCells(cellTypes, outCells);

  outCells->Delete();
  cellTypes->Delete();
}

//------------------------------------------------------------------------------
void vtkXMLPUnstructuredGridReader::SetupNextPiece()
{
  this->Superclass::SetupNextPiece();
  if (this->PieceReaders[this->Piece])
  {
    this->StartCell += this->PieceReaders[this->Piece]->GetNumberOfCells();
  }
}

//------------------------------------------------------------------------------
int vtkXMLPUnstructuredGridReader::ReadPieceData()
{
  if (!this->Superclass::ReadPieceData())
  {
    return 0;
  }

  vtkPointSet* ips = this->GetPieceInputAsPointSet(this->Piece);
  vtkUnstructuredGrid* input = static_cast<vtkUnstructuredGrid*>(ips);
  vtkUnstructuredGrid* output = vtkUnstructuredGrid::SafeDownCast(this->GetCurrentOutput());

  // Copy the Cells.
  this->CopyCellArray(this->TotalNumberOfCells, input->GetCells(), output->GetCells());

  // Copy Faces and FaceLocations with offset adjustment if they exist
  if (vtkCellArray* inputFaces = input->GetPolyhedronFaces())
  {
    vtkCellArray* inputFaceLocations = input->GetPolyhedronFaceLocations();
    vtkCellArray* outputFaces = output->GetPolyhedronFaces();
    if (!outputFaces)
    {
      output->InitializeFacesRepresentation(0);
      outputFaces = output->GetPolyhedronFaces();
    }
    vtkCellArray* outputFaceLocations = output->GetPolyhedronFaceLocations();
    const vtkIdType numFaceLocs = inputFaceLocations->GetNumberOfCells();

    vtkNew<vtkIdList> faceIds;
    const vtkIdType* faces;
    vtkNew<vtkIdList> ptsIds;
    const vtkIdType* nodes;
    for (vtkIdType i = 0; i < numFaceLocs; ++i)
    {
      vtkIdType size = inputFaceLocations->GetCellSize(i);
      if (size < 1) // the face offsets array contains -1 for regular cells
      {
        outputFaceLocations->InsertNextCell(0);
        continue;
      }
      vtkIdType numFaces;
      inputFaceLocations->GetCellAtId(i, numFaces, faces, faceIds);
      outputFaceLocations->InsertNextCell(numFaces);
      for (vtkIdType f = 0; f < numFaces; f++)
      {
        outputFaceLocations->InsertCellPoint(outputFaces->GetNumberOfCells() + f);
      }
      for (vtkIdType f = 0; f < numFaces; f++)
      {
        vtkIdType numPoints;
        inputFaces->GetCellAtId(faces[f], numPoints, nodes, ptsIds);
        outputFaces->InsertNextCell(numPoints);
        for (vtkIdType p = 0; p < numPoints; p++)
        {
          // only the point ids get the offset
          outputFaces->InsertCellPoint(nodes[p] + this->StartPoint);
        }
      }
    }
  }

  // Copy the corresponding cell types.
  vtkUnsignedCharArray* inTypes = input->GetCellTypesArray();
  vtkUnsignedCharArray* outTypes = output->GetCellTypesArray();
  vtkIdType components = outTypes->GetNumberOfComponents();
  memcpy(outTypes->GetVoidPointer(this->StartCell * components), inTypes->GetVoidPointer(0),
    inTypes->GetNumberOfTuples() * components * inTypes->GetDataTypeSize());

  return 1;
}

//------------------------------------------------------------------------------
void vtkXMLPUnstructuredGridReader::CopyArrayForCells(
  vtkAbstractArray* inArray, vtkAbstractArray* outArray)
{
  if (!this->PieceReaders[this->Piece])
  {
    return;
  }
  if (!inArray || !outArray)
  {
    return;
  }

  vtkIdType numCells = this->PieceReaders[this->Piece]->GetNumberOfCells();
  vtkIdType components = outArray->GetNumberOfComponents();
  vtkIdType tupleSize = inArray->GetDataTypeSize() * components;
  if (auto outStringArray = vtkArrayDownCast<vtkStringArray>(outArray))
  {
    outStringArray->InsertTuples(this->StartCell, numCells, 0, inArray);
  }
  else
  {
    memcpy(outArray->GetVoidPointer(this->StartCell * components), inArray->GetVoidPointer(0),
      numCells * tupleSize);
  }
}

//------------------------------------------------------------------------------
vtkXMLDataReader* vtkXMLPUnstructuredGridReader::CreatePieceReader()
{
  return vtkXMLUnstructuredGridReader::New();
}

//------------------------------------------------------------------------------
int vtkXMLPUnstructuredGridReader::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkUnstructuredGrid");
  return 1;
}

//------------------------------------------------------------------------------
void vtkXMLPUnstructuredGridReader::SqueezeOutputArrays(vtkDataObject* output)
{
  vtkUnstructuredGrid* grid = vtkUnstructuredGrid::SafeDownCast(output);
  if (vtkCellArray* outputFaces = grid->GetPolyhedronFaces())
  {
    outputFaces->Squeeze();
  }
  if (vtkCellArray* outputFaceLocations = grid->GetPolyhedronFaceLocations())
  {
    outputFaceLocations->Squeeze();
  }
}
VTK_ABI_NAMESPACE_END
