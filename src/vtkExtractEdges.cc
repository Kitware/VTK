/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractEdges.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkExtractEdges.hh"
#include "vtkEdgeTable.hh"

// Description:
// Construct object.
vtkExtractEdges::vtkExtractEdges()
{
}

// Generate feature edges for mesh
void vtkExtractEdges::Execute()
{
  vtkDataSet *input=(vtkDataSet *)this->Input;
  vtkPolyData *output=(vtkPolyData *)this->Output;
  vtkFloatPoints *newPts;
  vtkCellArray *newLines;
  int numCells, cellNum, numEdges, edgeNum, numEdgePts, numCellEdges;
  int numPts, numNewPts, i, id, pts[2], pt1, pt2;
  vtkEdgeTable *edgeTable;
  vtkCell *cell, *edge;

  vtkDebugMacro(<<"Executing edge extractor");
  //
  //  Check input
  //
  if ( (numCells=input->GetNumberOfCells()) < 1 || 
  (numPts=input->GetNumberOfPoints()) < 1 )
    {
    vtkErrorMacro(<<"No input data!");
    return;
    }
  //
  // Set up processing
  //
  numNewPts = 0;
  numEdges = 0;
  edgeTable = new vtkEdgeTable(numPts);
  newPts = new vtkFloatPoints(numPts);
  newLines = new vtkCellArray;
  newLines->EstimateSize(numPts*4,2);

  //
  // Loop over all cells, extracting non-visited edges. 
  //
  for (cellNum=0; cellNum < numCells; cellNum++ )
    {
    cell = input->GetCell(cellNum);
    numCellEdges = cell->GetNumberOfEdges();
    for (edgeNum=0; edgeNum < numCellEdges; edgeNum++ )
      {
      edge = cell->GetEdge(edgeNum);
      numEdgePts = edge->GetNumberOfPoints();
      
      for ( i=0; i < numEdgePts; i++, pt1=pt2, pts[0]=pts[1] )
        {
        pt2 = edge->PointIds.GetId(i);
        pts[1] = newPts->InsertNextPoint(input->GetPoint(pt2));

        if ( i > 0 && !edgeTable->IsEdge(pt1,pt2) )
          {
          edgeTable->InsertEdge(pt1, pt2);
          newLines->InsertNextCell(2,pts);
          }
        }
      }//for all edges of cell
    }//for all cells

  vtkDebugMacro(<<"Created " << newLines->GetNumberOfCells() << " edges");

  //
  //  Update ourselves.
  //
  delete edgeTable;

  output->SetPoints(newPts);
  newPts->Delete();

  output->SetLines(newLines);
  newLines->Delete();

  output->GetPointData()->PassData(input->GetPointData());
  output->Squeeze();
}

void vtkExtractEdges::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToPolyFilter::PrintSelf(os,indent);
}

