/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImporter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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

#include "vtkImporter.h"
#include "vtkRendererCollection.h"

vtkImporter::vtkImporter ()
{
  this->FileName = NULL;
  this->FileFD = NULL;
  this->Renderer = NULL;
  this->RenderWindow = NULL;
  this->ComputeNormals = 0;
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

  // Open the import file
  if (this->OpenImportFile ())
    {

    if (this->ImportBegin ())
      {
      // this->Import actors, cameras, lights and properties
      this->ImportActors (renderer);
      this->ImportCameras (renderer);
      this->ImportLights (renderer);
      this->ImportProperties (renderer);
      }
    // Close the import file
    this->CloseImportFile ();
    }
}

// Description:
// Open an import file. Returns zero if error.
int vtkImporter::OpenImportFile ()
{
  vtkDebugMacro(<< "Opening import file");

  if ( !this->FileName )
    {
    vtkErrorMacro(<< "No file specified!");
    return 0;
    }
  this->FileFD = fopen (this->FileName, "r");
  if (this->FileFD == NULL)
    {
    vtkErrorMacro(<< "Unable to open file: "<< this->FileName);
    return 0;
    }
  return 1;
}

// Description:
// Close an import file.
void vtkImporter::CloseImportFile()
{
  vtkDebugMacro(<<"Closing import file");
  if ( this->FileFD != NULL ) fclose (this->FileFD);
  this->FileFD = NULL;
}

void vtkImporter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);
}






