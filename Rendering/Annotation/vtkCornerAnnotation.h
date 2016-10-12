/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCornerAnnotation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCornerAnnotation
 * @brief   text annotation in four corners
 *
 * This is an annotation object that manages four text actors / mappers
 * to provide annotation in the four corners of a viewport
 *
 * @par Special input text::
 * - <image> : will be replaced with slice number (relative number)
 * - <slice> : will be replaced with slice number (relative number)
 * - <image_and_max> : will be replaced with slice number and slice max (relative)
 * - <slice_and_max> : will be replaced with slice number and slice max (relative)
 * - <slice_pos> : will be replaced by the position of the current slice
 * - <window> : will be replaced with window value
 * - <level> : will be replaced with level value
 * - <window_level> : will be replaced with window and level value
 *
 * @sa
 * vtkActor2D vtkTextMapper
*/

#ifndef vtkCornerAnnotation_h
#define vtkCornerAnnotation_h

#include "vtkRenderingAnnotationModule.h" // For export macro
#include "vtkActor2D.h"

class vtkTextMapper;
class vtkImageMapToWindowLevelColors;
class vtkImageActor;
class vtkTextProperty;

class VTKRENDERINGANNOTATION_EXPORT vtkCornerAnnotation : public vtkActor2D
{
public:
  vtkTypeMacro(vtkCornerAnnotation,vtkActor2D);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Instantiate object with a rectangle in normaled view coordinates
   * of (0.2,0.85, 0.8, 0.95).
   */
  static vtkCornerAnnotation *New();

  //@{
  /**
   * Draw the scalar bar and annotation text to the screen.
   */
  int RenderOpaqueGeometry(vtkViewport* viewport);
  virtual int RenderTranslucentPolygonalGeometry(vtkViewport* ) {return 0;};
  int RenderOverlay(vtkViewport* viewport);
  //@}

  /**
   * Does this prop have some translucent polygonal geometry?
   */
  virtual int HasTranslucentPolygonalGeometry();

  //@{
  /**
   * Set/Get the maximum height of a line of text as a
   * percentage of the vertical area allocated to this
   * scaled text actor. Defaults to 1.0
   */
  vtkSetMacro(MaximumLineHeight,double);
  vtkGetMacro(MaximumLineHeight,double);
  //@}

  //@{
  /**
   * Set/Get the minimum/maximum size font that will be shown.
   * If the font drops below the minimum size it will not be rendered.
   */
  vtkSetMacro(MinimumFontSize,int);
  vtkGetMacro(MinimumFontSize,int);
  vtkSetMacro(MaximumFontSize,int);
  vtkGetMacro(MaximumFontSize,int);
  //@}

  //@{
  /**
   * Set/Get font scaling factors
   * The font size, f, is calculated as the largest possible value
   * such that the annotations for the given viewport do not overlap.
   * This font size is scaled non-linearly with the viewport size,
   * to maintain an acceptable readable size at larger viewport sizes,
   * without being too big.
   * f' = linearScale * pow(f,nonlinearScale)
   */
  vtkSetMacro( LinearFontScaleFactor, double );
  vtkGetMacro( LinearFontScaleFactor, double );
  vtkSetMacro( NonlinearFontScaleFactor, double );
  vtkGetMacro( NonlinearFontScaleFactor, double );
  //@}

  /**
   * Release any graphics resources that are being consumed by this actor.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  virtual void ReleaseGraphicsResources(vtkWindow *);

  //@{
  /**
   * Position used to get or set the corner annotation text.
   * \sa GetText(), SetText()
   */
  enum TextPosition
  {
    LowerLeft = 0, ///< Uses the lower left corner.
    LowerRight,    ///< Uses the lower right corner.
    UpperLeft,     ///< Uses the upper left corner.
    UpperRight,    ///< Uses the upper right corner.
    LowerEdge,     ///< Uses the lower edge center.
    RightEdge,     ///< Uses the right edge center.
    LeftEdge,      ///< Uses the left edge center
    UpperEdge      ///< Uses the upper edge center.
  };
  static const int NumTextPositions = 8;
  //@}

  //@{
  /**
   * Set/Get the text to be displayed for each corner
   * \sa TextPosition
   */
  void SetText(int i, const char *text);
  const char* GetText(int i);
  void ClearAllTexts();
  void CopyAllTextsFrom(vtkCornerAnnotation *ca);
  //@}

  //@{
  /**
   * Set an image actor to look at for slice information
   */
  void SetImageActor(vtkImageActor*);
  vtkGetObjectMacro(ImageActor,vtkImageActor);
  //@}

  //@{
  /**
   * Set an instance of vtkImageMapToWindowLevelColors to use for
   * looking at window level changes
   */
  void SetWindowLevel(vtkImageMapToWindowLevelColors*);
  vtkGetObjectMacro(WindowLevel,vtkImageMapToWindowLevelColors);
  //@}

  //@{
  /**
   * Set the value to shift the level by.
   */
  vtkSetMacro(LevelShift, double);
  vtkGetMacro(LevelShift, double);
  //@}

  //@{
  /**
   * Set the value to scale the level by.
   */
  vtkSetMacro(LevelScale, double);
  vtkGetMacro(LevelScale, double);
  //@}

  //@{
  /**
   * Set/Get the text property of all corners.
   */
  virtual void SetTextProperty(vtkTextProperty *p);
  vtkGetObjectMacro(TextProperty,vtkTextProperty);
  //@}

  //@{
  /**
   * Even if there is an image actor, should `slice' and `image' be displayed?
   */
  vtkBooleanMacro(ShowSliceAndImage, int);
  vtkSetMacro(ShowSliceAndImage, int);
  vtkGetMacro(ShowSliceAndImage, int);
  //@}

protected:
  vtkCornerAnnotation();
  ~vtkCornerAnnotation();

  double MaximumLineHeight;

  vtkTextProperty *TextProperty;

  vtkImageMapToWindowLevelColors *WindowLevel;
  double LevelShift;
  double LevelScale;
  vtkImageActor *ImageActor;
  vtkImageActor *LastImageActor;

  char *CornerText[NumTextPositions];

  int FontSize;
  vtkActor2D    *TextActor[NumTextPositions];
  vtkTimeStamp   BuildTime;
  int            LastSize[2];
  vtkTextMapper *TextMapper[NumTextPositions];

  int MinimumFontSize;
  int MaximumFontSize;

  double LinearFontScaleFactor;
  double NonlinearFontScaleFactor;

  int ShowSliceAndImage;

  /**
   * Search for replacable tokens and replace
   */
  virtual void TextReplace(
    vtkImageActor *ia, vtkImageMapToWindowLevelColors *wl);

  //@{
  /**
   * Set text actor positions given a viewport size and justification
   */
  virtual void SetTextActorsPosition(int vsize[2]);
  virtual void SetTextActorsJustification();
  //@}

private:
  vtkCornerAnnotation(const vtkCornerAnnotation&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCornerAnnotation&) VTK_DELETE_FUNCTION;
};


#endif



