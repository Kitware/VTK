/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEdgePoints.cxx
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
#include "vtkEdgePoints.h"

// Description:
// Construct object with contour value of 0.0.
vtkEdgePoints::vtkEdgePoints()
{
  this->Value = 0.0;
}

//
// General filter: handles arbitrary input.
//
void vtkEdgePoints::Execute()
{
  vtkScalars *inScalars;
  vtkFloatPoints *newPts;
  vtkCellArray *newVerts;
  int cellId, above, below, ptId, i, numEdges, edgeId;
  vtkCell *cell, *edge;
  float range[2];
  float s0, s1, x0[3], x1[3], x[3], r;
  vtkFloatScalars cellScalars(VTK_CELL_SIZE);
  int pts[1], p1, p2;
  vtkPolyData *output = this->GetOutput();
  cellScalars.ReferenceCountingOff();
  vtkPointData *inPd, *outPd;

  vtkDebugMacro(<< "Generating edge points");
  //
  // Initialize and check input
  //

  if ( ! (inScalars = this->Input->GetPointData()->GetScalars()) )
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

  newPts = vtkFloatPoints::New();
  newPts->Allocate(5000,10000);
  newVerts = vtkCellArray::New();
  newVerts->Allocate(5000,10000);

  this->Locator.InitPointInsertion (newPts, this->Input->GetBounds());

  // interpolate data along edge
  inPd = this->Input->GetPointData();
  outPd = output->GetPointData();
  outPd->InterpolateAllocate(inPd,5000,10000);
//
// Traverse all edges. Since edges are not explicitly represented, use a
// trick: traverse all cells and obtain cell edges and then cell edge
// neighbors. If cell id < all edge neigbors ids, then this edge has not
// yet been visited and is processed.
//
  for (cellId=0; cellId<Input->GetNumberOfCells(); cellId++)
    {
    cell = this->Input->GetCell(cellId);
    inScalars->GetScalars(cell->PointIds,cellScalars);

    // loop over cell points to check if cell straddles isosurface value
    for ( above=below=0, ptId=0; ptId < cell->GetNumberOfPoints(); ptId++ )
      {
      if ( cellScalars.GetScalar(ptId) >= this->Value )
        above = 1;
      else if ( cellScalars.GetScalar(ptId) < this->Value )
        below = 1;
      }

    if ( above && below ) //contour passes through cell
      {
      if ( cell->GetCellDimension() < 2 ) //only points can be generated
        {
        cell->Contour(this->Value, &cellScalars, &this->Locator, newVerts, 
                      NULL, NULL, inPd, outPd);
        }

      else //
        {
        numEdges = cell->GetNumberOfEdges();
        for (edgeId=0; edgeId < numEdges; edgeId++)
          {
          edge = cell->GetEdge(edgeId);
          inScalars->GetScalars(edge->PointIds,cellScalars);

          s0 = cellScalars.GetScalar(0);
          s1 = cellScalars.GetScalar(1);
          if ( (s0 < this->Value && s1 >= this->Value) ||
          (s0 >= this->Value && s1 < this->Value) )
            {
            edge->Points.GetPoint(0,x0);
            edge->Points.GetPoint(1,x1);
            r = (this->Value - s0) / (s1 - s0);
            for (i=0; i<3; i++) x[i] = x0[i] + r * (x1[i] - x0[i]);
            if ( (pts[0] = this->Locator.IsInsertedPoint(x)) < 0 )
              {
              pts[0] = this->Locator.InsertNextPoint(x);
              newVerts->InsertNextCell(1,pts);

              p1 = edge->PointIds.GetId(0);
              p2 = edge->PointIds.GetId(1);
              outPd->InterpolateEdge(inPd,pts[0],p1,p2,r);
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

  this->Locator.Initialize();//free up any extra memory
  output->Squeeze();
}

void vtkEdgePoints::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToPolyFilter::PrintSelf(os,indent);

  os << indent << "Contour Value: " << this->Value << "\n";
}


