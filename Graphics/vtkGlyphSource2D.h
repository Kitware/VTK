/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGlyphSource2D.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Tim Smith who sponsored and encouraged the development
             of this class.


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
// .NAME vtkGlyphSource2D - create 2D glyphs represented by vtkPolyData
// .SECTION Description
// vtkGlyphSource2D can generate a family of 2D glyphs each of which lies
// in the x-y plane (i.e., the z-coordinate is zero). The class is a helper 
// class to be used with vtkGlyph2D and vtkXYPlotActor.
//
// To use this class, specify the glyph type to use and its
// attributes. Attributes include its position (i.e., center point), scale,
// color, and whether the symbol is filled or not (a polygon or closed line
// sequence). You can also put a short line through the glyph running from -x
// to +x (the glyph looks like it's on a line), or a cross.

#ifndef __vtkGlyphSource2D_h
#define __vtkGlyphSource2D_h

#include "vtkPolyDataSource.h"

#define VTK_NO_GLYPH 0
#define VTK_VERTEX_GLYPH 1
#define VTK_DASH_GLYPH 2
#define VTK_CROSS_GLYPH 3
#define VTK_THICKCROSS_GLYPH 4
#define VTK_TRIANGLE_GLYPH 5
#define VTK_SQUARE_GLYPH 6
#define VTK_CIRCLE_GLYPH 7
#define VTK_DIAMOND_GLYPH 8
#define VTK_ARROW_GLYPH 9
#define VTK_THICKARROW_GLYPH 10
#define VTK_HOOKEDARROW_GLYPH 11

class VTK_GRAPHICS_EXPORT vtkGlyphSource2D : public vtkPolyDataSource 
{
public:
  vtkTypeMacro(vtkGlyphSource2D,vtkPolyDataSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct a vertex glyph centered at the origin, scale 1.0, white in
  // color, filled, with line segment passing through the point.
  static vtkGlyphSource2D *New();

  // Description:
  // Set the center of the glyph. By default the center is (0,0,0).
  vtkSetVector3Macro(Center,float);
  vtkGetVectorMacro(Center,float,3);

  // Description:
  // Set the scale of the glyph. Note that the glyphs are designed
  // to fit in the (1,1) rectangle.
  vtkSetClampMacro(Scale,float,0.0,VTK_LARGE_FLOAT);
  vtkGetMacro(Scale,float);

  // Description:
  // Set the scale of optional portions of the glyph (e.g., the
  // dash and cross is DashOn() and CrossOn()).
  vtkSetClampMacro(Scale2,float,0.0,VTK_LARGE_FLOAT);
  vtkGetMacro(Scale2,float);

  // Description:
  // Set the color of the glyph. The default color is white.
  vtkSetVector3Macro(Color,float);
  vtkGetVectorMacro(Color,float,3);

  // Description:
  // Specify whether the glyph is filled (a polygon) or not (a
  // closed polygon defined by line segments). This only applies
  // to 2D closed glyphs.
  vtkSetMacro(Filled,int);
  vtkGetMacro(Filled,int);
  vtkBooleanMacro(Filled,int);

  // Description:
  // Specify whether a short line segment is drawn through the
  // glyph. (This is in addition to the glyph. If the glyph type
  // is set to "Dash" there is no need to enable this flag.)
  vtkSetMacro(Dash,int);
  vtkGetMacro(Dash,int);
  vtkBooleanMacro(Dash,int);

  // Description:
  // Specify whether a cross is drawn as part of the glyph. (This 
  // is in addition to the glyph. If the glyph type is set to 
  // "Cross" there is no need to enable this flag.)
  vtkSetMacro(Cross,int);
  vtkGetMacro(Cross,int);
  vtkBooleanMacro(Cross,int);

  // Description:
  // Specify an angle (in degrees) to rotate the glyph around
  // the z-axis. Using this ivar, it is possible to generate
  // rotated glyphs (e.g., crosses, arrows, etc.)
  vtkSetMacro(RotationAngle,float);
  vtkGetMacro(RotationAngle,float);

  // Description:
  // Specify the type of glyph to generate.
  vtkSetClampMacro(GlyphType,int,VTK_NO_GLYPH,VTK_HOOKEDARROW_GLYPH);
  vtkGetMacro(GlyphType,int);
  void SetGlyphTypeToNone() {this->SetGlyphType(VTK_NO_GLYPH);}  
  void SetGlyphTypeToVertex() {this->SetGlyphType(VTK_VERTEX_GLYPH);}  
  void SetGlyphTypeToDash() {this->SetGlyphType(VTK_DASH_GLYPH);}  
  void SetGlyphTypeToCross() {this->SetGlyphType(VTK_CROSS_GLYPH);}
  void SetGlyphTypeToThickCross() {this->SetGlyphType(VTK_THICKCROSS_GLYPH);}
  void SetGlyphTypeToTriangle() {this->SetGlyphType(VTK_TRIANGLE_GLYPH);}
  void SetGlyphTypeToSquare() {this->SetGlyphType(VTK_SQUARE_GLYPH);}
  void SetGlyphTypeToCircle() {this->SetGlyphType(VTK_CIRCLE_GLYPH);}
  void SetGlyphTypeToDiamond() {this->SetGlyphType(VTK_DIAMOND_GLYPH);}
  void SetGlyphTypeToArrow() {this->SetGlyphType(VTK_ARROW_GLYPH);}
  void SetGlyphTypeToThickArrow() {this->SetGlyphType(VTK_THICKARROW_GLYPH);}
  void SetGlyphTypeToHookedArrow() {this->SetGlyphType(VTK_HOOKEDARROW_GLYPH);}

protected:
  vtkGlyphSource2D();
  ~vtkGlyphSource2D() {};

  void Execute();

  float Center[3];
  float Scale;
  float Scale2;
  float Color[3];
  int   Filled;
  int   Dash;
  int   Cross;
  int   GlyphType;
  float RotationAngle;
  
  void TransformGlyph(vtkPoints *pts);
  void ConvertColor();
  unsigned char RGB[3];
  
  void CreateVertex(vtkPoints *pts, vtkCellArray *verts, 
                    vtkUnsignedCharArray *colors);
  void CreateDash(vtkPoints *pts, vtkCellArray *lines, 
                  vtkCellArray *polys, vtkUnsignedCharArray *colors, float scale);
  void CreateCross(vtkPoints *pts, vtkCellArray *lines, 
                   vtkCellArray *polys, vtkUnsignedCharArray *colors, float scale);
  void CreateThickCross(vtkPoints *pts, vtkCellArray *lines, 
                        vtkCellArray *polys, vtkUnsignedCharArray *colors);
  void CreateTriangle(vtkPoints *pts, vtkCellArray *lines,
                      vtkCellArray *polys, vtkUnsignedCharArray *colors);
  void CreateSquare(vtkPoints *pts, vtkCellArray *lines,
                    vtkCellArray *polys, vtkUnsignedCharArray *colors);
  void CreateCircle(vtkPoints *pts, vtkCellArray *lines,
                    vtkCellArray *polys, vtkUnsignedCharArray *colors);
  void CreateDiamond(vtkPoints *pts, vtkCellArray *lines,
                     vtkCellArray *polys, vtkUnsignedCharArray *colors);
  void CreateArrow(vtkPoints *pts, vtkCellArray *lines,
                   vtkCellArray *polys, vtkUnsignedCharArray *colors);
  void CreateThickArrow(vtkPoints *pts, vtkCellArray *lines,
                        vtkCellArray *polys, vtkUnsignedCharArray *colors);
  void CreateHookedArrow(vtkPoints *pts, vtkCellArray *lines,
                         vtkCellArray *polys, vtkUnsignedCharArray *colors);

private:
  vtkGlyphSource2D(const vtkGlyphSource2D&);  // Not implemented.
  void operator=(const vtkGlyphSource2D&);  // Not implemented.
};

#endif


