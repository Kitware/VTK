/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ContourF.cc
  Language:  C++
  Date:      02/07/94
  Version:   1.4


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
#include "vtkContourFilter.hh"
#include "vtkFloatScalars.hh"
#include "vtkCell.hh"
#include "vtkMergePoints.hh"
#include "vtkMarchingSquares.hh"
#include "vtkMarchingCubes.hh"
#include "vtkMath.hh"

// Description:
// Construct object with initial range (0,1) and single contour value
// of 0.0.
vtkContourFilter::vtkContourFilter()
{
  for (int i=0; i<VTK_MAX_CONTOURS; i++) this->Values[i] = 0.0;
  this->NumberOfContours = 1;
  this->Range[0] = 0.0;
  this->Range[1] = 1.0;
  this->ComputeNormals = 1;
  this->ComputeGradients = 0;
  this->ComputeScalars = 1;

  this->Locator = NULL;
  this->SelfCreatedLocator = 0;
}

// Description:
// Set a particular contour value at contour number i. The index i ranges 
// between 0<=i<NumberOfContours.
void vtkContourFilter::SetValue(int i, float value)
{
  i = (i >= VTK_MAX_CONTOURS ? VTK_MAX_CONTOURS-1 : (i < 0 ? 0 : i) );
  if ( this->Values[i] != value )
    {
    this->Modified();
    this->Values[i] = value;
    if ( i >= this->NumberOfContours ) this->NumberOfContours = i + 1;
    if ( value < this->Range[0] ) this->Range[0] = value;
    if ( value > this->Range[1] ) this->Range[1] = value;
    }
}

void vtkContourFilter::GenerateValues(int numContours, float range1, 
				     float range2)
{
  float rng[2];

  rng[0] = range1;
  rng[1] = range2;
  this->GenerateValues(numContours,rng);
}

// Description:
// Generate numContours equally spaced contour values between specified
// range. Contour values will include min/max range values.
void vtkContourFilter::GenerateValues(int numContours, float range[2])
{
  float val, incr;
  int i;

  numContours = (numContours >= VTK_MAX_CONTOURS ? VTK_MAX_CONTOURS-1 : 
                (numContours > 1 ? numContours : 2) );

  incr = (range[1] - range[0]) / (numContours-1);
  for (i=0, val=range[0]; i < numContours; i++, val+=incr)
    {
    this->SetValue(i,val);
    }

  this->NumberOfContours = numContours;
}


//
// General contouring filter.  Handles arbitrary input.
//
void vtkContourFilter::Execute()
{
  int cellId, i;
  vtkIdList *cellPts;
  vtkScalars *inScalars;
  vtkFloatScalars cellScalars(VTK_CELL_SIZE);
  vtkCell *cell;
  float *range, value;
  vtkFloatScalars *newScalars;
  vtkCellArray *newVerts, *newLines, *newPolys;
  vtkFloatPoints *newPts;
  cellScalars.ReferenceCountingOff();
  vtkPolyData *output = this->GetOutput();
  int numCells, estimatedSize;
  
  vtkDebugMacro(<< "Executing contour filter");

  numCells = this->Input->GetNumberOfCells();
  inScalars = this->Input->GetPointData()->GetScalars();
  if ( ! inScalars || numCells < 1 )
    {
    vtkErrorMacro(<<"No data to contour");
    return;
    }

  // If structured points, use more efficient algorithms
  if ( ! strcmp(this->Input->GetDataType(),"vtkStructuredPoints") )
    {
    int dim = this->Input->GetCell(0)->GetCellDimension();

    if ( this->Input->GetCell(0)->GetCellDimension() >= 2 ) 
      {
      this->StructuredPointsContour(dim);
      return;
      }
    }

  range = inScalars->GetRange();
//
// Create objects to hold output of contour operation. First estimate allocation size.
//
  estimatedSize = (int) pow ((double) numCells, .75);
  estimatedSize = estimatedSize / 1024 * 1024; //multiple of 1024
  if (estimatedSize < 1024) estimatedSize = 1024;

  newPts = new vtkFloatPoints(estimatedSize,estimatedSize);
  newVerts = new vtkCellArray(estimatedSize,estimatedSize);
  newLines = new vtkCellArray(estimatedSize,estimatedSize);
  newPolys = new vtkCellArray(estimatedSize,estimatedSize);
  newScalars = new vtkFloatScalars(estimatedSize,estimatedSize);

  // locator used to merge potentially duplicate points
  if ( this->Locator == NULL ) this->CreateDefaultLocator();
  this->Locator->InitPointInsertion (newPts, this->Input->GetBounds());
//
// Loop over all contour values.  Then for each contour value, 
// loop over all cells.
//
  for (i=0; i < this->NumberOfContours; i++)
    {
    value = this->Values[i];
    for (cellId=0; cellId < numCells; cellId++)
      {
      cell = Input->GetCell(cellId);
      cellPts = cell->GetPointIds();
      inScalars->GetScalars(*cellPts,cellScalars);

      cell->Contour(value, &cellScalars, this->Locator, 
                    newVerts, newLines, newPolys, newScalars);

      } // for all cells
    } // for all contour values

  vtkDebugMacro(<<"Created: " 
               << newPts->GetNumberOfPoints() << " points, " 
               << newVerts->GetNumberOfCells() << " verts, " 
               << newLines->GetNumberOfCells() << " lines, " 
               << newPolys->GetNumberOfCells() << " triangles");
//
// Update ourselves.  Because we don't know up front how many verts, lines,
// polys we've created, take care to reclaim memory. 
//
  output->SetPoints(newPts);
  newPts->Delete();

  output->GetPointData()->SetScalars(newScalars);
  newScalars->Delete();

  if (newVerts->GetNumberOfCells()) output->SetVerts(newVerts);
  newVerts->Delete();

  if (newLines->GetNumberOfCells()) output->SetLines(newLines);
  newLines->Delete();

  if (newPolys->GetNumberOfCells()) output->SetPolys(newPolys);
  newPolys->Delete();

  this->Locator->Initialize();//releases leftover memory
  output->Squeeze();
}

//
// Special method handles structured points
//
void vtkContourFilter::StructuredPointsContour(int dim)
{
  vtkPolyData *output;
  vtkPolyData *thisOutput = (vtkPolyData *)this->Output;
  int i;

  if ( dim == 2 ) //marching squares
    {
    static vtkMarchingSquares msquares;

    msquares.SetInput((vtkStructuredPoints *)this->Input);
    msquares.SetDebug(this->Debug);
    msquares.SetNumberOfContours(this->NumberOfContours);
    for (i=0; i < this->NumberOfContours; i++)
      msquares.SetValue(i,this->Values[i]);
         
    msquares.Update();
    output = msquares.GetOutput();
    }

  else //marching cubes
    {
    static vtkMarchingCubes mcubes;

    mcubes.SetInput((vtkStructuredPoints *)this->Input);
    mcubes.SetComputeNormals (this->ComputeNormals);
    mcubes.SetComputeGradients (this->ComputeGradients);
    mcubes.SetComputeScalars (this->ComputeScalars);
    mcubes.SetDebug(this->Debug);
    mcubes.SetNumberOfContours(this->NumberOfContours);
    for (i=0; i < this->NumberOfContours; i++)
      mcubes.SetValue(i,this->Values[i]);

    mcubes.Update();
    output = mcubes.GetOutput();
    }

  thisOutput->CopyStructure(output);
  *thisOutput->GetPointData() = *output->GetPointData();
  output->Initialize();
}

// Description:
// Specify a spatial locator for merging points. By default, 
// an instance of vtkMergePoints is used.
void vtkContourFilter::SetLocator(vtkPointLocator *locator)
{
  if ( this->Locator != locator ) 
    {
    if ( this->SelfCreatedLocator ) this->Locator->Delete();
    this->SelfCreatedLocator = 0;
    this->Locator = locator;
    this->Modified();
    }
}

void vtkContourFilter::CreateDefaultLocator()
{
  if ( this->SelfCreatedLocator ) this->Locator->Delete();
  this->Locator = new vtkMergePoints;
  this->SelfCreatedLocator = 1;
}

void vtkContourFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  int i;

  vtkDataSetToPolyFilter::PrintSelf(os,indent);

  os << indent << "Number Of Contours : " << this->NumberOfContours << "\n";
  os << indent << "Contour Values: \n";
  for ( i=0; i<this->NumberOfContours; i++)
    {
    os << indent << "  Value " << i << ": " << this->Values[i] << "\n";
    }

  if ( this->Locator )
    {
    os << indent << "Locator: " << this->Locator << "\n";
    }
  else
    {
    os << indent << "Locator: (none)\n";
    }
}

