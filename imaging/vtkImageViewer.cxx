/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageViewer.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkImageViewer.h"

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
    this->ImageMapper->GetInput()->UpdateImageInformation();
    int *ext = this->ImageMapper->GetInput()->GetWholeExtent();
    this->ImageWindow->SetSize(ext[1] - ext[0] + 1, ext[3] - ext[2] + 1);
    }
  
  this->ImageWindow->Render();
}
