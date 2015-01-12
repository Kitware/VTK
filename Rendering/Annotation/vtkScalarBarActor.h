/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkScalarBarActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkScalarBarActor - Create a scalar bar with labels
// .SECTION Description
// vtkScalarBarActor creates a scalar bar with tick marks. A scalar
// bar is a legend that indicates to the viewer the correspondence between
// color value and data value. The legend consists of a rectangular bar
// made of rectangular pieces each colored a constant value. Since
// vtkScalarBarActor is a subclass of vtkActor2D, it is drawn in the image
// plane (i.e., in the renderer's viewport) on top of the 3D graphics window.
//
// To use vtkScalarBarActor you must associate a vtkScalarsToColors (or
// subclass) with it. The lookup table defines the colors and the
// range of scalar values used to map scalar data.  Typically, the
// number of colors shown in the scalar bar is not equal to the number
// of colors in the lookup table, in which case sampling of
// the lookup table is performed.
//
// Other optional capabilities include specifying the fraction of the
// viewport size (both x and y directions) which will control the size
// of the scalar bar and the number of tick labels. The actual position
// of the scalar bar on the screen is controlled by using the
// vtkActor2D::SetPosition() method (by default the scalar bar is
// centered in the viewport).  Other features include the ability to
// orient the scalar bar horizontally of vertically and controlling
// the format (printf style) with which to print the labels on the
// scalar bar. Also, the vtkScalarBarActor's property is applied to
// the scalar bar and annotations (including layer, and
// compositing operator).
//
// Set the text property/attributes of the title and the labels through the
// vtkTextProperty objects associated to this actor.
//
// .SECTION Caveats
// If a vtkLogLookupTable is specified as the lookup table to use, then the
// labels are created using a logarithmic scale.
//
// .SECTION See Also
// vtkActor2D vtkTextProperty vtkTextMapper vtkPolyDataMapper2D

#ifndef vtkScalarBarActor_h
#define vtkScalarBarActor_h

#include "vtkRenderingAnnotationModule.h" // For export macro
#include "vtkActor2D.h"

class vtkColor3ub;
class vtkPolyData;
class vtkPolyDataMapper2D;
class vtkProperty2D;
class vtkScalarsToColors;
class vtkScalarBarActorInternal;
class vtkTextActor;
class vtkTextMapper;
class vtkTextProperty;
class vtkTexture;
class vtkTexturedActor2D;

#define VTK_ORIENT_HORIZONTAL 0
#define VTK_ORIENT_VERTICAL 1

class VTKRENDERINGANNOTATION_EXPORT vtkScalarBarActor : public vtkActor2D
{
public:
  vtkTypeMacro(vtkScalarBarActor,vtkActor2D);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Instantiate object with 64 maximum colors; 5 labels; %%-#6.3g label
  // format, no title, and vertical orientation. The initial scalar bar
  // size is (0.05 x 0.8) of the viewport size.
  static vtkScalarBarActor* New();

  // Description:
  // Draw the scalar bar and annotation text to the screen.
  int RenderOpaqueGeometry(vtkViewport* viewport);
  virtual int RenderTranslucentPolygonalGeometry(vtkViewport*) { return 0; };
  int RenderOverlay(vtkViewport* viewport);

  // Description:
  // Does this prop have some translucent polygonal geometry?
  virtual int HasTranslucentPolygonalGeometry();

  // Description:
  // Release any graphics resources that are being consumed by this actor.
  // The parameter window could be used to determine which graphic
  // resources to release.
  virtual void ReleaseGraphicsResources(vtkWindow*);

  // Description:
  // Fills rect with the dimensions of the scalar bar in viewport coordinates.
  // Only the color bar is considered -- text labels are not considered.
  // rect is {xmin, xmax, width, height}
  virtual void GetScalarBarRect(int rect[4], vtkViewport* viewport);

  // Description:
  // Set/Get the lookup table to use. The lookup table specifies the number
  // of colors to use in the table (if not overridden), the scalar range,
  // and any annotated values.
  // Annotated values are rendered using vtkTextActor.
  virtual void SetLookupTable(vtkScalarsToColors*);
  vtkGetObjectMacro(LookupTable,vtkScalarsToColors);

  // Description:
  // Should be display the opacity as well. This is displayed by changing
  // the opacity of the scalar bar in accordance with the opacity of the
  // given color. For clarity, a texture grid is placed in the background
  // if Opacity is ON. You might also want to play with SetTextureGridWith
  // in that case. [Default: off]
  vtkSetMacro( UseOpacity, int );
  vtkGetMacro( UseOpacity, int );
  vtkBooleanMacro( UseOpacity, int );

  // Description:
  // Set/Get the maximum number of scalar bar segments to show. This may
  // differ from the number of colors in the lookup table, in which case
  // the colors are samples from the lookup table.
  vtkSetClampMacro(MaximumNumberOfColors, int, 2, VTK_INT_MAX);
  vtkGetMacro(MaximumNumberOfColors, int);

  // Description:
  // Set/Get the number of tick labels to show.
  vtkSetClampMacro(NumberOfLabels, int, 0, 64);
  vtkGetMacro(NumberOfLabels, int);

  // Description:
  // Control the orientation of the scalar bar.
  vtkSetClampMacro(Orientation,int,VTK_ORIENT_HORIZONTAL, VTK_ORIENT_VERTICAL);
  vtkGetMacro(Orientation, int);
  void SetOrientationToHorizontal()
       {this->SetOrientation(VTK_ORIENT_HORIZONTAL);}
  void SetOrientationToVertical() {this->SetOrientation(VTK_ORIENT_VERTICAL);}

  // Description:
  // Set/Get the title text property.
  virtual void SetTitleTextProperty(vtkTextProperty* p);
  vtkGetObjectMacro(TitleTextProperty,vtkTextProperty);

  // Description:
  // Set/Get the labels text property.
  virtual void SetLabelTextProperty(vtkTextProperty* p);
  vtkGetObjectMacro(LabelTextProperty,vtkTextProperty);

  // Description:
  // Set/Get the format with which to print the labels on the scalar
  // bar.
  vtkSetStringMacro(LabelFormat);
  vtkGetStringMacro(LabelFormat);

  // Description:
  // Set/Get the title of the scalar bar actor,
  vtkSetStringMacro(Title);
  vtkGetStringMacro(Title);

  // Description:
  // Set/Get the title for the component that is selected,
  vtkSetStringMacro(ComponentTitle);
  vtkGetStringMacro(ComponentTitle);

  // Description:
  // Shallow copy of a scalar bar actor. Overloads the virtual vtkProp method.
  void ShallowCopy(vtkProp* prop);

  // Description:
  // Set the width of the texture grid. Used only if UseOpacity is ON.
  vtkSetMacro( TextureGridWidth, double );
  vtkGetMacro( TextureGridWidth, double );

  // Description:
  // Get the texture actor.. you may want to change some properties on it
  vtkGetObjectMacro( TextureActor, vtkTexturedActor2D );

//BTX
  enum { PrecedeScalarBar = 0, SucceedScalarBar };
//ETX

  // Description:
  // Should the title and tick marks precede the scalar bar or succeed it?
  // This is measured along the viewport coordinate direction perpendicular
  // to the long axis of the scalar bar, not the reading direction.
  // Thus, succeed implies the that the text is above scalar bar if
  // the orientation is horizontal or right of scalar bar if the orientation
  // is vertical. Precede is the opposite.
  vtkSetClampMacro( TextPosition, int, PrecedeScalarBar, SucceedScalarBar);
  vtkGetMacro( TextPosition, int );
  virtual void SetTextPositionToPrecedeScalarBar()
    { this->SetTextPosition( vtkScalarBarActor::PrecedeScalarBar ); }
  virtual void SetTextPositionToSucceedScalarBar()
    { this->SetTextPosition( vtkScalarBarActor::SucceedScalarBar ); }

  // Description:
  // Set/Get the maximum width and height in pixels. Specifying the size as
  // a relative fraction of the viewport can sometimes undesirably stretch
  // the size of the actor too much. These methods allow the user to set
  // bounds on the maximum size of the scalar bar in pixels along any
  // direction. Defaults to unbounded.
  vtkSetMacro( MaximumWidthInPixels, int );
  vtkGetMacro( MaximumWidthInPixels, int );
  vtkSetMacro( MaximumHeightInPixels, int );
  vtkGetMacro( MaximumHeightInPixels, int );

  // Description:
  // Set/get the padding between the scalar bar and the text annotations.
  // This space is used to draw leader lines.
  // The default is 8 pixels.
  vtkSetMacro(AnnotationLeaderPadding, double);
  vtkGetMacro(AnnotationLeaderPadding, double);

  // Description:
  // Set/get whether text annotations should be rendered or not.
  // Currently, this only affects rendering when \a IndexedLookup is true.
  // The default is true.
  vtkSetMacro(DrawAnnotations, int);
  vtkGetMacro(DrawAnnotations, int);
  vtkBooleanMacro(DrawAnnotations, int);

  // Description:
  // Set/get whether the NaN annotation should be rendered or not.
  // This only affects rendering when \a DrawAnnotations is true.
  // The default is false.
  vtkSetMacro(DrawNanAnnotation, int);
  vtkGetMacro(DrawNanAnnotation, int);
  vtkBooleanMacro(DrawNanAnnotation, int);

  // Description:
  // Set/get how leader lines connecting annotations to values should be colored.
  //
  // When true, leader lines are all the same color (and match the LabelTextProperty color).
  // When false, leader lines take on the color of the value they correspond to.
  // This only affects rendering when \a DrawAnnotations is true.
  // The default is false.
  vtkSetMacro(FixedAnnotationLeaderLineColor, int);
  vtkGetMacro(FixedAnnotationLeaderLineColor, int);
  vtkBooleanMacro(FixedAnnotationLeaderLineColor, int);

  // Description:
  // Set/get the annotation text for "NaN" values.
  vtkSetStringMacro(NanAnnotation);
  vtkGetStringMacro(NanAnnotation);

  // Description:
  // Set/get whether annotation labels should be scaled with the viewport.
  //
  // The default value is 0 (no scaling).
  // If non-zero, the vtkTextActor instances used to render annotation
  // labels will have their TextScaleMode set to viewport-based scaling,
  // which nonlinearly scales font size with the viewport size.
  vtkSetMacro(AnnotationTextScaling,int);
  vtkGetMacro(AnnotationTextScaling,int);

  // Description:
  // Set/Get whether a background should be drawn around the scalar bar.
  // Default is off.
  vtkSetMacro(DrawBackground, int);
  vtkGetMacro(DrawBackground, int);
  vtkBooleanMacro(DrawBackground, int);

  // Description:
  // Set/Get whether a frame should be drawn around the scalar bar.
  // Default is off.
  vtkSetMacro(DrawFrame, int);
  vtkGetMacro(DrawFrame, int);
  vtkBooleanMacro(DrawFrame, int);

  // Description:
  // Set/Get whether the color bar should be drawn. If off, only the tickmarks
  // and text will be drawn. Default is on.
  vtkSetMacro(DrawColorBar, int);
  vtkGetMacro(DrawColorBar, int);
  vtkBooleanMacro(DrawColorBar, int);

  // Description:
  // Set/Get whether the tick labels should be drawn. Default is on.
  vtkSetMacro(DrawTickLabels, int);
  vtkGetMacro(DrawTickLabels, int);
  vtkBooleanMacro(DrawTickLabels, int);

  // Description:
  // Set/Get the background property.
  virtual void SetBackgroundProperty(vtkProperty2D* p);
  vtkGetObjectMacro(BackgroundProperty,vtkProperty2D);

  // Description:
  // Set/Get the frame property.
  virtual void SetFrameProperty(vtkProperty2D* p);
  vtkGetObjectMacro(FrameProperty,vtkProperty2D);

  // Description:
  // Set/get the amount of padding around text boxes.
  // The default is 1 pixel.
  vtkGetMacro(TextPad,int);
  vtkSetMacro(TextPad,int);

  // Description:
  // Set/get the margin in pixels, between the title and the bar,
  // when the \a Orientation is vertical.
  // The default is 0 pixels.
  vtkGetMacro(VerticalTitleSeparation,int);
  vtkSetMacro(VerticalTitleSeparation,int);

  // Description:
  // Set/get the thickness of the color bar relative to the widget frame.
  // The default is 0.375 and must always be in the range ]0, 1[.
  vtkGetMacro(BarRatio,double);
  vtkSetClampMacro(BarRatio,double,0.,1.);

  // Description:
  // Set/get the ratio of the title height to the tick label height
  // (used only when the \a Orientation is horizontal).
  // The default is 0.5, which attempts to make the labels and title
  // the same size. This must be a number in the range ]0, 1[.
  vtkGetMacro(TitleRatio,double);
  vtkSetClampMacro(TitleRatio,double,0.,1.);

protected:
  vtkScalarBarActor();
  ~vtkScalarBarActor();

  // Description:
  // Called from within \a RenderOpaqueGeometry when the internal state
  // members need to be updated before rendering.
  //
  // This method invokes many virtual methods that first lay out the
  // scalar bar and then use the layout to position actors and create
  // datasets used to represent the scalar bar.
  // Specifically, it invokes: FreeLayoutStorage, ComputeFrame,
  // ComputeScalarBarThickness, LayoutNanSwatch, PrepareTitleText,
  // LayoutTitle, ComputeScalarBarLength, LayoutTicks, and LayoutAnnotations
  // to perform the layout step.
  // Then, it invokes ConfigureAnnotations, ConfigureFrame,
  // ConfigureScalarBar, ConfigureTitle, ConfigureTicks, and
  // ConfigureNanSwatch to create and position actors used to render
  // portions of the scalar bar.
  //
  // By overriding one or more of these virtual methods, subclasses
  // may change the appearance of the scalar bar.
  //
  // In the layout phase, text actors must have their text properties
  // and input strings updated, but the position of the actors should
  // not be set or relied upon as subsequent layout steps may alter
  // their placement.
  virtual void RebuildLayout(vtkViewport* viewport);

  // Description:
  // Free internal storage used by the previous layout.
  virtual void FreeLayoutStorage();

  // Description:
  // If the scalar bar should be inset into a frame or rendered with a
  // solid background, this method will inset the outermost scalar bar
  // rectangle by a small amount to avoid having the scalar bar
  // illustration overlap any edges.
  //
  // This method must set the frame coordinates (this->P->Frame).
  virtual void ComputeFrame();

  // Description:
  // Determine how thick the scalar bar should be (on an axis perpendicular
  // to the direction in which scalar values vary).
  //
  // This method must set the scalar bar thickness
  // (this->P->ScalarBarBox.Size[0]).
  // It may depend on layout performed by ComputeFrame
  // (i.e., the frame coordinates in this->P->Frame).
  virtual void ComputeScalarBarThickness();

  // Description:
  // Determine the size of the NaN swatch if it is to be rendered.
  //
  // This method must set this->P->NanSwatchSize and this->P->NanBox.
  // It may depend on layout performed by ComputeScalarBarThickness.
  virtual void LayoutNanSwatch();

  // Description:
  // Set the title actor's input to the latest title (and subtitle) text.
  virtual void PrepareTitleText();

  // Description:
  // Determine the position and size of the scalar bar title box.
  //
  // This method must set this->P->TitleBox
  // It may depend on layout performed by LayoutNanSwatch.
  // If useTickBox is true, it should increase the target area
  // for the label to touch the tick box. It is called in this
  // way when the tick labels are small due to constraints other
  // than the title box.
  virtual void LayoutTitle();

  // Description:
  // Determine how long the scalar bar should be (on an axis parallel
  // to the direction in which scalar values vary).
  //
  // This method must set this->P->ScalarBarBox.Size[1] and should
  // estimate this->P->ScalarBarBox.Posn.
  // It may depend on layout performed by LayoutTitle.
  virtual void ComputeScalarBarLength();

  // Description:
  // Determine the size and placement of any tick marks to be rendered.
  //
  // This method must set this->P->TickBox.
  // It may depend on layout performed by ComputeScalarBarLength.
  //
  // The default implementation creates exactly this->NumberOfLabels
  // tick marks, uniformly spaced on a linear or logarithmic scale.
  virtual void LayoutTicks();

  // Description:
  // This method must lay out annotation text and leader lines so
  // they do not overlap.
  //
  // This method must set this->P->AnnotationAnchors.
  // It may depend on layout performed by LayoutTicks.
  virtual void LayoutAnnotations();

  // Description:
  // Generate/configure the annotation labels using the laid-out geometry.
  virtual void ConfigureAnnotations();

  // Description:
  // Generate/configure the representation of the frame from laid-out geometry.
  virtual void ConfigureFrame();

  // Description:
  // For debugging, add placement boxes to the frame polydata.
  virtual void DrawBoxes();

  // Description:
  // Generate/configure the scalar bar representation from laid-out geometry.
  virtual void ConfigureScalarBar();

  // Description:
  // Generate/configure the title actor using the laid-out geometry.
  virtual void ConfigureTitle();

  // Description:
  // Generate/configure the tick-mark actors using the laid-out geometry.
  virtual void ConfigureTicks();

  // Description:
  // Generate/configure the NaN swatch using the laid-out geometry.
  //
  // Currently the NaN swatch is rendered by the same actor as the scalar bar.
  // This may change in the future.
  virtual void ConfigureNanSwatch();

  // Description:
  // Subclasses may override this method to alter this->P->Labels, allowing
  // the addition and removal of annotations. The member maps viewport coordinates
  // along the long axis of the scalar bar to text (which may include MathText;
  // see vtkTextActor). It is a single-valued map, so you must perturb
  // the coordinate if you wish multiple labels to annotate the same position.
  // Each entry in this->P->Labels must have a matching entry in this->P->LabelColors.
  virtual void EditAnnotations() { }

  // Description:
  // Compute the best size for the legend title.
  //
  // This guarantees that the title will fit within the frame defined by Position and Position2.
  virtual void SizeTitle(double* titleSize, int* size, vtkViewport* viewport);

  // Description:
  // Allocate actors for lookup table annotations and position them properly.
  int MapAnnotationLabels(
    vtkScalarsToColors* lkup, double start, double delta, const double* range);

  // Description:
  // This method is called by \a ConfigureAnnotationLabels when Orientation is VTK_ORIENT_VERTICAL.
  int PlaceAnnotationsVertically(
    double barX, double barY, double barWidth, double barHeight,
    double delta, double pad);
  // Description:
  // This method is called by \a ConfigureAnnotationLabels when Orientation is VTK_ORIENT_HORIZONTAL.
  int PlaceAnnotationsHorizontally(
    double barX, double barY, double barWidth, double barHeight,
    double delta, double pad);

  /// User-changeable settings
  //@{
  int MaximumNumberOfColors;
  int NumberOfLabels;
  int NumberOfLabelsBuilt;
  int Orientation;
  int DrawBackground; // off by default
  int DrawFrame; // off by default
  int DrawColorBar; // on by default
  int DrawTickLabels; // on by default
  int DrawAnnotations;
  int DrawNanAnnotation;
  int AnnotationTextScaling; // off by default
  int FixedAnnotationLeaderLineColor;
  vtkProperty2D* BackgroundProperty;
  vtkProperty2D* FrameProperty;
  char* Title;
  char* ComponentTitle;
  char* LabelFormat;
  int UseOpacity; // off by default
  double TextureGridWidth;
  int TextPosition;
  char* NanAnnotation;
  double AnnotationLeaderPadding;
  int MaximumWidthInPixels;
  int MaximumHeightInPixels;
  int TextPad;
  int VerticalTitleSeparation;
  double BarRatio;
  double TitleRatio;
  //@}

  /// Internal state used for rendering
  //@{
  vtkTimeStamp BuildTime; //!< Last time internal state changed.
  int LastSize[2]; //!< Projected size in viewport coordinates of last build.
  int LastOrigin[2]; //!< Projected origin (viewport coordinates) of last build.

  vtkScalarBarActorInternal* P; //!< Containers shared with subclasses

  vtkScalarsToColors* LookupTable; //!< The object this actor illustrates

  vtkTextProperty* TitleTextProperty; //!< Font for the legend title.
  vtkTextProperty* LabelTextProperty; //!< Font for tick+annotation labels.
  vtkTextActor* TitleActor; //!< The legend title text renderer.

  vtkPolyData* ScalarBar; //!< Polygon(s) colored by \a LookupTable.
  vtkPolyDataMapper2D* ScalarBarMapper; //!< Mapper for \a ScalarBar.
  vtkActor2D* ScalarBarActor; //!< Actor for \a ScalarBar.
  vtkPolyData* TexturePolyData; //!< Polygon colored when UseOpacity is true.
  vtkTexture* Texture; //!< Color data for \a TexturePolyData.
  vtkTexturedActor2D* TextureActor; //!< Actor for \a TexturePolyData.

  vtkPolyData* Background; //!< Polygon used to fill the background.
  vtkPolyDataMapper2D* BackgroundMapper; //!< Mapper for \a Background.
  vtkActor2D* BackgroundActor; //!< Actor for \a Background.

  vtkPolyData* Frame; //!< Polyline used to highlight frame.
  vtkPolyDataMapper2D* FrameMapper; //!< Mapper for \a Frame.
  vtkActor2D* FrameActor; //!< Actor for \a Frame.
  //@}

private:
  vtkScalarBarActor(const vtkScalarBarActor&);  // Not implemented.
  void operator=(const vtkScalarBarActor&);  // Not implemented.
};


#endif

