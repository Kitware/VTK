/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWarpVector.cxx
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
#include "vtkWarpVector.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkWarpVector* vtkWarpVector::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkWarpVector");
  if(ret)
    {
    return (vtkWarpVector*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkWarpVector;
}

vtkWarpVector::vtkWarpVector()
{
  this->ScaleFactor = 1.0;
}

vtkWarpVector::~vtkWarpVector()
{
}

template <class T1, class T2>
static void vtkWarpVectorExecute2(vtkWarpVector *self, T1 *inPts, 
                                  T1 *outPts, T2 *inVec, vtkIdType max)
{
  vtkIdType ptId;
  T1 scaleFactor = (T1)self->GetScaleFactor();
  
  // Loop over all points, adjusting locations
  for (ptId=0; ptId < max; ptId++)
    {
    if (!(ptId & 0xfff)) 
      {
      self->UpdateProgress ((float)ptId/(max+1));
      if (self->GetAbortExecute())
        {
        break;
        }
      }
    
    *outPts = *inPts + scaleFactor * (T1)(*inVec);
    outPts++; inPts++; inVec++;
    *outPts = *inPts + scaleFactor * (T1)(*inVec);
    outPts++; inPts++; inVec++;
    *outPts = *inPts + scaleFactor * (T1)(*inVec);
    outPts++; inPts++; inVec++;
    }
}
          
template <class T>
static void vtkWarpVectorExecute(vtkWarpVector *self,
                                 T *inPts, T *outPts, vtkIdType max)
{
  void *inVec = self->GetInput()->GetPointData()->
    GetVectors()->GetVoidPointer(0);

  // call templated function
  switch (self->GetInput()->GetPointData()->GetVectors()->GetDataType())
    {
    vtkTemplateMacro5(vtkWarpVectorExecute2,self, inPts, outPts, 
                      (VTK_TT *)(inVec), max);
    default:
      break;
    }  
}

//----------------------------------------------------------------------------
void vtkWarpVector::Execute()
{
  vtkPointSet *input = this->GetInput();
  vtkPointSet *output = this->GetOutput();
  vtkPoints *points;
  vtkIdType numPts;

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );

  if (input == NULL || input->GetPoints() == NULL)
    {
    return;
    }
  numPts = input->GetPoints()->GetNumberOfPoints();
  if ( !input->GetPointData()->GetVectors() || !numPts)
    {
    vtkErrorMacro(<<"No input data");
    return;
    }

  // SETUP AND ALLOCATE THE OUTPUT
  numPts = input->GetNumberOfPoints();
  points = vtkPoints::SafeDownCast(input->GetPoints()->MakeObject());
  points->Allocate(numPts);
  points->SetNumberOfPoints(numPts);
  output->SetPoints(points);
  points->Delete();

  void *inPtr = input->GetPoints()->GetVoidPointer(0);
  void *outPtr = output->GetPoints()->GetVoidPointer(0);

  // call templated function
  switch (input->GetPoints()->GetDataType())
    {
    vtkTemplateMacro4(vtkWarpVectorExecute, this, 
                      (VTK_TT *)(inPtr), (VTK_TT *)(outPtr), numPts);
    default:
      break;
    }
  
  // now pass the data.
  output->GetPointData()->CopyNormalsOff(); // distorted geometry
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());
}

void vtkWarpVector::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPointSetToPointSetFilter::PrintSelf(os,indent);

  os << indent << "Scale Factor: " << this->ScaleFactor << "\n";
}
