/*=========================================================================

  Program:   Visualization Toolkit
  Module:    BrownPts.cc
  Language:  C++
  Date:      9/14/94
  Version:   1.1


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
#include "vtkBrownianPoints.hh"
#include "vtkFloatVectors.hh"
#include "vtkMath.hh"

vtkBrownianPoints::vtkBrownianPoints()
{
  this->MinimumSpeed = 0.0;
  this->MaximumSpeed = 1.0;
}

void vtkBrownianPoints::Execute()
{
  int i, numPts;
  vtkFloatVectors *newVectors;
  float v[3], norm, speed;

  vtkDebugMacro(<< "Executing Brownian filter");

  if ( ((numPts=this->Input->GetNumberOfPoints()) < 1) )
    {
    vtkErrorMacro(<< "No input!\n");
    return;
    }

  newVectors = new vtkFloatVectors(numPts);
//
// Check consistency of minumum and maximum speed
//  
  if ( this->MinimumSpeed > this->MaximumSpeed )
    {
    vtkErrorMacro(<< " Minimum speed > maximum speed; reset to (0,1).");
    this->MinimumSpeed = 0.0;
    this->MaximumSpeed = 1.0;
    }

  for (i=0; i<numPts; i++)
    {
    speed = vtkMath::Random(this->MinimumSpeed,this->MaximumSpeed);
    if ( speed != 0.0 )
      {
      for (i=0; i<3; i++) v[i] = vtkMath::Random(0,speed);
      norm = vtkMath::Norm(v);
      for (i=0; i<3; i++) v[i] *= (speed / norm);
      }
    else
      {
      for (i=0; i<3; i++) v[i] = 0.0;
      }

    newVectors->InsertNextVector(v);
    }
//
// Update ourselves
//
  this->Output->GetPointData()->CopyVectorsOff();
  this->Output->GetPointData()->PassData(this->Input->GetPointData());

  this->Output->GetPointData()->SetVectors(newVectors);
  newVectors->Delete();
}

void vtkBrownianPoints::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToDataSetFilter::PrintSelf(os,indent);

  os << indent << "Minimum Speed: " << this->MinimumSpeed << "\n";
  os << indent << "Maximum Speed: " << this->MaximumSpeed << "\n";
}
