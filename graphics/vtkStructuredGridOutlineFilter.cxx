/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredGridOutlineFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkStructuredGridOutlineFilter.h"

//----------------------------------------------------------------------------
// ComputeDivisionExtents has done most of the work for us.
// Now just connect the points.
void vtkStructuredGridOutlineFilter::Execute()
{
  vtkStructuredGrid *input=this->GetInput();
  int *ext, xInc, yInc, zInc;
  vtkPoints *inPts;
  vtkPoints *newPts;
  vtkCellArray *newLines;
  vtkPolyData *output=this->GetOutput();
  int numPts, idx, offset, ids[2];
  // for marching through the points along an edge.
  int start, num, inc, id;
  
  if (this->ExecutePiece >= 12)
    {
    // thing should not have gotten this far.
    return;
    }
  

  // If all StructuredPointsSources were forced to give you exactly
  // the update extent, this execute method would be trivial.
  // However, Imaging does not have this requirement, and I have not
  // made the readers "StreamingReady" ...
  
  // Find the start of this edge, the length of this edge, and increment.
  ext = input->GetExtent();
  xInc = 1;
  yInc = ext[1]-ext[0]+1;
  zInc = yInc * (ext[3]-ext[2]+1);
  switch (this->ExecutePiece)
    {
    case 0:
      // start (0, 0, 0) increment z axis.
      num = ext[5]-ext[4]+1;
      start = (0-ext[0])*xInc + (0-ext[2])*yInc + (0-ext[4])*zInc;
      inc = zInc;
      break;
    case 1:
      // start (xMax, 0, 0) increment z axis.
      num = ext[5]-ext[4]+1;
      start = (ext[1]-ext[0])*xInc + (0-ext[2])*yInc + (0-ext[4])*zInc;
      inc = zInc;
      break;
    case 2:
      // start (0, yMax, 0) increment z axis.
      num = ext[5]-ext[4]+1;
      start = (0-ext[0])*xInc + (ext[3]-ext[2])*yInc + (0-ext[4])*zInc;
      inc = zInc;
      break;
    case 3:
      // start (xMax, yMax, 0) increment z axis.
      num = ext[5]-ext[4]+1;
      start = (ext[1]-ext[0])*xInc + (ext[3]-ext[2])*yInc + (0-ext[4])*zInc;
      inc = zInc;
      break;
    case 4:
      // start (0, 0, 0) increment y axis.
      num = ext[3]-ext[2]+1;
      start = (0-ext[0])*xInc + (0-ext[2])*yInc + (0-ext[4])*zInc;
      inc = yInc;
      break;
    case 5:
      // start (xMax, 0, 0) increment y axis.
      num = ext[3]-ext[2]+1;
      start = (ext[1]-ext[0])*xInc + (0-ext[2])*yInc + (0-ext[4])*zInc;
      inc = yInc;
      break;
    case 6:
      // start (0, 0, zMax) increment y axis.
      num = ext[3]-ext[2]+1;
      start = (0-ext[0])*xInc + (0-ext[2])*yInc + (ext[5]-ext[4])*zInc;
      inc = yInc;
      break;
    case 7:
      // start (xMax, 0, zMax) increment y axis.
      num = ext[3]-ext[2]+1;
      start = (ext[1]-ext[0])*xInc + (0-ext[2])*yInc + (ext[5]-ext[4])*zInc;
      inc = yInc;
      break;
    case 8:
      // start (0, 0, 0) increment x axis.
      num = ext[1]-ext[0]+1;
      start = (0-ext[0])*xInc + (0-ext[2])*yInc + (0-ext[4])*zInc;
      inc = xInc;
      break;
    case 9:
      // start (0, yMax, 0) increment x axis.
      num = ext[1]-ext[0]+1;
      start = (0-ext[0])*xInc + (ext[3]-ext[2])*yInc + (0-ext[4])*zInc;
      inc = xInc;
      break;
    case 10:
      // start (0, 0, zMax) increment x axis.
      num = ext[1]-ext[0]+1;
      start = (0-ext[0])*xInc + (0-ext[2])*yInc + (ext[5]-ext[4])*zInc;
      inc = xInc;
      break;
    case 11:
      // start (0, yMax, zMax) increment x axis.
      num = ext[1]-ext[0]+1;
      start = (0-ext[0])*xInc + (ext[3]-ext[2])*yInc + (ext[5]-ext[4])*zInc;
      inc = xInc;
      break;
    }
  
  if (num < 2)
    {
    return;
    }

  // these already created in StreamExecuteStart
  newPts = output->GetPoints();
  newLines = output->GetLines();
  offset = newPts->GetNumberOfPoints();
  inPts = input->GetPoints();
  numPts = inPts->GetNumberOfPoints();

  // add points
  for (idx = 0; idx < num; ++idx)
    {
    id = start + idx * inc;
    // sanity check
    if (id < 0 || id >= numPts)
      {
      vtkErrorMacro("Error stepping through points.");
      return;
      }
    newPts->InsertNextPoint(inPts->GetPoint(id));
    }

  // add lines
  for (idx = 1; idx < num; ++idx)
    {
    ids[0] = idx+offset-1;
    ids[1] = idx+offset;
    newLines->InsertNextCell(2, ids);
    }
}



//----------------------------------------------------------------------------
// Always stream into 12 pieces.
int vtkStructuredGridOutlineFilter::GetNumberOfStreamDivisions()
{
  vtkPolyData *output = this->GetOutput();
  int piece, numPieces;
  int start, end;

  output->GetUpdateExtent(piece, numPieces);
  if (piece >= 12)
    { // we do not support more than 12 pieces, so do not execute.
    return 0;
    }

  this->ConvertPiece(piece, numPieces, start, end);
  return (end - start + 1);
}



//----------------------------------------------------------------------------
// Always stream into 12 pieces.
int vtkStructuredGridOutlineFilter::ComputeDivisionExtents(vtkDataObject *out,
						   int idx, int NumDivisions)
{
  vtkStructuredGrid *input = this->GetInput();
  vtkPolyData *output = (vtkPolyData *)out;
  int piece, numPieces;
  int start, end;
  int *ext, xMax, yMax, zMax;

  output->GetUpdateExtent(piece, numPieces);

  // special case: No output for a piece.
  if (piece >= 12)
    {
    return 0;
    }

  // get range of edges for this request.
  this->ConvertPiece(piece, numPieces, start, end);
  
  // GetNumberOfStreamDivisions magical split up request so the
  // total is 12, and we are updating one of these 12.
  piece = start + idx;
  
  // sanity check: Did GetNumberOfStreamDivisions do its job?
  if (piece >= 12 || piece > end)
    {
    vtkErrorMacro("Force 12 divisions did not work.");
    return 0;
    }
  
  // Save the piece so execute can use this information.
  this->ExecutePiece = piece;
  this->ExecuteNumberOfPieces = 12;
  
  ext = input->GetWholeExtent();
  xMax = ext[1];
  yMax = ext[3];
  zMax = ext[5];
  switch (piece)
    {
    case 0:
      input->SetUpdateExtent(0, 0, 0, 0, 0, zMax);
      break;
    case 1:
      input->SetUpdateExtent(xMax, xMax, 0, 0, 0, zMax);
      break;
    case 2:
      input->SetUpdateExtent(0, 0, yMax, yMax, 0, zMax);
      break;
    case 3:
      input->SetUpdateExtent(xMax, xMax, yMax, yMax, 0, zMax);
      break;
    case 4:
      input->SetUpdateExtent(0, 0, 0, yMax, 0, 0);
      break;
    case 5:
      input->SetUpdateExtent(xMax, xMax, 0, yMax, 0, 0);
      break;
    case 6:
      input->SetUpdateExtent(0, 0, 0, yMax, zMax, zMax);
      break;
    case 7:
      input->SetUpdateExtent(xMax, xMax, 0, yMax, zMax, zMax);
      break;
    case 8:
      input->SetUpdateExtent(0, xMax, 0, 0, 0, 0);
      break;
    case 9:
      input->SetUpdateExtent(0, xMax, yMax, yMax, 0, 0);
      break;
    case 10:
      input->SetUpdateExtent(0, xMax, 0, 0, zMax, zMax);
      break;
    case 11:
      input->SetUpdateExtent(0, xMax, yMax, yMax, zMax, zMax);
      break;
    default:
      vtkErrorMacro("Bad piece: This should never have happend.");
      return 0;
    }
  return 1;
}



//----------------------------------------------------------------------------
// Here we need to setup the output polydata.
void vtkStructuredGridOutlineFilter::StreamExecuteStart()
{
  vtkPolyData *output = this->GetOutput();
  vtkCellArray *lines;
  vtkPoints *points;

  lines = vtkCellArray::New();
  output->SetLines(lines);
  lines->Delete();

  points = vtkPoints::New();
  output->SetPoints(points);
  points->Delete();
}



//----------------------------------------------------------------------------
// For estimated size, we are ignoring alot of information....
void vtkStructuredGridOutlineFilter::UpdateInformation()
{
  vtkStructuredGrid *input = this->GetInput();
  vtkPolyData *output = this->GetOutput();
  int *ext;
  long t1, t2;
  long numPts, numLines;
  long sizePt, sizeLine;
  long size;

  input->UpdateInformation();
  
  // for MPI stuff
  output->SetLocality(input->GetLocality() + 1);

  // this portion could be done in superclass.
  t1 = input->GetPipelineMTime();
  t2 = this->GetMTime();
  if (t2 > t1)
    {
    t1 = t2;
    }
  output->SetPipelineMTime(t1);

  // Estimate the size of the output.
  ext = input->GetWholeExtent();
  numPts = 4 * ((ext[1]-ext[0]+1) + (ext[3]-ext[2]+1) + (ext[5]-ext[4]+1));
  numLines = numPts - 12;
  sizePt = 3*sizeof(float);
  sizeLine = 3*sizeof(int);
  // size in kb.
  size = (numPts*sizePt + numLines*sizeLine) / 1000;
  if (size < 1)
    {
    size = 1;
    }
  output->SetEstimatedWholeMemorySize(size);
}







//----------------------------------------------------------------------------
// Since this filter produces 12 pieces (no more and no less) we need to 
// convert the PieceOfNum into a range of 12.
void vtkStructuredGridOutlineFilter::ConvertPiece(int piece, int numPieces,
                                                  int &start, int &end)
{
  if (numPieces >= 12)
    {
    // start and end may be equal to larger than twelve. (empty pieces)
    start = end = piece;
    return;
    }
  start = piece * 12 / numPieces;
  end = ((piece+1) * 12 / numPieces) - 1;
}















