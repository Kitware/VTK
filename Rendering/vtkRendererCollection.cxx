/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRendererCollection.cxx
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
#include <stdlib.h>
#include "vtkRendererCollection.h"
#include "vtkRenderWindow.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkRendererCollection* vtkRendererCollection::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkRendererCollection");
  if(ret)
    {
    return (vtkRendererCollection*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkRendererCollection;
}




// Forward the Render() method to each renderer in the list.
void vtkRendererCollection::Render()
{
  vtkRenderer      *ren, *firstRen;
  vtkRenderWindow  *renWin;
  int               numLayers, i;

  this->InitTraversal();
  firstRen = this->GetNextItem();
  if (firstRen == NULL)
    {
    // We cannot determine the number of layers because there are no
    // renderers.  No problem, just return.
    return;
    }
  renWin = firstRen->GetRenderWindow();
  numLayers = renWin->GetNumLayers();

  // Only have the renderers render from back to front.  This is necessary
  // because transparent renderers clear the z-buffer before each render and
  // then overlay their image.
  for (i = numLayers-1 ; i >= 0 ; i--)
    {
    for (this->InitTraversal(); (ren = this->GetNextItem()); )
      {
      if (ren->GetLayer() == i)
        {
        ren->Render();
        }
      }
    }

  // Let the user know if they have put a renderer at an unused layer.
  for (this->InitTraversal(); (ren = this->GetNextItem()); )
    {
    if (ren->GetLayer() < 0 || ren->GetLayer() >= numLayers)
      {
      vtkErrorMacro(<< "Invalid layer for renderer: not rendered.");
      }
    }
}

void vtkRendererCollection::RenderOverlay()
{
  vtkRenderer      *ren, *firstRen;
  vtkRenderWindow  *renWin;
  int               numLayers, i;

  this->InitTraversal();
  firstRen = this->GetNextItem();
  if (firstRen == NULL)
    {
    // We cannot determine the number of layers because there are no
    // renderers.  No problem, just return.
    return;
    }
  renWin = firstRen->GetRenderWindow();
  numLayers = renWin->GetNumLayers();

  // Only have the renderers render from back to front.  This is necessary
  // because transparent renderers clear the z-buffer before each render and
  // then overlay their image.
  for (i = numLayers-1 ; i >= 0 ; i--)
    {
    for (this->InitTraversal(); (ren = this->GetNextItem()); )
      {
      if (ren->GetLayer() == i)
        {
        ren->RenderOverlay();
        }
      }
    }

  // Let the user know if they have put a renderer at an unused layer.
  for (this->InitTraversal(); (ren = this->GetNextItem()); )
    {
    if (ren->GetLayer() < 0 || ren->GetLayer() >= numLayers)
      {
      vtkErrorMacro(<< "Invalid layer for renderer: not render overlayed.");
      }
    }
}


