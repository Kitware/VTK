/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCubeAxesActor2D.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thorsten Dowe who modified and improved this class.

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
// .NAME vtkCubeAxesActor2D - create a 2D plot of a bounding box edges - used for navigation
// .SECTION Description
// vtkCubeAxesActor2D is a composite actor that draws three axes of the 
// bounding box of an input dataset. The axes include labels and titles
// for the x-y-z axes. The algorithm selects the axes that are on the 
// "exterior" of the bounding box, exterior as determined from examining
// outer edges of the bounding box in projection (display) space. Alternatively,
// the edges closest to the viewer (i.e., camera position) can be drawn.
// 
// To use this object you must define a bounding box and the camera used
// to render the vtkCubeAxesActor2D. You may optionally define font family,
// font size, bolding on/off, italics on/off, and text shadows on/off. (The
// camera is used to control the scaling and position of the
// vtkCubeAxesActor2D so that it fits in the viewport and always remains
// visible.)
//
// The bounding box to use is defined in one of three ways. First, if the Input
// ivar is defined, then the input dataset's bounds is used. If the Input is 
// not defined, and the Prop (superclass of all actors) is defined, then the
// Prop's bounds is used. If neither the Input or Prop is defined, then the
// Bounds instance variable (an array of six floats) is used.
// 
// .SECTION See Also
// vtkActor2D vtkAxis2DActor vtkXYPlotActor

#ifndef __vtkCubeAxesActor2D_h
#define __vtkCubeAxesActor2D_h

#include "vtkAxisActor2D.h"
#include "vtkCamera.h"

#define VTK_FLY_OUTER_EDGES 0
#define VTK_FLY_CLOSEST_TRIAD 1

class VTK_EXPORT vtkCubeAxesActor2D : public vtkActor2D
{
public:
  vtkTypeMacro(vtkCubeAxesActor2D,vtkActor2D);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Instantiate object with bold, italic, and shadow enabled; font family
  // set to Arial; and label format "6.3g". The number of labels per axis
  // is set to 3.
  static vtkCubeAxesActor2D *New();
  
  // Description:
  // Draw the axes as per the vtkProp superclass' API.
  int RenderOverlay(vtkViewport*);
  int RenderOpaqueGeometry(vtkViewport*);
  int RenderTranslucentGeometry(vtkViewport *) {return 0;}

  // Description:
  // Use the bounding box of this input dataset to draw the cube axes. If this
  // is not specified, then the class will attempt to determine the bounds from
  // the defined Prop or Bounds.
  vtkSetObjectMacro(Input, vtkDataSet);
  vtkGetObjectMacro(Input, vtkDataSet);

  // Description:
  // Use the bounding box of this prop to draw the cube axes. The Prop is used 
  // to determine the bounds only if the Input is not defined.
  vtkSetObjectMacro(Prop, vtkProp);
  vtkGetObjectMacro(Prop, vtkProp);
  
  // Description:
  // Explicitly specify the region in space around which to draw the bounds.
  // The bounds is used only when no Input or Prop is specified. The bounds
  // are specified according to (xmin,xmax, ymin,ymax, zmin,zmax), making
  // sure that the min's are less than the max's.
  vtkSetVector6Macro(Bounds,float);
  float *GetBounds();
  void GetBounds(float& xmin, float& xmax, float& ymin, float& ymax, 
                 float& zmin, float& zmax);
  void GetBounds(float bounds[6]);

  // Description:
  // Set/Get the camera to perform scaling and translation of the 
  // vtkCubeAxesActor2D.
  vtkSetObjectMacro(Camera,vtkCamera);
  vtkGetObjectMacro(Camera,vtkCamera);

  // Description:
  // Specify a mode to control how the axes are drawn: either outer edges
  // or closest triad to the camera position.
  vtkSetClampMacro(FlyMode, int, VTK_FLY_OUTER_EDGES, VTK_FLY_CLOSEST_TRIAD);
  vtkGetMacro(FlyMode, int);
  void SetFlyModeToOuterEdges()
    {this->SetFlyMode(VTK_FLY_OUTER_EDGES);};
  void SetFlyModeToClosestTriad()
    {this->SetFlyMode(VTK_FLY_CLOSEST_TRIAD);};

  // Description:
  // Set/Get a flag that controls whether the axes are scaled to fit in
  // the viewport. If off, the axes size remains constant (i.e., stay the
  // size of the bounding box). By default scaling is on so the axes are
  // scaled to fit inside the viewport.
  vtkSetMacro(Scaling,int);
  vtkGetMacro(Scaling,int);
  vtkBooleanMacro(Scaling,int);

  // Description:
  // Set/Get the number of annotation labels to show along the x, y, and 
  // z axes. This values is a suggestion: the number of labels may vary
  // depending on the particulars of the data.
  vtkSetClampMacro(NumberOfLabels, int, 0, 50);
  vtkGetMacro(NumberOfLabels, int);
  
  // Description:
  // Set/Get the labels for the x, y, and z axes. By default, 
  // use "X", "Y" and "Z".
  vtkSetStringMacro(XLabel);
  vtkGetStringMacro(XLabel);
  vtkSetStringMacro(YLabel);
  vtkGetStringMacro(YLabel);
  vtkSetStringMacro(ZLabel);
  vtkGetStringMacro(ZLabel);

  // Description:
  // Enable/Disable bolding annotation text.
  vtkSetMacro(Bold, int);
  vtkGetMacro(Bold, int);
  vtkBooleanMacro(Bold, int);

  // Description:
  // Enable/Disable italicizing annotation text.
  vtkSetMacro(Italic, int);
  vtkGetMacro(Italic, int);
  vtkBooleanMacro(Italic, int);

  // Description:
  // Enable/Disable creating shadows on the annotation text. Shadows make 
  // the text easier to read.
  vtkSetMacro(Shadow, int);
  vtkGetMacro(Shadow, int);
  vtkBooleanMacro(Shadow, int);

  // Description:
  // Set/Get the font family for the annotation text. Three font types 
  // are available: Arial (VTK_ARIAL), Courier (VTK_COURIER), and 
  // Times (VTK_TIMES).
  vtkSetMacro(FontFamily, int);
  vtkGetMacro(FontFamily, int);
  void SetFontFamilyToArial() {this->SetFontFamily(VTK_ARIAL);};
  void SetFontFamilyToCourier() {this->SetFontFamily(VTK_COURIER);};
  void SetFontFamilyToTimes() {this->SetFontFamily(VTK_TIMES);};

  // Description:
  // Set/Get the format with which to print the labels on each of the
  // x-y-z axes.
  vtkSetStringMacro(LabelFormat);
  vtkGetStringMacro(LabelFormat);
  
  // Description:
  // Set/Get the factor that controls the overall size of the fonts used
  // to label and title the axes. 
  vtkSetClampMacro(FontFactor, float, 0.1, 2.0);
  vtkGetMacro(FontFactor, float);

  // Description:
  // Set/Get the inertial factor that controls how often (i.e, how
  // many renders) the axes can switch position (jump from one axes 
  // to another).
  vtkSetClampMacro(Inertia, int, 1, VTK_LARGE_INTEGER);
  vtkGetMacro(Inertia, float);

  // Description:
  // Specify an offset value to "pull back" the axes from the corner at
  // which they are joined to avoid overlap of axes labels. The 
  // "COrnerOffset" is the fraction of the axis length to pull back.
  vtkSetMacro(CornerOffset, float);
  vtkGetMacro(CornerOffset, float);

  // Description:
  // Release any graphics resources that are being consumed by this actor.
  // The parameter window could be used to determine which graphic
  // resources to release.
  void ReleaseGraphicsResources(vtkWindow *);

  // Description:
  // Turn on and off the visibility of each axis.
  vtkSetMacro(XAxisVisibility,int);
  vtkGetMacro(XAxisVisibility,int);
  vtkBooleanMacro(XAxisVisibility,int);
  vtkSetMacro(YAxisVisibility,int);
  vtkGetMacro(YAxisVisibility,int);
  vtkBooleanMacro(YAxisVisibility,int);
  vtkSetMacro(ZAxisVisibility,int);
  vtkGetMacro(ZAxisVisibility,int);
  vtkBooleanMacro(ZAxisVisibility,int);

  // Description:
  // Shallow copy of a CubeAxesActor2D.
  void ShallowCopy(vtkCubeAxesActor2D *actor);

protected:
  vtkCubeAxesActor2D();
  ~vtkCubeAxesActor2D();
  vtkCubeAxesActor2D(const vtkCubeAxesActor2D&);
  void operator=(const vtkCubeAxesActor2D&);

  vtkDataSet *Input;    //Define bounds from input data, or
  vtkProp    *Prop;     //Define bounds from actor/assembly, or
  float      Bounds[6]; //Define bounds explicitly

  vtkCamera *Camera;
  int FlyMode;
  int Scaling;
  
  vtkAxisActor2D *XAxis;
  vtkAxisActor2D *YAxis;
  vtkAxisActor2D *ZAxis;
  
  int   NumberOfLabels;
  char *XLabel;
  char *YLabel;
  char *ZLabel;
  char *Labels[3];

  int XAxisVisibility;
  int YAxisVisibility;
  int ZAxisVisibility;

  int   Bold;
  int   Italic;
  int   Shadow;
  int   FontFamily;
  char  *LabelFormat;
  float FontFactor;
  float CornerOffset;
  int   Inertia;
  int   RenderCount;
  int   InertiaAxes[8];
  
  int RenderSomething;
  
  // various helper methods
  void TransformBounds(vtkViewport *viewport, float bounds[6], 
                       float pts[8][3]);
  int ClipBounds(vtkViewport *viewport, float pts[8][3], float bounds[6]);
  float EvaluatePoint(float planes[24], float x[3]);
  float EvaluateBounds(float planes[24], float bounds[6]);
  void AdjustAxes(float pts[8][3], float bounds[6], 
                  int idx, int xIdx, int yIdx, int zIdx, int zIdx2, 
                  int xAxes, int yAxes, int zAxes,
                  float xCoords[4], float yCoords[4], float zCoords[4],
                  float xRange[2], float yRange[2], float zRange[2]);

private:
  // hide the superclass' ShallowCopy() from the user and the compiler.
  void ShallowCopy(vtkProp *prop) { this->vtkProp::ShallowCopy( prop ); };
};


#endif
