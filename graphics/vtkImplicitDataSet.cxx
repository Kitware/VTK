/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitDataSet.cxx
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
#include "vtkImplicitDataSet.h"

// Description
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
  if ( this->Weights ) delete [] this->Weights;
}

// Description
// Evaluate the implicit function. This returns the interpolated scalar value
// at x[3].
float vtkImplicitDataSet::EvaluateFunction(float x[3])
{
  vtkScalars *scalars;
  vtkCell *cell;
  int subId, i, id, numPts;
  float pcoords[3], s;

  if ( this->DataSet->GetMaxCellSize() > this->Size )
    {
    if ( this->Weights ) delete [] this->Weights;
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
      id = cell->PointIds.GetId(i);
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


// Description
// Evaluate implicit function gradient.
void vtkImplicitDataSet::EvaluateGradient(float x[3], float n[3])
{
  vtkScalars *scalars;
  vtkCell *cell;
  int subId, i, id, numPts;
  float pcoords[3];

  if ( this->DataSet->GetMaxCellSize() > this->Size )
    {
    if ( this->Weights ) delete [] this->Weights;
    this->Weights = new float[this->DataSet->GetMaxCellSize()];
    this->Size = this->DataSet->GetMaxCellSize();
    }

  // See if a dataset has been specified
  if ( !this->DataSet || 
  !(scalars = this->DataSet->GetPointData()->GetScalars()) )
    {
    vtkErrorMacro(<<"Can't evaluate gradient!");
    for ( i=0; i < 3; i++ ) n[i] = this->OutGradient[i];
    return;
    }

  // Find the cell that contains xyz and get it
  cell = this->DataSet->FindAndGetCell(x,NULL,-1,0.0,subId,pcoords,this->Weights);

  if (cell)
    { // Interpolate the point data
    numPts = cell->GetNumberOfPoints();

    for ( i=0; i < numPts; i++ ) //Weights used to hold scalar values
      {
      id = cell->PointIds.GetId(i);
      this->Weights[i] = scalars->GetScalar(id);
      }
    cell->Derivatives(subId, pcoords, this->Weights, 1, n);
    }

  else
    { // use outside value
    for ( i=0; i < 3; i++ ) n[i] = this->OutGradient[i];
    }
}

void vtkImplicitDataSet::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImplicitFunction::PrintSelf(os,indent);

  os << indent << "Out Value: " << this->OutValue << "\n";
  os << indent << "Out Gradient: (" << this->OutGradient[0] << ", " 
     << this->OutGradient[1] << ", " << this->OutGradient[2] << ")\n";
}
