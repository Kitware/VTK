/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkElevationFilter.cxx
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
#include "vtkElevationFilter.h"
#include "vtkMath.h"
#include "vtkFloatScalars.h"

// Description:
// Construct object with LowPoint=(0,0,0) and HighPoint=(0,0,1). Scalar
// range is (0,1).
vtkElevationFilter::vtkElevationFilter()
{
  this->LowPoint[0] = 0.0;
  this->LowPoint[1] = 0.0;
  this->LowPoint[2] = 0.0;
 
  this->HighPoint[0] = 0.0;
  this->HighPoint[1] = 0.0;
  this->HighPoint[2] = 1.0;

  this->ScalarRange[0] = 0.0;
  this->ScalarRange[1] = 1.0;
}

//
// Convert position along ray into scalar value.  Example use includes 
// coloring terrain by elevation.
//
void vtkElevationFilter::Execute()
{
  int i, j, numPts;
  vtkFloatScalars *newScalars;
  float l, *bounds, *x, s, v[3];
  float diffVector[3], diffScalar;
  
  //
  // Initialize
  //
  vtkDebugMacro(<<"Generating elevation scalars!");

  if ( ((numPts=this->Input->GetNumberOfPoints()) < 1) )
    {
    vtkErrorMacro(<< "No input!");
    return;
    }
//
// Allocate
//
  newScalars = new vtkFloatScalars(numPts);
//
// Set up 1D parametric system
//
  bounds = this->Input->GetBounds();

  for (i=0; i<3; i++) diffVector[i] = this->HighPoint[i] - this->LowPoint[i];
  if ( (l = vtkMath::Dot(diffVector,diffVector)) == 0.0)
    {
    vtkErrorMacro(<< this << ": Bad vector, using (0,0,1)\n");
    diffVector[0] = diffVector[1] = 0.0; diffVector[2] = 1.0;
    l = 1.0;
    }
//
// Compute parametric coordinate and map into scalar range
//
  diffScalar = this->ScalarRange[1] - this->ScalarRange[0];
  for (i=0; i<numPts; i++)
    {
    x = this->Input->GetPoint(i);
    for (j=0; j<3; j++) v[j] = x[j] - this->LowPoint[j];
    s = vtkMath::Dot(v,diffVector) / l;
    s = (s < 0.0 ? 0.0 : s > 1.0 ? 1.0 : s);
    newScalars->InsertScalar(i,this->ScalarRange[0]+s*diffScalar);
    }
//
// Update self
//
  this->Output->GetPointData()->CopyScalarsOff();
  this->Output->GetPointData()->PassData(this->Input->GetPointData());

  this->Output->GetPointData()->SetScalars(newScalars);
  newScalars->Delete();
}

void vtkElevationFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToDataSetFilter::PrintSelf(os,indent);

  os << indent << "Low Point: (" << this->LowPoint[0] << ", "
                                << this->LowPoint[1] << ", "
                                << this->LowPoint[2] << ")\n";
  os << indent << "High Point: (" << this->HighPoint[0] << ", "
                                << this->HighPoint[1] << ", "
                                << this->HighPoint[2] << ")\n";
  os << indent << "Scalar Range: (" << this->ScalarRange[0] << ", "
                                << this->ScalarRange[1] << ")\n";
}
