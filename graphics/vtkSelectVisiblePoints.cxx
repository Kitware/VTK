/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSelectVisiblePoints.cxx
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
#include "vtkSelectVisiblePoints.h"
#include "vtkRenderer.h"

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

void vtkSelectVisiblePoints::Execute()
{
  int ptId, id, visible;
  vtkPoints *outPts;
  vtkDataSet *input=(vtkDataSet *)this->Input;
  vtkPolyData *output=this->GetOutput();
  vtkPointData *inPD=input->GetPointData();
  vtkPointData *outPD=output->GetPointData();
  int numPts=input->GetNumberOfPoints();
  float x[4], dx[3], z, diff;

  if ( numPts < 1 ) return;
  
  outPts = vtkPoints::New();
  outPts->Allocate(numPts/2+1);
  outPD->CopyAllocate(inPD);

  x[3] = 1.0;
  for (id=(-1), ptId=0; ptId < numPts; ptId++)
    {
    // perform conversion
    input->GetPoint(ptId,x);
    this->Renderer->SetWorldPoint(x);
    this->Renderer->WorldToDisplay();
    this->Renderer->GetDisplayPoint(dx);
    visible = 0;

    // check whether visible and in selection window 
    if ( !this->SelectionWindow ||
    (dx[0] >= this->Selection[0] && dx[0] <= this->Selection[1] &&
     dx[1] >= this->Selection[2] && dx[1] <= this->Selection[3]) )
      {
      z = this->Renderer->GetZ(dx[0], dx[1]);
      diff = fabs(z-dx[2]);
      if ( diff <= this->Tolerance ) visible = 1;
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
