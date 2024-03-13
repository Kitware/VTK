// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkBalloonRepresentation
 * @brief   represent the vtkBalloonWidget
 *
 * The vtkBalloonRepresentation is used to represent the vtkBalloonWidget.
 * This representation is defined by two items: a text string and an image.
 * At least one of these two items must be defined, but it is allowable to
 * specify both, or just an image or just text. If both the text and image
 * are specified, then methods are available for positioning the text and
 * image with respect to each other.
 *
 * The balloon representation consists of three parts: text, a rectangular
 * frame behind the text, and an image placed next to the frame and sized
 * to match the frame.
 *
 * The size of the balloon is ultimately controlled by the text properties
 * (i.e., font size). This representation uses a layout policy as follows.
 *
 * If there is just text and no image, then the text properties and padding
 * are used to control the size of the balloon.
 *
 * If there is just an image and no text, then the ImageSize[2] member is
 * used to control the image size. (The image will fit into this rectangle,
 * but will not necessarily fill the whole rectangle, i.e., the image is not
 * stretched).
 *
 * If there is text and an image, the following approach ia used. First,
 * based on the font size and other related properties (e.g., padding),
 * determine the size of the frame. Second, depending on the layout of the
 * image and text frame, control the size of the neighboring image (since the
 * frame and image share a common edge). However, if this results in an image
 * that is smaller than ImageSize[2], then the image size will be set to
 * ImageSize[2] and the frame will be adjusted accordingly. The text is
 * always placed in the center of the frame if the frame is resized.
 *
 * @sa
 * vtkBalloonWidget
 */

#ifndef vtkBalloonRepresentation_h
#define vtkBalloonRepresentation_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkWidgetRepresentation.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
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
class vtkTexturedActor2D;

class VTKINTERACTIONWIDGETS_EXPORT VTK_MARSHALAUTO vtkBalloonRepresentation
  : public vtkWidgetRepresentation
{
public:
  /**
   * Instantiate the class.
   */
  static vtkBalloonRepresentation* New();

  ///@{
  /**
   * Standard VTK methods.
   */
  vtkTypeMacro(vtkBalloonRepresentation, vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Specify/retrieve the image to display in the balloon.
   */
  virtual void SetBalloonImage(vtkImageData* img);
  vtkGetObjectMacro(BalloonImage, vtkImageData);
  ///@}

  ///@{
  /**
   * Specify/retrieve the text to display in the balloon.
   */
  vtkGetStringMacro(BalloonText);
  vtkSetStringMacro(BalloonText);
  ///@}

  ///@{
  /**
   * Specify the minimum size for the image. Note that this is a bounding
   * rectangle, the image will fit inside of it. However, if the balloon
   * consists of text plus an image, then the image may be bigger than
   * ImageSize[2] to fit into the balloon frame.
   */
  vtkSetVector2Macro(ImageSize, int);
  vtkGetVector2Macro(ImageSize, int);
  ///@}

  ///@{
  /**
   * Set/get the text property (relevant only if text is shown).
   */
  virtual void SetTextProperty(vtkTextProperty* p);
  vtkGetObjectMacro(TextProperty, vtkTextProperty);
  ///@}

  ///@{
  /**
   * Set/get the frame property (relevant only if text is shown).
   * The frame lies behind the text.
   */
  virtual void SetFrameProperty(vtkProperty2D* p);
  vtkGetObjectMacro(FrameProperty, vtkProperty2D);
  ///@}

  ///@{
  /**
   * Set/get the image property (relevant only if an image is shown).
   */
  virtual void SetImageProperty(vtkProperty2D* p);
  vtkGetObjectMacro(ImageProperty, vtkProperty2D);
  ///@}

  enum
  {
    ImageLeft = 0,
    ImageRight,
    ImageBottom,
    ImageTop
  };

  ///@{
  /**
   * Specify the layout of the image and text within the balloon. Note that
   * there are reduncies in these methods, for example
   * SetBalloonLayoutToImageLeft() results in the same effect as
   * SetBalloonLayoutToTextRight(). If only text is specified, or only an
   * image is specified, then it doesn't matter how the layout is specified.
   */
  vtkSetMacro(BalloonLayout, int);
  vtkGetMacro(BalloonLayout, int);
  void SetBalloonLayoutToImageLeft() { this->SetBalloonLayout(ImageLeft); }
  void SetBalloonLayoutToImageRight() { this->SetBalloonLayout(ImageRight); }
  void SetBalloonLayoutToImageBottom() { this->SetBalloonLayout(ImageBottom); }
  void SetBalloonLayoutToImageTop() { this->SetBalloonLayout(ImageTop); }
  void SetBalloonLayoutToTextLeft() { this->SetBalloonLayout(ImageRight); }
  void SetBalloonLayoutToTextRight() { this->SetBalloonLayout(ImageLeft); }
  void SetBalloonLayoutToTextTop() { this->SetBalloonLayout(ImageBottom); }
  void SetBalloonLayoutToTextBottom() { this->SetBalloonLayout(ImageTop); }
  ///@}

  ///@{
  /**
   * Set/Get the offset from the mouse pointer from which to place the
   * balloon. The representation will try and honor this offset unless there
   * is a collision with the side of the renderer, in which case the balloon
   * will be repositioned to lie within the rendering window.
   */
  vtkSetVector2Macro(Offset, int);
  vtkGetVector2Macro(Offset, int);
  ///@}

  ///@{
  /**
   * Set/Get the padding (in pixels) that is used between the text and the
   * frame.
   */
  vtkSetClampMacro(Padding, int, 0, 100);
  vtkGetMacro(Padding, int);
  ///@}

  ///@{
  /**
   * These are methods that satisfy vtkWidgetRepresentation's API.
   */
  void StartWidgetInteraction(double e[2]) override;
  void EndWidgetInteraction(double e[2]) override;
  void BuildRepresentation() override;
  int ComputeInteractionState(int X, int Y, int modify = 0) override;
  ///@}

  ///@{
  /**
   * Methods required by vtkProp superclass.
   */
  void ReleaseGraphicsResources(vtkWindow* w) override;
  int RenderOverlay(vtkViewport* viewport) override;
  ///@}

  /**
   * State is either outside, or inside (on the text portion of the image).
   */
  enum InteractionStateType
  {
    Outside = 0,
    OnText,
    OnImage
  };

protected:
  vtkBalloonRepresentation();
  ~vtkBalloonRepresentation() override;

  // The balloon text and image
  char* BalloonText;
  vtkImageData* BalloonImage;

  // The layout of the balloon
  int BalloonLayout;

  // Controlling placement
  int Padding;
  int Offset[2];
  int ImageSize[2];

  // Represent the text
  vtkTextMapper* TextMapper;
  vtkActor2D* TextActor;
  vtkTextProperty* TextProperty;

  // Represent the image
  vtkTexture* Texture;
  vtkPolyData* TexturePolyData;
  vtkPoints* TexturePoints;
  vtkPolyDataMapper2D* TextureMapper;
  vtkTexturedActor2D* TextureActor;
  vtkProperty2D* ImageProperty;

  // The frame
  vtkPoints* FramePoints;
  vtkCellArray* FramePolygon;
  vtkPolyData* FramePolyData;
  vtkPolyDataMapper2D* FrameMapper;
  vtkActor2D* FrameActor;
  vtkProperty2D* FrameProperty;

  // Internal variable controlling rendering process
  int TextVisible;
  int ImageVisible;

  // Helper methods
  void AdjustImageSize(double imageSize[2]);
  void ScaleImage(double imageSize[2], double scale);

private:
  vtkBalloonRepresentation(const vtkBalloonRepresentation&) = delete;
  void operator=(const vtkBalloonRepresentation&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
