// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkLegendBoxEntryInternal
 * @brief   A single entry to use inside vtkLegendBoxActor
 *
 * vtkLegendBoxEntryInternal represents an entry of the vtkLegendBoxActor: a text with optional
 * symbol (vtkPolyData) and icon (vtkImageData).
 * It is intended to be used from inside VTK RenderingAnnotation module only.
 */

#ifndef vtkLegendBoxEntryInternal_h
#define vtkLegendBoxEntryInternal_h

#include "vtkNew.h"
#include "vtkSmartPointer.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkActor2D;
class vtkImageData;
class vtkPlaneSource;
class vtkPolyData;
class vtkPolyDataMapper2D;
class vtkProperty2D;
class vtkTextMapper;
class vtkTextProperty;
class vtkTexturedActor2D;
class vtkTransform;
class vtkTransformFilter;
class vtkViewport;
class vtkWindow;

class vtkLegendBoxEntryInternal
{
public:
  vtkLegendBoxEntryInternal();
  ~vtkLegendBoxEntryInternal();

  ///@{
  /**
   * Set / Get the entry text.
   * Empty by default.
   */
  bool SetText(const char* text);
  const char* GetText();
  ///@}

  ///@{
  /**
   * Set / Get the entry symbol.
   * Default is nullptr.
   */
  bool SetSymbol(vtkPolyData*);
  vtkPolyData* GetSymbol();
  ///@}

  /**
   * Returns true if the entry has a symbol set (i.e. if GetSymbol is not nullptr)
   * Default is false.
   */
  bool HasSymbol();

  ///@{
  /**
   * Set / Get the entry icon.
   * Default is nullptr.
   */
  bool SetIcon(vtkImageData*);
  vtkImageData* GetIcon();
  ///@}

  /**
   * Returns true if the entry has an icon set (i.e. if GetIcon is not nullptr).
   * Default is false.
   */
  bool HasIcon();

  ///@{
  /**
   * Set / Get the entry color.
   * This applies to the text and the symbol.
   */
  bool SetColor(double color[3]);
  double* GetColor();
  ///@}

  ///@{
  /**
   * Forward usual rendering methods to internal actors.
   */
  void ReleaseGraphicsResources(vtkWindow* win);
  int RenderOverlay(vtkViewport*);
  int RenderOpaqueGeometry(vtkViewport*);
  ///@}

  /**
   * Set the text position.
   */
  void SetTextPosition(double X, double Y);

  /**
   * Update the icon transform (scale and position).
   * width and height being the targeted dimensions and posX/posY the targeted coordinates.
   */
  void UpdateIconTransform(int width, int height, double posX, double posY);

  /**
   * Update the symbol transform (scale and position).
   * width and height being the targeted dimensions and posX/posY the targeted coordinates.
   */
  void UpdateSymbolTransform(int width, int height, double posX, double posY);

  /**
   * Update internal actors properties.
   * Use prop as base. Override with Color. And set ScalarVisibility.
   */
  void UpdateProperties(bool visibility, vtkProperty2D* prop);

  /**
   * Copy given property into internal TextActor property.
   */
  void CopyTextProperty(vtkTextProperty* prop);

  /**
   * Get the entry symbol bounds.
   */
  double* GetSymbolBounds();

  /**
   * Get the entry symbol bounds.
   */
  double* GetIconBounds();

  ///@{
  /**
   * Forwarded to internal text mapper.
   */
  void SetFontSize(int size);
  int SetConstrainedFontSize(vtkViewport* viewport, int size[2]);
  void GetSize(vtkViewport* viewport, int size[2]);
  ///@}

private:
  /**
   * Update the given transform scale and position to match the target:
   * - scale input bounds to fit in width and height
   * - move to posX/posY coordinates.
   */
  void UpdateTransform(
    vtkTransform* tranform, double posX, double posY, double bounds[6], int width, int height);

  /**
   * Get the scale factor to apply to bounds so they fit into width / height.
   */
  double GetScale(double bounds[6], int width, int height);

  vtkNew<vtkTextMapper> TextMapper;
  vtkNew<vtkActor2D> TextActor;

  vtkSmartPointer<vtkPolyData> Symbol;
  vtkNew<vtkTransform> Transform;
  vtkNew<vtkTransformFilter> SymbolTransform;
  vtkNew<vtkPolyDataMapper2D> SymbolMapper;
  vtkNew<vtkActor2D> SymbolActor;

  vtkSmartPointer<vtkImageData> IconImage;
  vtkNew<vtkPlaneSource> Icon;
  vtkNew<vtkTransform> IconTransform;
  vtkNew<vtkTransformFilter> IconTransformFilter;
  vtkNew<vtkPolyDataMapper2D> IconMapper;
  vtkNew<vtkTexturedActor2D> IconActor;

  double Color[3] = { -1, -1, -1 };
};

VTK_ABI_NAMESPACE_END

#endif
