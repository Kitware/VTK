/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVectorNorm.cxx
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
#include <math.h>
#include "vtkVectorNorm.hh"
#include "vtkFloatScalars.hh"

// Description:
// Construct with normalize flag off.
vtkVectorNorm::vtkVectorNorm()
{
  this->Normalize = 0;
}

void vtkVectorNorm::Execute()
{
  int i, numVectors;
  vtkFloatScalars *newScalars;
  float *v, s, maxScalar;
  vtkPointData *pd, *outPD;
  vtkVectors *inVectors;
  vtkDataSet *output = this->GetOutput();
  //
  // Initialize
  //
  vtkDebugMacro(<<"Normalizing vectors!");

  pd = this->Input->GetPointData();
  outPD = output->GetPointData();
  inVectors = pd->GetVectors();

  if ( (numVectors=inVectors->GetNumberOfVectors()) < 1 )
    {
    vtkErrorMacro(<< "No input vectors!\n");
    return;
    }

//
// Allocate
//
  newScalars = new vtkFloatScalars(numVectors);
  for (maxScalar=0.0, i=0; i < numVectors; i++)
    {
    v = inVectors->GetVector(i);
    s = sqrt((double)v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
    if ( s > maxScalar ) maxScalar = s;
    newScalars->SetScalar(i,s);
    }
//
// If necessary, normalize
//
  if ( this->Normalize && maxScalar > 0.0 )
    {
    for (i=0; i < numVectors; i++)
      {
      s = newScalars->GetScalar(i);
      s /= maxScalar;
      newScalars->SetScalar(i,s);
      }
    }
//
// Update self and release memory
//
  outPD->CopyScalarsOff();
  outPD->PassData(pd);

  outPD->SetScalars(newScalars);
  newScalars->Delete();
}

void vtkVectorNorm::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToDataSetFilter::PrintSelf(os,indent);

  os << indent << "Normalize: " << (this->Normalize ? "On\n" : "Off\n");
}

