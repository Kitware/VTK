/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWarpVector.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkWarpVector.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
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
  this->Threader = vtkMultiThreader::New();
  this->NumberOfThreads = this->Threader->GetNumberOfThreads();
}
vtkWarpVector::~vtkWarpVector()
{
  this->Threader->Delete();
  this->Threader = NULL;
}

//----------------------------------------------------------------------------
VTK_THREAD_RETURN_TYPE vtkWarpVectorThreadedExecute( void *arg )
{
  vtkWarpVector *self;
  int threadId, threadCount;
  
  threadId = ((ThreadInfoStruct *)(arg))->ThreadID;
  threadCount = ((ThreadInfoStruct *)(arg))->NumberOfThreads;
  self = (vtkWarpVector *)
            (((ThreadInfoStruct *)(arg))->UserData);

  self->ThreadedExecute(threadId, threadCount);
  
  return VTK_THREAD_RETURN_VALUE;
}

//----------------------------------------------------------------------------
void vtkWarpVector::Execute()
{
  vtkPointSet *input = this->GetInput();
  vtkPointSet *output = this->GetOutput();
  vtkPoints *points;
  int numPts;

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );

  if (input == NULL)
    {
    return;
    }

  // SETUP AND ALLOCATE THE OUTPUT
  numPts = input->GetNumberOfPoints();
  points = vtkPoints::New();
  points->Allocate(numPts);
  points->SetNumberOfPoints(numPts);
  output->SetPoints(points);
  points->Delete();

  // NOW SPLIT THE EXECUTION INTO THREADS.
  if (this->NumberOfThreads == 1)
    {
    // just call the threaded execute directly.
    this->ThreadedExecute(0, 1);
    }
  else
    {
    this->Threader->SetNumberOfThreads(this->NumberOfThreads);

    // Setup threading and the invoke threadedExecute
    this->Threader->SetSingleMethod(vtkWarpVectorThreadedExecute, this);
    this->Threader->SingleMethodExecute();
  }

  // now pass the data.
  output->GetPointData()->CopyNormalsOff(); // distorted geometry
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());
}

//----------------------------------------------------------------------------
void vtkWarpVector::ThreadedExecute(int threadId, int threadCount)
{
  vtkPointSet *input=this->GetInput();
  vtkPointSet *output=this->GetOutput();
  vtkPoints *inPts = input->GetPoints();
  vtkPoints *outPts = output->GetPoints();
  vtkVectors *inVectors = input->GetPointData()->GetVectors();
  int min, max, ptId, i;
  float *x, *v, newX[3];

  if ( !inVectors || !inPts )
    {
    vtkErrorMacro(<<"No input data");
    return;
    }

  min = 0;
  max = inPts->GetNumberOfPoints() - 1;
  if (this->SplitPointRange(threadId, threadCount, min, max) == 0)
    {
    return;
    }

  // Loop over all points, adjusting locations
  for (ptId=min; ptId <= max; ptId++)
    {
    if (threadId == 0 && !(ptId % 10000) ) 
      {
      this->UpdateProgress ((float)ptId/(max-min+1));
      if (this->GetAbortExecute())
        {
        break;
        }
      }

    x = inPts->GetPoint(ptId);
    v = inVectors->GetVector(ptId);
    for (i=0; i<3; i++)
      {
      newX[i] = x[i] + this->ScaleFactor * v[i];
      }
    outPts->SetPoint(ptId, newX);
    }
}


//----------------------------------------------------------------------------
int vtkWarpVector::SplitPointRange(int threadId, int threadCount, 
                                    int &min, int &max)
{
  int num, temp;

  num = max-min+1;
  // special case: more threads than points.
  if (threadCount > num)
    {
    if (threadId >= num)
      {
      return 0;
      }
    min = max = min + threadId;
    return 1;
    }
  temp = min;
  min = temp + threadId * num / threadCount;
  max = temp + ((threadId+1)*num / threadCount) - 1;

  return 1;
}



void vtkWarpVector::PrintSelf(vtkOstream& os, vtkIndent indent)
{
  vtkPointSetToPointSetFilter::PrintSelf(os,indent);

  os << indent << "Number Of Threads: " << this->NumberOfThreads << "\n";
  os << indent << "Scale Factor: " << this->ScaleFactor << "\n";
}
