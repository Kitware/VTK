/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageViewer.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.


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
#include "vtkImageViewer.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkImageViewer* vtkImageViewer::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageViewer");
  if(ret)
    {
    return (vtkImageViewer*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageViewer;
}




//----------------------------------------------------------------------------
vtkImageViewer::vtkImageViewer()
{
  this->ImageWindow = vtkImageWindow::New();
  this->Imager      = vtkImager::New();
  this->ImageMapper = vtkImageMapper::New();
  this->Actor2D     = vtkActor2D::New();

  // setup the pipeline
  this->Actor2D->SetMapper(this->ImageMapper);
  this->Imager->AddActor2D(this->Actor2D);
  this->ImageWindow->AddImager(this->Imager);
}


//----------------------------------------------------------------------------
vtkImageViewer::~vtkImageViewer()
{
  this->ImageMapper->Delete();
  this->Actor2D->Delete();
  this->Imager->Delete();
  this->ImageWindow->Delete();
}

//----------------------------------------------------------------------------
void vtkImageViewer::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os, indent);
  os << indent << *this->ImageWindow << endl;
  os << indent << *this->Imager << endl;
  os << indent << *this->ImageMapper << endl;
}



//----------------------------------------------------------------------------
void vtkImageViewer::SetSize(int a[2])
{
  this->SetSize(a[0],a[1]);
}
//----------------------------------------------------------------------------
void vtkImageViewer::SetPosition(int a[2])
{
  this->SetPosition(a[0],a[1]);
}


void vtkImageViewer::Render()
{
  // initialize the size if not set yet
  if (this->ImageWindow->GetSize()[0] == 0 && this->ImageMapper->GetInput())
    {
    // get the size from the mappers input
    this->ImageMapper->GetInput()->UpdateInformation();
    int *ext = this->ImageMapper->GetInput()->GetWholeExtent();
    // if it would be smaller than 100 by 100 then limit to 100 by 100
    int xs = ext[1] - ext[0] + 1;
    int ys = ext[3] - ext[2] + 1;
    this->ImageWindow->SetSize(xs < 100 ? 100 : xs,
                               ys < 100 ? 100 : ys);
    }
  
  this->ImageWindow->Render();
}
