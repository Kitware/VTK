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

  this->GenerateClippedOutput = 0;
  this->ClippedOutput = new vtkUnstructuredGrid;
}

vtkClipper::~vtkClipper()
{
  this->ClippedOutput->Delete();
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
  vtkPointData *inPD, *outPD = output->GetPointData();
  vtkFloatPoints *cellPts, *newPoints;
  vtkFloatScalars *clipScalars;
  vtkIdList *cellIds;
  int numCells=input->GetNumberOfCells(), numPts=input->GetNumberOfPoints();
  int cellId, numberOfPoints;
  int i, *pts, npts, type=0, dim=0;
  vtkFloatScalars cellScalars(VTK_CELL_SIZE); cellScalars.ReferenceCountingOff();
  vtkCell *cell;
  float s;
  vtkCellArray *connList;
  
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

  clipScalars = new vtkFloatScalars(numPts);
  // Interpolate data along edge. If generating clip scalars, do the necessary setup.
  if ( this->GenerateClipScalars )
    {
    inPD = new vtkPointData(*(input->GetPointData()));
    inPD->SetScalars(clipScalars);
    }
  else 
    {
    inPD = input->GetPointData();
    }
    
  output->Allocate(numCells/2,numCells/2);
  outPD->InterpolateAllocate(inPD,numPts/2,numPts/2);

  // If generating second output, setup clipped output
  if ( this->GenerateClippedOutput )
    {
    this->ClippedOutput->Initialize();
    this->ClippedOutput->Allocate(numCells/2,numCells/2);
    }

  //
  // Loop over all cells creating scalar function determined by evaluating cell
  // points using clip function.
  //
  for ( i=0; i < numPts; i++ )
    {
    s = this->ClipFunction->FunctionValue(input->GetPoint(i));
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
    for ( i=0; i < numberOfPoints; i++ )
      {
      s = clipScalars->GetScalar(cellIds->GetId(i));
      cellScalars.InsertScalar(i, s);
      }

    dim = cell->GetCellDimension();

    connList->Reset();
    cell->Clip(this->Value, &cellScalars, this->Locator, connList,
               inPD, outPD, this->InsideOut);

    for (connList->InitTraversal(); connList->GetNextCell(npts,pts); )
      {
      if ( dim == 0 ) type = VTK_VERTEX;
      else if ( dim == 1 ) type = VTK_LINE;
      else if ( dim == 2 )
        {
        if ( npts == 3 ) type = VTK_TRIANGLE;
        else type = VTK_QUAD;
        }
      else type = VTK_TETRA;

      output->InsertNextCell(type,npts,pts);
      }

    if ( this->GenerateClippedOutput )
      {
      connList->Reset();
      cell->Clip(this->Value, &cellScalars, this->Locator, connList,
                 inPD, outPD, !this->InsideOut);

      for (connList->InitTraversal(); connList->GetNextCell(npts,pts); )
        {
        if ( dim == 0 ) type = VTK_VERTEX;
        else if ( dim == 1 ) type = VTK_LINE;
        else if ( dim == 2 )
          {
          if ( npts == 3 ) type = VTK_TRIANGLE;
          else type = VTK_QUAD;
          }
        else type = VTK_TETRA;

        this->ClippedOutput->InsertNextCell(type,npts,pts);
        }
      }

    } //for each cell

  vtkDebugMacro(<<"Created: " 
               << newPoints->GetNumberOfPoints() << " points, " 
               << output->GetNumberOfCells() << " cells");

  if ( this->GenerateClippedOutput )
    {
    vtkDebugMacro(<<"Created (clipped output): " 
               << newPoints->GetNumberOfPoints() << " points, " 
               << this->ClippedOutput->GetNumberOfCells() << " cells");
    }
//
// Update ourselves and release memory
//
  clipScalars->Delete();
  if ( this->GenerateClipScalars ) inPD->Delete();

  if ( this->GenerateClippedOutput )
    {
    this->ClippedOutput->SetPoints(newPoints);
    this->ClippedOutput->Squeeze();
    this->ClippedOutput->GetPointData()->PassData(outPD);
    }

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
