/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCubeAxesActor2D.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1993-1999 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkCubeAxesActor2D - create a 2D plot of a bounding box edges - used for navigation
// .SECTION Description
// vtkCubeAxesActor2D is a composite actor that draws three axes of the 
// bounding box of an input dataset. The axes include labels and titles
// for the x-y-z axes. The algorithm selects the axes that are on the 
// "exterior" of the bounding box, exterior as determined from examining
// outer edges of the bounding box in projection (display) space.
// 
// To use this object you must specify an input dataset and the camera used
// to render the vtkCubeAxesActor2D. You may optionally define font family,
// font size, bolding on/off, italics on/off, and text shadows on/off. (The
// camera is used to control the scaling and position of the
// vtkCubeAxesActor2D so that it fits in the viewport and always remains
// visible.)
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
  vtkCubeAxesActor2D();
  ~vtkCubeAxesActor2D();
  const char *GetClassName() {return "vtkCubeAxesActor2D";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Instantiate object with bold, italic, and shadow enabled; font family
  // set to Arial; and label format "6.3g". The number of labels per axis
  // is set to 3.
  static vtkCubeAxesActor2D *New() {return new vtkCubeAxesActor2D;};
  
  // Description:
  // Draw the axes as per the vtkProp superclass' API.
  int RenderOverlay(vtkViewport*);
  int RenderOpaqueGeometry(vtkViewport*);
  int RenderTranslucentGeometry(vtkViewport *viewport) {return 0;}

  // Description:
  // Use the bounding box of this input dataset to draw the cube axes.
  vtkSetObjectMacro(Input, vtkDataSet);
  vtkGetObjectMacro(Input, vtkDataSet);
  
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
  // Set/Get the number of annotation labels to show along the x, y, and 
  // z axes. This values is a suggestion: the number of labels may vary
  // depending on the particulars of the data.
  vtkSetClampMacro(NumberOfLabels, int, 0, 50);
  vtkGetMacro(NumberOfLabels, int);
  
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

protected:
  vtkDataSet *Input;
  vtkCamera *Camera;
  int FlyMode;
  
  vtkAxisActor2D *XAxis;
  vtkAxisActor2D *YAxis;
  vtkAxisActor2D *ZAxis;
  
  int   NumberOfLabels;
  int   Bold;
  int   Italic;
  int   Shadow;
  int   FontFamily;
  char  *LabelFormat;
  float FontFactor;
  float CornerOffset;

  vtkTimeStamp  BuildTime;
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
};


#endif
