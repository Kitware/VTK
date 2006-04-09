/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBalloonRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkBalloonRepresentation - represent the vtkBalloonWidget
// .SECTION Description
// The vtkBalloonRepresentation is used to represent the vtkBalloonWidget.
// This representation consists of two parts: a text string and an image.  At
// least one of these two parts must be defined, but it is allowable to
// specify both, or just an image or just text. If both the text and image
// are specified, then methods are available for positioning the text and
// image with respect to each other. 
//
// The size of the balloon is ultimately controlled by the text properties
// (i.e., font size). This representation uses a layout policy as follows.
// 
// If there is just text and no image, then the text properties are used to
// control the size of the balloon.
//
// If there is just an image and no text, then the ImageSize[2] member is
// used to control the image size. (The image will fit into this rectangle,
// but will not necessarily fill the whole rectangle, i.e., the image is not
// stretched).
//
// If there is text and an image, use the following approach. First, based on
// the font size and other text properties, get the size of the text. Second,
// depending on the layout of the image and text, use the text size to
// control the size of the neighboring image (since both the text and image have
// to share a common rectangular frame). However, if this results in an image
// that is smaller than ImageSize[2], then the image size will be set to
// ImageSize[2] and the frame will be adjusted accordingly.

// .SECTION See Also
// vtkBalloonWidget


#ifndef __vtkBalloonRepresentation_h
#define __vtkBalloonRepresentation_h

#include "vtkWidgetRepresentation.h"

class vtkTextMapper;
class vtkTextActor;
class vtkTextProperty;
class vtkPoints;
class vtkCellArray;
class vtkPolyData;
class vtkPolyDataMapper2D;
class vtkActor2D;
class vtkProperty2D;
class vtkImageData;
class vtkTexture;
class vtkPoints;
class vtkPolyData;
class vtkPolyDataMapper2D;
class vtkActor2D;

class VTK_WIDGETS_EXPORT vtkBalloonRepresentation : public vtkWidgetRepresentation
{
public:
  // Description:
  // Instantiate the class.
  static vtkBalloonRepresentation *New();

  // Description:
  // Standard VTK methods.
  vtkTypeRevisionMacro(vtkBalloonRepresentation,vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify/retrieve the image to display in the balloon.
  virtual void SetBalloonImage(vtkImageData *img);
  vtkGetObjectMacro(BalloonImage,vtkImageData);

  // Description:
  // Specify/retrieve the text to display in the balloon.
  vtkGetStringMacro(BalloonText);
  vtkSetStringMacro(BalloonText);

  // Description:
  // Specify the minimum size for the image. Note that this is a bounding
  // rectangle, the image will fit inside of it. However, if the balloon
  // consists of text plus an image, then the image may be bigger than
  // ImageSize[2] to fit into the balloon frame.
  vtkSetVector2Macro(ImageSize,int);
  vtkGetVector2Macro(ImageSize,int);

  // Description:
  // Set/get the text property (relevant only if text shown).
  virtual void SetTextProperty(vtkTextProperty *p);
  vtkGetObjectMacro(TextProperty,vtkTextProperty);
    
  // Description:
  // Set/get the frame property (relevant only if text shown).
  virtual void SetFrameProperty(vtkProperty2D *p);
  vtkGetObjectMacro(FrameProperty,vtkProperty2D);
    
  // Description:
  // Set/get the image property (relevant only if image shown).
  virtual void SetImageProperty(vtkProperty2D *p);
  vtkGetObjectMacro(ImageProperty,vtkProperty2D);
    
//BTX
  enum {ImageLeft=0,ImageRight,ImageBottom,ImageTop};
//ETX
  // Description:
  // Specify the layout of the image and text within the balloon. Note that
  // there are reduncies in these methods, for example
  // SetBalloonLayoutToImageLeft() results in the same effect as
  // SetBalloonLayoutToTextRight(). If only text is specified, or only an
  // image is specified, then it doesn't matter how the layout is specified.
  vtkSetMacro(BalloonLayout,int);
  vtkGetMacro(BalloonLayout,int);
  void SetBalloonLayoutToImageLeft() {this->SetBalloonLayout(ImageLeft);}
  void SetBalloonLayoutToImageRight() {this->SetBalloonLayout(ImageRight);}
  void SetBalloonLayoutToImageBottom() {this->SetBalloonLayout(ImageBottom);}
  void SetBalloonLayoutToImageTop() {this->SetBalloonLayout(ImageTop);}
  void SetBalloonLayoutToTextLeft() {this->SetBalloonLayout(ImageRight);}
  void SetBalloonLayoutToTextRight() {this->SetBalloonLayout(ImageLeft);}
  void SetBalloonLayoutToTextTop() {this->SetBalloonLayout(ImageBottom);}
  void SetBalloonLayoutToTextBottom() {this->SetBalloonLayout(ImageTop);}

  // Description:
  // Set/Get the offset from the mouse pointer from which to place the
  // balloon. The representation will try and honor this offset unless there
  // is a collision with the side of the renderer, in which case the balloon 
  // will be repositioned to lie within the rendering window.
  vtkSetVector2Macro(Offset,int);
  vtkGetVector2Macro(Offset,int);

  // Description:
  // Set/Get the padding (in pixels) that whould be used around the text
  // and/or image (i.e., between the frame, the text and the image).
  vtkSetClampMacro(Padding,int,0,100);
  vtkGetMacro(Padding,int);

  // Description:
  // These are methods that satisfy vtkWidgetRepresentation's API.
  virtual void StartWidgetInteraction(double e[2]);
  virtual void EndWidgetInteraction(double e[2]);
  virtual void BuildRepresentation();
  
  // Description:
  // Methods required by vtkProp superclass.
  virtual void ReleaseGraphicsResources(vtkWindow *w);
  virtual int RenderOverlay(vtkViewport *viewport);

protected:
  vtkBalloonRepresentation();
  ~vtkBalloonRepresentation();

  // The balloon text and image
  char         *BalloonText;
  vtkImageData *BalloonImage;

  // The layout of the balloon
  int BalloonLayout;

  // Controlling placement
  int Padding;
  int Offset[2];
  int ImageSize[2];

  // Represent the text
  vtkTextMapper       *TextMapper;
  vtkActor2D          *TextActor;
  vtkTextProperty     *TextProperty;
  
  // Represent the image
  vtkTexture          *Texture;
  vtkPolyData         *TexturePolyData;
  vtkPoints           *TexturePoints;
  vtkPolyDataMapper2D *TextureMapper;
  vtkActor2D          *TextureActor;
  vtkProperty2D       *ImageProperty;

  // The frame
  vtkPoints           *FramePoints;
  vtkCellArray        *FramePolygon;
  vtkPolyData         *FramePolyData;
  vtkPolyDataMapper2D *FrameMapper;
  vtkActor2D          *FrameActor;
  vtkProperty2D       *FrameProperty;
  
  // Internal variable controlling rendering process
  int TextVisible;
  int ImageVisible;
  
  // Helper method
  void AdjustImageSize(double imageSize[2]);
  void ScaleImage(double imageSize[2],double scale);

private:
  vtkBalloonRepresentation(const vtkBalloonRepresentation&);  //Not implemented
  void operator=(const vtkBalloonRepresentation&);  //Not implemented
};

#endif
