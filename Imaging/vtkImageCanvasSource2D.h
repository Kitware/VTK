/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageCanvasSource2D.h
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
// .NAME vtkImageCanvasSource2D - Paints on a canvas
// .SECTION Description
// vtkImageCanvasSource2D is a source that starts as a blank image.
// you may add to the image with two-dimensional drawing routines.
// It can paint multi-spectral images.  This object is unusual because
// it is a data object itself and not a source.


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


class VTK_IMAGING_EXPORT vtkImageCanvasSource2D : public vtkStructuredPoints
{
public:
  // Description:
  // Construct an instance of vtkImageCanvasSource2D with no data.
  static vtkImageCanvasSource2D *New();

  vtkTypeRevisionMacro(vtkImageCanvasSource2D,vtkStructuredPoints);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // To draw into a different image, set it with this method.
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
  // All pixels connected (and with the same value) to pixel (x, y) 
  // get replaced by the current "DrawColor".
  void FillPixel(int x, int y);

  // Description:
  // To make Canvas source more like other sources, this get output
  // method should be used.
  vtkImageData *GetOutput() {return this;}
  
  // Description:
  // These methods also set the WholeExtent of this "DataObject".
  // This is just like vtkImageData.  It sets the size of the canvas.
  // Extent is a min max 3D box.  Minimums and maximums are inclusive.
  void SetExtent(int *extent);
  void SetExtent(int x1, int x2, int y1, int y2, int z1, int z2);
  
  // Description:
  // The drawing operations can only draw into one 2D XY plane at a time.
  // If the canvas is a 3D volume, then this z value is used
  // as the default for 2D operations.
  vtkSetMacro(DefaultZ, int);
  vtkGetMacro(DefaultZ, int);

  // Description:
  // Set/Get Ratio. This is the value that is used to pre-multiply each
  // (x, y, z) drawing coordinates (including DefaultZ).
  vtkSetVector3Macro(Ratio, float);
  vtkGetVector3Macro(Ratio, float);

protected:
  vtkImageCanvasSource2D();
  // Destructor: Deleting a vtkImageCanvasSource2D automatically deletes the
  // associated vtkImageData.  However, since the data is reference counted,
  // it may not actually be deleted.
  ~vtkImageCanvasSource2D();

  vtkImageData *ImageData;
  float DrawColor[4];
  int DefaultZ;
  float Ratio[3];
  
  int ClipSegment(int &a0, int &a1, int &b0, int &b1);
private:
  vtkImageCanvasSource2D(const vtkImageCanvasSource2D&);  // Not implemented.
  void operator=(const vtkImageCanvasSource2D&);  // Not implemented.
};



#endif


