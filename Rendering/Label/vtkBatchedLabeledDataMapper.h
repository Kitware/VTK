// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkBatchedLabeledDataMapper
 * @brief   draw text labels at dataset points using batch rendering
 *
 * vtkBatchedLabeledDataMapper is a mapper that renders text at dataset
 * points. It inherits the full API of vtkLabeledDataMapper (coordinate
 * systems, transform, label mode, text properties) but replaces the
 * one-draw-call-per-label rendering with a glyph atlas batch approach so
 * that all labels are drawn in a single pass.
 *
 * This is an abstract base class. Use a backend-specific subclass such as
 * vtkOpenGLBatchedLabeledDataMapper to perform actual rendering.
 *
 * @sa
 * vtkLabeledDataMapper vtkOpenGLBatchedLabeledDataMapper
 * vtkWebGPUBatchedLabeledDataMapper
 */

#ifndef vtkBatchedLabeledDataMapper_h
#define vtkBatchedLabeledDataMapper_h

#include "vtkLabeledDataMapper.h"
#include "vtkRenderingLabelModule.h" // For export macro
#include "vtkWrappingHints.h"        // For VTK_MARSHALMANUAL

#include <memory> // For unique_ptr

VTK_ABI_NAMESPACE_BEGIN
class vtkDataSet;
class vtkFloatArray;
class vtkImageData;
class vtkIntArray;
class vtkPolyData;
class vtkTextProperty;
VTK_ABI_NAMESPACE_END

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGLABEL_EXPORT VTK_MARSHALMANUAL vtkBatchedLabeledDataMapper
  : public vtkLabeledDataMapper
{
public:
  static vtkBatchedLabeledDataMapper* New();
  vtkTypeMacro(vtkBatchedLabeledDataMapper, vtkLabeledDataMapper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Compile-time limits shared by the base class and both backend shaders.
   * MaxTextProperties is the hard upper bound on the number of distinct text
   * property indices (0..MaxTextProperties-1) a single mapper may use.
   * GlyphAtlasPadding is the per-glyph padding (pixels) in the atlas texture.
   * GlyphAtlasColumnSize controls when the atlas layout starts a new column.
   */
  static constexpr int MaxTextProperties = 128;
  static constexpr int GlyphAtlasPadding = 5;
  static constexpr int GlyphAtlasColumnSize = 10;
  ///@}

  ///@{
  /**
   * Set/Get the text property. An integer type argument selects among multiple
   * properties for different label types driven by an optional type input array.
   * Overridden to maintain the glyph atlas cache.
   */
  void SetLabelTextProperty(vtkTextProperty* p) override { this->SetLabelTextProperty(p, 0); }
  vtkTextProperty* GetLabelTextProperty() override { return this->GetLabelTextProperty(0); }
  void SetLabelTextProperty(vtkTextProperty* p, int type) override;
  vtkTextProperty* GetLabelTextProperty(int type) override;
  ///@}

  /**
   * Override TextProperty frame colors with a named, point-aligned color array.
   */
  vtkSetStringMacro(FrameColorsName);
  vtkGetStringMacro(FrameColorsName);

  ///@{
  /**
   * Anchor option for labels. Default is Center.
   */
  enum TextAnchorTypes
  {
    LowerLeft = 0,  ///< Uses the lower left corner.
    LowerRight = 1, ///< Uses the lower right corner.
    UpperLeft = 2,  ///< Uses the upper left corner.
    UpperRight = 3, ///< Uses the upper right corner.
    LowerEdge = 4,  ///< Uses the lower edge center.
    RightEdge = 5,  ///< Uses the right edge center.
    LeftEdge = 6,   ///< Uses the left edge center.
    UpperEdge = 7,  ///< Uses the upper edge center.
    Center = 8      ///< Uses the exact center.
  };
  ///@}

  ///@{
  /**
   * Set/Get the anchor point for label placement.
   * \sa TextAnchor
   */
  vtkSetClampMacro(TextAnchor, int, LowerLeft, Center);
  vtkGetMacro(TextAnchor, int);
  ///@}

  ///@{
  /**
   * Set/Get a pixel offset applied to every label in display space.
   * Useful to nudge labels away from their anchor point.
   * Default is [0, 0].
   */
  vtkSetVector2Macro(DisplayOffset, int);
  vtkGetVector2Macro(DisplayOffset, int);
  ///@}

  /**
   * Overridden to take into account LabelTextProperty's mtime.
   */
  vtkMTimeType GetMTime() override;

  /// No-op: backend subclasses override this to drive the render pipeline.
  void RenderOpaqueGeometry(vtkViewport*, vtkActor2D*) override {}
  /// No-op: all label drawing happens in RenderOpaqueGeometry.
  void RenderOverlay(vtkViewport*, vtkActor2D*) override {}
  /// Releases any GPU resources held by backend subclasses.
  void ReleaseGraphicsResources(vtkWindow*) override;

protected:
  vtkBatchedLabeledDataMapper();
  ~vtkBatchedLabeledDataMapper() override;

  int FillInputPortInformation(int, vtkInformation*) override;

  /// Tracks the high-water mark of labels seen so far; no per-label allocation is needed.
  void AllocateLabels(int numLabels) override;

  /// Calls the superclass pipeline, then refreshes text-property attribute arrays.
  void BuildLabels() override;

  /// Formats labels for one dataset and populates the glyph atlas and shader arrays.
  void BuildLabelsInternal(vtkDataSet*) override;

  /**
   * Converts formatted label strings into per-glyph vertex attributes (glyph extents,
   * character offsets, point IDs, property IDs, frame colors) and appends them to the
   * internal polydata. Only glyphs whose @a visible entry is true are emitted.
   */
  void MakeShaderArrays(int numCurLabels, const std::vector<std::string>&, vtkIntArray*,
    vtkFloatArray*, const std::vector<bool>& visible);

  /**
   * Called by BuildLabels when the glyph atlas image has been rebuilt or
   * updated. Backend subclasses must upload the image to the GPU.
   */
  virtual void UploadGlyphAtlas(vtkImageData* atlas);

  /**
   * Called before and after the helper renders to bind/unbind the atlas texture.
   */
  virtual void ActivateGlyphTexture();
  virtual void DeactivateGlyphTexture();

  ///@{
  /**
   * Accessors for backend helper use in SetMapperShaderParameters.
   * Returns pointers into contiguous internal arrays; valid until the next
   * BuildLabels call.
   */
  vtkImageData* GetGlyphAtlas() const;
  vtkPolyData* GetPreparedPolyData() const;
  const float* GetBackgroundColors() const;
  const int* GetFrameWidths() const;
  const int* GetMaxGlyphHeights() const;
  const int* GetDescenders() const;
  ///@}

  /**
   * Called by backend subclasses from RenderOpaqueGeometry to propagate the
   * render window DPI before BuildLabels. Triggers a rebuild if DPI changed.
   */
  void UpdateRenderWindowDPI(int dpi);

private:
  vtkBatchedLabeledDataMapper(const vtkBatchedLabeledDataMapper&) = delete;
  void operator=(const vtkBatchedLabeledDataMapper&) = delete;

  int TextAnchor = vtkBatchedLabeledDataMapper::Center;
  int DisplayOffset[2] = { 0, 0 };

  char* FrameColorsName = nullptr;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Impl;

  struct vtkBatchedLabeledDataMapperFormatter;
};
VTK_ABI_NAMESPACE_END

#endif
