/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDiskSource.cxx
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
#include "vtkDiskSource.hh"
#include "vtkMath.hh"

vtkDiskSource::vtkDiskSource()
{
  this->InnerRadius = 0.25;
  this->OuterRadius = 0.5;
  this->RadialResolution = 1;
  this->CircumferentialResolution = 6;
}

void vtkDiskSource::Execute()
{
  int numPolys, numPts;
  float x[3];
  int i, j;
  int pts[4];
  float theta, deltaRadius;
  float cosTheta, sinTheta;
  vtkFloatPoints *newPoints; 
  vtkCellArray *newPolys;
  vtkPolyData *output = this->GetOutput();
  
  //
  // Set things up; allocate memory
  //

  numPts = (this->RadialResolution + 1) * 
           (this->CircumferentialResolution + 1);
  numPolys = this->RadialResolution * this->CircumferentialResolution;
  newPoints = new vtkFloatPoints(numPts);
  newPolys = new vtkCellArray;
  newPolys->Allocate(newPolys->EstimateSize(numPolys,4));
//
// Create disk
//
  theta = 2.0 * vtkMath::Pi() / ((float)this->CircumferentialResolution);
  deltaRadius = (this->OuterRadius - InnerRadius) / 
                       ((float)this->RadialResolution);

  for (i=0; i<=this->CircumferentialResolution; i++) 
    {
    cosTheta = cos((double)i*theta);
    sinTheta = sin((double)i*theta);
    for (j=0; j <= this->RadialResolution; j++)
      {
      x[0] = (this->InnerRadius + j*deltaRadius) * cosTheta;
      x[1] = (this->InnerRadius + j*deltaRadius) * sinTheta;
      x[2] = 0.0;
      newPoints->InsertNextPoint(x);
      }
    }
//
//  Create connectivity
//
    for (i=0; i < this->CircumferentialResolution; i++) 
      {
      for (j=0; j < this->RadialResolution; j++) 
        {
        pts[0] = i*(this->RadialResolution+1) + j;
        pts[1] = pts[0] + 1;
        pts[2] = pts[1] + this->RadialResolution + 1;
        pts[3] = pts[2] - 1;
        newPolys->InsertNextCell(4,pts);
        }
      }
//
// Update ourselves and release memory
//
  output->SetPoints(newPoints);
  newPoints->Delete();

  output->SetPolys(newPolys);
  newPolys->Delete();
}

void vtkDiskSource::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolySource::PrintSelf(os,indent);

  os << indent << "InnerRadius: " << this->InnerRadius << "\n";
  os << indent << "OuterRadius: " << this->OuterRadius << "\n";
  os << indent << "RadialResolution: " << this->RadialResolution << "\n";
  os << indent << "CircumferentialResolution: " << this->CircumferentialResolution << "\n";
}
