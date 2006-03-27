/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTextActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTextActor - An actor that displays text. Scaled or unscaled
// .SECTION Description
// vtkTextActor can be used to place text annotation into a window.
// When ScaledText is false, the text is fixed font and operation is
// the same as a vtkPolyDataMapper2D/vtkActor2D pair.
// When ScaledText is true, the font resizes such that the text fits inside the
// box defined by the position 1 & 2 coordinates. This class replaces the
// deprecated vtkScaledTextActor and acts as a convenient wrapper for
// a vtkTextMapper/vtkActor2D pair.
// Set the text property/attributes through the vtkTextProperty associated to
// this actor.
//
// .SECTION See Also
// vtkActor2D vtkPolyDataMapper vtkTextProperty vtkFreeTypeUtilities

#ifndef __vtkTextActor_h
#define __vtkTextActor_h

#include "vtkActor2D.h"

class vtkTextProperty;
class vtkPolyDataMapper2D;
class vtkImageData;
class vtkFreeTypeUtilities;
class vtkTransform;
class vtkPolyData;
class vtkPoints;

class VTK_RENDERING_EXPORT vtkTextActor : public vtkActor2D
{
public:
  vtkTypeRevisionMacro(vtkTextActor,vtkActor2D);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Instantiate object with a rectangle in normaled view coordinates
  // of (0.2,0.85, 0.8, 0.95).
  static vtkTextActor *New();

  // Description:
  // Shallow copy of this text actor. Overloads the virtual
  // vtkProp method.
  void ShallowCopy(vtkProp *prop);

  // Description:
  // Override the vtkPolyDataMapper2D that defines the text to be drawn.
  // One will be created by default if none is supplied
  void SetMapper(vtkPolyDataMapper2D *mapper);

  // Description:
  // Set the text string to be displayed. "\n" is recognized
  // as a carriage return/linefeed (line separator).
  // Convenience method to the underlying mapper
  void SetInput(const char *inputString);
  char *GetInput();

  // Description:
  // Set/Get the minimum size in pixels for this actor.
  // Defaults to 10,10.
  // Not valid when ScaledText = false
  vtkSetVector2Macro(MinimumSize,int);
  vtkGetVector2Macro(MinimumSize,int);

  // Description:
  // Set/Get the maximum height of a line of text as a
  // percentage of the vertical area allocated to this
  // scaled text actor. Defaults to 1.0.
  // Not valid when ScaledText = false
  vtkSetMacro(MaximumLineHeight,float);
  vtkGetMacro(MaximumLineHeight,float);

  // Description:
  // Turn on or off the ScaledText option.
  // When text is scaled, the bounding rectangle is used to fit the text
  // When ScaledText is off, the text is rendered at a fixed font size
  vtkSetMacro(ScaledText,int);
  vtkGetMacro(ScaledText,int);
  vtkBooleanMacro(ScaledText,int);

  // Description:
  // This method is being depricated.  Use SetJustification and
  // SetVerticalJustification in text property instead.
  // Set/Get the Alignment point
  // if zero (default), the text aligns itself to the bottom left corner
  // (which is defined by the PositionCoordinate)
  // otherwise the text aligns itself to corner/midpoint or centre
  // @verbatim
  //      6   7   8
  //      3   4   5
  //      0   1   2
  // @endverbatim
  // This is the same as setting the TextProperty's justification.
  // Currently TextActor is not oriented around its AlignmentPoint.
  void SetAlignmentPoint(int point);
  int GetAlignmentPoint();
  
  // Description:
  // Counterclockwise rotation around the Alignment point.  
  // Units are in degrees and defaults to 0.
  // The orientation in the text property rotates the text in the 
  // texture map.  It will proba ly not give you the effect you
  // desire.
  void SetOrientation(float orientation);
  vtkGetMacro(Orientation,float);

  // Description:
  // Return the actual vtkCoordinate reference that the mapper should use
  // to position the actor. This is used internally by the mappers and should
  // be overridden in specialized subclasses and otherwise ignored.
  vtkCoordinate *GetActualPositionCoordinate(void)
    { return this->AdjustedPositionCoordinate; }

  // Description:
  // Set/Get the text property.
  virtual void SetTextProperty(vtkTextProperty *p);
  vtkGetObjectMacro(TextProperty,vtkTextProperty);
  
  // Description:
  // Enable non-linear scaling of font sizes. This is useful in combination
  // with scaled text. With small windows you want to use the entire scaled
  // text area. With larger windows you want to reduce the font size some so
  // that the entire area is not used. These values modify the computed font 
  // size as follows:  
  //   newFontSize = pow(FontSize,exponent)*pow(target,1.0 - exponent)
  // typically exponent should be around 0.7 and target should be around 10
  virtual void SetNonLinearFontScale(double exponent, int target);

  // Description:
  // This is just a simple coordinate conversion method used in the render
  // process.
  void SpecifiedToDisplay(double *pos, vtkViewport *vport, int specified);

  // Description:
  // This is just a simple coordinate conversion method used in the render
  // process.
  void DisplayToSpecified(double *pos, vtkViewport *vport, int specified);

//BTX
  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS.
  // Release any graphics resources that are being consumed by this actor.
  // The parameter window could be used to determine which graphic
  // resources to release.
  virtual void ReleaseGraphicsResources(vtkWindow *);

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS.
  // Draw the text actor to the screen.
  int RenderOpaqueGeometry(vtkViewport* viewport);
  int RenderTranslucentGeometry(vtkViewport* ) {return 0;};
  int RenderOverlay(vtkViewport* viewport);
//ETX

protected:
  // Description:
  // Hide access methods that use superclass vtkMapper2D and not vtkImageMapper
  void SetMapper(vtkMapper2D *mapper);

   vtkTextActor();
  ~vtkTextActor();

  int     MinimumSize[2];
  float   MaximumLineHeight;
  double  FontScaleExponent;
  double  FontScaleTarget;
  int     ScaledText;
  float   Orientation;

  vtkCoordinate *AdjustedPositionCoordinate;
  vtkTextProperty *TextProperty;
  vtkImageData *ImageData;
  // This used to be "Mapper" but I changed it to PDMapper because
  // Mapper is an ivar in Actor2D (bad form).
  vtkPolyDataMapper2D *PDMapper;
  vtkFreeTypeUtilities *FreeTypeUtilities;
  vtkTimeStamp  BuildTime;
  vtkTransform *Transform;
  int LastSize[2];
  int LastOrigin[2];
  char *Input;
  bool InputRendered;
  int FormerJustification[2];
  double FormerLineOffset;
  int FormerCoordinateSystem;
  double FormerOrientation;

  // Stuff needed to display the image text as a texture map.
  vtkPolyData* Rectangle;
  vtkPoints*   RectanglePoints;

  void ComputeRectangle(); 

private:
  vtkTextActor(const vtkTextActor&);  // Not implemented.
  void operator=(const vtkTextActor&);  // Not implemented.
};


#endif

