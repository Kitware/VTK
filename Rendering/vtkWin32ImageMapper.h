/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWin32ImageMapper.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Matt Turek who developed this class.

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
// .NAME vtkWin32ImageMapper - (obsolete) 2D image display support for Microsoft windows
// .SECTION Description
// vtkWin32ImageMapper is a concrete subclass of vtkImageMapper that
// renders images under Microsoft windows.

// .SECTION See Also
// vtkImageMapper

#ifndef __vtkWin32ImageMapper_h
#define __vtkWin32ImageMapper_h


#include "vtkImageMapper.h"
#include "vtkLookupTable.h"

class vtkImageActor2D;

#ifndef VTK_REMOVE_LEGACY_CODE
class VTK_RENDERING_EXPORT vtkWin32ImageMapper : public vtkImageMapper
{
public:
  static vtkWin32ImageMapper *New();
  vtkTypeMacro(vtkWin32ImageMapper,vtkImageMapper);

  // Description:
  // Handle the render method.
  void RenderOverlay(vtkViewport* viewport, vtkActor2D* actor) {
    this->RenderStart(viewport,actor);}

  // Description:
  // Called by the Render function in vtkImageMapper.  Actually draws
  // the image to the screen.
  void RenderData(vtkViewport* viewport, vtkImageData* data,
		  vtkActor2D* actor);

  // Description:
  // Compute modified time including lookuptable
  unsigned long int GetMTime();

  // Description:
  // standard Printself routine
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The ImageMappers convert ImageData into a greyscale image when a single
  // scalar component is present.
  // If a lookuptable is supplied, values are mapped through the lookuptable
  // to generate a colour image. If the number of scalar components is greater
  // then one, the lookuptable is ignored. If the lookuptable is NULL, a default
  // greyscale image is generated. Users should ensure that the range of the
  // lookuptable is {0,255} for full colour effects
  vtkSetObjectMacro(LookupTable, vtkLookupTable);
  vtkGetObjectMacro(LookupTable, vtkLookupTable);

  // Description:
  // CreateBitmapObject and GenerateBitmapData are utility functions which
  // allow one to hook into the display routines and provide the user
  // with an easy way of converting an ImageData object into a windows
  // bitmap object. They are also used internally by the mapper and should not
  // be modified or used standalone alone without caution.
  static HBITMAP CreateBitmapObject(
    HBITMAP oldBitmap, BITMAPINFO &dataHeader, HDC windowDC,
    unsigned char *&DataOut, vtkImageData *data, int width, int height);

  static void GenerateBitmapData(
    vtkImageData *data, void *inptr, unsigned char *DataOut, int dim,
    int DisplayExtent[6], float cwindow, float clevel, float cshift, float cscale,
    vtkLookupTable *lut);

  unsigned char *DataOut;	// the data in the DIBSection
  HBITMAP HBitmap;			// our handle to the DIBSection

protected:
  vtkLookupTable *LookupTable;

  vtkWin32ImageMapper();
  ~vtkWin32ImageMapper();
  vtkWin32ImageMapper(const vtkWin32ImageMapper&);
  void operator=(const vtkWin32ImageMapper&);

};
#endif

#endif









