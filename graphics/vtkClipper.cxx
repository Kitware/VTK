/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkClipper.cxx
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
#include <math.h>
#include "vtkClipper.h"
#include "vtkMergePoints.h"
#include "vtkLine.h"
#include "vtkTriangle.h"
#include "vtkTetra.h"

// Description:
// Construct with user-specified implicit function. The value is 0;
// and InsideOut is turned off.
vtkClipper::vtkClipper(vtkImplicitFunction *cf)
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
unsigned long vtkClipper::GetMTime()
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
// Loop over all dataset cells; triangulate each cell; clip each simplex

void vtkClipper::Execute()
{
  vtkDataSet *input = this->Input;
  vtkUnstructuredGrid *output = (vtkUnstructuredGrid *)this->GetOutput();
  vtkPointData *outPD = output->GetPointData();
  vtkPointData *inPD = input->GetPointData();
  vtkFloatPoints *cellPts, *newPoints;
  vtkFloatScalars *clipScalars;
  vtkIdList *cellIds, triIds(64);
  int numCells=input->GetNumberOfCells(), numPts=input->GetNumberOfPoints();
  int cellId, numberOfPoints;
  int i, j, idx, id, numSimplices, pts[1], npts, *ids;
  vtkFloatScalars cellScalars(VTK_CELL_SIZE);
  cellScalars.ReferenceCountingOff();
  vtkFloatPoints triPts(30); triPts.ReferenceCountingOff();
  vtkCell *cell;
  float range[2], s, *x;
  vtkCellArray *connList;
  vtkLine line;
  vtkTriangle triangle;
  vtkTetra tetra;
  
  vtkDebugMacro(<< "Clipping dataset");

  if ( ! this->ClipFunction )
    {
    vtkErrorMacro(<<"No clipping function specified");
    return;
    }

  if ( numPts < 1 || numCells < 1 )
    {
    vtkErrorMacro(<<"No data to clip");
    return;
    }

  newPoints = new vtkFloatPoints(numPts/2,numPts/2);
  connList = new vtkCellArray(128,128);

  // locator used to merge potentially duplicate points
  if ( this->Locator == NULL ) this->CreateDefaultLocator();
  this->Locator->InitPointInsertion (newPoints, this->Input->GetBounds());

  //allocate space for output
  output->Allocate(numCells/2,numCells/2);
  outPD->InterpolateAllocate(inPD,numPts/2,numPts/2);

  //
  // Loop over all cells creating scalar function determined by evaluating cell
  // points using clip function.
  //
  clipScalars = new vtkFloatScalars(numPts);
  for ( i=0; i < numPts; i++ )
    {
    s = this->ClipFunction->EvaluateFunction(input->GetPoint(i));
    clipScalars->InsertScalar(i,s);
    }

  // loop over all cells and clip them
  for ( cellId=0; cellId < numCells; cellId++ )
    {
    cell = input->GetCell(cellId);
    cellPts = cell->GetPoints();
    cellIds = cell->GetPointIds();
    numberOfPoints = cellPts->GetNumberOfPoints();

    // evaluate implicit cutting function
    range[0] = VTK_LARGE_FLOAT; range[1] = -VTK_LARGE_FLOAT;
    for ( i=0; i < numberOfPoints; i++ )
      {
      s = clipScalars->GetScalar(cellIds->GetId(i));
      cellScalars.InsertScalar(i, s);
      if ( s < range[0] ) range[0] = s;
      if ( s > range[1] ) range[1] = s;
      }

    // if cell is inside or cut by the implicit function, process it
    if ( ( !this->InsideOut && range[1] > this->Value ) ||
    ( this->InsideOut && range[0] <= this->Value) )
      {

      switch ( cell->GetCellDimension() )
        {

        case 0: //vertices-------------------------------

          for ( i=0; i < numberOfPoints; i++ )
            {
            s = cellScalars.GetScalar(i);
            if ( (!this->InsideOut && s > this->Value) ||
            (this->InsideOut && s <= this->Value) )
              {
              x = cellPts->GetPoint(i);
              if ( (pts[0] = this->Locator->IsInsertedPoint(x)) < 0 )
                {
                pts[0] = this->Locator->InsertNextPoint(x);
                outPD->CopyData(inPD,cellIds->GetId(i),pts[0]);
                }
              output->InsertNextCell(VTK_VERTEX,1,pts);
              }
            }
          break;

        case 1: //lines----------------------------------

          connList->Reset();
          if ( input->GetCellType(cellId) == VTK_LINE )
            {
            ((vtkLine *)cell)->Clip(this->Value, &cellScalars, this->Locator, connList,
                                    inPD, outPD, this->InsideOut);
            }
          else
            {
            cell->Triangulate(cellId, triIds, triPts);
            numSimplices = triPts.GetNumberOfPoints() / 2;
            for ( i=0; i < numSimplices; i++ )
              {
              for ( j=0; j < 2; j++ )
                {
                idx = 2*i + j;
                line.Points.SetPoint(j,triPts.GetPoint(idx));
                id = triIds.GetId(idx);
                line.PointIds.SetId(j,id);
                cellScalars.InsertScalar(j,clipScalars->GetScalar(id));
                }
              line.Clip(this->Value, &cellScalars, this->Locator,  connList,
                        inPD, outPD, this->InsideOut);
              }
            }

          for (connList->InitTraversal(); connList->GetNextCell(npts,ids); )
            {
            output->InsertNextCell(VTK_LINE,2,ids);
            }
          break;

        case 2: //triangles------------------------------

          connList->Reset();
          if ( input->GetCellType(cellId) == VTK_TRIANGLE )
            {
            ((vtkTriangle *)cell)->Clip(this->Value, &cellScalars, this->Locator, 
                                        connList, inPD, outPD, this->InsideOut);
            }
          else
            {
            cell->Triangulate(cellId, triIds, triPts);
            numSimplices = triPts.GetNumberOfPoints() / 3;
            for ( i=0; i < numSimplices; i++ )
              {
              for ( j=0; j < 3; j++ )
                {
                idx = 3*i + j;
                triangle.Points.SetPoint(j,triPts.GetPoint(idx));
                id = triIds.GetId(idx);
                triangle.PointIds.SetId(j,id);
                cellScalars.InsertScalar(j,clipScalars->GetScalar(id));
                }
              triangle.Clip(this->Value, &cellScalars, this->Locator, connList,
                            inPD, outPD, this->InsideOut);
              }
            }

          for (connList->InitTraversal(); connList->GetNextCell(npts,ids); )
            {
            output->InsertNextCell(VTK_TRIANGLE,3,ids);
            }
          break;

        case 3: //tetrahedra-----------------------------

          if ( input->GetCellType(cellId) == VTK_TETRA )
            {
            ((vtkTetra *)cell)->Clip(this->Value, &cellScalars, this->Locator, output,
                                    inPD, outPD, this->InsideOut);
            }
          else
            {
            cell->Triangulate(cellId, triIds, triPts);
            numSimplices = triPts.GetNumberOfPoints() / 4;
            for ( i=0; i < numSimplices; i++ )
              {
              for ( j=0; j < 4; j++ )
                {
                idx = 4*i + j;
                tetra.Points.SetPoint(j,triPts.GetPoint(idx));
                id = triIds.GetId(idx);
                tetra.PointIds.SetId(j,id);
                cellScalars.InsertScalar(j,clipScalars->GetScalar(id));
                }
              tetra.Clip(this->Value, &cellScalars, this->Locator, output,
                         inPD, outPD, this->InsideOut);
              }
            }
          break;

        } //switch
      } //if inside implicit function
    } //for each cell

  vtkDebugMacro(<<"Created: " 
               << newPoints->GetNumberOfPoints() << " points, " 
               << output->GetNumberOfCells() << " cells");
//
// Update ourselves and release memory
//
  clipScalars->Delete();

  output->SetPoints(newPoints);
  newPoints->Delete();

  output->Squeeze();
}

// Description:
// Specify a spatial locator for merging points. By default, 
// an instance of vtkMergePoints is used.
void vtkClipper::SetLocator(vtkPointLocator *locator)
{
  if ( this->Locator != locator ) 
    {
    if ( this->SelfCreatedLocator ) this->Locator->Delete();
    this->SelfCreatedLocator = 0;
    this->Locator = locator;
    this->Modified();
    }
}

void vtkClipper::CreateDefaultLocator()
{
  if ( this->SelfCreatedLocator ) this->Locator->Delete();
  this->Locator = new vtkMergePoints;
  this->SelfCreatedLocator = 1;
}

void vtkClipper::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToUnstructuredGridFilter::PrintSelf(os,indent);

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
