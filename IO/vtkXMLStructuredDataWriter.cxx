/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLStructuredDataWriter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLStructuredDataWriter.h"
#include "vtkDataSet.h"
#include "vtkDataArray.h"
#include "vtkDataCompressor.h"
#include "vtkExtentTranslator.h"

vtkCxxRevisionMacro(vtkXMLStructuredDataWriter, "1.1");
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
}

//----------------------------------------------------------------------------
vtkXMLStructuredDataWriter::~vtkXMLStructuredDataWriter()
{
  this->SetExtentTranslator(0);
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
int vtkXMLStructuredDataWriter::WriteData()
{
  vtkIndent indent = vtkIndent().GetNextIndent();
  vtkDataSet* input = this->GetInputAsDataSet();
  
  // Make sure our input's WholeExtent is up to date.
  input->UpdateInformation();
  
  // Prepare the extent translator to create the set of pieces.
  this->SetupExtentTranslator();  
  
  // Write the file.
  this->StartFile();
  if(this->DataMode == vtkXMLWriter::Appended)
    {
    this->WriteAppendedMode(indent);
    }
  else
    {
    this->WriteInlineMode(indent);
    }
  this->EndFile();
  
  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLStructuredDataWriter::WriteAppendedMode(vtkIndent indent)
{  
  int i;
  int extent[6];
  ostream& os = *(this->Stream);
  vtkIndent nextIndent = indent.GetNextIndent();
  
  // Prepare storage for the point and cell data array appended data
  // offsets for each piece.
  this->PointDataOffsets = new unsigned long*[this->NumberOfPieces];
  this->CellDataOffsets = new unsigned long*[this->NumberOfPieces];  
  
  // Update the first piece to get form of data setup.
  vtkDataSet* input = this->GetInputAsDataSet();
  this->ExtentTranslator->SetPiece(0);
  this->ExtentTranslator->PieceToExtent();  
  input->SetUpdateExtent(this->ExtentTranslator->GetExtent());
  input->Update();
  
  // Open the primary element.
  os << indent << "<" << this->GetDataSetName();
  this->WritePrimaryElementAttributes();
  os << ">\n";
  
  // Write each piece's XML.
  for(i=0;i < this->NumberOfPieces;++i)
    {
    // Update the piece's extent.
    this->ExtentTranslator->SetPiece(i);
    this->ExtentTranslator->PieceToExtent();
    this->ExtentTranslator->GetExtent(extent);
    
    os << nextIndent << "<Piece";
    this->WriteVectorAttribute("Extent", 6, extent);
    os << ">\n";
    
    this->WriteAppendedPiece(i, nextIndent.GetNextIndent());
    
    os << nextIndent << "</Piece>\n";
    }
  
  // Close the primary element.
  os << indent << "</" << this->GetDataSetName() << ">\n";  
  
  // Write each piece's data.
  this->StartAppendedData();
  
  for(i=0;i < this->NumberOfPieces;++i)
    {
    // Update the piece's data.
    this->ExtentTranslator->SetPiece(i);
    this->ExtentTranslator->PieceToExtent();    
    input->SetUpdateExtent(this->ExtentTranslator->GetExtent());
    input->Update();
    
    this->WriteAppendedPieceData(i);
    }
  
  this->EndAppendedData();  
  
  // Cleanup.
  delete [] this->PointDataOffsets;
  delete [] this->CellDataOffsets;
}

//----------------------------------------------------------------------------
void vtkXMLStructuredDataWriter::WriteInlineMode(vtkIndent indent)
{
  int i;
  int extent[6];
  vtkDataSet* input = this->GetInputAsDataSet();
  ostream& os = *(this->Stream);
  
  // Open the primary element.
  os << indent << "<" << this->GetDataSetName();
  this->WritePrimaryElementAttributes();
  os << ">\n";
  
  // Write each piece's XML and data.
  for(i=0; i < this->NumberOfPieces; ++i)
    { 
    // Update the piece's extent and data.
    this->ExtentTranslator->SetPiece(i);
    this->ExtentTranslator->PieceToExtent();
    this->ExtentTranslator->GetExtent(extent);  
    input->SetUpdateExtent(extent);
    input->Update();
    
    os << indent << "<Piece";
    this->WriteVectorAttribute("Extent", 6, extent);
    os << ">\n";
    
    this->WriteInlinePiece(i, indent.GetNextIndent());
    
    os << indent << "</Piece>\n";
    }
  
  // Close the primary element.
  os << indent << "</" << this->GetDataSetName() << ">\n";  
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
                << " in " << this->NumberOfPieces << " pices.");
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

  unsigned int tupleSize = (array->GetDataTypeSize() *
                            array->GetNumberOfComponents());  
  unsigned int rowTuples = outDimensions[0];
  unsigned int sliceTuples = rowTuples*outDimensions[1];
  unsigned int volumeTuples = sliceTuples*outDimensions[2];
  
  int inIncrements[3];
  inIncrements[0] = 1;
  inIncrements[1] = inDimensions[0]*inIncrements[0];
  inIncrements[2] = inDimensions[1]*inIncrements[1];
  
  int outIncrements[3];
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
      unsigned int sourceTuple =
        this->GetStartTuple(inExtent, inIncrements,
                            outExtent[0], outExtent[2], outExtent[4]+k);
      unsigned int destTuple =
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
        unsigned int sourceTuple =
          this->GetStartTuple(inExtent, inIncrements,
                              outExtent[0], outExtent[2]+j, outExtent[4]+k);
        unsigned int destTuple =
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
void vtkXMLStructuredDataWriter::WritePrimaryElementAttributes()
{
  this->WriteVectorAttribute("WholeExtent", 6, this->WriteExtent);
}

//----------------------------------------------------------------------------
void vtkXMLStructuredDataWriter::WriteAppendedPiece(int index,
                                                    vtkIndent indent)
{
  // Write the point data and cell data arrays.
  vtkDataSet* input = this->GetInputAsDataSet();  
  this->PointDataOffsets[index] =
    this->WritePointDataAppended(input->GetPointData(), indent);
  this->CellDataOffsets[index] =
    this->WriteCellDataAppended(input->GetCellData(), indent);
}

//----------------------------------------------------------------------------
void vtkXMLStructuredDataWriter::WriteAppendedPieceData(int index)
{  
  // Write the point data and cell data arrays.
  vtkDataSet* input = this->GetInputAsDataSet();  
  this->WritePointDataAppendedData(input->GetPointData(),
                                   this->PointDataOffsets[index]);  
  this->WriteCellDataAppendedData(input->GetCellData(),
                                  this->CellDataOffsets[index]);
}

//----------------------------------------------------------------------------
void vtkXMLStructuredDataWriter::WriteInlinePiece(int, vtkIndent indent)
{
  // Write the point data and cell data arrays.
  vtkDataSet* input = this->GetInputAsDataSet();  
  this->WritePointDataInline(input->GetPointData(), indent);
  this->WriteCellDataInline(input->GetCellData(), indent);
}

//----------------------------------------------------------------------------
unsigned int vtkXMLStructuredDataWriter::GetStartTuple(int* extent,
                                                       int* increments,
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
  this->ExtentTranslator->GetExtent(outExtent);
  return this->CreateExactExtent(inArray, inExtent, outExtent, 1);
}

//----------------------------------------------------------------------------
vtkDataArray*
vtkXMLStructuredDataWriter::CreateArrayForCells(vtkDataArray* inArray)
{
  int inExtent[6];
  int outExtent[6];
  this->GetInputExtent(inExtent);
  this->ExtentTranslator->GetExtent(outExtent);
  return this->CreateExactExtent(inArray, inExtent, outExtent, 0);
}
