/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWin32ImageMapper.h
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
  vtkTypeRevisionMacro(vtkWin32ImageMapper,vtkImageMapper);

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

  unsigned char *DataOut;       // the data in the DIBSection
  HBITMAP HBitmap;                      // our handle to the DIBSection

protected:
  vtkLookupTable *LookupTable;

  vtkWin32ImageMapper();
  ~vtkWin32ImageMapper();

private:
  vtkWin32ImageMapper(const vtkWin32ImageMapper&);  // Not implemented.
  void operator=(const vtkWin32ImageMapper&);  // Not implemented.
};
#endif

#endif









