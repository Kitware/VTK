// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSelectVisiblePoints
 * @brief   extract points that are visible (based on z-buffer calculation)
 *
 * vtkSelectVisiblePoints is a filter that selects points based on
 * whether they are visible or not. Visibility is determined by
 * accessing the z-buffer of a rendering window. (The position of each
 * input point is converted into display coordinates, and then the
 * z-value at that point is obtained. If within the user-specified
 * tolerance, the point is considered visible.)
 *
 * Points that are visible (or if the ivar SelectInvisible is on,
 * invisible points) are passed to the output. Associated data
 * attributes are passed to the output as well.
 *
 * This filter also allows you to specify a rectangular window in display
 * (pixel) coordinates in which the visible points must lie. This can be
 * used as a sort of local "brushing" operation to select just data within
 * a window.
 *
 *
 * @warning
 * You must carefully synchronize the execution of this filter. The
 * filter refers to a renderer, which is modified every time a render
 * occurs. Therefore, the filter is always out of date, and always
 * executes. You may have to perform two rendering passes, or if you
 * are using this filter in conjunction with vtkLabeledDataMapper,
 * things work out because 2D rendering occurs after the 3D rendering.
 */

#ifndef vtkSelectVisiblePoints_h
#define vtkSelectVisiblePoints_h

#include "vtkPolyDataAlgorithm.h"
#include "vtkRenderingCoreModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkRenderer;
class vtkMatrix4x4;

class VTKRENDERINGCORE_EXPORT vtkSelectVisiblePoints : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkSelectVisiblePoints, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Instantiate object with no renderer; window selection turned off;
   * tolerance set to 0.01; and select invisible off.
   */
  static vtkSelectVisiblePoints* New();

  ///@{
  /**
   * Specify the renderer in which the visibility computation is to be
   * performed.
   */
  void SetRenderer(vtkRenderer* ren)
  {
    if (this->Renderer != ren)
    {
      this->Renderer = ren;
      this->Modified();
    }
  }
  vtkRenderer* GetRenderer() { return this->Renderer; }
  ///@}

  ///@{
  /**
   * Set/Get the flag which enables selection in a rectangular display
   * region.
   */
  vtkSetMacro(SelectionWindow, vtkTypeBool);
  vtkGetMacro(SelectionWindow, vtkTypeBool);
  vtkBooleanMacro(SelectionWindow, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Specify the selection window in display coordinates. You must specify
   * a rectangular region using (xmin,xmax,ymin,ymax).
   */
  vtkSetVector4Macro(Selection, int);
  vtkGetVectorMacro(Selection, int, 4);
  ///@}

  ///@{
  /**
   * Set/Get the flag which enables inverse selection; i.e., invisible points
   * are selected.
   */
  vtkSetMacro(SelectInvisible, vtkTypeBool);
  vtkGetMacro(SelectInvisible, vtkTypeBool);
  vtkBooleanMacro(SelectInvisible, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/Get a tolerance in normalized display coordinate system
   * to use to determine whether a point is visible. A
   * tolerance is usually required because the conversion from world space
   * to display space during rendering introduces numerical round-off.
   */
  vtkSetClampMacro(Tolerance, double, 0.0, VTK_DOUBLE_MAX);
  vtkGetMacro(Tolerance, double);
  ///@}

  ///@{
  /**
   * Set/Get a tolerance in world coordinate system
   * to use to determine whether a point is visible.
   * This allows determining visibility of small spheroid objects
   * (such as glyphs) with known size in world coordinates.
   * By default it is set to 0.
   */
  vtkSetClampMacro(ToleranceWorld, double, 0.0, VTK_DOUBLE_MAX);
  vtkGetMacro(ToleranceWorld, double);
  ///@}

  /**
   * Requires the renderer to be set. Populates the composite perspective transform
   * and returns a pointer to the Z-buffer (that must be deleted) if getZbuff is set.
   */
  float* Initialize(bool getZbuff);

  /**
   * Tests if a point x is being occluded or not against the Z-Buffer array passed in by
   * zPtr. Call Initialize before calling this method.
   */
  bool IsPointOccluded(const double x[3], const float* zPtr);

  /**
   * Return MTime also considering the renderer.
   */
  vtkMTimeType GetMTime() override;

protected:
  vtkSelectVisiblePoints();
  ~vtkSelectVisiblePoints() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  vtkRenderer* Renderer;
  vtkMatrix4x4* CompositePerspectiveTransform;

  vtkTypeBool SelectionWindow;
  int Selection[4];
  int InternalSelection[4];
  vtkTypeBool SelectInvisible;
  double DirectionOfProjection[3];
  double Tolerance;
  double ToleranceWorld;

private:
  vtkSelectVisiblePoints(const vtkSelectVisiblePoints&) = delete;
  void operator=(const vtkSelectVisiblePoints&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
