// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkGridAxesActor3D
 * @brief actor for a cube-axes like prop in the 3D view.
 *
 * vtkGridAxesActor3D is an alternate implementation for something like the
 * vtkCubeAxesActor which can be used to render a 3D grid in a scene.
 * It uses vtkGridAxesActor3D to render individual axes planes for the box.
 *
 */

#ifndef vtkGridAxesActor3D_h
#define vtkGridAxesActor3D_h

#include "vtkProp3D.h"
#include "vtkRenderingGridAxesModule.h" //needed for exports

#include "vtkGridAxesHelper.h" //  needed for vtkGridAxesHelper.
#include "vtkNew.h"            // needed for vtkNew.

#include <functional> // for std::function

VTK_ABI_NAMESPACE_BEGIN
class vtkDoubleArray;
class vtkGridAxesActor2D;
class vtkProperty;
class vtkTextProperty;

class VTKRENDERINGGRIDAXES_EXPORT vtkGridAxesActor3D : public vtkProp3D
{
public:
  static vtkGridAxesActor3D* New();
  vtkTypeMacro(vtkGridAxesActor3D, vtkProp3D);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Shallow copy from another vtkGridAxesActor3D.
   */
  void ShallowCopy(vtkProp* prop) override;

  ///@{
  /**
   * Set the bounding box defining the grid space. This, together with the
   * \c Face identify which planar surface this class is interested in. This
   * class is designed to work with a single planar surface.
   */
  vtkSetVector6Macro(GridBounds, double);
  vtkGetVector6Macro(GridBounds, double);
  ///@}

  ///@{
  /**
   * Set the mask to select faces. The faces rendered can be a subset of the
   * faces selected using the FaceMask based on the BackfaceCulling and
   * FrontfaceCulling flags set on the Property.
   *
   * Defaults to 0.
   */
  virtual void SetFaceMask(unsigned int mask);
  vtkGetMacro(FaceMask, unsigned int);
  ///@}

  ///@{
  /**
   * Set the axis to label.
   *
   * Default is 0xff.
   */
  virtual void SetLabelMask(unsigned int mask);
  unsigned int GetLabelMask();
  ///@}

  /**
   * For some exporters and other other operations we must be
   * able to collect all the actors or volumes. These methods
   * are used in that process.
   * In case the viewport is not a consumer of this prop:
   * call UpdateGeometry() first for updated viewport-specific
   * billboard geometry.
   */
  void GetActors(vtkPropCollection*) override;

  /**
   * Updates the billboard geometry without performing any rendering,
   * to assist GetActors().
   */
  void UpdateGeometry(vtkViewport* vp);

  ///@{
  /**
   * Set to true to only label edges shared with 1 face. Note that
   * if all faces are being rendered, this will generate no labels.
   *
   * Default is true.
   */
  vtkSetMacro(LabelUniqueEdgesOnly, bool);
  vtkGetMacro(LabelUniqueEdgesOnly, bool);
  ///@}

  ///@{
  /**
   * Turn off to not generate polydata for the plane's grid.
   *
   * Default is true.
   */
  void SetGenerateGrid(bool val);
  bool GetGenerateGrid();
  vtkBooleanMacro(GenerateGrid, bool);
  ///@}

  ///@{
  /**
   * Turn off to not generate the polydata for the plane's edges. Which edges
   * are rendered is defined by the EdgeMask.
   *
   * Default is true.
   */
  void SetGenerateEdges(bool val);
  bool GetGenerateEdges();
  vtkBooleanMacro(GenerateEdges, bool);
  ///@}

  ///@{
  /**
   * Turn off to not generate the markers for the tick positions. Which edges
   * are rendered is defined by the TickMask.
   *
   * Default is true.
   */
  void SetGenerateTicks(bool val);
  bool GetGenerateTicks();
  vtkBooleanMacro(GenerateTicks, bool);
  ///@}

  ///@{
  /**
   * Get/Set the property used to control the appearance of the rendered grid.
   */
  void SetProperty(vtkProperty*);
  vtkProperty* GetProperty();
  ///@}

  //---------------------------------------------------------------------------
  // *** Properties to control the axis titles ***

  ///@{
  /**
   * Get/Set the vtkTextProperty for the title for each the axes.
   * Note that the alignment properties are not used.
   */
  void SetTitleTextProperty(int axis, vtkTextProperty*);
  void SetXTitleTextProperty(vtkTextProperty* prop) { this->SetTitleTextProperty(0, prop); }
  void SetYTitleTextProperty(vtkTextProperty* prop) { this->SetTitleTextProperty(1, prop); }
  void SetZTitleTextProperty(vtkTextProperty* prop) { this->SetTitleTextProperty(2, prop); }
  vtkTextProperty* GetTitleTextProperty(int axis);
  ///@}

  ///@{
  /**
   * Get/Set the text to use for titles for the axis. Setting the title to an
   * empty string will hide the title label for that axis.
   *
   * Default is empty.
   */
  void SetTitle(int axis, const std::string& title);
  void SetXTitle(const std::string& title) { this->SetTitle(0, title); }
  void SetYTitle(const std::string& title) { this->SetTitle(1, title); }
  void SetZTitle(const std::string& title) { this->SetTitle(2, title); }
  const std::string& GetTitle(int axis);
  ///@}

  ///@{
  /**
   * Set whether the specified axis should use custom labels instead of
   * automatically determined ones.
   *
   * Default is false.
   */
  void SetUseCustomLabels(int axis, bool val);
  void SetXUseCustomLabels(bool val) { this->SetUseCustomLabels(0, val); }
  void SetYUseCustomLabels(bool val) { this->SetUseCustomLabels(1, val); }
  void SetZUseCustomLabels(bool val) { this->SetUseCustomLabels(2, val); }

  void SetNumberOfLabels(int axis, vtkIdType val);
  void SetNumberOfXLabels(vtkIdType val) { this->SetNumberOfLabels(0, val); }
  void SetNumberOfYLabels(vtkIdType val) { this->SetNumberOfLabels(1, val); }
  void SetNumberOfZLabels(vtkIdType val) { this->SetNumberOfLabels(2, val); }

  void SetLabel(int axis, vtkIdType index, double value);
  void SetXLabel(vtkIdType index, double value) { this->SetLabel(0, index, value); }
  void SetYLabel(vtkIdType index, double value) { this->SetLabel(1, index, value); }
  void SetZLabel(vtkIdType index, double value) { this->SetLabel(2, index, value); }
  ///@}

  //---------------------------------------------------------------------------
  // *** Properties to control the axis data labels ***

  ///@{
  /**
   * Get/Set the vtkTextProperty that governs how the axis labels are displayed.
   * Note that the alignment properties are not used.
   */
  void SetLabelTextProperty(int axis, vtkTextProperty*);
  void SetXLabelTextProperty(vtkTextProperty* prop) { this->SetLabelTextProperty(0, prop); }
  void SetYLabelTextProperty(vtkTextProperty* prop) { this->SetLabelTextProperty(1, prop); }
  void SetZLabelTextProperty(vtkTextProperty* prop) { this->SetLabelTextProperty(2, prop); }
  vtkTextProperty* GetLabelTextProperty(int axis);
  ///@}

  ///@{
  /**
   * Get/set the numerical notation, standard, scientific or mixed (0, 1, 2).
   * Accepted values are vtkAxis::AUTO, vtkAxis::FIXED, vtkAxis::CUSTOM.
   *
   * By default, this is set to vtkAxis::AUTO.
   */
  void SetNotation(int axis, int notation);
  void SetXNotation(int notation) { this->SetNotation(0, notation); }
  void SetYNotation(int notation) { this->SetNotation(1, notation); }
  void SetZNotation(int notation) { this->SetNotation(2, notation); }
  int GetNotation(int axis);
  ///@}

  ///@{
  /**
   * Get/set the numerical precision to use, default is 2.
   */
  void SetPrecision(int axis, int val);
  void SetXPrecision(int val) { this->SetPrecision(0, val); }
  void SetYPrecision(int val) { this->SetPrecision(1, val); }
  void SetZPrecision(int val) { this->SetPrecision(2, val); }
  int GetPrecision(int axis);
  ///@}

  ///@{
  /**
   * Get/Set the function that will be applied to the tick label of each axis.
   * If nothing is set, then the default is to use whatever was generated by vtkAxis.
   *
   * The default it to use whatever was generated by vtkAxis.
   */
  void SetTickLabelFunction(int axis, std::function<double(double)> func);
  std::function<double(double)> GetTickLabelFunction(int axis);
  ///@}

  //--------------------------------------------------------------------------
  // Methods for vtkProp3D API.
  //--------------------------------------------------------------------------

  ///@{
  /**
   * Returns the prop bounds.
   */
  double* GetBounds() override;
  using Superclass::GetBounds;
  ///@}

  /**
   * Get an bounding box that is expected to contain all rendered elements,
   * since GetBounds() returns the bounding box the grid axes describes.
   */
  virtual void GetRenderedBounds(double bounds[6]);

  ///@{
  /**
   * If true, the actor will always be rendered during the opaque pass.
   *
   * Defaults to false.
   */
  vtkSetMacro(ForceOpaque, bool);
  vtkGetMacro(ForceOpaque, bool);
  vtkBooleanMacro(ForceOpaque, bool);
  ///@}

  ///@{
  /**
   * Standard render methods for different types of geometry
   */
  int RenderOpaqueGeometry(vtkViewport*) override;
  int RenderTranslucentPolygonalGeometry(vtkViewport* viewport) override;
  int RenderOverlay(vtkViewport* viewport) override;
  vtkTypeBool HasTranslucentPolygonalGeometry() override;
  ///@}

  /**
   * Release any graphics resources that are being consumed by this prop.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow*) override;

  ///@{
  /**
   * Set/Get the label display offset
   *
   * This is useful to offset axes labels if they overlap at the corners.
   *
   * \note Uses display space coordinates
   */
  virtual void SetLabelDisplayOffset(int xoffset, int yoffset);
  virtual void SetLabelDisplayOffset(const int offset[2]);
  virtual int* GetLabelDisplayOffset() VTK_SIZEHINT(2);
  VTK_WRAPEXCLUDE virtual void GetLabelDisplayOffset(int& xoffset, int& yoffset);
  VTK_WRAPEXCLUDE virtual void GetLabelDisplayOffset(int offset[2]);
  ///@}

protected:
  vtkGridAxesActor3D();
  ~vtkGridAxesActor3D() override;

  virtual void Update(vtkViewport* viewport);

private:
  vtkGridAxesActor3D(const vtkGridAxesActor3D&) = delete;
  void operator=(const vtkGridAxesActor3D&) = delete;

  vtkMTimeType GetBoundsMTime = 0;
  double GridBounds[6] = { -1.0, 1.0, -1.0, 1.0, -1.0, 1.0 };
  unsigned int FaceMask = 0;
  bool LabelUniqueEdgesOnly = true;
  vtkTuple<bool, 3> UseCustomLabels;
  vtkTuple<vtkNew<vtkDoubleArray>, 3> CustomLabels;
  vtkMTimeType CustomLabelsMTime = 0;

  vtkTuple<vtkNew<vtkGridAxesActor2D>, 6> GridAxes2DActors;

  bool ForceOpaque = false;
};

VTK_ABI_NAMESPACE_END
#endif
