/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitDataSet.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkImplicitDataSet.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkImplicitDataSet* vtkImplicitDataSet::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImplicitDataSet");
  if(ret)
    {
    return (vtkImplicitDataSet*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImplicitDataSet;
}




// Construct an vtkImplicitDataSet with no initial dataset; the OutValue
// set to a large negative number; and the OutGradient set to (0,0,1).
vtkImplicitDataSet::vtkImplicitDataSet()
{
  this->DataSet = NULL;

  this->OutValue = -VTK_LARGE_FLOAT;

  this->OutGradient[0] = 0.0;
  this->OutGradient[1] = 0.0;
  this->OutGradient[2] = 1.0;

  this->Weights = NULL;
  this->Size = 0;
}

vtkImplicitDataSet::~vtkImplicitDataSet()
{
  this->SetDataSet(NULL);
  if ( this->Weights )
    {
    delete [] this->Weights;
    }
}

// Evaluate the implicit function. This returns the interpolated scalar value
// at x[3].
float vtkImplicitDataSet::EvaluateFunction(float x[3])
{
  vtkScalars *scalars;
  vtkCell *cell;
  vtkIdType id;
  int subId, i, numPts;
  float pcoords[3], s;

  if ( this->DataSet->GetMaxCellSize() > this->Size )
    {
    if ( this->Weights )
      {
      delete [] this->Weights;
      }
    this->Weights = new float[this->DataSet->GetMaxCellSize()];
    this->Size = this->DataSet->GetMaxCellSize();
    }

  // See if a dataset has been specified
  if ( !this->DataSet || 
  !(scalars = this->DataSet->GetPointData()->GetScalars()) )
    {
    vtkErrorMacro(<<"Can't evaluate dataset!");
    return this->OutValue;
    }

  // Find the cell that contains xyz and get it
  cell = this->DataSet->FindAndGetCell(x,NULL,-1,0.0,subId,pcoords,this->Weights);

  if (cell)
    { // Interpolate the point data
    numPts = cell->GetNumberOfPoints();
    for (s=0.0, i=0; i < numPts; i++)
      {
      id = cell->PointIds->GetId(i);
      s += scalars->GetScalar(id) * this->Weights[i];
      }
    return s;
    }
  else
    { // use outside value
    return this->OutValue;
    }
}

unsigned long vtkImplicitDataSet::GetMTime()
{
  unsigned long mTime=this->vtkImplicitFunction::GetMTime();
  unsigned long DataSetMTime;

  if ( this->DataSet != NULL )
    {
    this->DataSet->Update ();
    DataSetMTime = this->DataSet->GetMTime();
    mTime = ( DataSetMTime > mTime ? DataSetMTime : mTime );
    }

  return mTime;
}


// Evaluate implicit function gradient.
void vtkImplicitDataSet::EvaluateGradient(float x[3], float n[3])
{
  vtkScalars *scalars;
  vtkCell *cell;
  vtkIdType id;
  int subId, i, numPts;
  float pcoords[3];

  if ( this->DataSet->GetMaxCellSize() > this->Size )
    {
    if ( this->Weights )
      {
      delete [] this->Weights;
      }
    this->Weights = new float[this->DataSet->GetMaxCellSize()];
    this->Size = this->DataSet->GetMaxCellSize();
    }

  // See if a dataset has been specified
  if ( !this->DataSet || 
       !(scalars = this->DataSet->GetPointData()->GetScalars()) )
    {
    vtkErrorMacro(<<"Can't evaluate gradient!");
    for ( i=0; i < 3; i++ )
      {
      n[i] = this->OutGradient[i];
      }
    return;
    }

  // Find the cell that contains xyz and get it
  cell = this->DataSet->FindAndGetCell(x,NULL,-1,0.0,subId,pcoords,this->Weights);

  if (cell)
    { // Interpolate the point data
    numPts = cell->GetNumberOfPoints();

    for ( i=0; i < numPts; i++ ) //Weights used to hold scalar values
      {
      id = cell->PointIds->GetId(i);
      this->Weights[i] = scalars->GetScalar(id);
      }
    cell->Derivatives(subId, pcoords, this->Weights, 1, n);
    }

  else
    { // use outside value
    for ( i=0; i < 3; i++ )
      {
      n[i] = this->OutGradient[i];
      }
    }
}

void vtkImplicitDataSet::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImplicitFunction::PrintSelf(os,indent);

  os << indent << "Out Value: " << this->OutValue << "\n";
  os << indent << "Out Gradient: (" << this->OutGradient[0] << ", " 
     << this->OutGradient[1] << ", " << this->OutGradient[2] << ")\n";

  if ( this->DataSet )
    {
    os << indent << "Data Set: " << this->DataSet << "\n";
    }
  else
    {
    os << indent << "Data Set: (none)\n";
    }
}
