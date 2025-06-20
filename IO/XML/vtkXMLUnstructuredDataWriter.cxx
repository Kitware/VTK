// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkXMLUnstructuredDataWriter.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCellIterator.h"
#include "vtkDataArray.h"
#include "vtkDataCompressor.h"
#include "vtkDataSetAttributes.h"
#include "vtkErrorCode.h"
#include "vtkGenericCell.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkPolyhedron.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUpdateCellsV8toV9.h"
#define vtkXMLOffsetsManager_DoNotInclude
#include "vtkXMLOffsetsManager.h"
#undef vtkXMLOffsetsManager_DoNotInclude

#include <cassert>
#include <utility>

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkXMLUnstructuredDataWriter::vtkXMLUnstructuredDataWriter()
{
  this->NumberOfPieces = 1;
  this->WritePiece = -1;
  this->GhostLevel = 0;

  this->CurrentPiece = 0;
  this->FieldDataOM->Allocate(0);
  this->PointsOM = new OffsetsManagerGroup;
  this->PointDataOM = new OffsetsManagerArray;
  this->CellDataOM = new OffsetsManagerArray;
}

//------------------------------------------------------------------------------
vtkXMLUnstructuredDataWriter::~vtkXMLUnstructuredDataWriter()
{
  delete this->PointsOM;
  delete this->PointDataOM;
  delete this->CellDataOM;
}

//------------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NumberOfPieces: " << this->NumberOfPieces << "\n";
  os << indent << "WritePiece: " << this->WritePiece << "\n";
  os << indent << "GhostLevel: " << this->GhostLevel << "\n";
}

//------------------------------------------------------------------------------
vtkPointSet* vtkXMLUnstructuredDataWriter::GetPointSetInput()
{
  return static_cast<vtkPointSet*>(this->Superclass::GetInput());
}

//------------------------------------------------------------------------------
inline bool vtkNeedsNewFileVersionPolyhedronV2(vtkUnsignedCharArray* distinctCellTypes)
{
  vtkIdType nCellTypes = distinctCellTypes->GetNumberOfValues();
  for (vtkIdType i = 0; i < nCellTypes; ++i)
  {
    unsigned char type = distinctCellTypes->GetValue(i);
    if (type == VTK_POLYHEDRON)
    {
      return true;
    }
  }
  return false;
}

//------------------------------------------------------------------------------
vtkTypeBool vtkXMLUnstructuredDataWriter::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{

  if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    if ((this->WritePiece < 0) || (this->WritePiece >= this->NumberOfPieces))
    {
      this->SetInputUpdateExtent(this->CurrentPiece, this->NumberOfPieces, this->GhostLevel);
    }
    else
    {
      this->SetInputUpdateExtent(this->WritePiece, this->NumberOfPieces, this->GhostLevel);
    }
    return 1;
  }

  // generate the data
  else if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    this->SetErrorCode(vtkErrorCode::NoError);

    if (!this->Stream && !this->FileName && !this->WriteToOutputString)
    {
      this->SetErrorCode(vtkErrorCode::NoFileNameError);
      vtkErrorMacro("The FileName or Stream must be set first or "
                    "the output must be written to a string.");
      return 0;
    }

    int numPieces = this->NumberOfPieces;

    if (this->WritePiece >= 0)
    {
      this->CurrentPiece = this->WritePiece;
    }
    else
    {
      float wholeProgressRange[2] = { 0, 1 };
      this->SetProgressRange(wholeProgressRange, this->CurrentPiece, this->NumberOfPieces);
    }

    int result = 1;
    if ((this->CurrentPiece == 0 && this->CurrentTimeIndex == 0) || this->WritePiece >= 0)
    {
      // We are just starting to write.  Do not call
      // UpdateProgressDiscrete because we want a 0 progress callback the
      // first time.
      this->UpdateProgress(0);

      // Initialize progress range to entire 0..1 range.
      if (this->WritePiece >= 0)
      {
        float wholeProgressRange[2] = { 0, 1 };
        this->SetProgressRange(wholeProgressRange, 0, 1);
      }

      if (!this->OpenStream())
      {
        this->NumberOfPieces = numPieces;
        return 0;
      }

      if (vtkDataSet* dataSet = this->GetDataSetInput())
      {
        if (dataSet->GetPointGhostArray() != nullptr && dataSet->GetCellGhostArray() != nullptr)
        {
          // use the current version for the file.
          this->UsePreviousVersion = false;
        }
        else
        {
          vtkNew<vtkUnsignedCharArray> cellTypesArray;
          if (auto ug = vtkUnstructuredGrid::SafeDownCast(dataSet))
          {
            cellTypesArray->ShallowCopy(ug->GetDistinctCellTypesArray());
          }
          else
          {
            vtkNew<vtkCellTypes> cellTypes;
            dataSet->GetCellTypes(cellTypes);
            cellTypesArray->ShallowCopy(cellTypes->GetCellTypesArray());
          }
          if (vtkNeedsNewFileVersionV8toV9(cellTypesArray))
          {
            this->UsePreviousVersion = false;
          }
          if (vtkNeedsNewFileVersionPolyhedronV2(cellTypesArray))
          {
            this->UsePreviousVersion = false;
          }
        }
      }

      // Write the file.
      if (!this->StartFile())
      {
        this->NumberOfPieces = numPieces;
        return 0;
      }

      if (!this->WriteHeader())
      {
        this->NumberOfPieces = numPieces;
        return 0;
      }

      this->CurrentTimeIndex = 0;
      if (this->DataMode == vtkXMLWriter::Appended && this->FieldDataOM->GetNumberOfElements())
      {
        vtkNew<vtkFieldData> fieldDataCopy;
        this->UpdateFieldData(fieldDataCopy);

        // Write the field data arrays.
        this->WriteFieldDataAppendedData(fieldDataCopy, this->CurrentTimeIndex, this->FieldDataOM);
        if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
        {
          this->DeletePositionArrays();
          return 0;
        }
      }
    }

    if (!(this->UserContinueExecuting == 0)) // if user ask to stop do not try to write a piece
    {
      result = this->WriteAPiece();
    }

    if ((this->WritePiece < 0) || (this->WritePiece >= this->NumberOfPieces))
    {
      // Tell the pipeline to start looping.
      if (this->CurrentPiece == 0)
      {
        request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
      }
      this->CurrentPiece++;
    }

    if (this->CurrentPiece == this->NumberOfPieces || this->WritePiece >= 0)
    {
      request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
      this->CurrentPiece = 0;
      // We are done writing all the pieces, lets loop over time now:
      this->CurrentTimeIndex++;

      if (this->UserContinueExecuting != 1)
      {
        if (!this->WriteFooter())
        {
          this->NumberOfPieces = numPieces;
          return 0;
        }

        if (!this->EndFile())
        {
          this->NumberOfPieces = numPieces;
          return 0;
        }

        this->CloseStream();
        this->CurrentTimeIndex = 0; // Reset
      }
    }
    this->NumberOfPieces = numPieces;

    // We have finished writing (at least this piece)
    this->SetProgressPartial(1);
    return result;
  }
  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//------------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::AllocatePositionArrays()
{
  this->NumberOfPointsPositions = new vtkTypeInt64[this->NumberOfPieces];

  this->PointsOM->Allocate(this->NumberOfPieces, this->NumberOfTimeSteps);
  this->PointDataOM->Allocate(this->NumberOfPieces);
  this->CellDataOM->Allocate(this->NumberOfPieces);
}

//------------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::DeletePositionArrays()
{
  delete[] this->NumberOfPointsPositions;
  this->NumberOfPointsPositions = nullptr;
}

//------------------------------------------------------------------------------
int vtkXMLUnstructuredDataWriter::WriteHeader()
{
  vtkIndent indent = vtkIndent().GetNextIndent();

  ostream& os = *(this->Stream);

  if (!this->WritePrimaryElement(os, indent))
  {
    return 0;
  }

  this->WriteFieldData(indent.GetNextIndent());

  if (this->DataMode == vtkXMLWriter::Appended)
  {
    vtkIndent nextIndent = indent.GetNextIndent();

    this->AllocatePositionArrays();

    if ((this->WritePiece < 0) || (this->WritePiece >= this->NumberOfPieces))
    {
      // Loop over each piece and write its structure.
      int i;
      for (i = 0; i < this->NumberOfPieces; ++i)
      {
        // Open the piece's element.
        os << nextIndent << "<Piece";
        this->WriteAppendedPieceAttributes(i);
        if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
        {
          this->DeletePositionArrays();
          return 0;
        }
        os << ">\n";

        this->WriteAppendedPiece(i, nextIndent.GetNextIndent());
        if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
        {
          this->DeletePositionArrays();
          return 0;
        }

        // Close the piece's element.
        os << nextIndent << "</Piece>\n";
      }
    }
    else
    {
      // Write just the requested piece.
      // Open the piece's element.
      os << nextIndent << "<Piece";
      this->WriteAppendedPieceAttributes(this->WritePiece);
      if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
      {
        this->DeletePositionArrays();
        return 0;
      }
      os << ">\n";

      this->WriteAppendedPiece(this->WritePiece, nextIndent.GetNextIndent());
      if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
      {
        this->DeletePositionArrays();
        return 0;
      }

      // Close the piece's element.
      os << nextIndent << "</Piece>\n";
    }

    // Close the primary element.
    os << indent << "</" << this->GetDataSetName() << ">\n";
    os.flush();
    if (os.fail())
    {
      this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
      this->DeletePositionArrays();
      return 0;
    }

    this->StartAppendedData();
    if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
    {
      this->DeletePositionArrays();
      return 0;
    }
  }

  return 1;
}

void CreatePolyFace(
  vtkCellIterator* cellIter, vtkCellArray* faceArray, vtkCellArray* polyhedronArray)
{
  vtkNew<vtkGenericCell> cell;

  faceArray->Reset();
  polyhedronArray->Reset();

  vtkIdType faceId(0);
  for (cellIter->InitTraversal(); !cellIter->IsDoneWithTraversal(); cellIter->GoToNextCell())
  {
    vtkIdType ct = cellIter->GetCellType();
    if (ct != VTK_POLYHEDRON)
    {
      polyhedronArray->InsertNextCell(0);
      continue;
    }
    cellIter->GetCell(cell.GetPointer());
    vtkCell* theCell = cell->GetRepresentativeCell();
    vtkPolyhedron* poly = vtkPolyhedron::SafeDownCast(theCell);
    if (!poly || !poly->GetNumberOfFaces())
    {
      polyhedronArray->InsertNextCell(0);
      continue;
    }

    vtkCellArray* faces = poly->GetCellFaces();
    int nfaces = static_cast<int>(faces->GetNumberOfCells());
    polyhedronArray->InsertNextCell(nfaces);
    for (int faceNum = 0; faceNum < nfaces; ++faceNum)
    {
      polyhedronArray->InsertCellPoint(faceId++);
    }
    faceArray->Append(faces, 0);
  }
}

//------------------------------------------------------------------------------
int vtkXMLUnstructuredDataWriter::WriteAPiece()
{
  vtkIndent indent = vtkIndent().GetNextIndent();

  int result = 1;

  if (this->DataMode == vtkXMLWriter::Appended)
  {
    this->WriteAppendedPieceData(this->CurrentPiece);
  }
  else
  {
    result = this->WriteInlineMode(indent);
  }

  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    this->DeletePositionArrays();
    result = 0;
  }
  return result;
}

//------------------------------------------------------------------------------
int vtkXMLUnstructuredDataWriter::WriteFooter()
{
  vtkIndent indent = vtkIndent().GetNextIndent();

  ostream& os = *(this->Stream);

  if (this->DataMode == vtkXMLWriter::Appended)
  {
    this->DeletePositionArrays();
    this->EndAppendedData();
  }
  else
  {
    // Close the primary element.
    os << indent << "</" << this->GetDataSetName() << ">\n";
    os.flush();
    if (os.fail())
    {
      return 0;
    }
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkXMLUnstructuredDataWriter::WriteInlineMode(vtkIndent indent)
{
  ostream& os = *(this->Stream);
  vtkIndent nextIndent = indent.GetNextIndent();

  // Open the piece's element.
  os << nextIndent << "<Piece";
  this->WriteInlinePieceAttributes();
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    return 0;
  }
  os << ">\n";

  this->WriteInlinePiece(nextIndent.GetNextIndent());
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    return 0;
  }

  // Close the piece's element.
  os << nextIndent << "</Piece>\n";

  return 1;
}

//------------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::WriteInlinePieceAttributes()
{
  vtkPointSet* input = this->GetPointSetInput();
  this->WriteScalarAttribute("NumberOfPoints", input->GetNumberOfPoints());
}

//------------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::WriteInlinePiece(vtkIndent indent)
{
  vtkPointSet* input = this->GetPointSetInput();

  // Split progress among point data, cell data, and point arrays.
  float progressRange[2] = { 0, 0 };
  this->GetProgressRange(progressRange);
  float fractions[4];
  this->CalculateDataFractions(fractions);

  // Set the range of progress for the point data arrays.
  this->SetProgressRange(progressRange, 0, fractions);

  // Write the point data arrays.
  this->WritePointDataInline(input->GetPointData(), indent);
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  // Set the range of progress for the cell data arrays.
  this->SetProgressRange(progressRange, 1, fractions);

  // Write the cell data arrays.
  this->WriteCellDataInline(input->GetCellData(), indent);
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  // Set the range of progress for the point specification array.
  this->SetProgressRange(progressRange, 2, fractions);

  // Write the point specification array.
  this->WritePointsInline(input->GetPoints(), indent);
}

//------------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::WriteAppendedPieceAttributes(int index)
{
  this->NumberOfPointsPositions[index] = this->ReserveAttributeSpace("NumberOfPoints");
}

//------------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::WriteAppendedPiece(int index, vtkIndent indent)
{
  vtkPointSet* input = this->GetPointSetInput();

  this->WritePointDataAppended(input->GetPointData(), indent, &this->PointDataOM->GetPiece(index));
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  this->WriteCellDataAppended(input->GetCellData(), indent, &this->CellDataOM->GetPiece(index));
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  this->WritePointsAppended(input->GetPoints(), indent, &this->PointsOM->GetPiece(index));
}

//------------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::WriteAppendedPieceData(int index)
{
  ostream& os = *(this->Stream);
  vtkPointSet* input = this->GetPointSetInput();

  std::streampos returnPosition = os.tellp();
  os.seekp(std::streampos(this->NumberOfPointsPositions[index]));
  vtkPoints* points = input->GetPoints();
  this->WriteScalarAttribute("NumberOfPoints", (points ? points->GetNumberOfPoints() : 0));
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }
  os.seekp(returnPosition);

  // Split progress among point data, cell data, and point arrays.
  float progressRange[2] = { 0, 0 };
  this->GetProgressRange(progressRange);
  float fractions[4];
  this->CalculateDataFractions(fractions);

  // Set the range of progress for the point data arrays.
  this->SetProgressRange(progressRange, 0, fractions);

  // Write the point data arrays.
  this->WritePointDataAppendedData(
    input->GetPointData(), this->CurrentTimeIndex, &this->PointDataOM->GetPiece(index));
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  // Set the range of progress for the cell data arrays.
  this->SetProgressRange(progressRange, 1, fractions);

  // Write the cell data arrays.
  this->WriteCellDataAppendedData(
    input->GetCellData(), this->CurrentTimeIndex, &this->CellDataOM->GetPiece(index));
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  // Set the range of progress for the point specification array.
  this->SetProgressRange(progressRange, 2, fractions);

  // Write the point specification array.
  // Since we are writing the point let save the Modified Time of vtkPoints:
  this->WritePointsAppendedData(
    input->GetPoints(), this->CurrentTimeIndex, &this->PointsOM->GetPiece(index));
}

//------------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::WriteCellsInline(const char* name, vtkCellIterator* cellIter,
  vtkIdType numCells, vtkIdType cellSizeEstimate, vtkIndent indent)
{
  this->ConvertCells(cellIter, numCells, cellSizeEstimate);

  vtkNew<vtkUnsignedCharArray> types;
  types->Allocate(numCells);
  vtkIdType nPolyhedra(0);
  for (cellIter->InitTraversal(); !cellIter->IsDoneWithTraversal(); cellIter->GoToNextCell())
  {
    vtkIdType ct = cellIter->GetCellType();
    if (ct == VTK_POLYHEDRON)
    {
      ++nPolyhedra;
    }
    types->InsertNextValue(static_cast<unsigned char>(ct));
  }

  vtkNew<vtkCellArray> faces, polyhedron;
  if (nPolyhedra > 0)
  {
    CreatePolyFace(cellIter, faces.GetPointer(), polyhedron.GetPointer());
    this->ConvertPolyFaces(faces.GetPointer(), polyhedron.GetPointer());
  }

  this->WriteCellsInlineWorker(name, types.GetPointer(), indent);
}

//------------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::WriteCellsInline(
  const char* name, vtkCellArray* cells, vtkDataArray* types, vtkIndent indent)
{
  this->WritePolyCellsInline(name, cells, types, nullptr, nullptr, indent);
}

//------------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::WritePolyCellsInline(const char* name, vtkCellArray* cells,
  vtkDataArray* types, vtkCellArray* faces, vtkCellArray* faceOffsets, vtkIndent indent)
{
  if (cells)
  {
    this->ConvertCells(cells);
  }
  this->ConvertPolyFaces(faces, faceOffsets);

  this->WriteCellsInlineWorker(name, types, indent);
}

//------------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::WriteCellsInlineWorker(
  const char* name, vtkDataArray* types, vtkIndent indent)
{
  ostream& os = *(this->Stream);
  os << indent << "<" << name << ">\n";

  // Split progress by cell connectivity, offset, and type arrays.
  float progressRange[2] = { 0, 0 };
  this->GetProgressRange(progressRange);
  float fractions[8];
  this->CalculateCellFractions(fractions, types ? types->GetNumberOfTuples() : 0);

  // Set the range of progress for the connectivity array.
  this->SetProgressRange(progressRange, 0, fractions);

  // Write the connectivity array.
  this->WriteArrayInline(this->CellPoints, indent.GetNextIndent());
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  // Set the range of progress for the offsets array.
  this->SetProgressRange(progressRange, 1, fractions);

  // Write the offsets array.
  this->WriteArrayInline(this->CellOffsets, indent.GetNextIndent());
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  if (types)
  {
    // Set the range of progress for the types array.
    this->SetProgressRange(progressRange, 2, fractions);

    // Write the types array.
    this->WriteArrayInline(types, indent.GetNextIndent(), "types");
    if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
    {
      return;
    }
  }

  if (this->FaceConnectivity && this->FaceConnectivity->GetNumberOfTuples())
  {
    // Set the range of progress for the faces array.
    this->SetProgressRange(progressRange, 3, fractions);

    // Write the face connectivity array.
    this->WriteArrayInline(this->FaceConnectivity, indent.GetNextIndent(), "face_connectivity");
    if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
    {
      return;
    }

    this->SetProgressRange(progressRange, 4, fractions);
    // Write the face connectivity offsets array.
    this->WriteArrayInline(this->FaceOffsets, indent.GetNextIndent(), "face_offsets");
    if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
    {
      return;
    }
  }

  if (this->PolyhedronToFaces && this->PolyhedronToFaces->GetNumberOfTuples())
  {
    // Set the range of progress for the polyhedron_to_faces array.
    this->SetProgressRange(progressRange, 5, fractions);

    // Write the polyhedron to faces array.
    this->WriteArrayInline(this->PolyhedronToFaces, indent.GetNextIndent(), "polyhedron_to_faces");
    if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
    {
      return;
    }

    this->SetProgressRange(progressRange, 6, fractions);
    // Write the polyhedron offsets array.
    this->WriteArrayInline(this->PolyhedronOffsets, indent.GetNextIndent(), "polyhedron_offsets");
    if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
    {
      return;
    }
  }

  os << indent << "</" << name << ">\n";
  os.flush();
  if (os.fail())
  {
    this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
  }
}

//------------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::WritePolyCellsAppended(const char* name, vtkDataArray* types,
  vtkCellArray* faces, vtkCellArray* faceOffsets, vtkIndent indent,
  OffsetsManagerGroup* cellsManager)
{
  this->ConvertPolyFaces(faces, faceOffsets);
  ostream& os = *(this->Stream);
  os << indent << "<" << name << ">\n";

  // Helper for the 'for' loop
  vtkDataArray* allcells[7];
  allcells[0] = this->CellPoints;
  allcells[1] = this->CellOffsets;
  allcells[2] = types;
  allcells[3] = this->FaceConnectivity;
  allcells[4] = this->FaceOffsets;
  allcells[5] = this->PolyhedronToFaces;
  allcells[6] = this->PolyhedronOffsets;
  const char* names[] = { nullptr, nullptr, "types", "face_connectivity", "face_offsets",
    "polyhedron_to_faces", "polyhedron_offsets" };

  for (int t = 0; t < this->NumberOfTimeSteps; t++)
  {
    for (int i = 0; i < 7; i++)
    {
      if (allcells[i])
      {
        this->WriteArrayAppended(
          allcells[i], indent.GetNextIndent(), cellsManager->GetElement(i), names[i], 0, t);
        if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
        {
          return;
        }
      }
    }
  }
  os << indent << "</" << name << ">\n";
  os.flush();
  if (os.fail())
  {
    this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
    return;
  }
}

//------------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::WriteCellsAppended(
  const char* name, vtkDataArray* types, vtkIndent indent, OffsetsManagerGroup* cellsManager)
{
  this->WritePolyCellsAppended(name, types, nullptr, nullptr, indent, cellsManager);
}

//------------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::WriteCellsAppended(const char* name, vtkCellIterator* cellIter,
  vtkIdType numCells, vtkIndent indent, OffsetsManagerGroup* cellsManager)
{
  this->ConvertCells(cellIter, numCells, 3);

  vtkNew<vtkUnsignedCharArray> types;
  types->Allocate(numCells);
  vtkIdType nPolyhedra(0);
  for (cellIter->InitTraversal(); !cellIter->IsDoneWithTraversal(); cellIter->GoToNextCell())
  {
    vtkIdType ct = cellIter->GetCellType();
    if (ct == VTK_POLYHEDRON)
    {
      ++nPolyhedra;
    }
    types->InsertNextValue(static_cast<unsigned char>(ct));
  }
  if (nPolyhedra > 0)
  {
    vtkNew<vtkCellArray> faces, polyhedron;
    CreatePolyFace(cellIter, faces.GetPointer(), polyhedron.GetPointer());
    this->WritePolyCellsAppended(
      name, types.GetPointer(), faces.GetPointer(), polyhedron.GetPointer(), indent, cellsManager);
  }
  else
  {
    this->WritePolyCellsAppended(name, types.GetPointer(), nullptr, nullptr, indent, cellsManager);
  }
}

//------------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::WriteCellsAppendedData(
  vtkCellArray* cells, vtkDataArray* types, int timestep, OffsetsManagerGroup* cellsManager)
{
  this->WritePolyCellsAppendedData(cells, types, nullptr, nullptr, timestep, cellsManager);
}

//------------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::WriteCellsAppendedData(vtkCellIterator* cellIter,
  vtkIdType numCells, vtkIdType cellSizeEstimate, int timestep, OffsetsManagerGroup* cellsManager)
{
  this->ConvertCells(cellIter, numCells, cellSizeEstimate);

  vtkNew<vtkUnsignedCharArray> types;
  types->Allocate(this->CellOffsets->GetNumberOfTuples() + 1);
  int nPolyhedra(0);
  for (cellIter->InitTraversal(); !cellIter->IsDoneWithTraversal(); cellIter->GoToNextCell())
  {
    vtkIdType ct = cellIter->GetCellType();
    if (ct == VTK_POLYHEDRON)
    {
      ++nPolyhedra;
    }
    types->InsertNextValue(static_cast<unsigned char>(ct));
  }

  vtkNew<vtkCellArray> faces, polyhedron;
  if (nPolyhedra > 0)
  {
    // even though it looks like we do this for the second time
    // the test points out that it is needed here.
    CreatePolyFace(cellIter, faces.GetPointer(), polyhedron.GetPointer());
    this->ConvertPolyFaces(faces.GetPointer(), polyhedron.GetPointer());
  }

  this->WriteCellsAppendedDataWorker(types.GetPointer(), timestep, cellsManager);
}

//------------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::WritePolyCellsAppendedData(vtkCellArray* cells,
  vtkDataArray* types, vtkCellArray* faces, vtkCellArray* faceOffsets, int timestep,
  OffsetsManagerGroup* cellsManager)
{
  if (cells)
  {
    this->ConvertCells(cells);
  }

  this->ConvertPolyFaces(faces, faceOffsets);
  this->WriteCellsAppendedDataWorker(types, timestep, cellsManager);
}

//------------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::WriteCellsAppendedDataWorker(
  vtkDataArray* types, int timestep, OffsetsManagerGroup* cellsManager)
{
  // Split progress by cell connectivity, offset, and type arrays.
  float progressRange[2] = { 0, 0 };
  this->GetProgressRange(progressRange);
  float fractions[8];
  this->CalculateCellFractions(fractions, types ? types->GetNumberOfTuples() : 0);

  // Helper for the 'for' loop
  vtkDataArray* allcells[7];
  allcells[0] = this->CellPoints;
  allcells[1] = this->CellOffsets;
  allcells[2] = types;
  allcells[3] = this->FaceConnectivity;
  allcells[4] = this->FaceOffsets;
  allcells[5] = this->PolyhedronToFaces;
  allcells[6] = this->PolyhedronOffsets;

  for (int i = 0; i < 7; i++)
  {
    if (allcells[i])
    {
      // Set the range of progress for the connectivity array.
      this->SetProgressRange(progressRange, i, fractions);

      vtkMTimeType mtime = allcells[i]->GetMTime();
      vtkMTimeType& cellsMTime = cellsManager->GetElement(i).GetLastMTime();
      // Only write cells if MTime has changed
      if (cellsMTime != mtime)
      {
        cellsMTime = mtime;
        // Write the connectivity array.
        this->WriteArrayAppendedData(allcells[i], cellsManager->GetElement(i).GetPosition(timestep),
          cellsManager->GetElement(i).GetOffsetValue(timestep));
        if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
        {
          return;
        }
      }
      else
      {
        // One timestep must have already been written or the
        // mtime would have changed and we would not be here.
        assert(timestep > 0);
        cellsManager->GetElement(i).GetOffsetValue(timestep) =
          cellsManager->GetElement(i).GetOffsetValue(timestep - 1);
        this->ForwardAppendedDataOffset(cellsManager->GetElement(i).GetPosition(timestep),
          cellsManager->GetElement(i).GetOffsetValue(timestep), "offset");
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::ConvertCells(
  vtkCellIterator* cellIter, vtkIdType numCells, vtkIdType cellSizeEstimate)
{
  vtkNew<vtkAOSDataArrayTemplate<vtkIdType>> conn;
  vtkNew<vtkAOSDataArrayTemplate<vtkIdType>> offsets;

  conn->SetName("connectivity");
  offsets->SetName("offsets");

  conn->Allocate(numCells * cellSizeEstimate);
  offsets->Allocate(numCells);

  // Offsets array skips the leading 0 and includes the connectivity array size
  // at the end.

  for (cellIter->InitTraversal(); !cellIter->IsDoneWithTraversal(); cellIter->GoToNextCell())
  {
    vtkIdType* begin = cellIter->GetPointIds()->GetPointer(0);
    vtkIdType* end = begin + cellIter->GetNumberOfPoints();
    while (begin != end)
    {
      conn->InsertNextValue(*begin++);
    }

    offsets->InsertNextValue(conn->GetNumberOfTuples());
  }

  conn->Squeeze();
  offsets->Squeeze();

  this->CellPoints = conn;
  this->CellOffsets = offsets;
}

namespace
{

struct ConvertCellsVisitor
{
  vtkSmartPointer<vtkDataArray> Offsets;
  vtkSmartPointer<vtkDataArray> Connectivity;

  template <typename CellStateT>
  void operator()(CellStateT& state)
  {
    using ArrayT = typename CellStateT::ArrayType;

    vtkNew<ArrayT> offsets;
    vtkNew<ArrayT> conn;

    // Shallow copy will let us change the name of the array to what the
    // writer expects without actually copying the array data:
    conn->ShallowCopy(state.GetConnectivity());
    conn->SetName("connectivity");
    this->Connectivity = std::move(conn);

    // The file format for offsets always skips the first offset, because
    // it's always zero. Use SetArray and GetPointer to create a view
    // of the offsets array that starts at index=1:
    auto* offsetsIn = state.GetOffsets();
    const vtkIdType numOffsets = offsetsIn->GetNumberOfValues();
    if (numOffsets >= 2)
    {
      offsets->SetArray(offsetsIn->GetPointer(1), numOffsets - 1, 1 /*save*/);
    }
    offsets->SetName("offsets");

    this->Offsets = std::move(offsets);
  }
};

} // end anon namespace

//------------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::ConvertCells(vtkCellArray* cells)
{
  ConvertCellsVisitor visitor;
  if (cells)
  {
    cells->Visit(visitor);
  }
  this->CellPoints = visitor.Connectivity;
  this->CellOffsets = visitor.Offsets;
}

//------------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::ConvertPolyFaces(vtkCellArray* faces, vtkCellArray* faceOffsets)
{
  ConvertCellsVisitor faceVisitor, polyhedronVisitor;
  if (faces && faces->GetNumberOfCells() > 0)
  {
    faces->Visit(faceVisitor);
    faceVisitor.Connectivity->SetName("face_connectivity");
    faceVisitor.Offsets->SetName("face_offsets");
  }
  this->FaceConnectivity = faceVisitor.Connectivity;
  this->FaceOffsets = faceVisitor.Offsets;
  if (faceOffsets && faceOffsets->GetNumberOfCells() > 0)
  {
    faceOffsets->Visit(polyhedronVisitor);
    polyhedronVisitor.Connectivity->SetName("polyhedron_to_faces");
    polyhedronVisitor.Offsets->SetName("polyhedron_offsets");
  }
  this->PolyhedronToFaces = polyhedronVisitor.Connectivity;
  this->PolyhedronOffsets = polyhedronVisitor.Offsets;
}

//------------------------------------------------------------------------------
vtkIdType vtkXMLUnstructuredDataWriter::GetNumberOfInputPoints()
{
  vtkPointSet* input = this->GetPointSetInput();
  vtkPoints* points = input->GetPoints();
  return points ? points->GetNumberOfPoints() : 0;
}

//------------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::CalculateDataFractions(float* fractions)
{
  // Calculate the fraction of point/cell data and point
  // specifications contributed by each component.
  vtkPointSet* input = this->GetPointSetInput();
  int pdArrays = input->GetPointData()->GetNumberOfArrays();
  int cdArrays = input->GetCellData()->GetNumberOfArrays();
  vtkIdType pdSize = pdArrays * this->GetNumberOfInputPoints();
  vtkIdType cdSize = cdArrays * this->GetNumberOfInputCells();
  int total = (pdSize + cdSize + this->GetNumberOfInputPoints());
  if (total == 0)
  {
    total = 1;
  }
  fractions[0] = 0;
  fractions[1] = float(pdSize) / total;
  fractions[2] = float(pdSize + cdSize) / total;
  fractions[3] = 1;
}

//------------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::CalculateCellFractions(float* fractions, vtkIdType typesSize)
{
  // Calculate the fraction of cell specification data contributed by
  // each of the connectivity, offset, and type arrays.
  vtkIdType connectSize = this->CellPoints ? this->CellPoints->GetNumberOfTuples() : 0;
  vtkIdType offsetSize = this->CellOffsets ? this->CellOffsets->GetNumberOfTuples() : 0;
  vtkIdType faceSize = this->FaceConnectivity ? this->FaceConnectivity->GetNumberOfTuples() : 0;
  vtkIdType faceoffsetSize = this->FaceOffsets ? this->FaceOffsets->GetNumberOfTuples() : 0;
  vtkIdType polyhedronSize =
    this->PolyhedronToFaces ? this->PolyhedronToFaces->GetNumberOfTuples() : 0;
  vtkIdType polyhedronOffsetSize =
    this->PolyhedronOffsets ? this->PolyhedronOffsets->GetNumberOfTuples() : 0;
  vtkIdType total = connectSize + offsetSize + faceSize + faceoffsetSize + polyhedronSize +
    polyhedronOffsetSize + typesSize;
  if (total == 0)
  {
    total = 1;
  }
  fractions[0] = 0;
  fractions[1] = float(connectSize) / total;
  fractions[2] = float(connectSize + offsetSize) / total;
  fractions[3] = float(connectSize + offsetSize + faceSize) / total;
  fractions[4] = float(connectSize + offsetSize + faceSize + faceoffsetSize) / total;
  fractions[5] =
    float(connectSize + offsetSize + faceSize + faceoffsetSize + polyhedronSize) / total;
  fractions[6] = float(connectSize + offsetSize + faceSize + faceoffsetSize + polyhedronSize +
                   polyhedronOffsetSize) /
    total;
  fractions[7] = 1;
}

//------------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::SetInputUpdateExtent(int piece, int numPieces, int ghostLevel)
{
  vtkInformation* inInfo = this->GetExecutive()->GetInputInformation(0, 0);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), numPieces);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), piece);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), ghostLevel);
}
VTK_ABI_NAMESPACE_END
