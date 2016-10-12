/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLStructuredDataWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLStructuredDataWriter.h"

#include "vtkArrayIteratorIncludes.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataCompressor.h"
#include "vtkDataSet.h"
#include "vtkErrorCode.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerVectorKey.h"
#include "vtkInformationVector.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#define vtkXMLOffsetsManager_DoNotInclude
#include "vtkXMLOffsetsManager.h"
#undef  vtkXMLOffsetsManager_DoNotInclude

//----------------------------------------------------------------------------
vtkXMLStructuredDataWriter::vtkXMLStructuredDataWriter()
{
  this->WritePiece = -1;
  this->NumberOfPieces = 1;
  this->GhostLevel = 0;

  this->WriteExtent[0] = 0; this->WriteExtent[1] = -1;
  this->WriteExtent[2] = 0; this->WriteExtent[3] = -1;
  this->WriteExtent[4] = 0; this->WriteExtent[5] = -1;

  this->CurrentPiece = 0;
  this->ProgressFractions = 0;
  this->FieldDataOM->Allocate(0);
  this->PointDataOM = new OffsetsManagerArray;
  this->CellDataOM  = new OffsetsManagerArray;
}

//----------------------------------------------------------------------------
vtkXMLStructuredDataWriter::~vtkXMLStructuredDataWriter()
{
  delete[] this->ProgressFractions;
  delete this->PointDataOM;
  delete this->CellDataOM;
}

//----------------------------------------------------------------------------
void vtkXMLStructuredDataWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "WriteExtent: "
     << this->WriteExtent[0] << " " << this->WriteExtent[1] << "  "
     << this->WriteExtent[2] << " " << this->WriteExtent[3] << "  "
     << this->WriteExtent[4] << " " << this->WriteExtent[5] << "\n";
  os << indent << "NumberOfPieces" << this->NumberOfPieces << "\n";
  os << indent << "WritePiece: " << this->WritePiece << "\n";
}

//----------------------------------------------------------------------------
void vtkXMLStructuredDataWriter::SetInputUpdateExtent(int piece)
{
  vtkInformation* inInfo =
    this->GetExecutive()->GetInputInformation(0, 0);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
    piece);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
    this->NumberOfPieces);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
    this->GhostLevel);
  if ((this->WriteExtent[0] == 0) && (this->WriteExtent[1] == -1) &&
     (this->WriteExtent[2] == 0) && (this->WriteExtent[3] == -1) &&
     (this->WriteExtent[4] == 0) && (this->WriteExtent[5] == -1))
  {
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
      inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()),
      6);
  }
  else
  {
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
      this->WriteExtent, 6);
  }
  inInfo->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);
}

//----------------------------------------------------------------------------
int vtkXMLStructuredDataWriter::ProcessRequest(
  vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{

  if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    if (this->WritePiece >= 0)
    {
      this->CurrentPiece = this->WritePiece;
    }
    return 1;
  }

  if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    this->SetInputUpdateExtent(this->CurrentPiece);

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

    // We are just starting to write.  Do not call
    // UpdateProgressDiscrete because we want a 0 progress callback the
    // first time.
    this->UpdateProgress(0);

    // Initialize progress range to entire 0..1 range.
    float wholeProgressRange[2] = {0,1};
    this->SetProgressRange(wholeProgressRange, 0, 1);

    int result = 1;
    if ((this->CurrentPiece == 0 || this->WritePiece >= 0) && this->CurrentTimeIndex == 0 )
    {
      if (!this->OpenStream())
      {
        return 0;
      }
      if (this->GetInputAsDataSet() != NULL &&
          (this->GetInputAsDataSet()->GetPointGhostArray() != NULL ||
           this->GetInputAsDataSet()->GetCellGhostArray() != NULL))
      {
        // use the current version for the file
        this->UsePreviousVersion = false;
      }
      // Write the file.
      if (!this->StartFile())
      {
        return 0;
      }

      if (!this->WriteHeader())
      {
        return 0;
      }

      this->CurrentTimeIndex = 0;
      if (this->DataMode == vtkXMLWriter::Appended && this->FieldDataOM->GetNumberOfElements())
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

    if (!(this->UserContinueExecuting == 0)) //if user ask to stop do not try to write a piece
    {
      result = this->WriteAPiece();
    }

    if (this->WritePiece < 0)
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
          return 0;
        }

        if (!this->EndFile())
        {
          return 0;
        }

        this->CloseStream();
        this->CurrentTimeIndex = 0; // Reset
      }
    }

    // We have finished writing.
    this->UpdateProgressDiscrete(1);
    return result;
  }
  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}


//----------------------------------------------------------------------------
void vtkXMLStructuredDataWriter::AllocatePositionArrays()
{
  this->ExtentPositions = new vtkTypeInt64[this->NumberOfPieces];

  // Prepare storage for the point and cell data array appended data
  // offsets for each piece.
  this->PointDataOM->Allocate(this->NumberOfPieces);
  this->CellDataOM->Allocate(this->NumberOfPieces);
}

//----------------------------------------------------------------------------
void vtkXMLStructuredDataWriter::DeletePositionArrays()
{
  delete[] this->ExtentPositions;
  this->ExtentPositions = NULL;
}

//----------------------------------------------------------------------------
int vtkXMLStructuredDataWriter::WriteHeader()
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
    int begin = this->WritePiece;
    int end = this->WritePiece + 1;
    if (this->WritePiece < 0)
    {
      begin = 0;
      end = this->NumberOfPieces;
    }
    vtkIndent nextIndent = indent.GetNextIndent();

    this->AllocatePositionArrays();

    // Loop over each piece and write its structure.
    for (int i=begin; i < end; ++i)
    {
      // Update the piece's extent.

      os << nextIndent << "<Piece";
      // We allocate 66 characters because that is as big as 6 integers
      // with spaces can get.
      this->ExtentPositions[i] = this->ReserveAttributeSpace("Extent", 66);
      os << ">\n";

      if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
      {
        this->DeletePositionArrays();
        return 0;
      }

      this->WriteAppendedPiece(i, nextIndent.GetNextIndent());

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
      this->DeletePositionArrays();
      this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
      return 0;
    }

    this->StartAppendedData();
    if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
    {
      this->DeletePositionArrays();
      return 0;
    }
  }

  // Split progress of the data write by the fraction contributed by
  // each piece.
  float progressRange[2] = { 0.f, 0.f };
  this->GetProgressRange(progressRange);
  this->ProgressFractions = new float[this->NumberOfPieces+1];
  this->CalculatePieceFractions(this->ProgressFractions);

  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLStructuredDataWriter::WriteAPiece()
{
  vtkIndent indent = vtkIndent().GetNextIndent();
  int result = 1;

  if (this->DataMode == vtkXMLWriter::Appended)
  {
    vtkDataSet* input = this->GetInputAsDataSet();

    // Make sure input is valid.
    if (input->CheckAttributes() == 0)
    {
      this->WriteAppendedPieceData(this->CurrentPiece);

      if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
      {
        this->DeletePositionArrays();
        return 0;
      }
    }
    else
    {
      vtkErrorMacro("Input is invalid for piece "
                    << this->CurrentPiece
                    << ".  Aborting.");
      result = 0;
    }
  }
  else
  {
    this->WriteInlineMode(indent);
  }

  return result;
}

//----------------------------------------------------------------------------
int vtkXMLStructuredDataWriter::WriteFooter()
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
      this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
    }
  }

  delete[] this->ProgressFractions;
  this->ProgressFractions = 0;

  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLStructuredDataWriter::WriteInlineMode(vtkIndent indent)
{
  vtkDataSet* input = this->GetInputAsDataSet();
  ostream& os = *(this->Stream);

  int* extent = input->GetInformation()->Get(vtkDataObject::DATA_EXTENT());

  // Split progress of the data write by the fraction contributed by
  // each piece.
  float progressRange[2] = { 0.f, 0.f };
  this->GetProgressRange(progressRange);

  // Write each piece's XML and data.
  int result = 1;

  // Set the progress range for this piece.
  this->SetProgressRange(progressRange, this->CurrentPiece, this->ProgressFractions);

  // Make sure input is valid.
  if (input->CheckAttributes() == 0)
  {
    os << indent << "<Piece";
    this->WriteVectorAttribute("Extent", 6, extent);
    if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
    {
      return 0;
    }

    os << ">\n";

    this->WriteInlinePiece(indent.GetNextIndent());
    if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
    {
      return 0;
    }
    os << indent << "</Piece>\n";
  }
  else
  {
    vtkErrorMacro("Input is invalid for piece " << this->CurrentPiece << ".  Aborting.");
    result = 0;
  }

  return result;
}

//----------------------------------------------------------------------------
template <class iterT>
inline void vtkXMLStructuredDataWriterCopyTuples(
  iterT* destIter, vtkIdType destTuple,
  iterT* srcIter, vtkIdType sourceTuple,
  vtkIdType numTuples)
{
  // for all contiguous-fixed component size arrays (except Bit).
  int tupleSize = (srcIter->GetDataTypeSize() *
                   srcIter->GetNumberOfComponents());

  memcpy(destIter->GetTuple(destTuple), srcIter->GetTuple(sourceTuple),
    numTuples*tupleSize);
}

//----------------------------------------------------------------------------
inline void vtkXMLStructuredDataWriterCopyTuples(
  vtkArrayIteratorTemplate<vtkStdString>* destIter, vtkIdType destTuple,
  vtkArrayIteratorTemplate<vtkStdString>* srcIter, vtkIdType sourceTuple,
  vtkIdType numTuples)
{
  vtkIdType numValues = numTuples * srcIter->GetNumberOfComponents();
  vtkIdType destIndex = destTuple * destIter->GetNumberOfComponents();
  vtkIdType srcIndex = sourceTuple * srcIter->GetNumberOfComponents();

  for (vtkIdType cc = 0; cc < numValues; cc++)
  {
    destIter->GetValue(destIndex++) = srcIter->GetValue(srcIndex++);
  }
}

//----------------------------------------------------------------------------
void vtkXMLStructuredDataWriter::WritePrimaryElementAttributes(ostream &os,
                                                               vtkIndent indent)
{
  this->Superclass::WritePrimaryElementAttributes(os, indent);

  int* ext = this->WriteExtent;
  if ((this->WriteExtent[0] == 0) && (this->WriteExtent[1] == -1) &&
     (this->WriteExtent[2] == 0) && (this->WriteExtent[3] == -1) &&
     (this->WriteExtent[4] == 0) && (this->WriteExtent[5] == -1))
  {
    ext = this->GetInputInformation(0, 0)->Get(
      vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  }

  if (this->WritePiece >= 0)
  {
    vtkDataSet* input = this->GetInputAsDataSet();
    ext = input->GetInformation()->Get(vtkDataObject::DATA_EXTENT());
  }

  this->WriteVectorAttribute("WholeExtent", 6, ext);
}

//----------------------------------------------------------------------------
void vtkXMLStructuredDataWriter::WriteAppendedPiece(int index,
                                                    vtkIndent indent)
{
  // Write the point data and cell data arrays.
  vtkDataSet* input = this->GetInputAsDataSet();
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
}

//----------------------------------------------------------------------------
void vtkXMLStructuredDataWriter::WriteAppendedPieceData(int index)
{
  // Write the point data and cell data arrays.
  vtkDataSet* input = this->GetInputAsDataSet();

  int* ext = input->GetInformation()->Get(vtkDataObject::DATA_EXTENT());

  ostream& os = *(this->Stream);

  std::streampos returnPosition = os.tellp();
  os.seekp(std::streampos(this->ExtentPositions[index]));
  this->WriteVectorAttribute("Extent", 6, ext);
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }
  os.seekp(returnPosition);

  // Split progress between point data and cell data arrays.
  float progressRange[2] = { 0.f, 0.f };
  this->GetProgressRange(progressRange);
  int pdArrays = input->GetPointData()->GetNumberOfArrays();
  int cdArrays = input->GetCellData()->GetNumberOfArrays();
  int total = (pdArrays+cdArrays)? (pdArrays+cdArrays):1;
  float fractions[3] =
    {
    0,
    static_cast<float>(pdArrays) / total,
    1
    };

  // Set the range of progress for the point data arrays.
  this->SetProgressRange(progressRange, 0, fractions);
  this->WritePointDataAppendedData(input->GetPointData(), this->CurrentTimeIndex,
                                   &this->PointDataOM->GetPiece(index));
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  // Set the range of progress for the cell data arrays.
  this->SetProgressRange(progressRange, 1, fractions);
  this->WriteCellDataAppendedData(input->GetCellData(), this->CurrentTimeIndex,
                                  &this->CellDataOM->GetPiece(index));
}

//----------------------------------------------------------------------------
void vtkXMLStructuredDataWriter::WriteInlinePiece(vtkIndent indent)
{
  // Write the point data and cell data arrays.
  vtkDataSet* input = this->GetInputAsDataSet();

  // Split progress between point data and cell data arrays.
  float progressRange[2] = { 0.f, 0.f };
  this->GetProgressRange(progressRange);
  int pdArrays = input->GetPointData()->GetNumberOfArrays();
  int cdArrays = input->GetCellData()->GetNumberOfArrays();
  int total = (pdArrays+cdArrays)? (pdArrays+cdArrays):1;
  float fractions[3] =
    {
      0,
      float(pdArrays)/total,
      1
    };

  // Set the range of progress for the point data arrays.
  this->SetProgressRange(progressRange, 0, fractions);
  this->WritePointDataInline(input->GetPointData(), indent);
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  // Set the range of progress for the cell data arrays.
  this->SetProgressRange(progressRange, 1, fractions);
  this->WriteCellDataInline(input->GetCellData(), indent);
}

//----------------------------------------------------------------------------
vtkIdType vtkXMLStructuredDataWriter::GetStartTuple(int* extent,
                                                    vtkIdType* increments,
                                                    int i, int j, int k)
{
  return (((i - extent[0]) * increments[0]) +
          ((j - extent[2]) * increments[1]) +
          ((k - extent[4]) * increments[2]));
}

//----------------------------------------------------------------------------
void vtkXMLStructuredDataWriter::CalculatePieceFractions(float* fractions)
{
  // Calculate the fraction of total data contributed by each piece.
  fractions[0] = 0;
  for (int i = 0; i < this->NumberOfPieces;++i)
  {
    int extent[6];
    this->GetInputExtent(extent);

    // Add this piece's size to the cumulative fractions array.
    fractions[i+1] = fractions[i] + ((extent[1]-extent[0]+1)*
                                     (extent[3]-extent[2]+1)*
                                     (extent[5]-extent[4]+1));
  }
  if (fractions[this->NumberOfPieces] == 0)
  {
    fractions[this->NumberOfPieces] = 1;
  }
  for (int i = 0; i < this->NumberOfPieces; ++i)
  {
    fractions[i+1] = fractions[i+1] / fractions[this->NumberOfPieces];
  }
}
