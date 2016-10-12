/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLUnstructuredDataWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLUnstructuredDataWriter.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCellIterator.h"
#include "vtkDataArray.h"
#include "vtkDataCompressor.h"
#include "vtkDataSetAttributes.h"
#include "vtkErrorCode.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnsignedCharArray.h"
#define vtkXMLOffsetsManager_DoNotInclude
#include "vtkXMLOffsetsManager.h"
#undef  vtkXMLOffsetsManager_DoNotInclude

#include <cassert>


//----------------------------------------------------------------------------
vtkXMLUnstructuredDataWriter::vtkXMLUnstructuredDataWriter()
{
  this->NumberOfPieces = 1;
  this->WritePiece = -1;
  this->GhostLevel = 0;
  this->CellPoints = vtkIdTypeArray::New();
  this->CellOffsets = vtkIdTypeArray::New();
  this->CellPoints->SetName("connectivity");
  this->CellOffsets->SetName("offsets");

  this->CurrentPiece = 0;
  this->FieldDataOM->Allocate(0);
  this->PointsOM    = new OffsetsManagerGroup;
  this->PointDataOM = new OffsetsManagerArray;
  this->CellDataOM  = new OffsetsManagerArray;

  this->Faces = vtkIdTypeArray::New();
  this->FaceOffsets = vtkIdTypeArray::New();
  this->Faces->SetName("faces");
  this->FaceOffsets->SetName("faceoffsets");
}

//----------------------------------------------------------------------------
vtkXMLUnstructuredDataWriter::~vtkXMLUnstructuredDataWriter()
{
  this->CellPoints->Delete();
  this->CellOffsets->Delete();
  this->Faces->Delete();
  this->FaceOffsets->Delete();

  delete this->PointsOM;
  delete this->PointDataOM;
  delete this->CellDataOM;
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "NumberOfPieces: " << this->NumberOfPieces << "\n";
  os << indent << "WritePiece: " << this->WritePiece << "\n";
  os << indent << "GhostLevel: " << this->GhostLevel << "\n";
}

//----------------------------------------------------------------------------
vtkPointSet* vtkXMLUnstructuredDataWriter::GetInputAsPointSet()
{
  return static_cast<vtkPointSet*>(this->Superclass::GetInput());
}

//----------------------------------------------------------------------------
int vtkXMLUnstructuredDataWriter::ProcessRequest(vtkInformation* request,
                                                 vtkInformationVector** inputVector,
                                                 vtkInformationVector* outputVector)
{

  if(request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    if((this->WritePiece < 0) || (this->WritePiece >= this->NumberOfPieces))
    {
      this->SetInputUpdateExtent(
        this->CurrentPiece, this->NumberOfPieces, this->GhostLevel);
    }
    else
    {
      this->SetInputUpdateExtent(
        this->WritePiece, this->NumberOfPieces, this->GhostLevel);
    }
    return 1;
  }

  // generate the data
  else if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    this->SetErrorCode(vtkErrorCode::NoError);

    if(!this->Stream && !this->FileName && !this->WriteToOutputString)
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
      float wholeProgressRange[2] = {0,1};
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
        float wholeProgressRange[2] = {0,1};
        this->SetProgressRange(wholeProgressRange, 0, 1);
      }

      if (!this->OpenStream())
      {
        this->NumberOfPieces = numPieces;
        return 0;
      }

      if (this->GetInputAsDataSet() != NULL &&
          (this->GetInputAsDataSet()->GetPointGhostArray() != NULL &&
           this->GetInputAsDataSet()->GetCellGhostArray() != NULL))
      {
        // use the current version for the file.
        this->UsePreviousVersion = false;
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
      if( this->DataMode == vtkXMLWriter::Appended && this->FieldDataOM->GetNumberOfElements())
      {
        // Write the field data arrays.
        this->WriteFieldDataAppendedData(this->GetInput()->GetFieldData(),
          this->CurrentTimeIndex, this->FieldDataOM);
        if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
        {
          this->DeletePositionArrays();
          return 0;
        }
      }
    }

    if( !(this->UserContinueExecuting == 0)) //if user ask to stop do not try to write a piece
    {
      result = this->WriteAPiece();
    }

    if((this->WritePiece < 0) || (this->WritePiece >= this->NumberOfPieces))
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

      if( this->UserContinueExecuting != 1 )
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

//----------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::AllocatePositionArrays()
{
  this->NumberOfPointsPositions = new vtkTypeInt64[this->NumberOfPieces];

  this->PointsOM->Allocate(this->NumberOfPieces, this->NumberOfTimeSteps);
  this->PointDataOM->Allocate(this->NumberOfPieces);
  this->CellDataOM->Allocate(this->NumberOfPieces);
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::DeletePositionArrays()
{
  delete [] this->NumberOfPointsPositions;
  this->NumberOfPointsPositions = 0;
}

//----------------------------------------------------------------------------
int vtkXMLUnstructuredDataWriter::WriteHeader()
{
  vtkIndent indent = vtkIndent().GetNextIndent();

  ostream& os = *(this->Stream);

  if(!this->WritePrimaryElement(os, indent))
  {
    return 0;
  }

  this->WriteFieldData(indent.GetNextIndent());

  if(this->DataMode == vtkXMLWriter::Appended)
  {
    vtkIndent nextIndent = indent.GetNextIndent();

    this->AllocatePositionArrays();

    if((this->WritePiece < 0) || (this->WritePiece >= this->NumberOfPieces))
    {
      // Loop over each piece and write its structure.
      int i;
      for(i=0; i < this->NumberOfPieces; ++i)
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

//----------------------------------------------------------------------------
int vtkXMLUnstructuredDataWriter::WriteAPiece()
{
  vtkIndent indent = vtkIndent().GetNextIndent();

  int result=1;

  if(this->DataMode == vtkXMLWriter::Appended)
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

//----------------------------------------------------------------------------
int vtkXMLUnstructuredDataWriter::WriteFooter()
{
  vtkIndent indent = vtkIndent().GetNextIndent();

  ostream& os = *(this->Stream);

  if(this->DataMode == vtkXMLWriter::Appended)
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

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::WriteInlinePieceAttributes()
{
  vtkPointSet* input = this->GetInputAsPointSet();
  this->WriteScalarAttribute("NumberOfPoints",
                             input->GetNumberOfPoints());
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::WriteInlinePiece(vtkIndent indent)
{
  vtkPointSet* input = this->GetInputAsPointSet();

  // Split progress among point data, cell data, and point arrays.
  float progressRange[2] = {0,0};
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

//----------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::WriteAppendedPieceAttributes(int index)
{
  this->NumberOfPointsPositions[index] =
    this->ReserveAttributeSpace("NumberOfPoints");
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::WriteAppendedPiece(int index,
                                                      vtkIndent indent)
{
  vtkPointSet* input = this->GetInputAsPointSet();

  this->WritePointDataAppended(input->GetPointData(), indent,
    &this->PointDataOM->GetPiece(index));
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  this->WriteCellDataAppended(input->GetCellData(), indent,
    &this->CellDataOM->GetPiece(index));
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  this->WritePointsAppended(input->GetPoints(), indent,
    &this->PointsOM->GetPiece(index));
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::WriteAppendedPieceData(int index)
{
  ostream& os = *(this->Stream);
  vtkPointSet* input = this->GetInputAsPointSet();

  std::streampos returnPosition = os.tellp();
  os.seekp(std::streampos(this->NumberOfPointsPositions[index]));
  vtkPoints* points = input->GetPoints();
  this->WriteScalarAttribute("NumberOfPoints",
                             (points?points->GetNumberOfPoints():0));
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }
  os.seekp(returnPosition);

  // Split progress among point data, cell data, and point arrays.
  float progressRange[2] = {0,0};
  this->GetProgressRange(progressRange);
  float fractions[4];
  this->CalculateDataFractions(fractions);

  // Set the range of progress for the point data arrays.
  this->SetProgressRange(progressRange, 0, fractions);

  // Write the point data arrays.
  this->WritePointDataAppendedData(input->GetPointData(), this->CurrentTimeIndex,
                                  &this->PointDataOM->GetPiece(index));
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  // Set the range of progress for the cell data arrays.
  this->SetProgressRange(progressRange, 1, fractions);

  // Write the cell data arrays.
  this->WriteCellDataAppendedData(input->GetCellData(), this->CurrentTimeIndex,
                                  &this->CellDataOM->GetPiece(index));
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  // Set the range of progress for the point specification array.
  this->SetProgressRange(progressRange, 2, fractions);

  // Write the point specification array.
  // Since we are writing the point let save the Modified Time of vtkPoints:
  this->WritePointsAppendedData(input->GetPoints(), this->CurrentTimeIndex,
                                &this->PointsOM->GetPiece(index));
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::WriteCellsInline(
    const char *name, vtkCellIterator *cellIter, vtkIdType numCells,
    vtkIdType cellSizeEstimate, vtkIndent indent)
{
  this->ConvertCells(cellIter, numCells, cellSizeEstimate);

  // Faces are not supported via this method.
  this->Faces->SetNumberOfTuples(0);
  this->FaceOffsets->SetNumberOfTuples(0);

  vtkNew<vtkUnsignedCharArray> types;
  types->Allocate(numCells);
  for (cellIter->InitTraversal(); !cellIter->IsDoneWithTraversal();
       cellIter->GoToNextCell())
  {
    types->InsertNextValue(static_cast<unsigned char>(cellIter->GetCellType()));
  }

  this->WriteCellsInlineWorker(name, types.GetPointer(), indent);
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::WriteCellsInline(const char* name,
                                                    vtkCellArray* cells,
                                                    vtkDataArray* types,
                                                    vtkIndent indent)
{
  this->WriteCellsInline(name, cells, types, NULL, NULL, indent);
}


//----------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::WriteCellsInline(const char* name,
                        vtkCellArray* cells,  vtkDataArray* types,
                        vtkIdTypeArray* faces, vtkIdTypeArray* faceOffsets,
                        vtkIndent indent)
{
  if(cells)
  {
    this->ConvertCells(cells);
  }
  this->ConvertFaces(faces, faceOffsets);

  this->WriteCellsInlineWorker(name, types, indent);
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::WriteCellsInlineWorker(
    const char *name, vtkDataArray *types, vtkIndent indent)
{
  ostream& os = *(this->Stream);
  os << indent << "<" << name << ">\n";

  // Split progress by cell connectivity, offset, and type arrays.
  float progressRange[2] = {0,0};
  this->GetProgressRange(progressRange);
  float fractions[6];
  this->CalculateCellFractions(fractions, types?types->GetNumberOfTuples():0);

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

  if(types)
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

  if (this->Faces->GetNumberOfTuples())
  {
    // Set the range of progress for the faces array.
    this->SetProgressRange(progressRange, 3, fractions);

    // Write the connectivity array.
    this->WriteArrayInline(this->Faces, indent.GetNextIndent(), "faces");
    if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
    {
      return;
    }
  }

  if (this->FaceOffsets->GetNumberOfTuples())
  {
    // Set the range of progress for the face offset array.
    this->SetProgressRange(progressRange, 4, fractions);

    // Write the face offsets array.
    this->WriteArrayInline(this->FaceOffsets, indent.GetNextIndent(), "faceoffsets");
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

//----------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::WriteCellsAppended(const char* name,
                                                      vtkDataArray* types,
                                                      vtkIndent indent,
                                                      OffsetsManagerGroup *cellsManager)
{
  this->WriteCellsAppended(name, types, 0, 0, indent, cellsManager);
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::WriteCellsAppended(const char* name,
                                                      vtkDataArray* types,
                                                      vtkIdTypeArray* faces,
                                                      vtkIdTypeArray* faceOffsets,
                                                      vtkIndent indent,
                                                      OffsetsManagerGroup *cellsManager)
{
  this->ConvertFaces(faces,faceOffsets);
  ostream& os = *(this->Stream);
  os << indent << "<" << name << ">\n";

  // Helper for the 'for' loop
  vtkDataArray *allcells[5];
  allcells[0] = this->CellPoints;
  allcells[1] = this->CellOffsets;
  allcells[2] = types;
  allcells[3] = this->Faces->GetNumberOfTuples() ? this->Faces : 0;
  allcells[4] = this->FaceOffsets->GetNumberOfTuples() ? this->FaceOffsets : 0;
  const char *names[] = {NULL, NULL, "types", NULL, NULL};

  for(int t=0; t<this->NumberOfTimeSteps; t++)
  {
    for(int i=0; i<5; i++)
    {
      if(allcells[i])
      {
        this->WriteArrayAppended(allcells[i], indent.GetNextIndent(),
          cellsManager->GetElement(i), names[i], 0, t);
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

//----------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::WriteCellsAppended(
    const char *name, vtkCellIterator *cellIter, vtkIdType numCells,
    vtkIndent indent, OffsetsManagerGroup *cellsManager)
{
  vtkNew<vtkUnsignedCharArray> types;
  types->Allocate(numCells);
  for (cellIter->InitTraversal(); !cellIter->IsDoneWithTraversal();
       cellIter->GoToNextCell())
  {
    types->InsertNextValue(static_cast<unsigned char>(cellIter->GetCellType()));
  }

  this->WriteCellsAppended(name, types.GetPointer(), 0, 0, indent, cellsManager);
}

//----------------------------------------------------------------------------
void
vtkXMLUnstructuredDataWriter::WriteCellsAppendedData(vtkCellArray* cells,
                                                     vtkDataArray* types,
                                                     int timestep,
                                                     OffsetsManagerGroup *cellsManager)
{
  this->WriteCellsAppendedData(cells, types, NULL, NULL, timestep, cellsManager);
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::WriteCellsAppendedData(
    vtkCellIterator *cellIter, vtkIdType numCells, vtkIdType cellSizeEstimate,
    int timestep, OffsetsManagerGroup *cellsManager)
{
  this->ConvertCells(cellIter, numCells, cellSizeEstimate);

  // Faces are not supported by this method:
  this->Faces->SetNumberOfTuples(0);
  this->FaceOffsets->SetNumberOfTuples(0);

  vtkNew<vtkUnsignedCharArray> types;
  types->Allocate(this->CellOffsets->GetNumberOfTuples() + 1);

  for(cellIter->InitTraversal(); !cellIter->IsDoneWithTraversal();
      cellIter->GoToNextCell())
  {
    types->InsertNextValue(static_cast<unsigned char>(cellIter->GetCellType()));
  }

  this->WriteCellsAppendedDataWorker(types.GetPointer(), timestep,
                                     cellsManager);
}

//----------------------------------------------------------------------------
void
vtkXMLUnstructuredDataWriter::WriteCellsAppendedData(vtkCellArray* cells,
                                                     vtkDataArray* types,
                                                     vtkIdTypeArray* faces,
                                                     vtkIdTypeArray* faceOffsets,
                                                     int timestep,
                                                     OffsetsManagerGroup *cellsManager)
{
  if (cells)
  {
    this->ConvertCells(cells);
  }

  this->ConvertFaces(faces, faceOffsets);
  this->WriteCellsAppendedDataWorker(types, timestep, cellsManager);
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::WriteCellsAppendedDataWorker(
    vtkDataArray *types, int timestep, OffsetsManagerGroup *cellsManager)
{
  // Split progress by cell connectivity, offset, and type arrays.
  float progressRange[5] = {0,0,0,0,0};
  this->GetProgressRange(progressRange);
  float fractions[6];
  this->CalculateCellFractions(fractions, types?types->GetNumberOfTuples():0);

  // Helper for the 'for' loop
  vtkDataArray *allcells[5];
  allcells[0] = this->CellPoints;
  allcells[1] = this->CellOffsets;
  allcells[2] = types;
  allcells[3] = this->Faces->GetNumberOfTuples() ? this->Faces : 0;
  allcells[4] = this->FaceOffsets->GetNumberOfTuples() ? this->FaceOffsets : 0;

  for(int i=0; i<5; i++)
  {
    if(allcells[i])
    {
      // Set the range of progress for the connectivity array.
      this->SetProgressRange(progressRange, i, fractions);

      vtkMTimeType mtime = allcells[i]->GetMTime();
      vtkMTimeType &cellsMTime = cellsManager->GetElement(i).GetLastMTime();
      // Only write cells if MTime has changed
      if( cellsMTime != mtime )
      {
        cellsMTime = mtime;
        // Write the connectivity array.
        this->WriteArrayAppendedData(allcells[i],
          cellsManager->GetElement(i).GetPosition(timestep),
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
        assert( timestep > 0 );
        cellsManager->GetElement(i).GetOffsetValue(timestep) =
          cellsManager->GetElement(i).GetOffsetValue(timestep-1);
        this->ForwardAppendedDataOffset(cellsManager->GetElement(i).GetPosition(timestep),
                                        cellsManager->GetElement(i).GetOffsetValue(timestep),
                                        "offset" );
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::ConvertCells(
    vtkCellIterator *cellIter, vtkIdType numCells, vtkIdType cellSizeEstimate)
{
  this->CellPoints->Allocate(numCells * cellSizeEstimate);
  this->CellOffsets->Allocate(numCells);

  for (cellIter->InitTraversal(); !cellIter->IsDoneWithTraversal();
       cellIter->GoToNextCell())
  {
    vtkIdType *begin = cellIter->GetPointIds()->GetPointer(0);
    vtkIdType *end = begin + cellIter->GetNumberOfPoints();
    while (begin != end)
    {
      this->CellPoints->InsertNextValue(*begin++);
    }

    this->CellOffsets->InsertNextValue(this->CellPoints->GetNumberOfTuples());
  }

  this->CellPoints->Squeeze();
  this->CellOffsets->Squeeze();
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::ConvertCells(vtkCellArray* cells)
{
  vtkIdTypeArray* connectivity = cells->GetData();
  vtkIdType numberOfCells = cells->GetNumberOfCells();
  vtkIdType numberOfTuples = connectivity->GetNumberOfTuples();

  this->CellPoints->SetNumberOfTuples(numberOfTuples - numberOfCells);
  this->CellOffsets->SetNumberOfTuples(numberOfCells);

  vtkIdType* inCell = connectivity->GetPointer(0);
  vtkIdType* outCellPointsBase = this->CellPoints->GetPointer(0);
  vtkIdType* outCellPoints = outCellPointsBase;
  vtkIdType* outCellOffset = this->CellOffsets->GetPointer(0);

  vtkIdType i;
  for(i=0;i < numberOfCells; ++i)
  {
    vtkIdType numberOfPoints = *inCell++;
    memcpy(outCellPoints, inCell, sizeof(vtkIdType)*numberOfPoints);
    outCellPoints += numberOfPoints;
    inCell += numberOfPoints;
    *outCellOffset++ = outCellPoints - outCellPointsBase;
  }
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::ConvertFaces(vtkIdTypeArray* faces,
                                                vtkIdTypeArray* faceOffsets)
{
  if (!faces || !faces->GetNumberOfTuples() ||
      !faceOffsets || !faceOffsets->GetNumberOfTuples())
  {
    this->Faces->SetNumberOfTuples(0);
    this->FaceOffsets->SetNumberOfTuples(0);
    return;
  }

  // copy faces stream.
  this->Faces->SetNumberOfTuples(faces->GetNumberOfTuples());
  vtkIdType * fromPtr = faces->GetPointer(0);
  vtkIdType * toPtr = this->Faces->GetPointer(0);
  for (vtkIdType i = 0; i < faces->GetNumberOfTuples(); i++)
  {
    *toPtr++ = *fromPtr++;
  }

  // this->FaceOffsets point to the face arrays of cells. Specifically
  // FaceOffsets[i] points to the end of the i-th cell's faces + 1. While
  // input faceOffsets[i] points to the beginning of the i-th cell. Note
  // that for both arrays, a non-polyhedron cell has an offset of -1.
  vtkIdType numberOfCells = faceOffsets->GetNumberOfTuples();
  this->FaceOffsets->SetNumberOfTuples(numberOfCells);
  vtkIdType* newOffsetPtr = this->FaceOffsets->GetPointer(0);
  vtkIdType* oldOffsetPtr = faceOffsets->GetPointer(0);
  vtkIdType* facesPtr = this->Faces->GetPointer(0);
  bool foundPolyhedronCell = false;
  for (vtkIdType i = 0; i < numberOfCells; i++)
  {
    if (oldOffsetPtr[i] < 0) //non-polyhedron cell
    {
      newOffsetPtr[i] = -1;
    }
    else // polyhedron cell
    {
      foundPolyhedronCell = true;
      // read numberOfFaces in a cell
      vtkIdType currLoc = oldOffsetPtr[i];
      vtkIdType numberOfCellFaces = facesPtr[currLoc];
      currLoc += 1;
      for (vtkIdType j = 0; j < numberOfCellFaces; j++)
      {
        // read numberOfPoints in a face
        vtkIdType numberOfFacePoints = facesPtr[currLoc];
        currLoc += numberOfFacePoints + 1;
      }
      newOffsetPtr[i] = currLoc;
    }
  }

  if (!foundPolyhedronCell)
  {
    this->Faces->SetNumberOfTuples(0);
    this->FaceOffsets->SetNumberOfTuples(0);
  }

  return;
}

//----------------------------------------------------------------------------
vtkIdType vtkXMLUnstructuredDataWriter::GetNumberOfInputPoints()
{
  vtkPointSet* input = this->GetInputAsPointSet();
  vtkPoints* points = input->GetPoints();
  return points?points->GetNumberOfPoints():0;
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::CalculateDataFractions(float* fractions)
{
  // Calculate the fraction of point/cell data and point
  // specifications contributed by each component.
  vtkPointSet* input = this->GetInputAsPointSet();
  int pdArrays = input->GetPointData()->GetNumberOfArrays();
  int cdArrays = input->GetCellData()->GetNumberOfArrays();
  vtkIdType pdSize = pdArrays*this->GetNumberOfInputPoints();
  vtkIdType cdSize = cdArrays*this->GetNumberOfInputCells();
  int total = (pdSize+cdSize+this->GetNumberOfInputPoints());
  if(total == 0)
  {
    total = 1;
  }
  fractions[0] = 0;
  fractions[1] = float(pdSize)/total;
  fractions[2] = float(pdSize+cdSize)/total;
  fractions[3] = 1;
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::CalculateCellFractions(float* fractions,
                                                          vtkIdType typesSize)
{
  // Calculate the fraction of cell specification data contributed by
  // each of the connectivity, offset, and type arrays.
  vtkIdType connectSize = this->CellPoints->GetNumberOfTuples();
  vtkIdType offsetSize = this->CellOffsets->GetNumberOfTuples();
  vtkIdType faceSize = this->Faces ? this->Faces->GetNumberOfTuples() : 0;
  vtkIdType faceoffsetSize = this->FaceOffsets ?
                               this->FaceOffsets->GetNumberOfTuples() : 0;
  vtkIdType total = connectSize+offsetSize+faceSize+faceoffsetSize+typesSize;
  if(total == 0)
  {
    total = 1;
  }
  fractions[0] = 0;
  fractions[1] = float(connectSize)/total;
  fractions[2] = float(connectSize+offsetSize)/total;
  fractions[3] = float(connectSize+offsetSize+faceSize)/total;
  fractions[4] = float(connectSize+offsetSize+faceSize+faceoffsetSize)/total;
  fractions[5] = 1;
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredDataWriter::SetInputUpdateExtent(
  int piece, int numPieces, int ghostLevel)
{
  vtkInformation* inInfo =
    this->GetExecutive()->GetInputInformation(0, 0);
  inInfo->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), numPieces);
  inInfo->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), piece);
  inInfo->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), ghostLevel);
}

