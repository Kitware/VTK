/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWarpVector.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkWarpVector.h"


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



void vtkWarpVector::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPointSetToPointSetFilter::PrintSelf(os,indent);

  os << indent << "Scale Factor: " << this->ScaleFactor << "\n";
}
