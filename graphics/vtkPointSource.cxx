/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointSource.cxx
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
#include "vtkPointSource.h"
#include "vtkMath.h"

vtkPointSource::vtkPointSource(int numPts)
{
  this->NumberOfPoints = (numPts > 0 ? numPts : 10);

  this->Center[0] = 0.0;
  this->Center[1] = 0.0;
  this->Center[2] = 0.0;

  this->Radius = 0.5;
}

void vtkPointSource::Execute()
{
  int i;
  float radius, theta, phi, x[3], rho;
  vtkFloatPoints *newPoints;
  vtkCellArray *newVerts;
  vtkPolyData *output = (vtkPolyData *)this->Output;
  
  vtkDebugMacro(<< "Generating random cloud of points...");

  newPoints = vtkFloatPoints::New();
  newPoints->Allocate(this->NumberOfPoints);
  newVerts = vtkCellArray::New();
  newVerts->Allocate(newVerts->EstimateSize(1,this->NumberOfPoints));

  newVerts->InsertNextCell(this->NumberOfPoints);
  for (i=0; i<this->NumberOfPoints; i++)
    {
    phi = vtkMath::Pi() * vtkMath::Random();
    rho = this->Radius * vtkMath::Random();
    radius = rho * sin((double)phi);
    theta = 2.0*vtkMath::Pi() * vtkMath::Random();
    x[0] = this->Center[0] + radius * cos((double)theta);
    x[1] = this->Center[1] + radius * sin((double)theta);
    x[2] = this->Center[2] + rho * cos((double)phi);
    newVerts->InsertCellPoint(newPoints->InsertNextPoint(x));
    }
//
// Update ourselves and release memory
//
  output->SetPoints(newPoints);
  newPoints->Delete();

  output->SetVerts(newVerts);
  newVerts->Delete();
}

void vtkPointSource::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolySource::PrintSelf(os,indent);

  os << indent << "Number Of Points: " << this->NumberOfPoints << "\n";
  os << indent << "Radius: " << this->Radius << "\n";
  os << indent << "Center: (" << this->Center[0] << ", "
                              << this->Center[1] << ", "
                              << this->Center[2] << ")\n";

}
