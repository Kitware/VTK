/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRendererSource.cxx
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
#include "vtkRendererSource.h"
#include "vtkRenderWindow.h"
#include "vtkFloatArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkObjectFactory.h"


//----------------------------------------------------------------------------
vtkRendererSource* vtkRendererSource::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkRendererSource");
  if(ret)
    {
    return (vtkRendererSource*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkRendererSource;
}




vtkRendererSource::vtkRendererSource()
{
  this->Input = NULL;
  this->WholeWindow = 0;
  this->RenderFlag = 0;
  this->DepthValues = 0;
}


vtkRendererSource::~vtkRendererSource()
{
  if (this->Input)
    {
    this->Input->UnRegister(this);
    this->Input = NULL;
    }
}

void vtkRendererSource::Execute()
{
  int numOutPts;
  vtkUnsignedCharArray *outScalars;
  float x1,y1,x2,y2;
  unsigned char *pixels, *ptr;
  int dims[3];
  vtkStructuredPoints *output = this->GetOutput();
  vtkRenderWindow *renWin;
  

  vtkDebugMacro(<<"Converting points");

  if (this->Input == NULL )
    {
    vtkErrorMacro(<<"Please specify a renderer as input!");
    return;
    }

  renWin = this->Input->GetRenderWindow();
  if (renWin == NULL)
    {
    return;
    }
  
  if (this->RenderFlag)
    {
    renWin->Render();
    }
  
  // calc the pixel range for the renderer
  x1 = this->Input->GetViewport()[0]*
    ((this->Input->GetRenderWindow())->GetSize()[0] - 1);
  y1 = this->Input->GetViewport()[1]*
    ((this->Input->GetRenderWindow())->GetSize()[1] - 1);
  x2 = this->Input->GetViewport()[2]*
    ((this->Input->GetRenderWindow())->GetSize()[0] - 1);
  y2 = this->Input->GetViewport()[3]*
    ((this->Input->GetRenderWindow())->GetSize()[1] - 1);

  if (this->WholeWindow)
    {
    x1 = 0;
    y1 = 0;
    x2 = (this->Input->GetRenderWindow())->GetSize()[0] - 1;
    y2 = (this->Input->GetRenderWindow())->GetSize()[1] - 1;
    }
  
  // Get origin, aspect ratio and dimensions from this->Input
  dims[0] = (int)(x2 - x1 + 1); dims[1] = (int)(y2 -y1 + 1); dims[2] = 1;
  output->SetDimensions(dims);
  output->SetSpacing(1,1,1);
  output->SetOrigin(0,0,0);

  // Allocate data.  Scalar type is FloatScalars.
  numOutPts = dims[0] * dims[1];
  outScalars = vtkUnsignedCharArray::New();
  outScalars->SetNumberOfComponents(3);

  pixels = (this->Input->GetRenderWindow())->GetPixelData((int)x1,(int)y1,
                                                          (int)x2,(int)y2,1);

  // copy scalars over
  ptr = outScalars->WritePointer(0,numOutPts*3);
  memcpy(ptr,pixels,3*numOutPts);

  // Lets get the ZBuffer also, if requested.
  if ( this->DepthValues )
    {
    float *zBuf, *zPtr;
    zBuf = (this->Input->GetRenderWindow())->GetZbufferData((int)x1,(int)y1,
                                                            (int)x2,(int)y2);

    vtkFloatArray *zArray = vtkFloatArray::New();
    zArray->Allocate(numOutPts);
    zArray->SetNumberOfTuples(numOutPts);
    zPtr = zArray->WritePointer(0, numOutPts);
    memcpy(zPtr,zBuf,numOutPts*sizeof(float));

    zArray->SetName("ZBuffer");
    output->GetPointData()->AddArray(zArray);
    zArray->Delete();
    delete [] zBuf;
    }
  
  // Update ourselves
  output->GetPointData()->SetScalars(outScalars);
  outScalars->Delete();
  delete [] pixels;
}

void vtkRendererSource::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkStructuredPointsSource::PrintSelf(os,indent);

  os << indent << "RenderFlag: " << (this->RenderFlag ? "On\n" : "Off\n");

  if ( this->Input )
    {
    os << indent << "Input:\n";
    this->Input->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Input: (none)\n";
    }

  os << indent << "Whole Window: " << (this->WholeWindow ? "On\n" : "Off\n");
  os << indent << "Depth Values: " << (this->DepthValues ? "On\n" : "Off\n");

}


unsigned long vtkRendererSource::GetMTime()
{
  unsigned long mTime=this->MTime.GetMTime();
  unsigned long transMTime;

  if ( this->Input )
    {
    transMTime = this->Input->GetMTime();
    mTime = ( transMTime > mTime ? transMTime : mTime );
    }

  return mTime;
}

//----------------------------------------------------------------------------
// Consider renderer for PiplineMTime
void vtkRendererSource::UpdateInformation()
{
  unsigned long t1, t2;
  vtkStructuredPoints *output = this->GetOutput();
  vtkRenderer *ren = this->GetInput();
  vtkActorCollection *actors;
  vtkActor *actor;
  vtkMapper *mapper;
  vtkDataObject *data;
  float x1,y1,x2,y2;
  
  if (output == NULL || ren == NULL || ren->GetRenderWindow() == NULL)
    {
    return;
    }
  
  // calc the pixel range for the renderer
  x1 = ren->GetViewport()[0] * ((ren->GetRenderWindow())->GetSize()[0] - 1);
  y1 = ren->GetViewport()[1] * ((ren->GetRenderWindow())->GetSize()[1] - 1);
  x2 = ren->GetViewport()[2] * ((ren->GetRenderWindow())->GetSize()[0] - 1);
  y2 = ren->GetViewport()[3] *((ren->GetRenderWindow())->GetSize()[1] - 1);
  if (this->WholeWindow)
    {
    x1 = 0;
    y1 = 0;
    x2 = (this->Input->GetRenderWindow())->GetSize()[0] - 1;
    y2 = (this->Input->GetRenderWindow())->GetSize()[1] - 1;
    }    
  output->SetWholeExtent(0, x2-x1, 0, y2-y1, 0, 0);
  output->SetScalarType(VTK_UNSIGNED_CHAR);
  output->SetNumberOfScalarComponents(3);
  
  // Update information on the input and
  // compute information that is general to vtkDataObject.
  t1 = this->GetMTime();
  t2 = ren->GetMTime();
  if (t2 > t1)
    {
    t1 = t2;
    }
  actors = ren->GetActors();
  actors->InitTraversal();
  while ( (actor = actors->GetNextItem()) )
    {
    t2 = actor->GetMTime();
    if (t2 > t1)
      {
      t1 = t2;
      }
    mapper = actor->GetMapper();
    if (mapper)
      {
      t2 = mapper->GetMTime();
      if (t2 > t1)
        {
        t1 = t2;
        }
      data = mapper->GetInput();
      if (data)
        {
        data->UpdateInformation();
        }
      t2 = data->GetMTime();
      if (t2 > t1)
        {
        t1 = t2;
        }
      t2 = data->GetPipelineMTime();
      if (t2 > t1)
        {
        t1 = t2;
        }
      }  
    }

  output->SetPipelineMTime(t1);
  this->InformationTime.Modified();
}
