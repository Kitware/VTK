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

#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataCompressor.h"
#include "vtkDataSet.h"
#include "vtkErrorCode.h"
#include "vtkExtentTranslator.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerVectorKey.h"
#include "vtkInformationVector.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#define vtkOffsetsManager_DoNotInclude
#include "vtkOffsetsManagerArray.h"
#undef  vtkOffsetsManager_DoNotInclude

vtkCxxRevisionMacro(vtkXMLStructuredDataWriter, "1.15");
vtkCxxSetObjectMacro(vtkXMLStructuredDataWriter, ExtentTranslator,
                     vtkExtentTranslator);

//----------------------------------------------------------------------------
vtkXMLStructuredDataWriter::vtkXMLStructuredDataWriter()
{
  this->ExtentTranslator = vtkExtentTranslator::New();
  this->NumberOfPieces = 1;
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
  this->SetExtentTranslator(0);
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
  if(this->ExtentTranslator)
    {
    os << indent << "ExtentTranslator: " << this->ExtentTranslator << "\n";
    }
  else
    {
    os << indent << "ExtentTranslator: (none)\n";
    }
  os << indent << "NumberOfPieces" << this->NumberOfPieces << "\n";
}

//----------------------------------------------------------------------------
void vtkXMLStructuredDataWriter::SetInputUpdateExtent(int piece, int timestep)
{
  this->ExtentTranslator->SetPiece(piece);
  this->ExtentTranslator->PieceToExtent();

  vtkInformation* inInfo = 
    this->GetExecutive()->GetInputInformation(0, 0);
  inInfo->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), 
    this->ExtentTranslator->GetExtent(),
    6);
  inInfo->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_TIME_INDEX(), timestep);
}

//----------------------------------------------------------------------------
int vtkXMLStructuredDataWriter::ProcessRequest(
  vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{

  if(request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
    {
    // Prepare the extent translator to create the set of pieces.
    this->SetupExtentTranslator();
    this->SetInputUpdateExtent(this->CurrentPiece, this->CurrentTimeIndex);

    return 1;
    }
  
  // generate the data
  else if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
    {
    this->SetErrorCode(vtkErrorCode::NoError);

    if(!this->Stream && !this->FileName)
      {
      this->SetErrorCode(vtkErrorCode::NoFileNameError);
      vtkErrorMacro("The FileName or Stream must be set first.");
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
    if (this->CurrentPiece == 0 && this->CurrentTimeIndex == 0 )
      {
      if (!this->OpenFile())
        {
        return 0;
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

    // Tell the pipeline to start looping.
    if (this->CurrentPiece == 0)
      {
      request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
      }
    this->CurrentPiece++;

    if (this->CurrentPiece == this->NumberOfPieces)
      {
      request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
      this->CurrentPiece = 0;
       // We are done writting all the pieces, lets loop over time now:
      this->CurrentTimeIndex++;
 
      if( this->UserContinueExecuting != 1)
        {
        if (!this->WriteFooter())
          {
          return 0;
          }

        if (!this->EndFile())
          {
          return 0;
          }

        this->CloseFile();
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
  // Prepare storage for the point and cell data array appended data
  // offsets for each piece.
  this->PointDataOM->Allocate(this->NumberOfPieces);
  this->CellDataOM->Allocate(this->NumberOfPieces);
}

//----------------------------------------------------------------------------
void vtkXMLStructuredDataWriter::DeletePositionArrays()
{
}

//----------------------------------------------------------------------------
int vtkXMLStructuredDataWriter::WriteHeader()
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

    int extent[6];
    // Loop over each piece and write its structure.
    int i;
    for(i=0; i < this->NumberOfPieces; ++i)
      {
      // Update the piece's extent.
      this->ExtentTranslator->SetPiece(i);
      this->ExtentTranslator->PieceToExtent();
      this->ExtentTranslator->GetExtent(extent);

      os << nextIndent << "<Piece";
      this->WriteVectorAttribute("Extent", 6, extent);
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
  float progressRange[2] = {0,0};
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

  if(this->DataMode == vtkXMLWriter::Appended)
    {
    vtkDataSet* input = this->GetInputAsDataSet();

    // Make sure input is valid.
    if(input->CheckAttributes() == 0)
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

  int extent[6];
  vtkInformation* inInfo = this->GetExecutive()->GetInputInformation(0, 0);
  inInfo->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), extent);
  
  // Split progress of the data write by the fraction contributed by
  // each piece.
  float progressRange[2] = {0,0};
  this->GetProgressRange(progressRange);
  
  // Write each piece's XML and data.
  int result = 1;

  // Set the progress range for this piece.
  this->SetProgressRange(progressRange, this->CurrentPiece, this->ProgressFractions);
    
  // Make sure input is valid.
  if(input->CheckAttributes() == 0)
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
void vtkXMLStructuredDataWriter::SetupExtentTranslator()
{
  vtkDataSet* input = this->GetInputAsDataSet();
  
  // If no write extent has been set, use the whole extent.
  if((this->WriteExtent[0] == 0) && (this->WriteExtent[1] == -1) &&
     (this->WriteExtent[2] == 0) && (this->WriteExtent[3] == -1) &&
     (this->WriteExtent[4] == 0) && (this->WriteExtent[5] == -1))
    {
    this->SetWriteExtent(input->GetWholeExtent());
    }
  
  // Our WriteExtent becomes the WholeExtent of the file.
  this->ExtentTranslator->SetWholeExtent(this->WriteExtent);
  this->ExtentTranslator->SetNumberOfPieces(this->NumberOfPieces);
  
  vtkDebugMacro("Writing Extent: "
                << this->WriteExtent[0] << " " << this->WriteExtent[1] << " "
                << this->WriteExtent[2] << " " << this->WriteExtent[3] << " "
                << this->WriteExtent[4] << " " << this->WriteExtent[5]
                << " in " << this->NumberOfPieces << " pieces.");
}

//----------------------------------------------------------------------------
vtkDataArray*
vtkXMLStructuredDataWriter
::CreateExactExtent(vtkDataArray* array, int* inExtent, int* outExtent,
                    int isPoint)
{
  int outDimensions[3];
  outDimensions[0] = outExtent[1]-outExtent[0]+isPoint;
  outDimensions[1] = outExtent[3]-outExtent[2]+isPoint;
  outDimensions[2] = outExtent[5]-outExtent[4]+isPoint;
  
  int inDimensions[3];
  inDimensions[0] = inExtent[1]-inExtent[0]+isPoint;
  inDimensions[1] = inExtent[3]-inExtent[2]+isPoint;
  inDimensions[2] = inExtent[5]-inExtent[4]+isPoint;
  
  if((inDimensions[0] == outDimensions[0]) &&
     (inDimensions[1] == outDimensions[1]) &&
     (inDimensions[2] == outDimensions[2]))
    {
    array->Register(0);
    return array;
    }

  int tupleSize = (array->GetDataTypeSize() *
                   array->GetNumberOfComponents());
  vtkIdType rowTuples = outDimensions[0];
  vtkIdType sliceTuples = rowTuples*outDimensions[1];
  vtkIdType volumeTuples = sliceTuples*outDimensions[2];
  
  vtkIdType inIncrements[3];
  inIncrements[0] = 1;
  inIncrements[1] = inDimensions[0]*inIncrements[0];
  inIncrements[2] = inDimensions[1]*inIncrements[1];
  
  vtkIdType outIncrements[3];
  outIncrements[0] = 1;
  outIncrements[1] = outDimensions[0]*outIncrements[0];
  outIncrements[2] = outDimensions[1]*outIncrements[1];
  
  vtkDataArray* newArray = array->NewInstance();
  newArray->SetName(array->GetName());
  newArray->SetNumberOfComponents(array->GetNumberOfComponents());
  newArray->SetNumberOfTuples(volumeTuples);
  int components = newArray->GetNumberOfComponents();
  
  if((inDimensions[0] == outDimensions[0]) &&
     (inDimensions[1] == outDimensions[1]))
    {
    // Copy an entire slice at a time.
    int k;
    for(k=0;k < outDimensions[2];++k)
      {
      vtkIdType sourceTuple =
        this->GetStartTuple(inExtent, inIncrements,
                            outExtent[0], outExtent[2], outExtent[4]+k);
      vtkIdType destTuple =
        this->GetStartTuple(outExtent, outIncrements,
                            outExtent[0], outExtent[2], outExtent[4]+k);
      memcpy(newArray->GetVoidPointer(destTuple*components),
             array->GetVoidPointer(sourceTuple*components),
             sliceTuples*tupleSize);
      }
    }
  else
    {
    // Copy a row at a time.
    int j, k;
    for(k=0;k < outDimensions[2];++k)
      {
      for(j=0;j < outDimensions[1];++j)
        {
        vtkIdType sourceTuple =
          this->GetStartTuple(inExtent, inIncrements,
                              outExtent[0], outExtent[2]+j, outExtent[4]+k);
        vtkIdType destTuple =
          this->GetStartTuple(outExtent, outIncrements,
                              outExtent[0], outExtent[2]+j, outExtent[4]+k);
        memcpy(newArray->GetVoidPointer(destTuple*components),
               array->GetVoidPointer(sourceTuple*components),
               rowTuples*tupleSize);
        }
      }
    }
  
  return newArray;
}

//----------------------------------------------------------------------------
void vtkXMLStructuredDataWriter::WritePrimaryElementAttributes(ostream &os, 
                                                               vtkIndent indent)
{
  this->Superclass::WritePrimaryElementAttributes(os, indent);

  this->WriteVectorAttribute("WholeExtent", 6, this->WriteExtent);
}

//----------------------------------------------------------------------------
void vtkXMLStructuredDataWriter::WriteAppendedPiece(int index,
                                                    vtkIndent indent)
{
  // Write the point data and cell data arrays.
  vtkDataSet* input = this->GetInputAsDataSet();
  this->WritePointDataAppended(input->GetPointData(), indent, 
    &this->PointDataOM->GetPiece(index));
  if (!this->PointDataOM->GetPiece(index).GetNumberOfElements())
    {
    return;
    }
  this->WriteCellDataAppended(input->GetCellData(), indent, 
    &this->CellDataOM->GetPiece(index));
  if (!this->CellDataOM->GetPiece(index).GetNumberOfElements())
    {
    return;
    }
}

//----------------------------------------------------------------------------
void vtkXMLStructuredDataWriter::WriteAppendedPieceData(int index)
{
  // Write the point data and cell data arrays.
  vtkDataSet* input = this->GetInputAsDataSet();
  
  // Split progress between point data and cell data arrays.
  float progressRange[2] = {0,0};
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
  float progressRange[2] = {0,0};
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
vtkDataArray*
vtkXMLStructuredDataWriter::CreateArrayForPoints(vtkDataArray* inArray)
{
  int inExtent[6];
  int outExtent[6];
  this->GetInputExtent(inExtent);

  vtkInformation* inInfo = this->GetExecutive()->GetInputInformation(0, 0);
  inInfo->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), outExtent);

  return this->CreateExactExtent(inArray, inExtent, outExtent, 1);
}

//----------------------------------------------------------------------------
vtkDataArray*
vtkXMLStructuredDataWriter::CreateArrayForCells(vtkDataArray* inArray)
{
  int inExtent[6];
  int outExtent[6];
  this->GetInputExtent(inExtent);

  vtkInformation* inInfo = this->GetExecutive()->GetInputInformation(0, 0);
  inInfo->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), outExtent);
  return this->CreateExactExtent(inArray, inExtent, outExtent, 0);
}

//----------------------------------------------------------------------------
void vtkXMLStructuredDataWriter::CalculatePieceFractions(float* fractions)
{
  int i;
  int extent[6];
  
  // Calculate the fraction of total data contributed by each piece.
  fractions[0] = 0;
  for(i=0;i < this->NumberOfPieces;++i)
    {
    // Update the piece's extent.
    this->ExtentTranslator->SetPiece(i);
    this->ExtentTranslator->PieceToExtent();
    this->ExtentTranslator->GetExtent(extent);
    
    // Add this piece's size to the cumulative fractions array.
    fractions[i+1] = fractions[i] + ((extent[1]-extent[0]+1)*
                                     (extent[3]-extent[2]+1)*
                                     (extent[5]-extent[4]+1));
    }
  if(fractions[this->NumberOfPieces] == 0)
    {
    fractions[this->NumberOfPieces] = 1;
    }
  for(i=0;i < this->NumberOfPieces;++i)
    {
    fractions[i+1] = fractions[i+1] / fractions[this->NumberOfPieces];
    }
}
