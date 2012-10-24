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
// When TextScaleMode is NONE, the text is fixed font and operation is
// the same as a vtkPolyDataMapper2D/vtkActor2D pair.
// When TextScaleMode is VIEWPORT, the font resizes such that it maintains a
// consistent size relative to the viewport in which it is rendered.
// When TextScaleMode is PROP, the font resizes such that the text fits inside
// the box defined by the position 1 & 2 coordinates. This class replaces the
// deprecated vtkScaledTextActor and acts as a convenient wrapper for
// a vtkTextMapper/vtkActor2D pair.
// Set the text property/attributes through the vtkTextProperty associated to
// this actor.
//
// .SECTION See Also
// vtkActor2D vtkPolyDataMapper vtkTextProperty vtkFreeTypeUtilities

#ifndef __vtkTextActor_h
#define __vtkTextActor_h

#include "vtkRenderingFreeTypeModule.h" // For export macro
#include "vtkActor2D.h"

class vtkTextProperty;
class vtkPolyDataMapper2D;
class vtkImageData;
class vtkFreeTypeUtilities;
class vtkTransform;
class vtkPolyData;
class vtkPoints;
class vtkTexture;

class VTKRENDERINGFREETYPE_EXPORT vtkTextActor : public vtkActor2D
{
public:
  vtkTypeMacro(vtkTextActor,vtkActor2D);
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
  // The characters must be in the ISO-8859-1 encoding.
  // Convenience method to the underlying mapper
  void SetInput(const char *inputString);
  char *GetInput();

  // Description:
  // Set/Get the minimum size in pixels for this actor.
  // Defaults to 10,10.
  // Only valid when TextScaleMode is PROP.
  vtkSetVector2Macro(MinimumSize,int);
  vtkGetVector2Macro(MinimumSize,int);

  // Description:
  // Set/Get the maximum height of a line of text as a
  // percentage of the vertical area allocated to this
  // scaled text actor. Defaults to 1.0.
  // Only valid when TextScaleMode is PROP.
  vtkSetMacro(MaximumLineHeight,float);
  vtkGetMacro(MaximumLineHeight,float);

  // Description:
  // Set how text should be scaled.  If set to
  // vtkTextActor::TEXT_SCALE_MODE_NONE, the the font size will be fixed by the
  // size given in TextProperty.  If set to vtkTextActor::TEXT_SCALE_MODE_PROP,
  // the text will be scaled to fit exactly in the prop as specified by the
  // position 1 & 2 coordinates.  If set to
  // vtkTextActor::TEXT_SCALE_MODE_VIEWPORT, the text will be scaled based on
  // the size of the viewport it is displayed in.
  vtkSetClampMacro(TextScaleMode, int,
                     TEXT_SCALE_MODE_NONE, TEXT_SCALE_MODE_VIEWPORT);
  vtkGetMacro(TextScaleMode, int);
  void SetTextScaleModeToNone()
    { this->SetTextScaleMode(TEXT_SCALE_MODE_NONE); }
  void SetTextScaleModeToProp()
    { this->SetTextScaleMode(TEXT_SCALE_MODE_PROP); }
  void SetTextScaleModeToViewport()
    { this->SetTextScaleMode(TEXT_SCALE_MODE_VIEWPORT); }

//BTX
  enum {
    TEXT_SCALE_MODE_NONE = 0,
    TEXT_SCALE_MODE_PROP,
    TEXT_SCALE_MODE_VIEWPORT
  };
//ETX

  // Description:
  // Turn on or off the UseBorderAlign option.
  // When UseBorderAlign is on, the bounding rectangle is used to align the text,
  // which is the proper behavior when using vtkTextRepresentation
  vtkSetMacro(UseBorderAlign,int);
  vtkGetMacro(UseBorderAlign,int);
  vtkBooleanMacro(UseBorderAlign,int);

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
  // Set/Get the text property.
  virtual void SetTextProperty(vtkTextProperty *p);
  vtkGetObjectMacro(TextProperty,vtkTextProperty);

  // Description:
  // Return the bounding box coordinates of the text in viewport coordinates.
  // The bbox array is populated with [ xmin, xmax, ymin, ymax ]
  // values in that order.
  virtual void GetBoundingBox(double bbox[4]);

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

  // Description:
  // Compute the scale the font should be given the viewport.  The result
  // is placed in the ScaledTextProperty ivar.
  virtual void ComputeScaledFont(vtkViewport *viewport);

  // Description:
  // Get the scaled font.  Use ComputeScaledFont to set the scale for a given
  // viewport.
  vtkGetObjectMacro(ScaledTextProperty, vtkTextProperty);

  // Description:
  // Provide a font scaling based on a viewport.  This is the scaling factor
  // used when the TextScaleMode is set to VIEWPORT and has been made public for
  // other components to use.  This scaling assumes that the long dimension of
  // the viewport is meant to be 6 inches (a typical width of text in a paper)
  // and then resizes based on if that long dimension was 72 DPI.
  static float GetFontScale(vtkViewport *viewport);

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
  virtual int RenderOpaqueGeometry(vtkViewport* viewport);
  virtual int RenderTranslucentPolygonalGeometry(vtkViewport* ) {return 0;};
  virtual int RenderOverlay(vtkViewport* viewport);

  // Description:
  // Does this prop have some translucent polygonal geometry?
  virtual int HasTranslucentPolygonalGeometry();
//ETX

protected:
  // Description:
  // Hide access methods that use superclass vtkMapper2D and not vtkImageMapper
  void SetMapper(vtkMapper2D *mapper);

  // Description:
  // Render Input to Image using the supplied font property.
  virtual bool RenderImage(vtkTextProperty *tprop, vtkViewport *viewport);

  // Description:
  // Get the bounding box for Input using the supplied font property.
  virtual bool GetImageBoundingBox(
    vtkTextProperty *tprop, vtkViewport *viewport, int bbox[4]);

   vtkTextActor();
  ~vtkTextActor();

  int     MinimumSize[2];
  float   MaximumLineHeight;
  double  FontScaleExponent;
  int     TextScaleMode;
  float   Orientation;
  int     UseBorderAlign;

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
  double FormerOrientation;

  vtkTextProperty *ScaledTextProperty;

  // Stuff needed to display the image text as a texture map.
  vtkPolyData* Rectangle;
  vtkPoints*   RectanglePoints;
  vtkTexture *Texture;

  virtual void ComputeRectangle(vtkViewport *viewport);

  // Description:
  // Ensure that \a Rectangle and \a RectanglePoints are valid and up-to-date.
  //
  // Unlike ComputeRectangle(), this may do nothing (if the rectangle is valid),
  // or it may render the text to an image and recompute rectangle points by
  // calling ComputeRectangle.
  //
  // Returns a non-zero value upon success or zero upon failure to
  // render the image.
  //
  // This may be called with a NULL viewport when bounds are required before
  // a rendering has occurred.
  virtual int UpdateRectangle(vtkViewport* viewport);

  // Set/Get the texture object to control rendering texture maps.  This will
  // be a vtkTexture object. An actor does not need to have an associated
  // texture map and multiple actors can share one texture.
  // This was added for orienated text which is rendered with a
  // vtkPolyDataMaper2D and a texture.
  virtual void SetTexture(vtkTexture*);
  vtkGetObjectMacro(Texture,vtkTexture);

private:
  vtkTextActor(const vtkTextActor&);  // Not implemented.
  void operator=(const vtkTextActor&);  // Not implemented.
};


#endif

