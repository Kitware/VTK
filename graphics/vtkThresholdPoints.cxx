/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThresholdPoints.cxx
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
#include "vtkThresholdPoints.h"

// Construct with lower threshold=0, upper threshold=1, and threshold 
// function=upper.
vtkThresholdPoints::vtkThresholdPoints()
{
  this->LowerThreshold = 0.0;
  this->UpperThreshold = 1.0;

  this->ThresholdFunction = &vtkThresholdPoints::Upper;
}

// Description:
// Criterion is cells whose scalars are less than lower threshold.
void vtkThresholdPoints::ThresholdByLower(float lower) 
{
  if ( this->LowerThreshold != lower )
    {
    this->LowerThreshold = lower; 
    this->ThresholdFunction = &vtkThresholdPoints::Lower;
    this->Modified();
    }
}
                           
// Description:
// Criterion is cells whose scalars are less than upper threshold.
void vtkThresholdPoints::ThresholdByUpper(float upper)
{
  if ( this->UpperThreshold != upper )
    {
    this->UpperThreshold = upper; 
    this->ThresholdFunction = &vtkThresholdPoints::Upper;
    this->Modified();
    }
}
                           
// Description:
// Criterion is cells whose scalars are between lower and upper thresholds.
void vtkThresholdPoints::ThresholdBetween(float lower, float upper)
{
  if ( this->LowerThreshold != lower || this->UpperThreshold != upper )
    {
    this->LowerThreshold = lower; 
    this->UpperThreshold = upper;
    this->ThresholdFunction = &vtkThresholdPoints::Between;
    this->Modified();
    }
}
  
void vtkThresholdPoints::Execute()
{
  vtkScalars *inScalars;
  vtkFloatPoints *newPoints;
  vtkPointData *pd, *outPD;
  vtkCellArray *verts;
  int ptId, pts[1], numPts;
  float *x;
  vtkPolyData *output=(vtkPolyData *)this->Output;

  vtkDebugMacro(<< "Executing threshold points filter");

  if ( ! (inScalars = this->Input->GetPointData()->GetScalars()) )
    {
    vtkErrorMacro(<<"No scalar data to threshold");
    return;
    }
     
  numPts = this->Input->GetNumberOfPoints();
  newPoints = vtkFloatPoints::New();
  newPoints->Allocate(numPts);
  pd = this->Input->GetPointData();
  outPD = output->GetPointData();
  outPD->CopyAllocate(pd);
  verts = vtkCellArray::New();
  verts->Allocate(verts->EstimateSize(numPts,1));

  // Check that the scalars of each point satisfy the threshold criterion
  for (ptId=0; ptId < this->Input->GetNumberOfPoints(); ptId++)
    {
    if ( (this->*(this->ThresholdFunction))(inScalars->GetScalar(ptId)) ) 
      {
      x = this->Input->GetPoint(ptId);
      pts[0] = newPoints->InsertNextPoint(x);
      outPD->CopyData(pd,ptId,pts[0]);
      verts->InsertNextCell(1,pts);
      } // satisfied thresholding
    } // for all points

  vtkDebugMacro(<< "Extracted " << output->GetNumberOfPoints() << " points.");

//
// Update ourselves and release memory
//
  output->SetPoints(newPoints);
  newPoints->Delete();

  output->SetVerts(verts);
  verts->Delete();

  output->Squeeze();
}

void vtkThresholdPoints::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToPolyDataFilter::PrintSelf(os,indent);

  os << indent << "Lower Threshold: " << this->LowerThreshold << "\n";;
  os << indent << "Upper Threshold: " << this->UpperThreshold << "\n";;
}
