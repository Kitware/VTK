/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageViewer.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageViewer.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkImageViewer, "1.37");
vtkStandardNewMacro(vtkImageViewer);

//----------------------------------------------------------------------------
vtkImageViewer::vtkImageViewer()
{
  this->RenderWindow = vtkRenderWindow::New();
  this->Renderer = vtkRenderer::New();
  
  this->ImageMapper = vtkImageMapper::New();
  this->Actor2D     = vtkActor2D::New();

  // setup the pipeline
  this->Actor2D->SetMapper(this->ImageMapper);
  this->Renderer->AddActor2D(this->Actor2D);
  this->RenderWindow->AddRenderer(this->Renderer);
}


//----------------------------------------------------------------------------
vtkImageViewer::~vtkImageViewer()
{
  this->ImageMapper->Delete();
  this->Actor2D->Delete();
  this->RenderWindow->Delete();
  this->Renderer->Delete();
}

//----------------------------------------------------------------------------
void vtkImageViewer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << *this->ImageMapper << endl;
  os << indent << *this->RenderWindow << endl;
  os << indent << *this->Renderer << endl;
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
  if (this->RenderWindow->GetSize()[0] == 0 && this->ImageMapper->GetInput())
    {
    // get the size from the mappers input
    this->ImageMapper->GetInput()->UpdateInformation();
    int *ext = this->ImageMapper->GetInput()->GetWholeExtent();
    // if it would be smaller than 100 by 100 then limit to 100 by 100
    int xs = ext[1] - ext[0] + 1;
    int ys = ext[3] - ext[2] + 1;
    this->RenderWindow->SetSize(xs < 150 ? 150 : xs,
                                ys < 100 ? 100 : ys);
    }
  
  this->RenderWindow->Render();
}
