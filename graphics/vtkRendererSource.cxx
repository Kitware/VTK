/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRendererSource.cxx
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
#include "vtkRendererSource.h"
#include "vtkRenderWindow.h"
#include "vtkFloatArray.h"
#include "vtkObjectFactory.h"
#include "vtkStructuredInformation.h"


//------------------------------------------------------------------------------
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
  vtkScalars *outScalars;
  float x1,y1,x2,y2;
  unsigned char *pixels, *ptr;
  float *zBuf, *zPtr;
  int dims[3];
  vtkStructuredPoints *output = this->GetOutput();
  vtkFieldData *zField;
  vtkFloatArray *zArray;
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
  
  renWin->Render();
  
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
  outScalars = vtkScalars::New(VTK_UNSIGNED_CHAR,3);

  pixels = (this->Input->GetRenderWindow())->GetPixelData((int)x1,(int)y1,
							  (int)x2,(int)y2,1);

  // copy scalars over
  ptr = ((vtkUnsignedCharArray *)outScalars->GetData())->WritePointer(0,numOutPts*3);
  memcpy(ptr,pixels,3*numOutPts);

  // Lets get the ZBuffer also.
  zBuf = (this->Input->GetRenderWindow())->GetZbufferData((int)x1,(int)y1,
							  (int)x2,(int)y2);
  zArray = vtkFloatArray::New();
  zArray->Allocate(numOutPts);
  zArray->SetNumberOfTuples(numOutPts);
  zPtr = zArray->WritePointer(0, numOutPts);
  memcpy(zPtr,zBuf,numOutPts*sizeof(float));

  zField = vtkFieldData::New();
  zField->SetArray(0, zArray);
  zField->SetArrayName(0, "ZBuffer");
  zArray->Delete();

  // Update ourselves
  output->GetPointData()->SetScalars(outScalars);
  outScalars->Delete();
  output->GetPointData()->SetFieldData(zField);
  zField->Delete();

  // free the memory
  delete [] pixels;
  delete [] zBuf;
}

void vtkRendererSource::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkStructuredPointsSource::PrintSelf(os,indent);

  os << indent << "Whole Window: " << (this->WholeWindow ? "On\n" : "Off\n");

  if ( this->Input )
    {
    os << indent << "Input:\n";
    this->Input->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Input: (none)\n";
    }
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
  unsigned long t1, t2, size;
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
  size = 1 + (unsigned long)((x2-x1+1) * (y2-y1+1) / 1000);
  output->SetEstimatedWholeMemorySize(size);
  
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
  output->GetDataInformation()->SetLocality(0);
  //output->SetEstimatedWholeMemorySize(size);
  this->InformationTime.Modified();
}
