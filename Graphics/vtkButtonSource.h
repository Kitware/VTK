/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkButtonSource.h
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
// .NAME vtkButtonSource - create a ellipsoidal-shaped button
// .SECTION Description
// vtkButtonSource creates a ellipsoidal shaped button with
// texture coordinates suitable for application of a texture map. This
// provides a way to make nice looking 3D buttons. The buttons are 
// represented as vtkPolyData that includes texture coordinates and
// normals. The button lies in the x-y plane.
//
// To use this class you must define the major and minor axes lengths of an
// ellipsoid (expressed as width (x), height (y) and depth (z)). The button
// has a rectangular mesh region in the center with texture coordinates that
// range smoothly from (0,1). (This flat region is called the texture
// region.) The outer, curved portion of the button (called the shoulder) has
// texture coordinates set to a user specified value (by default (0,0).
// (This results in coloring the button curve the same color as the (s,t)
// location of the texture map.) The resolution in the radial direction, the
// texture region, and the shoulder region must also be set. The button can
// be moved by specifying an origin.

#ifndef __vtkButtonSource_h
#define __vtkButtonSource_h

#include "vtkPolyDataSource.h"

#define VTK_TEXTURE_STYLE_FIT_IMAGE    0
#define VTK_TEXTURE_STYLE_PROPORTIONAL 1

class VTK_GRAPHICS_EXPORT vtkButtonSource : public vtkPolyDataSource 
{
public:
  void PrintSelf(ostream& os, vtkIndent indent);
  vtkTypeRevisionMacro(vtkButtonSource,vtkPolyDataSource);

  // Description:
  // Construct a circular button with depth 10% of its height.
  static vtkButtonSource *New();

  // Description:
  // Set/Get the width of the button (the x-ellipsoid axis length * 2).
  vtkSetClampMacro(Width,float,0.0,VTK_LARGE_FLOAT);
  vtkGetMacro(Width,float);

  // Description:
  // Set/Get the height of the button (the y-ellipsoid axis length * 2).
  vtkSetClampMacro(Height,float,0.0,VTK_LARGE_FLOAT);
  vtkGetMacro(Height,float);

  // Description:
  // Set/Get the depth of the button (the z-eliipsoid axis length).
  vtkSetClampMacro(Depth,float,0.0,VTK_LARGE_FLOAT);
  vtkGetMacro(Depth,float);

  // Description:
  // Set/Get the radial ratio. This is the measure of the radius of the
  // outer ellipsoid to the inner ellipsoid of the button. The outer
  // ellipsoid is the boundary of the button defined by the height and
  // width. The inner ellipsoid circumscribes the texture region. Larger
  // RadialRatio's cause the button to be more rounded (and the texture
  // region to be smaller); smaller ratios produce sharply curved shoulders
  // with a larger texture region.
  vtkSetClampMacro(RadialRatio,float,1.0,VTK_LARGE_FLOAT);
  vtkGetMacro(RadialRatio,float);

  // Description:
  // Specify the resolution of the button in the circumferential direction.
  vtkSetClampMacro(CircumferentialResolution,int,4,VTK_LARGE_INTEGER);
  vtkGetMacro(CircumferentialResolution,int);

  // Description:
  // Specify the resolution of the texture in the radial direction in the
  // texture region.
  vtkSetClampMacro(TextureResolution,int,1,VTK_LARGE_INTEGER);
  vtkGetMacro(TextureResolution,int);

  // Description:
  // Specify the resolution of the texture in the radial direction in the
  // shoulder region.
  vtkSetClampMacro(ShoulderResolution,int,1,VTK_LARGE_INTEGER);
  vtkGetMacro(ShoulderResolution,int);

  // Description:
  // Specify a point defining the origin of the button.
  vtkSetVector3Macro(Origin,float);
  vtkGetVectorMacro(Origin,float,3);

  // Description:
  // Set/Get the style of the texture region: whether to size it
  // according to the x-y dimensions of the texture, or whether to make
  // the texture region proportional to the width/height of the button.
  // When using the texture dimensions, the texture region is adjusted to
  // be circumscribed by the elliptical, inner shape of the button.
  // (The inner elliptical radius is controled by the RadiusRatio.)
  vtkSetClampMacro(TextureStyle,int,VTK_TEXTURE_STYLE_FIT_IMAGE,
                                    VTK_TEXTURE_STYLE_PROPORTIONAL);
  vtkGetMacro(TextureStyle,int);
  void SetTextureStyleToFitImage() 
    {this->SetTextureStyle(VTK_TEXTURE_STYLE_FIT_IMAGE);}
  void SetTextureStyleToProportional() 
    {this->SetTextureStyle(VTK_TEXTURE_STYLE_PROPORTIONAL);}

  // Description:
  // Set/get the texture dimension. This needs to be set if the texture
  // style is set to fit the image.
  vtkSetVector2Macro(TextureDimensions,int);
  vtkGetVector2Macro(TextureDimensions,int);

  // Description:
  // Set/Get the default texture coordinate to set the shoulder region to.
  vtkSetVector2Macro(ShoulderTextureCoordinate,float);
  vtkGetVector2Macro(ShoulderTextureCoordinate,float);

  // Description:
  // Indicate whether the button is single or double sided. A double sided
  // button can be viewed from two sides...it looks sort of like a "pill."
  // A single-sided button is meant to viewed from a single side; it looks 
  // like a "clam-shell."
  vtkSetMacro(TwoSided,int);
  vtkGetMacro(TwoSided,int);
  vtkBooleanMacro(TwoSided,int);

protected:
  vtkButtonSource();
  ~vtkButtonSource() {}

  void Execute();

  float Width;
  float Height;
  float Depth;

  int   CircumferentialResolution;
  int   TextureResolution;
  int   ShoulderResolution;

  float Origin[3];
  float ShoulderTextureCoordinate[2];

  float RadialRatio;
  int TextureStyle;
  int TextureDimensions[2];
  int TwoSided;

private:
  vtkButtonSource(const vtkButtonSource&);  // Not implemented.
  void operator=(const vtkButtonSource&);  // Not implemented.

  //internal variable related to axes of ellipsoid
  float A;
  float A2;
  float B;
  float B2;
  float C;
  float C2;
  
  float ComputeDepth(int inTextureRegion, float x, float y, float n[3]);
  void InterpolateCurve(int inTextureRegion, vtkPoints *newPts, int numPts,
                        vtkFloatArray *normals, vtkFloatArray *tcoords, 
                        int res, int c1StartPoint,int c1Incr,
                        int c2StartPoint,int s2Incr, int startPoint,int incr);
  void CreatePolygons(vtkCellArray *newPolys, int num, int res, int startIdx);
  void IntersectEllipseWithLine(float a2, float b2, float dX, float dY, 
                                float& xe, float& ye);
  
    
};

#endif


