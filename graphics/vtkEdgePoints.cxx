/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEdgePoints.cxx
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
#include "vtkEdgePoints.h"

// Construct object with contour value of 0.0.
vtkEdgePoints::vtkEdgePoints()
{
  this->Value = 0.0;
  this->Locator = vtkMergePoints::New();
}

vtkEdgePoints::~vtkEdgePoints()
{
  this->Locator->Delete();
  this->Locator = NULL;
}

//
// General filter: handles arbitrary input.
//
void vtkEdgePoints::Execute()
{
  vtkScalars *inScalars;
  vtkPoints *newPts;
  vtkCellArray *newVerts;
  int cellId, above, below, ptId, i, numEdges, edgeId, newCellId;
  vtkCell *cell, *edge;
  float range[2];
  float s0, s1, x0[3], x1[3], x[3], t;
  float e0Scalar, deltaScalar;
  int e0, e1;
  int pts[1], p1, p2;
  int estimatedSize;
  vtkDataSet *input = (vtkDataSet *)this->Input;
  vtkPolyData *output = this->GetOutput();
  vtkScalars *cellScalars;
  vtkPointData *inPd=input->GetPointData(), *outPd=output->GetPointData();
  vtkCellData *inCd=input->GetCellData(), *outCd=output->GetCellData();

  vtkDebugMacro(<< "Generating edge points");
  //
  // Initialize and check input
  //

  if ( ! (inScalars = input->GetPointData()->GetScalars()) )
    {
    vtkErrorMacro(<<"No scalar data to contour");
    return;
    }

  inScalars->GetRange(range);
  if ( this->Value < range[0] || this->Value > range[1] )
    {
    vtkWarningMacro(<<"Value lies outside of scalar range");
    return;
    }

  estimatedSize = (int) (input->GetNumberOfCells () * .75);
  estimatedSize = estimatedSize / 1024 * 1024; //multiple of 1024
  if (estimatedSize < 1024) estimatedSize = 1024;

  newPts = vtkPoints::New();
  newPts->Allocate(estimatedSize, estimatedSize/2);
  newVerts = vtkCellArray::New();
  newVerts->Allocate(estimatedSize, estimatedSize/2);
  cellScalars = vtkScalars::New();
  cellScalars->Allocate(VTK_CELL_SIZE);
  
  this->Locator->InitPointInsertion (newPts, input->GetBounds());

  // interpolate data along edge; copy cell data
  outPd->InterpolateAllocate(inPd,5000,10000);
  outCd->CopyAllocate(inCd,5000,10000);
  //
  // Traverse all edges. Since edges are not explicitly represented, use a
  // trick: traverse all cells and obtain cell edges and then cell edge
  // neighbors. If cell id < all edge neigbors ids, then this edge has not
  // yet been visited and is processed.
  //
  for (cellId=0; cellId < input->GetNumberOfCells(); cellId++)
    {
    cell = input->GetCell(cellId);
    inScalars->GetScalars(cell->PointIds, cellScalars);

    // loop over cell points to check if cell straddles isosurface value
    for ( above=below=0, ptId=0; ptId < cell->GetNumberOfPoints(); ptId++ )
      {
      if ( cellScalars->GetScalar(ptId) >= this->Value )
        above = 1;
      else if ( cellScalars->GetScalar(ptId) < this->Value )
        below = 1;
      }

    if ( above && below ) //contour passes through cell
      {
      if ( cell->GetCellDimension() < 2 ) //only points can be generated
        {
        cell->Contour(this->Value, cellScalars, this->Locator, newVerts, 
                      NULL, NULL, inPd, outPd, inCd, cellId, outCd);
        }

      else //
        {
        numEdges = cell->GetNumberOfEdges();
        for (edgeId=0; edgeId < numEdges; edgeId++)
          {
          edge = cell->GetEdge(edgeId);
          inScalars->GetScalars(edge->PointIds, cellScalars);

          s0 = cellScalars->GetScalar(0);
          s1 = cellScalars->GetScalar(1);
          if ( (s0 < this->Value && s1 >= this->Value) ||
          (s0 >= this->Value && s1 < this->Value) )
            {
	    //ordering intersection direction avoids numerical problems
	    deltaScalar = s1 - s0; 
	    if (deltaScalar > 0)
	      {
	      e0 = 0; e1 = 1;
	      e0Scalar = s0;
	      }
	    else
	      {
	      e0 = 1; e1 = 0;
	      e0Scalar = s1;
	      deltaScalar = -deltaScalar;
	      }

	    t = (this->Value - e0Scalar) / deltaScalar;

            edge->Points->GetPoint(e0,x0);
            edge->Points->GetPoint(e1,x1);

            for (i=0; i<3; i++) x[i] = x0[i] + t * (x1[i] - x0[i]);
            if ( (pts[0] = this->Locator->IsInsertedPoint(x)) < 0 )
              {
              pts[0] = this->Locator->InsertNextPoint(x);
              newCellId = newVerts->InsertNextCell(1,pts);
	      outCd->CopyData(inCd,cellId,newCellId);
              p1 = edge->PointIds->GetId(e0);
              p2 = edge->PointIds->GetId(e1);
              outPd->InterpolateEdge(inPd,pts[0],p1,p2,t);
              } //if point not created before
            } //if edge straddles contour value
          } //for each edge
        } //dimension 2 and higher
      } //above and below
    } //for all cells

  vtkDebugMacro(<<"Created: " << newPts->GetNumberOfPoints() << " points");
  //
  // Update ourselves.  Because we don't know up front how many verts we've 
  // created, take care to reclaim memory. 
  //
  output->SetPoints(newPts);
  newPts->Delete();

  output->SetVerts(newVerts);
  newVerts->Delete();

  this->Locator->Initialize();//free up any extra memory
  output->Squeeze();
  
  cellScalars->Delete();
}

void vtkEdgePoints::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToPolyDataFilter::PrintSelf(os,indent);

  os << indent << "Contour Value: " << this->Value << "\n";
}


