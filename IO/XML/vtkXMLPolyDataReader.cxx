/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPolyDataReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLPolyDataReader.h"
#include "vtkObjectFactory.h"
#include "vtkXMLDataElement.h"
#include "vtkPolyData.h"
#include "vtkIdTypeArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkCellArray.h"
#include "vtkInformation.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <cassert>

vtkStandardNewMacro(vtkXMLPolyDataReader);

//----------------------------------------------------------------------------
vtkXMLPolyDataReader::vtkXMLPolyDataReader()
{
  this->VertElements = 0;
  this->LineElements = 0;
  this->StripElements = 0;
  this->PolyElements = 0;
  this->TotalNumberOfVerts = 0;
  this->TotalNumberOfLines = 0;
  this->TotalNumberOfStrips = 0;
  this->TotalNumberOfPolys = 0;

  // TimeStep
  this->VertsTimeStep = -1;
  this->VertsOffset = static_cast<unsigned long>(-1);
  this->LinesTimeStep = -1;
  this->LinesOffset = static_cast<unsigned long>(-1);
  this->StripsTimeStep = -1;
  this->StripsOffset = static_cast<unsigned long>(-1);
  this->PolysTimeStep = -1;
  this->PolysOffset = static_cast<unsigned long>(-1);
}

//----------------------------------------------------------------------------
vtkXMLPolyDataReader::~vtkXMLPolyDataReader()
{
  if(this->NumberOfPieces)
    {
    this->DestroyPieces();
    }
}

//----------------------------------------------------------------------------
void vtkXMLPolyDataReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkPolyData* vtkXMLPolyDataReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
vtkPolyData* vtkXMLPolyDataReader::GetOutput(int idx)
{
  return vtkPolyData::SafeDownCast( this->GetOutputDataObject(idx) );
}

//----------------------------------------------------------------------------
vtkIdType vtkXMLPolyDataReader::GetNumberOfVerts()
{
  return this->TotalNumberOfVerts;
}

//----------------------------------------------------------------------------
vtkIdType vtkXMLPolyDataReader::GetNumberOfLines()
{
  return this->TotalNumberOfLines;
}

//----------------------------------------------------------------------------
vtkIdType vtkXMLPolyDataReader::GetNumberOfStrips()
{
  return this->TotalNumberOfStrips;
}

//----------------------------------------------------------------------------
vtkIdType vtkXMLPolyDataReader::GetNumberOfPolys()
{
  return this->TotalNumberOfPolys;
}

//----------------------------------------------------------------------------
const char* vtkXMLPolyDataReader::GetDataSetName()
{
  return "PolyData";
}

//----------------------------------------------------------------------------
void vtkXMLPolyDataReader::GetOutputUpdateExtent(int& piece,
                                                 int& numberOfPieces,
                                                 int& ghostLevel)
{
  vtkInformation* outInfo = this->GetCurrentOutputInformation();
  piece = outInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  numberOfPieces= outInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  ghostLevel = outInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());
}

//----------------------------------------------------------------------------
void vtkXMLPolyDataReader::SetupOutputTotals()
{
  this->Superclass::SetupOutputTotals();
  // Find the total size of the output.
  int i;
  this->TotalNumberOfCells = 0;
  this->TotalNumberOfVerts = 0;
  this->TotalNumberOfLines = 0;
  this->TotalNumberOfStrips = 0;
  this->TotalNumberOfPolys = 0;
  for(i=this->StartPiece; i < this->EndPiece; ++i)
    {
    this->TotalNumberOfCells += (this->NumberOfVerts[i] +
                                 this->NumberOfLines[i] +
                                 this->NumberOfStrips[i] +
                                 this->NumberOfPolys[i]);
    this->TotalNumberOfVerts += this->NumberOfVerts[i];
    this->TotalNumberOfLines += this->NumberOfLines[i];
    this->TotalNumberOfStrips += this->NumberOfStrips[i];
    this->TotalNumberOfPolys += this->NumberOfPolys[i];
    }

  // Data reading will start at the beginning of the output.
  this->StartVert = 0;
  this->StartLine = 0;
  this->StartStrip = 0;
  this->StartPoly = 0;
}

//----------------------------------------------------------------------------
void vtkXMLPolyDataReader::SetupPieces(int numPieces)
{
  this->Superclass::SetupPieces(numPieces);
  this->NumberOfVerts = new vtkIdType[numPieces];
  this->NumberOfLines = new vtkIdType[numPieces];
  this->NumberOfStrips = new vtkIdType[numPieces];
  this->NumberOfPolys = new vtkIdType[numPieces];
  this->VertElements = new vtkXMLDataElement*[numPieces];
  this->LineElements = new vtkXMLDataElement*[numPieces];
  this->StripElements = new vtkXMLDataElement*[numPieces];
  this->PolyElements = new vtkXMLDataElement*[numPieces];
  for(int i=0;i < numPieces; ++i)
    {
    this->VertElements[i] = 0;
    this->LineElements[i] = 0;
    this->StripElements[i] = 0;
    this->PolyElements[i] = 0;
    }
}

//----------------------------------------------------------------------------
void vtkXMLPolyDataReader::DestroyPieces()
{
  delete [] this->PolyElements;
  delete [] this->StripElements;
  delete [] this->LineElements;
  delete [] this->VertElements;
  delete [] this->NumberOfPolys;
  delete [] this->NumberOfStrips;
  delete [] this->NumberOfLines;
  delete [] this->NumberOfVerts;
  this->Superclass::DestroyPieces();
}

//----------------------------------------------------------------------------
vtkIdType vtkXMLPolyDataReader::GetNumberOfCellsInPiece(int piece)
{
  return (this->NumberOfVerts[piece]+
          this->NumberOfLines[piece]+
          this->NumberOfStrips[piece]+
          this->NumberOfPolys[piece]);
}

//----------------------------------------------------------------------------
void vtkXMLPolyDataReader::SetupOutputData()
{
  this->Superclass::SetupOutputData();
  vtkPolyData* output = vtkPolyData::SafeDownCast(this->GetCurrentOutput());

  // Setup the output's cell arrays.
  vtkCellArray* outVerts = vtkCellArray::New();
  vtkCellArray* outLines = vtkCellArray::New();
  vtkCellArray* outStrips = vtkCellArray::New();
  vtkCellArray* outPolys = vtkCellArray::New();

  output->SetVerts(outVerts);
  output->SetLines(outLines);
  output->SetStrips(outStrips);
  output->SetPolys(outPolys);

  outPolys->Delete();
  outStrips->Delete();
  outLines->Delete();
  outVerts->Delete();
}

//----------------------------------------------------------------------------
int vtkXMLPolyDataReader::ReadPiece(vtkXMLDataElement* ePiece)
{
  if(!this->Superclass::ReadPiece(ePiece))
    {
    return 0;
    }

  if(!ePiece->GetScalarAttribute("NumberOfVerts",
                                 this->NumberOfVerts[this->Piece]))
    {
    this->NumberOfVerts[this->Piece] = 0;
    }
  if(!ePiece->GetScalarAttribute("NumberOfLines",
                                 this->NumberOfLines[this->Piece]))
    {
    this->NumberOfLines[this->Piece] = 0;
    }
  if(!ePiece->GetScalarAttribute("NumberOfStrips",
                                 this->NumberOfStrips[this->Piece]))
    {
    this->NumberOfStrips[this->Piece] = 0;
    }
  if(!ePiece->GetScalarAttribute("NumberOfPolys",
                                 this->NumberOfPolys[this->Piece]))
    {
    this->NumberOfPolys[this->Piece] = 0;
    }

  // Find the cell elements in the piece.
  int i;
  for(i=0; i < ePiece->GetNumberOfNestedElements(); ++i)
    {
    vtkXMLDataElement* eNested = ePiece->GetNestedElement(i);
    if((strcmp(eNested->GetName(), "Verts") == 0)
       && (eNested->GetNumberOfNestedElements() > 1))
      {
      this->VertElements[this->Piece] = eNested;
      }
    if((strcmp(eNested->GetName(), "Lines") == 0)
       && (eNested->GetNumberOfNestedElements() > 1))
      {
      this->LineElements[this->Piece] = eNested;
      }
    if((strcmp(eNested->GetName(), "Strips") == 0)
       && (eNested->GetNumberOfNestedElements() > 1))
      {
      this->StripElements[this->Piece] = eNested;
      }
    if((strcmp(eNested->GetName(), "Polys") == 0)
       && (eNested->GetNumberOfNestedElements() > 1))
      {
      this->PolyElements[this->Piece] = eNested;
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLPolyDataReader::SetupNextPiece()
{
  this->Superclass::SetupNextPiece();
  this->StartVert += this->NumberOfVerts[this->Piece];
  this->StartLine += this->NumberOfLines[this->Piece];
  this->StartStrip += this->NumberOfStrips[this->Piece];
  this->StartPoly += this->NumberOfPolys[this->Piece];
}

//----------------------------------------------------------------------------
int vtkXMLPolyDataReader::ReadPieceData()
{
  // The amount of data read by the superclass's ReadPieceData comes
  // from point/cell data and point specifications (we read cell
  // specifications here).
  vtkIdType superclassPieceSize =
    ((this->NumberOfPointArrays+1)*this->GetNumberOfPointsInPiece(this->Piece)+
     this->NumberOfCellArrays*this->GetNumberOfCellsInPiece(this->Piece));

  // Total amount of data in this piece comes from point/cell data
  // arrays and the point/cell specifications themselves (cell
  // specifications for vtkPolyData take two data arrays split across
  // cell types).
  vtkIdType totalPieceSize =
    superclassPieceSize + 2*this->GetNumberOfCellsInPiece(this->Piece);
  if(totalPieceSize == 0)
    {
    totalPieceSize = 1;
    }

  // Split the progress range based on the approximate fraction of
  // data that will be read by each step in this method.
  float progressRange[2] = {0,0};
  this->GetProgressRange(progressRange);
  float fractions[6] =
    {
      0,
      float(superclassPieceSize) / totalPieceSize,
      (float(superclassPieceSize)+
       this->NumberOfVerts[this->Piece]) / totalPieceSize,
      (float(superclassPieceSize)+
       this->NumberOfVerts[this->Piece]+
       this->NumberOfLines[this->Piece]) / totalPieceSize,
      (float(superclassPieceSize)+
       this->NumberOfVerts[this->Piece]+
       this->NumberOfLines[this->Piece]+
       this->NumberOfStrips[this->Piece]) / totalPieceSize,
      1
    };

  // Set the range of progress for the superclass.
  this->SetProgressRange(progressRange, 0, fractions);

  // Let the superclass read its data.
  if(!this->Superclass::ReadPieceData())
    {
    return 0;
    }

  vtkPolyData* output = vtkPolyData::SafeDownCast(this->GetCurrentOutput());

  // Set the range of progress for the Verts.
  this->SetProgressRange(progressRange, 1, fractions);

  // Read the Verts.
  vtkXMLDataElement* eVerts = this->VertElements[this->Piece];
  if(eVerts)
    {
//    int needToRead = this->CellsNeedToReadTimeStep(eNested,
//      this->VertsTimeStep, this->VertsOffset);
//    if( needToRead )
      {
      // Read the array.
      if(!this->ReadCellArray(this->NumberOfVerts[this->Piece],
                              this->TotalNumberOfVerts,
                              eVerts,
                              output->GetVerts()))
        {
        return 0;
        }
      }
    }

  // Set the range of progress for the Lines.
  this->SetProgressRange(progressRange, 2, fractions);

  // Read the Lines.
  vtkXMLDataElement* eLines = this->LineElements[this->Piece];
  if(eLines)
    {
//    int needToRead = this->CellsNeedToReadTimeStep(eNested,
//      this->LinesTimeStep, this->LinesOffset);
//    if( needToRead )
      {
      // Read the array.
      if(!this->ReadCellArray(this->NumberOfLines[this->Piece],
                              this->TotalNumberOfLines,
                              eLines,
                              output->GetLines()))
        {
        return 0;
        }
      }
    }

  // Set the range of progress for the Strips.
  this->SetProgressRange(progressRange, 3, fractions);

  // Read the Strips.
  vtkXMLDataElement* eStrips = this->StripElements[this->Piece];
  if(eStrips)
    {
//    int needToRead = this->CellsNeedToReadTimeStep(eNested,
//      this->StripsTimeStep, this->StripsOffset);
//    if( needToRead )
      {
      // Read the array.
      if(!this->ReadCellArray(this->NumberOfStrips[this->Piece],
                              this->TotalNumberOfStrips,
                              eStrips,
                              output->GetStrips()))
        {
        return 0;
        }
      }
    }

  // Set the range of progress for the Polys.
  this->SetProgressRange(progressRange, 4, fractions);

  // Read the Polys.
  vtkXMLDataElement* ePolys = this->PolyElements[this->Piece];
  if(ePolys)
    {
//    int needToRead = this->CellsNeedToReadTimeStep(eNested,
//      this->PolysTimeStep, this->PolysOffset);
//    if( needToRead )
      {
      // Read the array.
      if(!this->ReadCellArray(this->NumberOfPolys[this->Piece],
                              this->TotalNumberOfPolys,
                              ePolys,
                              output->GetPolys()))
        {
        return 0;
        }
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLPolyDataReader::ReadArrayForCells(vtkXMLDataElement* da,
                                            vtkAbstractArray* outArray)
{
  // Split progress range according to the fraction of data that will
  // be read for each type of cell.
  float progressRange[2] = {0,0};
  this->GetProgressRange(progressRange);
  int total = this->TotalNumberOfCells?this->TotalNumberOfCells:1;
  float fractions[5] =
    {
      0,
      float(this->NumberOfVerts[this->Piece])/total,
      float(this->NumberOfVerts[this->Piece]+
            this->NumberOfLines[this->Piece])/total,
      float(this->NumberOfVerts[this->Piece]+
            this->NumberOfLines[this->Piece]+
            this->NumberOfStrips[this->Piece])/total,
      1
    };

  vtkIdType components = outArray->GetNumberOfComponents();

  // Set range of progress for the Verts.
  this->SetProgressRange(progressRange, 0, fractions);

  // Read the cell data for the Verts in the piece.
  vtkIdType inStartCell = 0;
  vtkIdType outStartCell = this->StartVert;
  vtkIdType numCells = this->NumberOfVerts[this->Piece];
  if(!this->ReadArrayValues(da, outStartCell*components, outArray,
      inStartCell*components, numCells*components))
    {
    return 0;
    }

  // Set range of progress for the Lines.
  this->SetProgressRange(progressRange, 1, fractions);

  // Read the cell data for the Lines in the piece.
  inStartCell += numCells;
  outStartCell = this->TotalNumberOfVerts + this->StartLine;
  numCells = this->NumberOfLines[this->Piece];
  if(!this->ReadArrayValues(da, outStartCell*components, outArray,
      inStartCell*components, numCells*components))
    {
    return 0;
    }

  // Set range of progress for the Strips.
  this->SetProgressRange(progressRange, 2, fractions);

  // Read the cell data for the Strips in the piece.
  inStartCell += numCells;
  outStartCell = (this->TotalNumberOfVerts + this->TotalNumberOfLines +
                  this->StartStrip);
  numCells = this->NumberOfStrips[this->Piece];
  if(!this->ReadArrayValues(da, outStartCell*components, outArray,
      inStartCell*components, numCells*components))
    {
    return 0;
    }

  // Set range of progress for the Polys.
  this->SetProgressRange(progressRange, 3, fractions);

  // Read the cell data for the Polys in the piece.
  inStartCell += numCells;
  outStartCell = (this->TotalNumberOfVerts + this->TotalNumberOfLines +
                  this->TotalNumberOfStrips + this->StartPoly);
  numCells = this->NumberOfPolys[this->Piece];
  if(!this->ReadArrayValues(da, outStartCell*components, outArray,
      inStartCell*components, numCells*components))
    {
    return 0;
    }

  return 1;
}


//----------------------------------------------------------------------------
int vtkXMLPolyDataReader::FillOutputPortInformation(int,
                                                 vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
  return 1;
}
