/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredGridOutlineFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkStructuredGridOutlineFilter.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStructuredGridOutlineFilter* vtkStructuredGridOutlineFilter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkStructuredGridOutlineFilter");
  if(ret)
    {
    return (vtkStructuredGridOutlineFilter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkStructuredGridOutlineFilter;
}

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
  int idx;
  vtkIdType ids[2], numPts, offset;
  // for marching through the points along an edge.
  vtkIdType start = 0, id;
  int num = 0, inc = 0;

  for ( int i = 0; i < 12; i++ )
    {
    this->ComputeDivisionExtents( output, i, 12 );

    if ( i == 0 )
      {
      this->StreamExecuteStart();
      }

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
}

//----------------------------------------------------------------------------
// Always stream into 12 pieces.
int vtkStructuredGridOutlineFilter::GetNumberOfStreamDivisions()
{
  vtkPolyData *output = this->GetOutput();
  int piece, numPieces, ghostLevel;
  int start, end;

  output->GetUpdateExtent(piece, numPieces, ghostLevel);
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
					          int idx,
						  int vtkNotUsed(NumDivisions))
{
  vtkStructuredGrid *input = this->GetInput();
  vtkPolyData *output = (vtkPolyData *)out;
  int piece, numPieces, ghostLevel;
  int start, end;
  int *ext, xMax, yMax, zMax;

  output->GetUpdateExtent(piece, numPieces, ghostLevel);

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
