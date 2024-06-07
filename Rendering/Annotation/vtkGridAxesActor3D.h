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
#include "vtkRenderingAnnotationModule.h" //needed for exports

#include "vtkGridAxesHelper.h" //  needed for vtkGridAxesHelper.
#include "vtkNew.h"            // needed for vtkNew.

#include <functional> // for std::function

class vtkDoubleArray;
class vtkGridAxesActor2D;
class vtkProperty;
class vtkTextProperty;

class VTKRENDERINGANNOTATION_EXPORT vtkGridAxesActor3D : public vtkProp3D
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

  /**
   * Values for FaceMask.
   * Developer note: these are deliberately in the same order as
   * vtkGridAxesHelper::Faces which is same order as faces in vtkVoxel.
   */
  enum FaceMasks
  {
    MIN_YZ = 0x01,
    MIN_ZX = 0x02,
    MIN_XY = 0x04,
    MAX_YZ = 0x08,
    MAX_ZX = 0x010,
    MAX_XY = 0x020
  };

  ///@{
  /**
   * Set the mask to select faces. The faces rendered can be a subset of the
   * faces selected using the FaceMask based on the BackfaceCulling and
   * FrontfaceCulling flags set on the Property.
   */
  virtual void SetFaceMask(unsigned int mask);
  vtkGetMacro(FaceMask, unsigned int);
  ///@}

  enum LabelMasks
  {
    MIN_X = vtkGridAxesHelper::MIN_X,
    MIN_Y = vtkGridAxesHelper::MIN_Y,
    MIN_Z = vtkGridAxesHelper::MIN_Z,
    MAX_X = vtkGridAxesHelper::MAX_X,
    MAX_Y = vtkGridAxesHelper::MAX_Y,
    MAX_Z = vtkGridAxesHelper::MAX_Z
  };

  ///@{
  /**
   * Set the axis to label.
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
   */
  vtkSetMacro(LabelUniqueEdgesOnly, bool);
  vtkGetMacro(LabelUniqueEdgesOnly, bool);
  ///@}

  ///@{
  /**
   * Turn off to not generate polydata for the plane's grid.
   */
  void SetGenerateGrid(bool val);
  bool GetGenerateGrid();
  vtkBooleanMacro(GenerateGrid, bool);
  ///@}

  ///@{
  /**
   * Turn off to not generate the polydata for the plane's edges. Which edges
   * are rendered is defined by the EdgeMask.
   */
  void SetGenerateEdges(bool val);
  bool GetGenerateEdges();
  vtkBooleanMacro(GenerateEdges, bool);
  ///@}

  ///@{
  /**
   * Turn off to not generate the markers for the tick positions. Which edges
   * are rendered is defined by the TickMask.
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
   */
  void SetTitle(int axis, const std::string& title);
  void SetXTitle(const std::string& title) { this->SetTitle(0, title); }
  void SetYTitle(const std::string& title) { this->SetTitle(1, title); }
  void SetZTitle(const std::string& title) { this->SetTitle(2, title); }
  const std::string& GetTitle(int axis);
  ///@}

  /**
   * Set whether the specified axis should use custom labels instead of
   * automatically determined ones.
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
   */
  vtkSetMacro(ForceOpaque, bool);
  vtkGetMacro(ForceOpaque, bool);
  vtkBooleanMacro(ForceOpaque, bool);
  ///@}

  int RenderOpaqueGeometry(vtkViewport*) override;
  int RenderTranslucentPolygonalGeometry(vtkViewport* viewport) override;
  int RenderOverlay(vtkViewport* viewport) override;
  int HasTranslucentPolygonalGeometry() override;
  void ReleaseGraphicsResources(vtkWindow*) override;

protected:
  vtkGridAxesActor3D();
  ~vtkGridAxesActor3D() override;

  virtual void Update(vtkViewport* viewport);

  double GridBounds[6];
  unsigned int FaceMask;
  unsigned int LabelMask;
  bool LabelUniqueEdgesOnly;
  vtkTuple<bool, 3> UseCustomLabels;
  vtkTuple<vtkNew<vtkDoubleArray>, 3> CustomLabels;
  vtkMTimeType CustomLabelsMTime;

  vtkTuple<vtkNew<vtkGridAxesActor2D>, 6> GridAxes2DActors;

  bool ForceOpaque;

private:
  vtkGridAxesActor3D(const vtkGridAxesActor3D&) = delete;
  void operator=(const vtkGridAxesActor3D&) = delete;

  vtkMTimeType GetBoundsMTime;
};

#endif
