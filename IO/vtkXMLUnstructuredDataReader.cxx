/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLUnstructuredDataReader.cxx
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
#include "vtkXMLUnstructuredDataReader.h"
#include "vtkObjectFactory.h"
#include "vtkXMLDataElement.h"
#include "vtkPoints.h"
#include "vtkIdTypeArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkCellArray.h"
#include "vtkPointSet.h"

vtkCxxRevisionMacro(vtkXMLUnstructuredDataReader, "1.7");

//----------------------------------------------------------------------------
vtkXMLUnstructuredDataReader::vtkXMLUnstructuredDataReader()
{  
  this->PointElements = 0;
  this->NumberOfPoints = 0;
  this->TotalNumberOfPoints = 0;
  this->TotalNumberOfCells = 0;
}

//----------------------------------------------------------------------------
vtkXMLUnstructuredDataReader::~vtkXMLUnstructuredDataReader()
{
  if(this->NumberOfPieces) { this->DestroyPieces(); }
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredDataReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkPointSet* vtkXMLUnstructuredDataReader::GetOutputAsPointSet()
{
  if(this->NumberOfOutputs < 1)
    {
    return 0;
    }
  return static_cast<vtkPointSet*>(this->Outputs[0]);  
}

//----------------------------------------------------------------------------
vtkXMLDataElement*
vtkXMLUnstructuredDataReader
::FindDataArrayWithName(vtkXMLDataElement* eParent, const char* name)
{
  // Find a nested element that represents a data array with the given name.
  int i;
  for(i=0;i < eParent->GetNumberOfNestedElements(); ++i)
    {
    vtkXMLDataElement* eNested = eParent->GetNestedElement(i);
    if(strcmp(eNested->GetName(), "DataArray") == 0)
      {
      const char* aName = eNested->GetAttribute("Name");
      if(aName && (strcmp(aName, name) == 0))
        {
        return eNested;
        }
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
template <class TIn, class TOut>
void vtkXMLUnstructuredDataReaderCopyArray(TIn* in, TOut* out,
                                           vtkIdType length)
{
  for(vtkIdType i = 0; i < length; ++i)
    {
    out[i] = static_cast<TOut>(in[i]);
    }
}

//----------------------------------------------------------------------------
vtkIdTypeArray*
vtkXMLUnstructuredDataReader
::ConvertToIdTypeArray(vtkDataArray* a)
{
  // If it is already a vtkIdTypeArray, just return it.
  vtkIdTypeArray* ida = vtkIdTypeArray::SafeDownCast(a);
  if(ida)
    {
    return ida;
    }
  
  // Need to convert the data.
  ida = vtkIdTypeArray::New();
  ida->SetNumberOfComponents(a->GetNumberOfComponents());
  ida->SetNumberOfTuples(a->GetNumberOfTuples());
  vtkIdType length = a->GetNumberOfComponents() * a->GetNumberOfTuples();
  vtkIdType* idBuffer = ida->GetPointer(0);
  switch (a->GetDataType())
    {
    vtkTemplateMacro3(vtkXMLUnstructuredDataReaderCopyArray,
                      static_cast<VTK_TT*>(a->GetVoidPointer(0)),
                      idBuffer, length);
    default:
      vtkErrorMacro("Cannot convert vtkDataArray of type " << a->GetDataType()
                    << " to vtkIdTypeArray.");
      ida->Delete();
      ida = 0;
    }
  a->Delete();
  return ida;
}

//----------------------------------------------------------------------------
vtkUnsignedCharArray*
vtkXMLUnstructuredDataReader
::ConvertToUnsignedCharArray(vtkDataArray* a)
{
  // If it is already a vtkUnsignedCharArray, just return it.
  vtkUnsignedCharArray* uca = vtkUnsignedCharArray::SafeDownCast(a);
  if(uca)
    {
    return uca;
    }
  
  // Need to convert the data.
  uca = vtkUnsignedCharArray::New();
  uca->SetNumberOfComponents(a->GetNumberOfComponents());
  uca->SetNumberOfTuples(a->GetNumberOfTuples());
  vtkIdType length = a->GetNumberOfComponents() * a->GetNumberOfTuples();
  unsigned char* ucBuffer = uca->GetPointer(0);
  switch (a->GetDataType())
    {
    vtkTemplateMacro3(vtkXMLUnstructuredDataReaderCopyArray,
                      static_cast<VTK_TT*>(a->GetVoidPointer(0)),
                      ucBuffer, length);
    default:
      vtkErrorMacro("Cannot convert vtkDataArray of type " << a->GetDataType()
                    << " to vtkUnsignedCharArray.");
      uca->Delete();
      uca = 0;
    }
  a->Delete();
  return uca;
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredDataReader::SetupEmptyOutput()
{
  // No pieces means no input.
  this->GetOutputAsDataSet()->SetUpdateExtent(0, 0);
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredDataReader::SetupOutputTotals()
{
  int i;
  this->TotalNumberOfPoints = 0;
  for(i=this->StartPiece; i < this->EndPiece; ++i)
    {
    this->TotalNumberOfPoints += this->NumberOfPoints[i];
    }  
  this->StartPoint = 0;
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredDataReader::SetupNextPiece()
{
  this->StartPoint += this->NumberOfPoints[this->Piece];
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredDataReader::SetupUpdateExtent(int piece,
                                                     int numberOfPieces,
                                                     int ghostLevel)
{
  this->UpdatePiece = piece;
  this->UpdateNumberOfPieces = numberOfPieces;
  this->UpdateGhostLevel = ghostLevel;
  
  // If more pieces are requested than available, just return empty
  // pieces for the extra ones.
  if(this->UpdateNumberOfPieces > this->NumberOfPieces)
    {
    this->UpdateNumberOfPieces = this->NumberOfPieces;
    }
  
  // Find the range of pieces to read.
  if(this->UpdatePiece < this->UpdateNumberOfPieces)
    {
    this->StartPiece = ((this->UpdatePiece*this->NumberOfPieces) /
                        this->UpdateNumberOfPieces);
    this->EndPiece = (((this->UpdatePiece+1)*this->NumberOfPieces) /
                      this->UpdateNumberOfPieces);
    }
  else
    {
    this->StartPiece = 0;
    this->EndPiece = 0;
    }
  
  // Find the total size of the output.
  this->SetupOutputTotals(); 
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredDataReader::ReadXMLData()
{  
  // Get the update request.
  int piece;
  int numberOfPieces;
  int ghostLevel;
  this->GetOutputUpdateExtent(piece, numberOfPieces, ghostLevel);
  
  vtkDebugMacro("Updating piece " << piece << " of " << numberOfPieces
                << " with ghost level " << ghostLevel);
  
  // Setup the range of pieces that will be read.
  this->SetupUpdateExtent(piece, numberOfPieces, ghostLevel);
  
  // If there are no data to read, stop now.
  if(this->StartPiece == this->EndPiece)
    {
    return;
    }
  
  vtkDebugMacro("Reading piece range [" << this->StartPiece
                << ", " << this->EndPiece << ") from file.");  
  
  // Let superclasses read data.  This also allocates output data.
  this->Superclass::ReadXMLData();  
  
  // Split current progress range based on fraction contributed by
  // each piece.
  float progressRange[2] = {0,0};
  this->GetProgressRange(progressRange);
  
  // Calculate the cumulative fraction of data contributed by each
  // piece (for progress).
  float* fractions = new float[this->EndPiece-this->StartPiece+1];
  int i;
  fractions[0] = 0;
  for(i=this->StartPiece; i < this->EndPiece; ++i)
    {
    int index = i-this->StartPiece;
    fractions[index+1] = (fractions[index] +
                          this->GetNumberOfPointsInPiece(i) + 
                          this->GetNumberOfCellsInPiece(i));
    }
  for(i=this->StartPiece; i < this->EndPiece; ++i)
    {
    int index = i-this->StartPiece;
    fractions[index+1] = fractions[index+1] / fractions[this->EndPiece-this->StartPiece];
    }
  
  // Read the data needed from each piece.
  for(i=this->StartPiece; i < this->EndPiece; ++i)
    {
    // Set the range of progress for this piece.
    this->SetProgressRange(progressRange, i-this->StartPiece, fractions);
    
    if(!this->Superclass::ReadPieceData(i))
      {
      // An error occurred while reading the piece.
      this->DataError = 1;
      }
    this->SetupNextPiece();
    }
  
  delete [] fractions;
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredDataReader::SetupPieces(int numPieces)
{
  this->Superclass::SetupPieces(numPieces);
  this->NumberOfPoints = new vtkIdType[numPieces];
  this->PointElements = new vtkXMLDataElement*[numPieces];
  int i;
  for(i=0;i < numPieces; ++i)
    {
    this->PointElements[i] = 0;
    this->NumberOfPoints[i] = 0;
    }
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredDataReader::DestroyPieces()
{
  delete [] this->PointElements;
  delete [] this->NumberOfPoints;
  this->PointElements = 0;
  this->NumberOfPoints = 0;
  this->Superclass::DestroyPieces();
}

//----------------------------------------------------------------------------
vtkIdType vtkXMLUnstructuredDataReader::GetNumberOfPoints()
{
  return this->TotalNumberOfPoints;
}

//----------------------------------------------------------------------------
vtkIdType vtkXMLUnstructuredDataReader::GetNumberOfCells()
{
  return this->TotalNumberOfCells;
}

//----------------------------------------------------------------------------
vtkIdType vtkXMLUnstructuredDataReader::GetNumberOfPointsInPiece(int piece)
{
  return this->NumberOfPoints[piece];
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredDataReader::SetupOutputInformation()
{
  this->Superclass::SetupOutputInformation();
  
  vtkPointSet* output = this->GetOutputAsPointSet();
  
  // Set the maximum number of pieces that can be provided by this
  // reader.
  output->SetMaximumNumberOfPieces(this->NumberOfPieces);
  
  // Create the points array.
  vtkPoints* points = vtkPoints::New();
  
  // Use the configuration of the first piece since all are the same.
  vtkXMLDataElement* ePoints = this->PointElements[0];
  if(ePoints)
    {
    // Non-zero volume.
    vtkDataArray* a = this->CreateDataArray(ePoints->GetNestedElement(0));
    if(a)
      {
      points->SetData(a);
      a->Delete();
      }
    else
      {
      this->InformationError = 1;
      }
    }
  
  output->SetPoints(points);
  points->Delete();
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredDataReader::SetupOutputData()
{
  this->Superclass::SetupOutputData();
  
  // Allocate the points array.
  vtkPointSet* output = this->GetOutputAsPointSet();
  output->GetPoints()->GetData()->SetNumberOfTuples(this->GetNumberOfPoints());
}

//----------------------------------------------------------------------------
int vtkXMLUnstructuredDataReader::ReadPiece(vtkXMLDataElement* ePiece)
{
  if(!this->Superclass::ReadPiece(ePiece)) { return 0; }
  
  if(!ePiece->GetScalarAttribute("NumberOfPoints",
                                 this->NumberOfPoints[this->Piece]))
    {
    vtkErrorMacro("Piece " << this->Piece
                  << " is missing its NumberOfPoints attribute.");
    this->NumberOfPoints[this->Piece] = 0;
    return 0;
    }
  
  // Find the Points element in the piece.
  int i;
  this->PointElements[this->Piece] = 0;
  for(i=0; i < ePiece->GetNumberOfNestedElements(); ++i)
    {
    vtkXMLDataElement* eNested = ePiece->GetNestedElement(i);
    if((strcmp(eNested->GetName(), "Points") == 0)
       && (eNested->GetNumberOfNestedElements() == 1))
      {
      this->PointElements[this->Piece] = eNested;
      }
    }
  
  // If there are some points, we require a Points element.
  if(!this->PointElements[this->Piece] &&
     (this->NumberOfPoints[this->Piece] > 0))
    {
    vtkErrorMacro("A piece is missing its Points element "
                  "or element does not have exactly 1 array.");
    return 0;
    }  
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLUnstructuredDataReader::ReadPieceData()
{
  // The amount of data read by the superclass's ReadPieceData comes
  // from point/cell data (we read point specifications here).
  vtkIdType superclassPieceSize =
    (this->NumberOfPointArrays*this->GetNumberOfPointsInPiece(this->Piece)+
     this->NumberOfCellArrays*this->GetNumberOfCellsInPiece(this->Piece));
  
  // Total amount of data in this piece comes from point/cell data
  // arrays and the point specifications themselves.
  vtkIdType totalPieceSize =
    superclassPieceSize + 1*this->GetNumberOfPointsInPiece(this->Piece);
  if(totalPieceSize == 0)
    {
    totalPieceSize = 1;
    }
  
  // Split the progress range based on the approximate fraction of
  // data that will be read by each step in this method.
  float progressRange[2] = {0,0};
  this->GetProgressRange(progressRange);
  float fractions[3] =
    {
      0,
      float(superclassPieceSize)/totalPieceSize,
      1
    };
  
  // Set the range of progress for the superclass.
  this->SetProgressRange(progressRange, 0, fractions);
  
  // Let the superclass read its data.
  if(!this->Superclass::ReadPieceData()) { return 0; }
  
  vtkPointSet* output = this->GetOutputAsPointSet();
  
  // Set the range of progress for the Points.
  this->SetProgressRange(progressRange, 1, fractions);
  
  // Read the points array.
  vtkXMLDataElement* ePoints = this->PointElements[this->Piece];
  if(ePoints)
    {
    if(!this->ReadArrayForPoints(ePoints->GetNestedElement(0),
                                 output->GetPoints()->GetData()))
      {
      return 0;
      }
    }
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLUnstructuredDataReader::ReadCellArray(vtkIdType numberOfCells,
                                                vtkIdType totalNumberOfCells,
                                                vtkXMLDataElement* eCells,
                                                vtkCellArray* outCells)
{
  if(numberOfCells <= 0)
    {
    return 1;
    }
  else 
    {
    if(!eCells)
      {
      return 0;
      }
    }
  
  // Split progress range into 1/5 for offsets array and 4/5 for
  // connectivity array.  This assumes an average of 4 points per
  // cell.  Unfortunately, we cannot know the length of the
  // connectivity array ahead of time to calculate the real fraction.
  float progressRange[2] = {0,0};
  this->GetProgressRange(progressRange);
  float fractions[3] = {0, 0.2, 1};
  
  // Set range of progress for offsets array.
  this->SetProgressRange(progressRange, 0, fractions);
  
  // Read the cell offsets.
  vtkXMLDataElement* eOffsets = this->FindDataArrayWithName(eCells, "offsets");
  if(!eOffsets)
    {
    vtkErrorMacro("Cannot read cell offsets from " << eCells->GetName()
                  << " in piece " << this->Piece
                  << " because the \"offsets\" array could not be found.");
    return 0;
    }
  vtkDataArray* c1 = this->CreateDataArray(eOffsets);
  if(!c1 || (c1->GetNumberOfComponents() != 1))
    {
    vtkErrorMacro("Cannot read cell offsets from " << eCells->GetName()
                  << " in piece " << this->Piece
                  << " because the \"offsets\" array could not be created"
                  << " with one component.");
    return 0;
    }
  c1->SetNumberOfTuples(numberOfCells);
  if(!this->ReadData(eOffsets, c1->GetVoidPointer(0), c1->GetDataType(),
                     0, numberOfCells))
    {
    vtkErrorMacro("Cannot read cell offsets from " << eCells->GetName()
                  << " in piece " << this->Piece
                  << " because the \"offsets\" array is not long enough.");
    return 0;
    }  
  vtkIdTypeArray* cellOffsets = this->ConvertToIdTypeArray(c1);
  if(!cellOffsets)
    {
    vtkErrorMacro("Cannot read cell offsets from " << eCells->GetName()
                  << " in piece " << this->Piece
                  << " because the \"offsets\" array could not be"
                  << " converted to a vtkIdTypeArray.");
    return 0;
    }
  
  // Set range of progress for connectivity array.
  this->SetProgressRange(progressRange, 1, fractions);
  
  // Read the cell point connectivity array.
  vtkIdType cpLength = cellOffsets->GetValue(numberOfCells-1);
  vtkXMLDataElement* eConn = this->FindDataArrayWithName(eCells, "connectivity");
  if(!eConn)
    {
    vtkErrorMacro("Cannot read cell connectivity from " << eCells->GetName()
                  << " in piece " << this->Piece
                  << " because the \"connectivity\" array could not be found.");
    cellOffsets->Delete();
    return 0;
    }
  vtkDataArray* c0 = this->CreateDataArray(eConn);
  if(!c0 || (c0->GetNumberOfComponents() != 1))
    {
    vtkErrorMacro("Cannot read cell connectivity from " << eCells->GetName()
                  << " in piece " << this->Piece
                  << " because the \"connectivity\" array could not be created"
                  << " with one component.");
    cellOffsets->Delete();
    return 0;
    }
  c0->SetNumberOfTuples(cpLength);
  if(!this->ReadData(eConn, c0->GetVoidPointer(0), c0->GetDataType(),
                     0, cpLength))
    {
    vtkErrorMacro("Cannot read cell connectivity from " << eCells->GetName()
                  << " in piece " << this->Piece
                  << " because the \"connectivity\" array is not long enough.");
    cellOffsets->Delete();
    return 0;
    }
  vtkIdTypeArray* cellPoints = this->ConvertToIdTypeArray(c0);
  if(!cellPoints)
    {
    vtkErrorMacro("Cannot read cell connectivity from " << eCells->GetName()
                  << " in piece " << this->Piece
                  << " because the \"connectivity\" array could not be"
                  << " converted to a vtkIdTypeArray.");
    cellOffsets->Delete();
    return 0;
    }
  
  // Allocate memory in the output connectivity array.
  vtkIdType curSize = 0;
  if(outCells->GetData())
    {
    curSize = outCells->GetData()->GetNumberOfTuples();
    }
  vtkIdType newSize = curSize+numberOfCells+cellPoints->GetNumberOfTuples();
  vtkIdType* cptr = outCells->WritePointer(totalNumberOfCells, newSize);
  cptr += curSize;
  
  // Copy the connectivity data.
  vtkIdType i;
  vtkIdType previousOffset = 0;
  for(i=0; i < numberOfCells; ++i)
    {
    vtkIdType length = cellOffsets->GetValue(i)-previousOffset;
    *cptr++ = length;
    vtkIdType* sptr = cellPoints->GetPointer(previousOffset);
    // Copy the point indices, but increment them for the appended
    // version's index.
    vtkIdType j;
    for(j=0;j < length; ++j)
      {
      cptr[j] = sptr[j]+this->StartPoint;
      }
    cptr += length;
    previousOffset += length;
    }
  
  cellPoints->Delete();
  cellOffsets->Delete();
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLUnstructuredDataReader::ReadArrayForPoints(vtkXMLDataElement* da,
                                                     vtkDataArray* outArray)
{
  vtkIdType startPoint = this->StartPoint;
  vtkIdType numPoints = this->NumberOfPoints[this->Piece];  
  vtkIdType components = outArray->GetNumberOfComponents();
  return this->ReadData(da, outArray->GetVoidPointer(startPoint*components),
                        outArray->GetDataType(), 0, numPoints*components);
}
