/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSelectVisiblePoints.cxx
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
#include "vtkSelectVisiblePoints.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkSelectVisiblePoints* vtkSelectVisiblePoints::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkSelectVisiblePoints");
  if(ret)
    {
    return (vtkSelectVisiblePoints*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkSelectVisiblePoints;
}




// Instantiate object with no renderer; window selection turned off; 
// tolerance set to 0.01; and select invisible off.
vtkSelectVisiblePoints::vtkSelectVisiblePoints()
{
  this->Renderer = NULL;
  this->SelectionWindow = 0;
  this->Selection[0] = this->Selection[2] = 0;
  this->Selection[1] = this->Selection[3] = 1600;
  this->Tolerance = 0.01;
  this->SelectInvisible = 0;
}

vtkSelectVisiblePoints::~vtkSelectVisiblePoints()
{
  this->SetRenderer(NULL);
}

void vtkSelectVisiblePoints::Execute()
{
  vtkIdType ptId, id;
  int visible;
  vtkPoints *outPts;
  vtkDataSet *input= this->GetInput();
  vtkPolyData *output=this->GetOutput();
  vtkPointData *inPD=input->GetPointData();
  vtkPointData *outPD=output->GetPointData();
  vtkIdType numPts=input->GetNumberOfPoints();
  float x[4], dx[3], z, diff;
  int selection[4];
  
  if ( this->Renderer == NULL )
    {
    vtkErrorMacro(<<"Renderer must be set");
    return;
    }

  if ( numPts < 1 )
    {
    return;
    }
  
  outPts = vtkPoints::New();
  outPts->Allocate(numPts/2+1);
  outPD->CopyAllocate(inPD);

  int *size = this->Renderer->GetRenderWindow()->GetSize();

  // specify a selection window to avoid querying 
  if ( this->SelectionWindow )
    {
      for (int i=0; i<4; i++)
      {
      selection[i] = this->Selection[i];
      }
    }
  else
    {
    selection[0] = selection[2] = 0;
    selection[1] = size[0] - 1;
    selection[3] = size[1] - 1;
    }

  // Grab the composite perspective transform.  This matrix is used to convert
  // each point to view coordinates.  vtkRenderer provides a WorldToView()
  // method but it computes the composite perspective transform each time
  // WorldToView() is called.  This is expensive, so we get the matrix once
  // and handle the transformation ourselves.
  vtkMatrix4x4 *matrix = vtkMatrix4x4::New();
  float view[4];
  matrix->DeepCopy(this->Renderer->GetActiveCamera()->
                   GetCompositePerspectiveTransformMatrix(1,0,1));

  // If we have more than a few query points, we grab the z-buffer for the
  // selection region all at once and probe the resulting array.  When we
  // have just a few points, we perform individual z-buffer queries.
  const int SimpleQueryLimit = 25;
  float *zPtr = NULL;
  if (numPts > SimpleQueryLimit)
    {
    zPtr = this->Renderer->GetRenderWindow()->
      GetZbufferData(selection[0], selection[2], selection[1], selection[3]);
    }
  
  int abort=0;
  vtkIdType progressInterval=numPts/20+1;
  x[3] = 1.0;
  for (id=(-1), ptId=0; ptId < numPts && !abort; ptId++)
    {
    // perform conversion
    input->GetPoint(ptId,x);
    matrix->MultiplyPoint(x, view);
    if (view[3] == 0.0)
      {
      continue;
      }
    this->Renderer->SetViewPoint(view[0]/view[3], view[1]/view[3],
                                 view[2]/view[3]);
    this->Renderer->ViewToDisplay();
    this->Renderer->GetDisplayPoint(dx);
    visible = 0;

    if ( ! (ptId % progressInterval) ) 
      {
      this->UpdateProgress((float)ptId/numPts);
      abort = this->GetAbortExecute();
      }

    // check whether visible and in selection window 
    if ( dx[0] >= selection[0] && dx[0] <= selection[1] &&
         dx[1] >= selection[2] && dx[1] <= selection[3] )
      {
      if (numPts > SimpleQueryLimit)
        {
        // Access the value from the captured zbuffer.  Note, we only
        // captured a portion of the zbuffer, so we need to offset dx by
        // the selection window.
        z = zPtr[(int)dx[0] - selection[0]
                 + ((int)dx[1] - selection[2])
                 *(selection[1] - selection[0] + 1)];
        }
      else
        {
        z = this->Renderer->GetZ(dx[0], dx[1]);
        }
      diff = fabs(z-dx[2]);
      if ( diff <= this->Tolerance )
        {
        visible = 1;
        }
      }

    if ( (visible && !this->SelectInvisible) ||
         (!visible && this->SelectInvisible) )
      {
      id = outPts->InsertNextPoint(x);
      outPD->CopyData(inPD,ptId,id);
      }
    }//for all points

  output->SetPoints(outPts);
  outPts->Delete();
  output->Squeeze();

  matrix->Delete();

  if (zPtr)
    {
    delete [] zPtr;
    }

  vtkDebugMacro(<<"Selected " << id + 1 << " out of " 
                << numPts << " original points");
}

unsigned long int vtkSelectVisiblePoints::GetMTime()
{
  unsigned long mTime=this->vtkDataSetToPolyDataFilter::GetMTime();
  unsigned long time;
 
  if ( this->Renderer != NULL )
    {
    time = this->Renderer->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }

  return mTime;
}

void vtkSelectVisiblePoints::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToPolyDataFilter::PrintSelf(os,indent);

  os << indent << "Renderer: " << this->Renderer << "\n";
  os << indent << "Selection Window: " 
     << (this->SelectionWindow ? "On\n" : "Off\n");

  os << indent << "Selection: \n";
  os << indent << "  Xmin,Xmax: (" << this->Selection[0] << ", " 
     << this->Selection[1] << ")\n";
  os << indent << "  Ymin,Ymax: (" << this->Selection[2] << ", " 
     << this->Selection[3] << ")\n";

  os << indent << "Tolerance: " << this->Tolerance << "\n";
  os << indent << "Select Invisible: " 
     << (this->SelectInvisible ? "On\n" : "Off\n");
}
