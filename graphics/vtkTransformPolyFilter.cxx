/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransformPolyFilter.cxx
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
#include "vtkTransformPolyFilter.h"
#include "vtkFloatNormals.h"
#include "vtkFloatVectors.h"

void vtkTransformPolyFilter::Execute()
{
  vtkPoints *inPts;
  vtkFloatPoints *newPts;
  vtkPointData *pd, *outPD;
  vtkVectors *inVectors;
  vtkFloatVectors *newVectors=NULL;
  vtkNormals *inNormals;
  vtkFloatNormals *newNormals=NULL;
  int numPts;
  vtkPolyData *input=(vtkPolyData *)this->Input;
  vtkPolyData *output=(vtkPolyData *)this->Output;

  vtkDebugMacro(<<"Executing polygonal transformation");

  //
  // Check input
  //
  if ( this->Transform == NULL )
    {
    vtkErrorMacro(<<"No transform defined!");
    return;
    }

  inPts = input->GetPoints();
  pd = input->GetPointData();
  outPD = output->GetPointData();
  inVectors = pd->GetVectors();
  inNormals = pd->GetNormals();

  if ( !inPts )
    {
    vtkErrorMacro(<<"No input data");
    return;
    }

  numPts = inPts->GetNumberOfPoints();
  newPts = vtkFloatPoints::New();
  newPts->Allocate(numPts);
  if ( inVectors ) 
    {
    newVectors = vtkFloatVectors::New();
    newVectors->Allocate(numPts);
    }
  if ( inNormals ) 
    {
    newNormals = vtkFloatNormals::New();
    newNormals->Allocate(numPts);
    }
//
// Loop over all points, updating position
//
  this->Transform->MultiplyPoints(inPts,newPts);
//
// Ditto for vectors and normals
//
  if ( inVectors )
    {
    this->Transform->MultiplyVectors(inVectors,newVectors);
    }

  if ( inNormals )
    {
    this->Transform->MultiplyNormals(inNormals,newNormals);
    }
//
// Update ourselves and release memory
//
  outPD->CopyVectorsOff();
  outPD->CopyNormalsOff();
  outPD->PassData(input->GetPointData());

  output->SetPoints(newPts);
  newPts->Delete();

  if (newNormals)
    {
    outPD->SetNormals(newNormals);
    newNormals->Delete();
    }

  if (newVectors)
    {
    outPD->SetVectors(newVectors);
    newVectors->Delete();
    }

  output->SetVerts(input->GetVerts());
  output->SetLines(input->GetLines());
  output->SetPolys(input->GetPolys());
  output->SetStrips(input->GetStrips());
}

unsigned long vtkTransformPolyFilter::GetMTime()
{
  unsigned long mTime=this->MTime.GetMTime();
  unsigned long transMTime;

  if ( this->Transform )
    {
    transMTime = this->Transform->GetMTime();
    mTime = ( transMTime > mTime ? transMTime : mTime );
    }

  return mTime;
}

void vtkTransformPolyFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyToPolyFilter::PrintSelf(os,indent);

  os << indent << "Transform: " << this->Transform << "\n";
}
