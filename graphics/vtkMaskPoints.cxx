/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMaskPoints.cxx
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
#include "vtkMaskPoints.h"
#include "vtkMath.h"

vtkMaskPoints::vtkMaskPoints()
{
  this->OnRatio = 2;
  this->Offset = 0;
  this->RandomMode = 0;
  this->MaximumNumberOfPoints = VTK_LARGE_INTEGER;
}

void vtkMaskPoints::Execute()
{
  vtkFloatPoints *newPts;
  vtkPointData *pd;
  int numPts=this->Input->GetNumberOfPoints();
  int numNewPts;
  float *x;
  int ptId, id;
  vtkPolyData *output = this->GetOutput();
  vtkPointData *outputPD = output->GetPointData();
  
  //
  // Check input
  //
  vtkDebugMacro(<<"Masking points");

  if ( numPts < 1 )
    {
    vtkErrorMacro(<<"No data to mask!");
    return;
    }

  pd = this->Input->GetPointData();
  id = 0;
  
  //
  // Allocate space
  //
  numNewPts = numPts / this->OnRatio;
  if (numNewPts > this->MaximumNumberOfPoints)
    {
    numNewPts = this->MaximumNumberOfPoints;
    }
  newPts = vtkFloatPoints::New();
  newPts->Allocate(numNewPts);
  outputPD->CopyAllocate(pd);
  //
  // Traverse points and copy
  //
  if ( this->RandomMode ) // retro mode
    {
    float cap;
    
    if (((float)numPts/this->OnRatio) > this->MaximumNumberOfPoints)
      {
      cap = 2.0*numPts/this->MaximumNumberOfPoints - 1;
      }
    else 
      {
      cap = 2.0*this->OnRatio - 1;
      }

    for (ptId = this->Offset; 
    (ptId < numPts) && (id < this->MaximumNumberOfPoints);  
    ptId += (1 + (int)((float)vtkMath::Random()*cap)) )
      {
      x =  this->Input->GetPoint(ptId);
      id = newPts->InsertNextPoint(x);
      outputPD->CopyData(pd,ptId,id);
      }
    }
  else // a.r. mode
    {
    for ( ptId = this->Offset; 
    (ptId < numPts) && (id < (this->MaximumNumberOfPoints-1));
    ptId += this->OnRatio )
      {
      x =  this->Input->GetPoint(ptId);
      id = newPts->InsertNextPoint(x);
      outputPD->CopyData(pd,ptId,id);
      }
    }
//
// Update ourselves
//
  output->SetPoints(newPts);
  output->Squeeze();

  vtkDebugMacro(<<"Masked " << numPts << " original points to " << id+1 << " points");
}

void vtkMaskPoints::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToPolyDataFilter::PrintSelf(os,indent);

  os << indent << "MaximumNumberOfPoints: " << 
    this->MaximumNumberOfPoints << "\n";
  os << indent << "On Ratio: " << this->OnRatio << "\n";
  os << indent << "Offset: " << this->Offset << "\n";
  os << indent << "Random Mode: " << (this->RandomMode ? "On\n" : "Off\n");
}
