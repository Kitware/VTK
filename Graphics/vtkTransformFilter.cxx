/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransformFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkTransformFilter.h"
#include "vtkLinearTransform.h"
#include "vtkObjectFactory.h"
#include "vtkFloatArray.h"

//----------------------------------------------------------------------------
vtkTransformFilter* vtkTransformFilter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkTransformFilter");
  if(ret)
    {
    return (vtkTransformFilter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkTransformFilter;
}

vtkTransformFilter::vtkTransformFilter()
{
  this->Transform = NULL;
}

vtkTransformFilter::~vtkTransformFilter()
{
  this->SetTransform(NULL);
}

void vtkTransformFilter::Execute()
{
  vtkPoints *inPts;
  vtkPoints *newPts;
  vtkDataArray *inVectors, *inCellVectors;;
  vtkFloatArray *newVectors=NULL, *newCellVectors=NULL;
  vtkDataArray *inNormals, *inCellNormals;
  vtkFloatArray *newNormals=NULL, *newCellNormals=NULL;
  vtkIdType numPts, numCells;
  vtkPointSet *input = this->GetInput();
  vtkPointSet *output = this->GetOutput();
  vtkPointData *pd=input->GetPointData(), *outPD=output->GetPointData();
  vtkCellData *cd=input->GetCellData(), *outCD=output->GetCellData();

  vtkDebugMacro(<<"Executing transform filter");

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );

  // Check input
  //
  if ( this->Transform == NULL )
    {
    vtkErrorMacro(<<"No transform defined!");
    return;
    }

  inPts = input->GetPoints();
  inVectors = pd->GetActiveVectors();
  inNormals = pd->GetActiveNormals();
  inCellVectors = cd->GetActiveVectors();
  inCellNormals = cd->GetActiveNormals();

  if ( !inPts )
    {
    vtkErrorMacro(<<"No input data");
    return;
    }

  numPts = inPts->GetNumberOfPoints();
  numCells = input->GetNumberOfCells();

  newPts = vtkPoints::New();
  newPts->Allocate(numPts);
  if ( inVectors ) 
    {
    newVectors = vtkFloatArray::New();
    newVectors->SetNumberOfComponents(3);
    newVectors->Allocate(3*numPts);
    }
  if ( inNormals ) 
    {
    newNormals = vtkFloatArray::New();
    newNormals->SetNumberOfComponents(3);
    newNormals->Allocate(3*numPts);
    }

  this->UpdateProgress (.2);
  // Loop over all points, updating position
  //

  if ( inVectors || inNormals )
    {
    this->Transform->TransformPointsNormalsVectors(inPts,newPts,
						   inNormals,newNormals,
						   inVectors,newVectors);
    }
  else
    {
    this->Transform->TransformPoints(inPts,newPts);
    }  

  this->UpdateProgress (.6);

  // Can only transform cell normals/vectors if the transform
  // is linear.
  vtkLinearTransform* lt=vtkLinearTransform::SafeDownCast(this->Transform);
  if (lt)
    {
    if ( inCellVectors ) 
      {
      newCellVectors = vtkFloatArray::New();
      newCellVectors->SetNumberOfComponents(3);
      newCellVectors->Allocate(3*numCells);
      lt->TransformVectors(inCellVectors,newCellVectors);
      }
    if ( inCellNormals ) 
      {
      newCellNormals = vtkFloatArray::New();
      newCellNormals->SetNumberOfComponents(3);
      newCellNormals->Allocate(3*numCells);
      lt->TransformNormals(inCellNormals,newCellNormals);
      }
    }

  this->UpdateProgress (.8);

  // Update ourselves and release memory
  //
  output->SetPoints(newPts);
  newPts->Delete();

  if (newNormals)
    {
    outPD->SetNormals(newNormals);
    newNormals->Delete();
    outPD->CopyNormalsOff();
    }

  if (newVectors)
    {
    outPD->SetVectors(newVectors);
    newVectors->Delete();
    outPD->CopyVectorsOff();
    }

  if (newCellNormals)
    {
    outCD->SetNormals(newCellNormals);
    newCellNormals->Delete();
    outCD->CopyNormalsOff();
    }

  if (newCellVectors)
    {
    outCD->SetVectors(newCellVectors);
    newCellVectors->Delete();
    outCD->CopyVectorsOff();
    }

  outPD->PassData(pd);
  outCD->PassData(cd);
}

unsigned long vtkTransformFilter::GetMTime()
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

void vtkTransformFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPointSetToPointSetFilter::PrintSelf(os,indent);

  os << indent << "Transform: " << this->Transform << "\n";
}
