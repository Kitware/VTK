/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataStreamer.cxx
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
#include "vtkPolyDataStreamer.h"
#include "vtkAppendPolyData.h"
#include "vtkObjectFactory.h"
#include "vtkFloatArray.h"

vtkCxxRevisionMacro(vtkPolyDataStreamer, "1.12");
vtkStandardNewMacro(vtkPolyDataStreamer);

//----------------------------------------------------------------------------
vtkPolyDataStreamer::vtkPolyDataStreamer()
{
  this->NumberOfStreamDivisions = 2;
  this->ColorByPiece = 0;
}

//----------------------------------------------------------------------------
vtkPolyDataStreamer::~vtkPolyDataStreamer()
{
}

//----------------------------------------------------------------------------
void vtkPolyDataStreamer::SetNumberOfStreamDivisions(int num)
{
  if (this->NumberOfStreamDivisions == num)
    {
    return;
    }

  this->Modified();
  this->NumberOfStreamDivisions = num;
}

//----------------------------------------------------------------------------
void vtkPolyDataStreamer::ComputeInputUpdateExtents(vtkDataObject *output)
{  
  if (this->GetInput() == NULL)
    {
    vtkErrorMacro("Missing input");
    return;
    }

  this->vtkPolyDataSource::ComputeInputUpdateExtents(output);

  // If we are actually streaming, then bypass the normal update process.
  if (this->NumberOfStreamDivisions > 1)
    {
    this->GetInput()->SetUpdateExtent(-1, 0, 0);
    }
}

//----------------------------------------------------------------------------
// Append data sets into single unstructured grid
void vtkPolyDataStreamer::Execute()
{
  vtkPolyData *input = this->GetInput();
  vtkPolyData *output = this->GetOutput();
  vtkPolyData *copy;
  vtkAppendPolyData *append = vtkAppendPolyData::New();
  int outPiece, outNumPieces, outGhost;
  int i, j, inPiece;
  vtkFloatArray *pieceColors = NULL;
  float tmp;

  if (this->ColorByPiece)
    {
    pieceColors = vtkFloatArray::New();
    }

  outGhost = output->GetUpdateGhostLevel();
  outPiece = output->GetUpdatePiece();
  outNumPieces = output->GetUpdateNumberOfPieces();
  for (i = 0; i < this->NumberOfStreamDivisions; ++i)
    {
    inPiece = outPiece * this->NumberOfStreamDivisions + i;
    input->SetUpdateExtent(inPiece,outNumPieces *this->NumberOfStreamDivisions);
    input->Update();
    copy = vtkPolyData::New();
    copy->ShallowCopy(input);
    append->AddInput(copy);
    copy->Delete();
    copy = NULL;
    if (pieceColors)
      {
      for (j = 0; j < input->GetNumberOfCells(); ++j)
        {
        tmp = static_cast<float>(inPiece);
        pieceColors->InsertNextTuple(&tmp);
        }
      }
    }

  append->Update();
  output->ShallowCopy(append->GetOutput());
  // set the piece and number of pieces back to the correct value
  // since the shallow copy of the append filter has overwritten them.
  output->SetUpdateNumberOfPieces(outNumPieces );
  output->SetUpdatePiece(outPiece);
  output->SetUpdateGhostLevel(outGhost);
  if (pieceColors)
    {
    output->GetCellData()->SetScalars(pieceColors);
    pieceColors->Delete();
    }
  append->Delete();
}


//----------------------------------------------------------------------------
void vtkPolyDataStreamer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "NumberOfStreamDivisions: " << this->NumberOfStreamDivisions << endl;
  os << indent << "ColorByPiece: " << this->ColorByPiece << endl;
}



