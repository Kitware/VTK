/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAxisActor2D.h
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
// .NAME vtkAxisActor2D - Create an axis with tick marks and labels
// .SECTION Description
// vtkAxisActor2D creates an axis with tick marks, labels, and/or a title,
// depending on the particular instance variable settings. To use this class,
// you typically specify two points defining the start and end points of the
// line (x-y definition using vtkCoordinate class), the number of labels, and
// the data range (min,max). You can also control what parts of the axis are
// visible including the line, the tick marks, the labels, and the title. It
// is also possible to control the font family, the font style (bold and/or
// italic), and whether font shadows are drawn. You can also specify the
// label format (a printf style format).
//
// This class decides what font size to use and how to locate the labels. It
// also decides how to create reasonable tick marks and labels. The number
// of labels and the range of values may not match the number specified, but
// should be close.
//
// Labels are drawn on the "right" side of the axis. The "right" side is
// the side of the axis on the right as you move from Point1 to Point2. The
// way the labels and title line up with the axis and tick marks depends on
// whether the line is considered horizontal or vertical.
//
// The instance variables Point1 and Point2 are instances of vtkCoordinate.
// What this means is that you can specify the axis in a variety of coordinate
// systems. Also, the axis does not have to be either horizontal or vertical.
// The tick marks are created so that they are perpendicular to the axis.

// .SECTION See Also
// vtkActor2D vtkTextMapper vtkPolyDataMapper2D vtkScalarBarActor
// vtkCoordinate

#ifndef __vtkAxisActor2D_h
#define __vtkAxisActor2D_h

#include "vtkActor2D.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkTextMapper.h"

#define VTK_MAX_LABELS 25

class VTK_RENDERING_EXPORT vtkAxisActor2D : public vtkActor2D
{
public:
  vtkTypeMacro(vtkAxisActor2D,vtkActor2D);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Instantiate object.
  static vtkAxisActor2D *New();
  
  // Description:
  // Specify the position of the first point defining the axis.
  vtkViewportCoordinateMacro(Point1);
  
  // Description:
  // Specify the position of the second point defining the axis. Note that
  // the order from Point1 to Point2 controls which side the tick marks
  // are drawn on (ticks are drawn on the right, if visible).
  vtkViewportCoordinateMacro(Point2);
  
  // Description:
  // Specify the (min,max) axis range. This will be used in the generation
  // of labels, if labels are visible.
  vtkSetVector2Macro(Range,float);
  vtkGetVectorMacro(Range,float,2);

  // Description:
  // Set/Get the number of annotation labels to show.
  vtkSetClampMacro(NumberOfLabels, int, 2, VTK_MAX_LABELS);
  vtkGetMacro(NumberOfLabels, int);
  
  // Description:
  // Set/Get the format with which to print the labels on the scalar
  // bar.
  vtkSetStringMacro(LabelFormat);
  vtkGetStringMacro(LabelFormat);

  // Description:
  // Set/Get the flag that controls whether the labels and ticks are
  // adjusted for "nice" numerical values to make it easier to read 
  // the labels. The adjustment is based in the Range instance variable.
  vtkSetMacro(AdjustLabels, int);
  vtkGetMacro(AdjustLabels, int);
  vtkBooleanMacro(AdjustLabels, int);

  // Description:
  // Set/Get the title of the scalar bar actor,
  vtkSetStringMacro(Title);
  vtkGetStringMacro(Title);

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
  // Set/Get the length of the tick marks (expressed in pixels or display
  // coordinates). 
  vtkSetClampMacro(TickLength, int, 0, 100);
  vtkGetMacro(TickLength, int);
  
  // Description:
  // Set/Get the offset of the labels (expressed in pixels or display
  // coordinates). The offset is the distance of labels from tick marks
  // or other objects.
  vtkSetClampMacro(TickOffset, int, 0, 100);
  vtkGetMacro(TickOffset, int);
  
  // Description:
  // Set/Get visibility of the axis line.
  vtkSetMacro(AxisVisibility, int);
  vtkGetMacro(AxisVisibility, int);
  vtkBooleanMacro(AxisVisibility, int);

  // Description:
  // Set/Get visibility of the axis tick marks.
  vtkSetMacro(TickVisibility, int);
  vtkGetMacro(TickVisibility, int);
  vtkBooleanMacro(TickVisibility, int);

  // Description:
  // Set/Get visibility of the axis labels.
  vtkSetMacro(LabelVisibility, int);
  vtkGetMacro(LabelVisibility, int);
  vtkBooleanMacro(LabelVisibility, int);

  // Description:
  // Set/Get visibility of the axis title.
  vtkSetMacro(TitleVisibility, int);
  vtkGetMacro(TitleVisibility, int);
  vtkBooleanMacro(TitleVisibility, int);

  // Description:
  // Set/Get the factor that controls the overall size of the fonts used
  // to label and title the axes. This ivar used in conjunction with
  // the LabelFactor can be used to control font sizes.
  vtkSetClampMacro(FontFactor, float, 0.1, 2.0);
  vtkGetMacro(FontFactor, float);

  // Description:
  // Set/Get the factor that controls the relative size of the axis labels
  // to the axis title.
  vtkSetClampMacro(LabelFactor, float, 0.1, 2.0);
  vtkGetMacro(LabelFactor, float);

  // Description:
  // Draw the axis. 
  int RenderOverlay(vtkViewport* viewport);
  int RenderOpaqueGeometry(vtkViewport* viewport);
  int RenderTranslucentGeometry(vtkViewport *) {return 0;}

  // Description:
  // Release any graphics resources that are being consumed by this actor.
  // The parameter window could be used to determine which graphic
  // resources to release.
  void ReleaseGraphicsResources(vtkWindow *);

  // Description:
  // This method computes the range of the axis given an input range. 
  // It also computes the number of tick marks given a suggested number.
  // (The number of tick marks includes end ticks as well.)
  // The number of tick marks computed (in conjunction with the output
  // range) will yield "nice" tick values. For example, if the input range
  // is (0.25,96.7) and the number of ticks requested is 10, the output range
  // will be (0,100) with the number of computed ticks to 11 to yield tick
  // values of (0,10,20,...,100).
  static void ComputeRange(float inRange[2], float outRange[2],
                           int inNumTicks, int &outNumTicks, float &interval);

  // Description:
  // General method computes font size from a representative size on the 
  // viewport (given by size[2]). The method returns the font size (in points)
  // and the string height/width (in pixels). It also sets the font size of the
  // instance of vtkTextMapper provided. The factor is used when you're trying
  // to create text of different size-factor is usually =1 but you can
  // adjust the font size by making factor larger or smaller.
  static int SetFontSize(vtkViewport *viewport, vtkTextMapper *textMapper, 
			 int *size, float factor, 
			 int &stringWidth, int &stringHeight);

  // Description:
  // Shallow copy of an axis actor. Overloads the virtual vtkProp method.
  void ShallowCopy(vtkProp *prop);

protected:
  vtkAxisActor2D();
  ~vtkAxisActor2D();

  vtkCoordinate *Point1Coordinate;
  vtkCoordinate *Point2Coordinate;
  char  *Title;
  float Range[2];
  int   NumberOfLabels;
  char  *LabelFormat;
  int   NumberOfLabelsBuilt;
  int   AdjustLabels;
  float FontFactor;
  float LabelFactor;
  int   TickLength;
  int   TickOffset;

  int   Bold;
  int   Italic;
  int   Shadow;
  int   FontFamily;
  
  int   AxisVisibility;
  int   TickVisibility;
  int   LabelVisibility;
  int   TitleVisibility;
  
  int   LastPoint1[2];
  int   LastPoint2[2];
  
  int   LastSize[2];
  int   LastTitleFontSize;
  int   LastLabelFontSize;
  
  virtual void BuildAxis(vtkViewport *viewport);
  static float ComputeStringOffset(float width, float height, float theta);
  static void SetOffsetPosition(float xTick[3], float theta, int stringHeight,
                                int stringWidth, int offset, vtkActor2D *actor);

  vtkTextMapper *TitleMapper;
  vtkActor2D    *TitleActor;

  vtkTextMapper **LabelMappers;
  vtkActor2D    **LabelActors;

  vtkPolyData         *Axis;
  vtkPolyDataMapper2D *AxisMapper;
  vtkActor2D          *AxisActor;

  vtkTimeStamp  BuildTime;
private:
  vtkAxisActor2D(const vtkAxisActor2D&);  // Not implemented.
  void operator=(const vtkAxisActor2D&);  // Not implemented.
};


#endif
