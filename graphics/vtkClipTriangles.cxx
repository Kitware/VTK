/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkClipTriangles.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkClipTriangles.h"
#include "vtkMergePoints.h"
#include <math.h>

// Description:
// Construct with user-specified implicit function.
vtkClipTriangles::vtkClipTriangles(vtkImplicitFunction *cf)
{

  this->ClipFunction = cf;
  this->InsideOut = 0;
  this->Locator = NULL;
  this->Value = 0.0;
  this->SelfCreatedLocator = 0;
}

// Description:
// Overload standard modified time function. If Clip functions is modified,
// then this object is modified as well.
unsigned long vtkClipTriangles::GetMTime()
{
  unsigned long mTime=this->vtkDataSetFilter::GetMTime();
  unsigned long ClipFuncMTime;

  if ( this->ClipFunction != NULL )
    {
    ClipFuncMTime = this->ClipFunction->GetMTime();
    mTime = ( ClipFuncMTime > mTime ? ClipFuncMTime : mTime );
    }

  return mTime;
}

//
// Clip through data generating surface.
//
void vtkClipTriangles::Execute()
{
  int cellId, i, iter;
  vtkFloatPoints *cellPts;
  vtkFloatScalars cellScalars(VTK_CELL_SIZE);
  vtkCell *cell;
  vtkFloatScalars *newScalars;
  vtkCellArray *newVerts, *newLines, *newPolys;
  vtkFloatPoints *newPoints;
  float value, *x, s;
  vtkPolyData *output = this->GetOutput();
  int estimatedSize, numCells=this->Input->GetNumberOfCells();
  
  vtkDebugMacro(<< "Executing Clipper");
  cellScalars.ReferenceCountingOff();
  
  //
  // Initialize self; create output objects
  //
  if ( !this->ClipFunction )
    {
    vtkErrorMacro(<<"No Clip function specified");
    return;
    }
//
// Create objects to hold output of contour operation
//
  estimatedSize = (int) (numCells);
  estimatedSize = estimatedSize / 1024 * 1024; //multiple of 1024
  if (estimatedSize < 1024) estimatedSize = 1024;

  newPoints = new vtkFloatPoints(estimatedSize,estimatedSize/2);
  newVerts = new vtkCellArray(estimatedSize,estimatedSize/2);
  newLines = new vtkCellArray(estimatedSize,estimatedSize/2);
  newPolys = new vtkCellArray(estimatedSize,estimatedSize/2);
  newScalars = new vtkFloatScalars(estimatedSize,estimatedSize/2);

  // locator used to merge potentially duplicate points
  if ( this->Locator == NULL ) this->CreateDefaultLocator();
  this->Locator->InitPointInsertion (newPoints, this->Input->GetBounds());

//
// Loop over all cells creating scalar function determined by evaluating cell
// points using Clip function.
//
  value = this->Value;
  for (cellId=0; cellId < numCells; cellId++)
    {
    cell = Input->GetCell(cellId);
    cellPts = cell->GetPoints();
    int numberOfPoints = cellPts->GetNumberOfPoints();
    for (i=0; i<numberOfPoints; i++)
      {
      x = cellPts->GetPoint(i);
      s = this->ClipFunction->EvaluateFunction(x);
      cellScalars.SetScalar(i,s);
      }

      Clip (cell, value, &cellScalars, this->Locator, 
            newVerts, newLines, newPolys, newScalars, this->InsideOut);
    } // for all cells

//
// Update ourselves.  Because we don't know upfront how many verts, lines,
// polys we've created, take care to reclaim memory. 
//
  output->SetPoints(newPoints);
  newPoints->Delete();

  if (newVerts->GetNumberOfCells()) output->SetVerts(newVerts);
  newVerts->Delete();

  if (newLines->GetNumberOfCells()) output->SetLines(newLines);
  newLines->Delete();

  if (newPolys->GetNumberOfCells()) output->SetPolys(newPolys);
  newPolys->Delete();

  output->GetPointData()->SetScalars(newScalars);
  newScalars->Delete();

  this->Locator->Initialize();//release any extra memory
  output->Squeeze();
}

typedef int TRIANGLE_EDGE_LIST;
typedef struct {
       TRIANGLE_EDGE_LIST edges[7];
} TRIANGLE_CASES;
 
static TRIANGLE_CASES triangleCases[] = { 
{{-1, -1, -1, -1, -1, -1, -1}},	// 0
{{0, 2, 100, -1, -1, -1, -1}},	// 1
{{1, 0, 101, -1, -1, -1, -1}},	// 2
{{1, 2, 100, 1, 100, 101, -1}},	// 3
{{2, 1, 102, -1, -1, -1, -1}},	// 4
{{0, 1, 102, 102, 100, 0, -1}},	// 5
{{0, 101, 2, 2, 101, 102, -1}},	// 6
{{100, 101, 102, -1, -1, -1, -1}}	// 7
};

static int edges[3][2] = { {0,1}, {1,2}, {2,0} };

void vtkClipTriangles::Clip(vtkCell *aCell, float value,
	                  vtkFloatScalars *cellScalars, 
			  vtkPointLocator *locator,
			  vtkCellArray *vtkNotUsed(verts), 
			  vtkCellArray *vtkNotUsed(lines),
			  vtkCellArray *polys, 
			  vtkFloatScalars *scalars, int insideOut)
{
  static int CASE_MASK[3] = {1,2,4};
  TRIANGLE_CASES *triangleCase;
  TRIANGLE_EDGE_LIST  *edge;
  int i, j, index, *vert;
  int e1, e2;
  int pts[3];
  int vertexId;
  float t, *x1, *x2, x[3], deltaScalar;

  // Build the case table
  if (insideOut)
    {    
    for ( i=0, index = 0; i < 3; i++)
        if (cellScalars->GetScalar(i) <= value)
            index |= CASE_MASK[i];
    }    
  else
    {
    for ( i=0, index = 0; i < 3; i++)
        if (cellScalars->GetScalar(i) > value)
            index |= CASE_MASK[i];
    }
  // Select the case based on the index
  //    and get the list of edges for this case
  triangleCase = triangleCases + index;
  edge = triangleCase->edges;

  // generate each triangle
  for ( ; edge[0] > -1; edge += 3 )
    {
    for (i=0; i<3; i++) // insert triangle
      {
      // vertex exists, and need not be interpolated
      if (edge[i] >= 100)
        {
	vertexId = edge[i] - 100;
        x1 = aCell->Points.GetPoint(vertexId);
	for (j=0; j < 3; j++) x[j] = x1[j];
	}
      // new vertex, interpolate
      else
        {
        vert = edges[edge[i]];

        // calculate a preferred interpolation direction
        deltaScalar = (cellScalars->GetScalar(vert[1]) - cellScalars->GetScalar(vert[0]));
        if (deltaScalar > 0)
          {
	  e1 = vert[0]; e2 = vert[1];
          }
        else
          {
	   e1 = vert[1]; e2 = vert[0];
           deltaScalar = -deltaScalar;
          }

	// linear interpolation
        if (deltaScalar == 0.0) t = 0.0;
        else t = (value - cellScalars->GetScalar(e1)) / deltaScalar;

        x1 = aCell->Points.GetPoint(e1);
        x2 = aCell->Points.GetPoint(e2);
        for (j=0; j<3; j++) x[j] = x1[j] + t * (x2[j] - x1[j]);
        }

      if ( (pts[i] = locator->IsInsertedPoint(x)) < 0 )
        {
        pts[i] = locator->InsertNextPoint(x);
        scalars->InsertScalar(pts[i],value);
        }
      }
    // check for degenerate tri's
    if (pts[0] == pts[1] || pts[0] == pts[2] || pts[1] == pts[2]) continue;

    polys->InsertNextCell(3,pts);
    }
}

// Description:
// Specify a spatial locator for merging points. By default, 
// an instance of vtkMergePoints is used.
void vtkClipTriangles::SetLocator(vtkPointLocator *locator)
{
  if ( this->Locator != locator ) 
    {
    if ( this->SelfCreatedLocator ) this->Locator->Delete();
    this->SelfCreatedLocator = 0;
    this->Locator = locator;
    this->Modified();
    }
}

void vtkClipTriangles::CreateDefaultLocator()
{
  if ( this->SelfCreatedLocator ) this->Locator->Delete();
  this->Locator = new vtkMergePoints;
  this->SelfCreatedLocator = 1;
}

void vtkClipTriangles::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToPolyFilter::PrintSelf(os,indent);

  os << indent << "Clip Function: " << this->ClipFunction << "\n";
  os << indent << "InsideOut: " << (this->InsideOut ? "On\n" : "Off\n");
  os << indent << "Value: " << this->Value << "\n";
  if ( this->Locator )
    {
    os << indent << "Locator: " << this->Locator << "\n";
    }
  else
    {
    os << indent << "Locator: (none)\n";
    }
}
