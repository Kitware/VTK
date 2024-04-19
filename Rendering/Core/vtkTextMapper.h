// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkTextMapper
 * @brief   2D text annotation
 *
 * vtkTextMapper provides 2D text annotation support for VTK.  It is a
 * vtkMapper2D that can be associated with a vtkActor2D and placed into a
 * vtkRenderer.
 *
 * To use vtkTextMapper, specify an input text string.
 *
 * @sa
 * vtkActor2D vtkTextActor vtkTextActor3D vtkTextProperty vtkTextRenderer
 */

#ifndef vtkTextMapper_h
#define vtkTextMapper_h

#include "vtkMapper2D.h"
#include "vtkRenderingCoreModule.h" // For export macro

#include "vtkNew.h" // For vtkNew

VTK_ABI_NAMESPACE_BEGIN
class vtkActor2D;
class vtkImageData;
class vtkPoints;
class vtkPolyData;
class vtkPolyDataMapper2D;
class vtkTextProperty;
class vtkTexture;
class vtkTimeStamp;
class vtkViewport;

class VTKRENDERINGCORE_EXPORT vtkTextMapper : public vtkMapper2D
{
public:
  vtkTypeMacro(vtkTextMapper, vtkMapper2D);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Creates a new text mapper.
   */
  static vtkTextMapper* New();

  ///@{
  /**
   * Return the size[2]/width/height of the rectangle required to draw this
   * mapper (in pixels).
   */
  virtual void GetSize(vtkViewport*, int size[2]);
  virtual int GetWidth(vtkViewport* v);
  virtual int GetHeight(vtkViewport* v);
  ///@}

  ///@{
  /**
   * The input text string to the mapper.
   */
  vtkSetStringMacro(Input);
  vtkGetStringMacro(Input);
  ///@}

  ///@{
  /**
   * Set/Get the text property.
   */
  virtual void SetTextProperty(vtkTextProperty* p);
  vtkGetObjectMacro(TextProperty, vtkTextProperty);
  ///@}

  /**
   * Shallow copy of an actor.
   */
  void ShallowCopy(vtkAbstractMapper* m) override;

  ///@{
  /**
   * Set and return the font size (in points) required to make this mapper fit
   * in a given
   * target rectangle (width x height, in pixels). A static version of the method
   * is also available for convenience to other classes (e.g., widgets).
   */
  virtual int SetConstrainedFontSize(vtkViewport*, int targetWidth, int targetHeight);
  static int SetConstrainedFontSize(
    vtkTextMapper*, vtkViewport*, int targetWidth, int targetHeight);
  ///@}

  /**
   * Set and return the font size (in points) required to make each element of
   * an array
   * of mappers fit in a given rectangle (width x height, in pixels).  This
   * font size is the smallest size that was required to fit the largest
   * mapper in this constraint.
   */
  static int SetMultipleConstrainedFontSize(vtkViewport*, int targetWidth, int targetHeight,
    vtkTextMapper** mappers, int nbOfMappers, int* maxResultingSize);

  ///@{
  /**
   * Use these methods when setting font size relative to the renderer's size. These
   * methods are static so that external classes (e.g., widgets) can easily use them.
   */
  static int SetRelativeFontSize(
    vtkTextMapper*, vtkViewport*, const int* winSize, int* stringSize, float sizeFactor = 0.0);
  static int SetMultipleRelativeFontSize(vtkViewport* viewport, vtkTextMapper** textMappers,
    int nbOfMappers, int* winSize, int* stringSize, float sizeFactor);
  ///@}

  void RenderOverlay(vtkViewport*, vtkActor2D*) override;
  void ReleaseGraphicsResources(vtkWindow*) override;
  vtkMTimeType GetMTime() override;

protected:
  vtkTextMapper();
  ~vtkTextMapper() override;

  char* Input;
  vtkTextProperty* TextProperty;

private:
  vtkTextMapper(const vtkTextMapper&) = delete;
  void operator=(const vtkTextMapper&) = delete;

  void UpdateQuad(vtkActor2D* actor, int dpi);
  void UpdateImage(int dpi);

  int TextDims[2];

  int RenderedDPI;
  vtkTimeStamp CoordsTime;
  vtkTimeStamp TCoordsTime;
  vtkNew<vtkImageData> Image;
  vtkNew<vtkPoints> Points;
  vtkNew<vtkPolyData> PolyData;
  vtkNew<vtkPolyDataMapper2D> Mapper;
  vtkNew<vtkTexture> Texture;
};

VTK_ABI_NAMESPACE_END
#endif
