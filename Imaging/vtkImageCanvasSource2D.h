/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageCanvasSource2D.h
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
// .NAME vtkImageCanvasSource2D - Paints on a canvas
// .SECTION Description
// vtkImageCanvasSource2D is a source that starts as a blank image.
// you may add to the image with two-dimensional drawing routines.
// It can paint multi-spectral images.


#ifndef __vtkImageCanvasSource2D_h
#define __vtkImageCanvasSource2D_h

#include <math.h>
#include "vtkStructuredPoints.h"

//
// Special classes for manipulating data
//
//BTX - begin tcl exclude
//
// For the fill functionality (use connector ??)
class vtkImageCanvasSource2DPixel { //;prevent man page generation
public:
  static vtkImageCanvasSource2DPixel *New() 
    { return new vtkImageCanvasSource2DPixel ;}
  int X;
  int Y;
  void *Pointer;
  vtkImageCanvasSource2DPixel *Next;
};
//ETX - end tcl exclude
//


class VTK_EXPORT vtkImageCanvasSource2D : public vtkStructuredPoints
{
public:
  // Description:
  // Construct an instance of vtkImageCanvasSource2D with no data.
  static vtkImageCanvasSource2D *New();

  vtkTypeMacro(vtkImageCanvasSource2D,vtkStructuredPoints);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // To drawing into a different image, set it with this method.
  void SetImageData(vtkImageData *image);
  vtkGetObjectMacro(ImageData, vtkImageData);
  
  // Description:
  // Set/Get DrawValue.  This is the value that is used when filling data
  // or drawing lines.
  vtkSetVector4Macro(DrawColor, float);
  vtkGetVector4Macro(DrawColor, float);
  void SetDrawColor(float a) {this->SetDrawColor(a, 0.0, 0.0, 0.0);}
  void SetDrawColor(float a,float b) {this->SetDrawColor(a, b, 0.0, 0.0);}
  void SetDrawColor(float a, float b, float c) {this->SetDrawColor(a, b, c, 0.0);}
    
  void FillBox(int min0, int max0, int min1, int max1);
  void FillTube(int x0, int y0, int x1, int y1, float radius);
  void FillTriangle(int x0, int y0, int x1, int y1, int x2, int y2);
  void DrawCircle(int c0, int c1, float radius);
  void DrawPoint(int p0, int p1);
  void DrawSegment(int x0, int y0, int x1, int y1);
  void DrawSegment3D(float *p0, float *p1);
  void DrawSegment3D(float x1, float y1, float z1, float x2, float y2, float z2) 
    { float p1[3], p2[3]; 
    p1[0] = x1; p1[1] = y1; p1[2] = z1; p2[0] = x2; p2[1] = y2; p2[2] = z2;
    this->DrawSegment3D(p1, p2);}

  // Description:
  // Fill a colored area with another color. (like connectivity)
  // All pixels connected to pixel (x, y) get replaced by draw color.
  void FillPixel(int x, int y);

  // Description:
  // To make Canvas source more like other sources, this get output
  // method should be used.
  vtkImageData *GetOutput() {return this;}
  
  // Description:
  // These methods also set the WholeExtent of this "DataObject".
  void SetExtent(int *extent);
  void SetExtent(int x1, int x2, int y1, int y2, int z1, int z2);
  
  // Description:
  // If the canvas is a 3D volume, then this z value is used
  // as the default for 2D operations.
  vtkSetMacro(DefaultZ, int);
  vtkGetMacro(DefaultZ, int);

protected:
  vtkImageCanvasSource2D();
  // Destructor: Deleting a vtkImageCanvasSource2D automatically deletes the
  // associated vtkImageData.  However, since the data is reference counted,
  // it may not actually be deleted.
  ~vtkImageCanvasSource2D();
  vtkImageCanvasSource2D(const vtkImageCanvasSource2D&);
  void operator=(const vtkImageCanvasSource2D&);

  vtkImageData *ImageData;
  float DrawColor[4];
  int DefaultZ;
  
  int ClipSegment(int &a0, int &a1, int &b0, int &b1);
};



#endif


