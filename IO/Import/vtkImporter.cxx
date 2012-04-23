/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImporter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImporter.h"
#include "vtkRendererCollection.h"
#include "vtkRenderWindow.h"


vtkCxxSetObjectMacro(vtkImporter,RenderWindow,vtkRenderWindow);

vtkImporter::vtkImporter ()
{
  this->Renderer = NULL;
  this->RenderWindow = NULL;
}

vtkImporter::~vtkImporter ()
{
  this->SetRenderWindow(NULL);

  if (this->Renderer)
    {
    this->Renderer->UnRegister( NULL );
    this->Renderer = NULL;
    }

}

void vtkImporter::ReadData()
{
  // this->Import actors, cameras, lights and properties
  this->ImportActors (this->Renderer);
  this->ImportCameras (this->Renderer);
  this->ImportLights (this->Renderer);
  this->ImportProperties (this->Renderer);
}

void vtkImporter::Read ()
{
  vtkRenderer *renderer;

  // if there is no render window, create one
  if (this->RenderWindow == NULL)
    {
    vtkDebugMacro( <<"Creating a RenderWindow\n");
    this->RenderWindow = vtkRenderWindow::New ();
    }

  // Get the first renderer in the render window
  renderer = this->RenderWindow->GetRenderers()->GetFirstRenderer();
  if (renderer == NULL)
    {
    vtkDebugMacro( <<"Creating a Renderer\n");
    this->Renderer = vtkRenderer::New ();
    renderer = this->Renderer;
    this->RenderWindow->AddRenderer (renderer);
    }
  else
    {
    if (this->Renderer)
      {
      this->Renderer->UnRegister(NULL);
      }
    this->Renderer = renderer;
    this->Renderer->Register( this );
    }

  if (this->ImportBegin ())
    {
    this->ReadData();
    this->ImportEnd();
    }
}

void vtkImporter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Render Window: ";
  if ( this->RenderWindow )
    {
    os << this->RenderWindow << "\n";
    }
  else
    {
    os << "(none)\n";
    }

  os << indent << "Renderer: ";
  if ( this->Renderer )
    {
    os << this->Renderer << "\n";
    }
  else
    {
    os << "(none)\n";
    }

}






