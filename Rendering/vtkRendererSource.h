/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRendererSource.h
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
// .NAME vtkRendererSource - take a renderer into the pipeline
// .SECTION Description
// vtkRendererSource is a source object that gets its input from a 
// renderer and converts it to structured points. This can then be 
// used in a visualization pipeline. You must explicitly send a 
// Modify() to this object to get it to reload its data from the
// renderer. Consider using vtkWindowToImageFilter instead of this
// class.
//
// The data placed into the output is the renderer's image rgb values.
// Optionally, you can also grab the image depth (e.g., z-buffer) values, and
// place then into the output (point) field data.

// .SECTION see also
// vtkWindowToImageFilter vtkRenderer vtkStructuredPoints

#ifndef __vtkRendererSource_h
#define __vtkRendererSource_h

#include "vtkStructuredPointsSource.h"
#include "vtkRenderer.h"

class VTK_RENDERING_EXPORT vtkRendererSource : public vtkStructuredPointsSource
{
public:
  static vtkRendererSource *New();
  vtkTypeMacro(vtkRendererSource,vtkStructuredPointsSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Return the MTime also considering the Renderer.
  unsigned long GetMTime();

  // Description:
  // Indicates what renderer to get the pixel data from.
  vtkSetObjectMacro(Input,vtkRenderer);

  // Description:
  // Returns which renderer is being used as the source for the pixel data.
  vtkGetObjectMacro(Input,vtkRenderer);

  // Description:
  // Use the entire RenderWindow as a data source or just the Renderer.
  // The default is zero, just the Renderer.
  vtkSetMacro(WholeWindow,int);
  vtkGetMacro(WholeWindow,int);
  vtkBooleanMacro(WholeWindow,int);
  
  // Description:
  // If this flag is on, the Executing causes a render first.
  vtkSetMacro(RenderFlag, int);
  vtkGetMacro(RenderFlag, int);
  vtkBooleanMacro(RenderFlag, int);

  // Description:
  // A boolean value to control whether to grab z-buffer 
  // (i.e., depth values) along with the image data. The z-buffer data
  // is placed into the field data attributes.
  vtkSetMacro(DepthValues,int);
  vtkGetMacro(DepthValues,int);
  vtkBooleanMacro(DepthValues,int);
  
protected:
  vtkRendererSource();
  ~vtkRendererSource();

  void Execute();
  
  void UpdateInformation();
  
  vtkRenderer *Input;
  int WholeWindow;
  int RenderFlag;
  int DepthValues;

private:
  vtkRendererSource(const vtkRendererSource&);  // Not implemented.
  void operator=(const vtkRendererSource&);  // Not implemented.
};

#endif


