/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImporter.cxx
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

#include "vtkImporter.h"
#include "vtkRendererCollection.h"

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
  this->RenderWindow->GetRenderers()->InitTraversal();
  renderer = this->RenderWindow->GetRenderers()->GetNextItem();
  if (renderer == NULL)
    {
    vtkDebugMacro( <<"Creating a Renderer\n");
    this->Renderer = vtkRenderer::New ();
    renderer = this->Renderer;
    this->RenderWindow->AddRenderer (renderer);
    }
  else
    {
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
  vtkObject::PrintSelf(os,indent);

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






