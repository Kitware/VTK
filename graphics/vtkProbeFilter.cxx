/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProbeFilter.cxx
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
#include "vtkProbeFilter.h"

vtkProbeFilter::vtkProbeFilter()
{
  this->Source = NULL;
}

void vtkProbeFilter::Execute()
{
  int ptId;
  float *x, tol2;
  vtkCell *cell;
  vtkPointData *pd, *outPD;
  int numPts, subId;
  vtkDataSet *source=this->Source, *input=this->Input;
  vtkDataSet *output=this->Output;
  float pcoords[3], *weights=new float[source->GetMaxCellSize()];

  vtkDebugMacro(<<"Probing data");

  pd = source->GetPointData();
  numPts = input->GetNumberOfPoints();
//
// Allocate storage for output PointData
//
  outPD = output->GetPointData();
  outPD->InterpolateAllocate(pd);
//
// Use tolerance as a function of size of source data
//
  tol2 = source->GetLength();
  tol2 = tol2*tol2 / 1000.0;
//
// Loop over all input points, interpolating source data
//
  for (ptId=0; ptId < numPts; ptId++)
    {
    // Get the xyz coordinate of the point in the input dataset
    x = input->GetPoint(ptId);

    // Find the cell that contains xyz and get it
    cell = source->FindAndGetCell(x,NULL,-1,tol2,subId,pcoords,weights);
    if (cell)
      {
      // Interpolate the point data
      outPD->InterpolatePoint(pd,ptId,&(cell->PointIds),weights);
      }
    else
      {
      outPD->NullPoint(ptId);
      }
    }
  delete [] weights;
}

// Description:
// Overload update method because execution can branch two ways (Input 
// and Source). Also input and output are abstract.
void vtkProbeFilter::Update()
{
  // make sure output has been created
  if ( !this->Output )
    {
    vtkErrorMacro(<< "No output has been created...need to set input");
    return;
    }

  // make sure input is available
  if ( this->Source == NULL || this->Input == NULL )
    {
    vtkErrorMacro(<< "No input...can't execute!");
    return;
    }

  // prevent chasing our tail
  if (this->Updating) return;

  this->Updating = 1;
  this->Source->Update();
  this->Input->Update();
  this->Updating = 0;

  if (this->Source->GetMTime() > this->ExecuteTime || 
  this->Input->GetMTime() > this->ExecuteTime || 
  this->GetMTime() > this->ExecuteTime )
    {
    if ( this->Input->GetDataReleased() ) this->Input->ForceUpdate();
    if ( this->Source->GetDataReleased() ) this->Source->ForceUpdate();

    if ( this->StartMethod ) (*this->StartMethod)(this->StartMethodArg);
    this->Output->CopyStructure(this->Input);
    this->Execute();
    this->ExecuteTime.Modified();
    this->SetDataReleased(0);
    if ( this->EndMethod ) (*this->EndMethod)(this->EndMethodArg);
    }

  if ( this->Source->ShouldIReleaseData() ) this->Source->ReleaseData();
  if ( this->Input->ShouldIReleaseData() ) this->Input->ReleaseData();
}

void vtkProbeFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToDataSetFilter::PrintSelf(os,indent);

  os << indent << "Source: " << this->Source << "\n";
}
