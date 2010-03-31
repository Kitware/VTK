/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLPolyhedronMeshReader.h"

#include "vtkPolyhedron.h"
#include "vtkCellArray.h"
#include "vtkIdTypeArray.h"
#include "vtkObjectFactory.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLDataElement.h"
#include "vtkInformation.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkCell.h"
#include "vtkGenericCell.h"
#include "vtkSmartPointer.h"

#include <assert.h>

vtkCxxRevisionMacro(vtkXMLPolyhedronMeshReader, "$Revision$");
vtkStandardNewMacro(vtkXMLPolyhedronMeshReader);

//----------------------------------------------------------------------------
vtkXMLPolyhedronMeshReader::vtkXMLPolyhedronMeshReader()
{
  this->FaceElements = 0;
  this->NumberOfFaces = 0;
}

//----------------------------------------------------------------------------
vtkXMLPolyhedronMeshReader::~vtkXMLPolyhedronMeshReader()
{
  if(this->NumberOfPieces)
    {
    this->DestroyPieces();
    }
}

//----------------------------------------------------------------------------
void vtkXMLPolyhedronMeshReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
const char* vtkXMLPolyhedronMeshReader::GetDataSetName()
{
  return "UnstructuredGrid";
}

//----------------------------------------------------------------------------
void vtkXMLPolyhedronMeshReader::SetupPieces(int numPieces)
{
  this->Superclass::SetupPieces(numPieces);
  this->NumberOfFaces = new vtkIdType[numPieces];
  this->FaceElements = new vtkXMLDataElement*[numPieces];
  for(int i=0;i < numPieces; ++i)
    {
    this->FaceElements[i] = 0;
    }
}

//----------------------------------------------------------------------------
int vtkXMLPolyhedronMeshReader::ReadPiece(vtkXMLDataElement* ePiece)
{
  if(!this->Superclass::ReadPiece(ePiece))
    {
    return 0;
    }
  int i;

  if(!ePiece->GetScalarAttribute("NumberOfFaces",
                                 this->NumberOfFaces[this->Piece]))
    {
    vtkErrorMacro("Piece " << this->Piece
                  << " is missing its NumberOfCells attribute.");
    this->NumberOfFaces[this->Piece] = 0;
    return 0;
    }

  // Find the Faces element in the piece.
  this->FaceElements[this->Piece] = 0;
  for(i=0; i < ePiece->GetNumberOfNestedElements(); ++i)
    {
    vtkXMLDataElement* eNested = ePiece->GetNestedElement(i);
    if((strcmp(eNested->GetName(), "Faces") == 0)
       && (eNested->GetNumberOfNestedElements() > 0))
      {
      this->FaceElements[this->Piece] = eNested;
      }
    }

  if(!this->FaceElements[this->Piece])
    {
    vtkErrorMacro("A piece is missing its Cells element.");
    return 0;
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLPolyhedronMeshReader::ReadPieceData()
{
  // The amount of data read by the superclass's ReadPieceData comes
  // from point/cell data and point specifications (we read cell
  // specifications here).
  vtkIdType superclassPieceSize =
    ((this->NumberOfPointArrays+1)*this->GetNumberOfPointsInPiece(this->Piece)+
     this->NumberOfCellArrays*this->GetNumberOfCellsInPiece(this->Piece) +
     3*this->GetNumberOfCellsInPiece(this->Piece));

  // Total amount of data in this piece comes from point/cell data
  // arrays and the point/cell specifications themselves (cell
  // specifications for vtkUnstructuredGrid take three data arrays).
  vtkIdType totalPieceSize =
    superclassPieceSize + 3*this->GetNumberOfCellsInPiece(this->Piece);
  if(totalPieceSize == 0)
    {
    totalPieceSize = 1;
    }

  // Split the progress range based on the approximate fraction of
  // data that will be read by each step in this method.  The cell
  // specification reads two arrays, and then the cell types array is
  // one more.
  float progressRange[2] = {0,0};
  this->GetProgressRange(progressRange);
  float fractions[4] =
    {
      0,
      float(superclassPieceSize) / totalPieceSize,
      ((float(superclassPieceSize) +
        2*this->GetNumberOfCellsInPiece(this->Piece)) / totalPieceSize),
      1
    };

  // Set the range of progress for the superclass.
  this->SetProgressRange(progressRange, 0, fractions);

  // Let the superclass read its data.
  if(!this->Superclass::ReadPieceData())
    {
    return 0;
    }

  vtkUnstructuredGrid* output = vtkUnstructuredGrid::SafeDownCast(
      this->GetCurrentOutput());
  if (!output)
    {
    return 0;
    }
  
  // Save the start location where the new cell connectivity will be
  // appended.
  vtkIdType startLoc = 0;
  if(output->GetCells()->GetData())
    {
    startLoc = output->GetCells()->GetData()->GetNumberOfTuples();
    }

  // Set the range of progress for the cell specifications.
  this->SetProgressRange(progressRange, 1, fractions);

  // Read the Cells.
  vtkXMLDataElement* fCells = this->FaceElements[this->Piece];
  vtkSmartPointer<vtkCellArray> faces = vtkSmartPointer<vtkCellArray>::New();
  
  if(fCells)
    {
    // Read the face array.
    if(!this->ReadCellArray(this->NumberOfCells[this->Piece],
                            this->TotalNumberOfCells,
                            fCells,
                            faces))
      {
      return 0;
      }
    }

  output->SetAllFacesAtOnce(faces);
  
  return 1;
}

