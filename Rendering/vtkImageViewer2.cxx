/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageViewer2.cxx
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
#include "vtkImageViewer2.h"
#include "vtkObjectFactory.h"
#include "vtkInteractorStyleImage.h"


//----------------------------------------------------------------------------
vtkImageViewer2* vtkImageViewer2::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageViewer2");
  if(ret)
    {
    return (vtkImageViewer2*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageViewer2;
}

//----------------------------------------------------------------------------
vtkImageViewer2::vtkImageViewer2()
{
  this->RenderWindow = vtkRenderWindow::New();
  this->Renderer     = vtkRenderer::New();
  this->ImageActor   = vtkImageActor::New();
  this->WindowLevel  = vtkImageMapToWindowLevelColors::New();
  
  // setup the pipeline
  this->ImageActor->SetInput(this->WindowLevel->GetOutput());
  this->Renderer->AddProp(this->ImageActor);
  this->RenderWindow->AddRenderer(this->Renderer);
  this->FirstRender = 1;
  
  this->Interactor = 0;
  this->InteractorStyle = 0;
}


//----------------------------------------------------------------------------
vtkImageViewer2::~vtkImageViewer2()
{
  this->WindowLevel->Delete();
  this->ImageActor->Delete();
  this->Renderer->Delete();
  this->RenderWindow->Delete();

  if (this->Interactor)
    {
    this->Interactor->Delete();
    }
  if (this->InteractorStyle)
    {
    this->InteractorStyle->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkImageViewer2::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os, indent);
  os << indent << *this->RenderWindow << endl;
  os << indent << *this->Renderer << endl;
  os << indent << *this->ImageActor << endl;
}



//----------------------------------------------------------------------------
void vtkImageViewer2::SetSize(int a[2])
{
  this->SetSize(a[0],a[1]);
}
//----------------------------------------------------------------------------
void vtkImageViewer2::SetPosition(int a[2])
{
  this->SetPosition(a[0],a[1]);
}

class vtkImageViewer2Callback : public vtkCommand
{
public:
  void Execute(vtkObject *caller, unsigned long event, void *callData)
    {
      if (callData)
        {
        this->InitialWindow = this->IV->GetWindowLevel()->GetWindow();
        this->InitialLevel = this->IV->GetWindowLevel()->GetLevel();
        return;
        }
      
      // adjust the window level here
      vtkInteractorStyleImage *isi = 
        static_cast<vtkInteractorStyleImage *>(caller);

      int *size = this->IV->GetRenderWindow()->GetSize();
      float window = this->InitialWindow;
      float level = this->InitialLevel;
      
      // compute normalized delta
      float dx = 4.0 * (isi->GetWindowLevelCurrentPosition()[0] - 
                        isi->GetWindowLevelStartPosition()[0]) / size[0];
      float dy = 4.0 * (isi->GetWindowLevelStartPosition()[1] - 
                        isi->GetWindowLevelCurrentPosition()[1]) / size[1];
      
      // scale by current values
      if (fabs(window) > 0.01)
	{
	dx = dx * window;
	}
      else
	{
	dx = dx * (window < 0 ? -0.01 : 0.01);
	}
      if (fabs(level) > 0.01)
	{
	dy = dy * level;
	}
      else
	{
	dy = dy * (level < 0 ? -0.01 : 0.01);
	}
      
      // abs so that direction does not flip
      if (window < 0.0) 
        {
        dx = -1*dx;
        }
      if (level < 0.0) 
        {
        dy = -1*dy;
        }
      
      // compute new window level
      float newWindow = dx + window;
      float newLevel;
      newLevel = level - dy;
      
      // stay away from zero and really
      if (fabs(newWindow) < 0.01)
        {
        newWindow = 0.01*(newWindow < 0 ? -1 : 1);
        }
      if (fabs(newLevel) < 0.01)
        {
        newLevel = 0.01*(newLevel < 0 ? -1 : 1);
        }
      
      this->IV->GetWindowLevel()->SetWindow(newWindow);
      this->IV->GetWindowLevel()->SetLevel(newLevel);
      this->IV->Render();
    }
  
  vtkImageViewer2 *IV;
  float InitialWindow;
  float InitialLevel;
};


void vtkImageViewer2::SetupInteractor(vtkRenderWindowInteractor *rwi)
{
  if (this->Interactor && rwi != this->Interactor)
    {
    this->Interactor->Delete();
    }
  if (!this->InteractorStyle)
    {
    this->InteractorStyle = vtkInteractorStyleImage::New();
    vtkImageViewer2Callback *cbk = new vtkImageViewer2Callback;
    cbk->IV = this;
    this->InteractorStyle->AddObserver(vtkCommand::WindowLevelEvent,cbk);
    }
  
  if (!this->Interactor)
    {
    this->Interactor = rwi;
    rwi->Register(this);
    }
  this->Interactor->SetInteractorStyle(this->InteractorStyle);
  this->Interactor->SetRenderWindow(this->RenderWindow);
}

void vtkImageViewer2::Render()
{
  if (this->FirstRender)
    {
    // initialize the size if not set yet
    if (this->RenderWindow->GetSize()[0] == 0 && this->ImageActor->GetInput())
      {
      // get the size from the mappers input
      this->WindowLevel->GetInput()->UpdateInformation();
      int *ext = this->WindowLevel->GetInput()->GetWholeExtent();
      // if it would be smaller than 100 by 100 then limit to 100 by 100
      int xs = ext[1] - ext[0] + 1;
      int ys = ext[3] - ext[2] + 1;
      this->RenderWindow->SetSize(xs < 150 ? 150 : xs,
                                  ys < 100 ? 100 : ys);
      this->Renderer->GetActiveCamera()->SetParallelScale(xs < 150 ? 75 : xs/2);
      }
    this->Renderer->GetActiveCamera()->ParallelProjectionOn();
    this->FirstRender = 0;  
    }
  
  this->RenderWindow->Render();
}

