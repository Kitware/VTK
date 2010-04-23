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
// vtkScalarBarActor creates a scalar bar with annotation text. A scalar
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
// of the scalar bar and the number of annotation labels. The actual position
// of the scalar bar on the screen is controlled by using the
// vtkActor2D::SetPosition() method (by default the scalar bar is
// centered in the viewport).  Other features include the ability to
// orient the scalar bar horizontally of vertically and controlling
// the format (printf style) with which to print the labels on the
// scalar bar. Also, the vtkScalarBarActor's property is applied to
// the scalar bar and annotation (including layer, and
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

#ifndef __vtkScalarBarActor_h
#define __vtkScalarBarActor_h

#include "vtkActor2D.h"

class vtkPolyData;
class vtkPolyDataMapper2D;
class vtkScalarsToColors;
class vtkTextMapper;
class vtkTextProperty;
class vtkTexture;

#define VTK_ORIENT_HORIZONTAL 0
#define VTK_ORIENT_VERTICAL 1

class VTK_RENDERING_EXPORT vtkScalarBarActor : public vtkActor2D
{
public:
  vtkTypeMacro(vtkScalarBarActor,vtkActor2D);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Instantiate object with 64 maximum colors; 5 labels; %%-#6.3g label
  // format, no title, and vertical orientation. The initial scalar bar
  // size is (0.05 x 0.8) of the viewport size.
  static vtkScalarBarActor *New();

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
  virtual void ReleaseGraphicsResources(vtkWindow *);

  // Description:
  // Set/Get the vtkLookupTable to use. The lookup table specifies the number
  // of colors to use in the table (if not overridden), as well as the scalar
  // range.
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
  vtkSetClampMacro(MaximumNumberOfColors, int, 2, VTK_LARGE_INTEGER);
  vtkGetMacro(MaximumNumberOfColors, int);
  
  // Description:
  // Set/Get the number of annotation labels to show.
  vtkSetClampMacro(NumberOfLabels, int, 0, 64);
  vtkGetMacro(NumberOfLabels, int);
  
  // Description:
  // Control the orientation of the scalar bar.
  vtkSetClampMacro(Orientation,int,VTK_ORIENT_HORIZONTAL, VTK_ORIENT_VERTICAL);
  vtkGetMacro(Orientation, int);
  void SetOrientationToHorizontal()
       {this->SetOrientation(VTK_ORIENT_HORIZONTAL);};
  void SetOrientationToVertical() {this->SetOrientation(VTK_ORIENT_VERTICAL);};

  // Description:
  // Set/Get the title text property.
  virtual void SetTitleTextProperty(vtkTextProperty *p);
  vtkGetObjectMacro(TitleTextProperty,vtkTextProperty);
  
  // Description:
  // Set/Get the labels text property.
  virtual void SetLabelTextProperty(vtkTextProperty *p);
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
  void ShallowCopy(vtkProp *prop);

  // Description:
  // Set the width of the texture grid. Used only if UseOpacity is ON.
  vtkSetMacro( TextureGridWidth, double );
  vtkGetMacro( TextureGridWidth, double );

  // Description:
  // Get the texture actor.. you may want to change some properties on it
  vtkGetObjectMacro( TextureActor, vtkActor2D );

//BTX
  enum { PrecedeScalarBar = 0, SucceedScalarBar };
//ETX

  // Description:
  // Have the text preceding the scalar bar or suceeding it ?
  // Succeed implies the that the text is Above scalar bar if orientation 
  // is horizontal or Right of scalar bar if orientation is vertical.
  // Precede is the opposite
  vtkSetClampMacro( TextPosition, int, PrecedeScalarBar, SucceedScalarBar);
  vtkGetMacro( TextPosition, int );
  virtual void SetTextPositionToPrecedeScalarBar()
    { this->SetTextPosition( vtkScalarBarActor::PrecedeScalarBar ); }
  virtual void SetTextPositionToSucceedScalarBar()
    { this->SetTextPosition( vtkScalarBarActor::SucceedScalarBar ); }

  // Description:
  // Set/Get the maximum width and height in pixels. Specifying the size as
  // a relative fraction of the viewport can sometimes undersirably strech 
  // the size of the actor too much. These methods allow the user to set 
  // bounds on the maximum size of the scalar bar in pixels along any 
  // direction. Defaults to unbounded.
  vtkSetMacro( MaximumWidthInPixels, int );
  vtkGetMacro( MaximumWidthInPixels, int );
  vtkSetMacro( MaximumHeightInPixels, int );
  vtkGetMacro( MaximumHeightInPixels, int );

protected:
  vtkScalarBarActor();
  ~vtkScalarBarActor();

  vtkScalarsToColors *LookupTable;
  vtkTextProperty *TitleTextProperty;
  vtkTextProperty *LabelTextProperty;

  int   MaximumNumberOfColors;
  int   NumberOfLabels;
  int   NumberOfLabelsBuilt;
  int   Orientation;
  char  *Title;
  char* ComponentTitle;
  char  *LabelFormat;
  int   UseOpacity; // off by default
  double TextureGridWidth;
  int TextPosition;

  vtkTextMapper **TextMappers;
  vtkActor2D    **TextActors;
  virtual void AllocateAndSizeLabels(int *labelSize, int *size,
                                     vtkViewport *viewport, double *range);

  vtkTextMapper *TitleMapper;
  vtkActor2D    *TitleActor;
  virtual void SizeTitle(int *titleSize, int *size, vtkViewport *viewport);

  vtkPolyData         *ScalarBar;
  vtkPolyDataMapper2D *ScalarBarMapper;
  vtkActor2D          *ScalarBarActor;

  vtkPolyData         *TexturePolyData;
  vtkTexture          *Texture;
  vtkActor2D          *TextureActor;

  vtkTimeStamp  BuildTime;
  int LastSize[2];
  int LastOrigin[2];

  int MaximumWidthInPixels;
  int MaximumHeightInPixels;

private:
  vtkScalarBarActor(const vtkScalarBarActor&);  // Not implemented.
  void operator=(const vtkScalarBarActor&);  // Not implemented.
};


#endif

