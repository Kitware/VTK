/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ProbeF.cc
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
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.


=========================================================================*/
#include "ProbeF.hh"

vtkProbeFilter::vtkProbeFilter()
{
  this->Source = NULL;
}

void vtkProbeFilter::Execute()
{
  int cellId, ptId;
  float *x, tol2;
  vtkCell *cell;
  vtkPointData *pd;
  int numPts, subId;
  float pcoords[3], weights[MAX_CELL_SIZE];
  vtkDataSet *input=this->Input, *source=this->Source;

  vtkDebugMacro(<<"Probing data");
  this->Initialize();

  pd = input->GetPointData();
  numPts = source->GetNumberOfPoints();
//
// Allocate storage for output PointData
//
  this->PointData.InterpolateAllocate(pd);
//
// Use tolerance as a function of size of input data
//
  tol2 = input->GetLength();
  tol2 = tol2*tol2 / 1000.0;
//
// Loop over all source points, interpolating input data
//
  for (ptId=0; ptId < numPts; ptId++)
    {
    x = source->GetPoint(ptId);
    cellId = input->FindCell(x,NULL,tol2,subId,pcoords,weights);
    if ( cellId >= 0 )
      {
      cell = input->GetCell(cellId);
      this->PointData.InterpolatePoint(pd,ptId,&(cell->PointIds),weights);
      }
    else
      {
      this->PointData.NullPoint(ptId);
      }
    }

  this->Modified(); //make sure something's changed
}

void vtkProbeFilter::Initialize()
{
  if ( this->Source )
    {
    // copies SOURCE geometry to internal data set
    if (this->DataSet) this->DataSet->Delete();
    this->DataSet = this->Source->MakeObject(); 
    }
}

// Description:
// Override update method because execution can branch two ways (Input 
// and Source)
void vtkProbeFilter::Update()
{
  // make sure input is available
  if ( this->Input == NULL || this->Source == NULL )
    {
    vtkErrorMacro(<< "No input!");
    return;
    }

  // prevent chasing our tail
  if (this->Updating) return;

  this->Updating = 1;
  this->Input->Update();
  this->Source->Update();
  this->Updating = 0;

  if (this->Input->GetMTime() > this->GetMTime() || 
  this->Source->GetMTime() > this->GetMTime() || 
  this->GetMTime() > this->ExecuteTime || this->GetDataReleased() )
    {
    if ( this->StartMethod ) (*this->StartMethod)(this->StartMethodArg);
    this->Execute();
    this->ExecuteTime.Modified();
    this->SetDataReleased(0);
    if ( this->EndMethod ) (*this->EndMethod)(this->EndMethodArg);
    }

  if ( this->Input->ShouldIReleaseData() ) this->Input->ReleaseData();
  if ( this->Source->ShouldIReleaseData() ) this->Source->ReleaseData();
}

void vtkProbeFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToDataSetFilter::PrintSelf(os,indent);

  os << indent << "Source: " << this->Source << "\n";
}
