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
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkProbeFilter* vtkProbeFilter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkProbeFilter");
  if(ret)
    {
    return (vtkProbeFilter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkProbeFilter;
}




//----------------------------------------------------------------------------
vtkProbeFilter::vtkProbeFilter()
{
}

//----------------------------------------------------------------------------
vtkProbeFilter::~vtkProbeFilter()
{
}


//----------------------------------------------------------------------------
void vtkProbeFilter::SetSource(vtkDataSet *input)
{
  this->vtkProcessObject::SetNthInput(1, input);
}

//----------------------------------------------------------------------------
vtkDataSet *vtkProbeFilter::GetSource()
{
  if (this->NumberOfInputs < 2)
    {
    return NULL;
    }
  
  return (vtkDataSet *)(this->Inputs[1]);
}


//----------------------------------------------------------------------------
void vtkProbeFilter::Execute()
{
  int ptId;
  float *x, tol2;
  vtkCell *cell;
  vtkPointData *pd, *outPD;
  int numPts, subId;
  vtkDataSet *source = this->GetSource();
  vtkDataSet *input = this->GetInput();
  vtkDataSet *output= this->GetOutput();
  float pcoords[3], *weights;

  vtkDebugMacro(<<"Probing data");

  if (source == NULL)
    {
    vtkErrorMacro (<< "Source is NULL.");
    return;
    }
  
  weights=new float[source->GetMaxCellSize()];

  pd = source->GetPointData();
  if (pd == NULL)
    {
    vtkErrorMacro(<< "PointData is NULL.");
    return;
    }

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );

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
      outPD->InterpolatePoint(pd,ptId,cell->PointIds,weights);
      }
    else
      {
      outPD->NullPoint(ptId);
      }
    }
  delete [] weights;
}


//----------------------------------------------------------------------------
void vtkProbeFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSet *source = this->GetSource();

  vtkDataSetToDataSetFilter::PrintSelf(os,indent);
  os << indent << "Source: " << source << "\n";
}
