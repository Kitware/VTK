/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVectorDot.cc
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
#include "vtkVectorDot.hh"
#include "vtkMath.hh"

// Description:
// Construct object with scalar range is (-1,1).
vtkVectorDot::vtkVectorDot()
{
  this->ScalarRange[0] = -1.0;
  this->ScalarRange[1] = 1.0;
}

//
// Compute dot product.
//
void vtkVectorDot::Execute()
{
  int ptId, numPts;
  vtkFloatScalars *newScalars;
  vtkDataSet *input=this->Input;
  vtkNormals *inNormals;
  vtkVectors *inVectors;
  float s, *n, *v, min, max, dR, dS;
  vtkDataSet *output=this->Output;
  vtkPointData *pd=input->GetPointData(), *outPD=output->GetPointData();
//
// Initialize
//
  vtkDebugMacro(<<"Generating vector/normal dot product!");

  if ( (numPts=input->GetNumberOfPoints()) < 1 )
    {
    vtkErrorMacro(<< "No points!");
    return;
    }
  if ( (inVectors=pd->GetVectors()) == NULL )
    {
    vtkErrorMacro(<< "No vectors defined!");
    return;
    }
  if ( (inNormals=pd->GetNormals()) == NULL )
    {
    vtkErrorMacro(<< "No normals defined!");
    return;
    }
//
// Allocate
//
  newScalars = new vtkFloatScalars(numPts);
//
// Compute initial scalars
//
  for (min=VTK_LARGE_FLOAT,max=(-VTK_LARGE_FLOAT),ptId=0; ptId < numPts; ptId++)
    {
    n = inNormals->GetNormal(ptId);
    v = inVectors->GetVector(ptId);
    s = vtkMath::Dot(n,v);
    if ( s < min ) min = s;
    if ( s > max ) max = s;
    newScalars->InsertScalar(ptId,s);
    }
//
// Map scalars into scalar range
//
  if ( (dR=this->ScalarRange[1]-this->ScalarRange[0]) == 0.0 ) dR = 1.0;
  if ( (dS=max-min) == 0.0 ) dS = 1.0;

  for ( ptId=0; ptId < numPts; ptId++ )
    {
    s = newScalars->GetScalar(ptId);
    s = ((s - min)/dS) * dR + this->ScalarRange[0];
    newScalars->InsertScalar(ptId,s);
    }
//
// Update self and relase memory
//
  outPD->CopyScalarsOff();
  outPD->PassData(this->Input->GetPointData());

  outPD->SetScalars(newScalars);
  newScalars->Delete();
}

void vtkVectorDot::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToDataSetFilter::PrintSelf(os,indent);

  os << indent << "Scalar Range: (" << this->ScalarRange[0] << ", "
                                    << this->ScalarRange[1] << ")\n";
}
