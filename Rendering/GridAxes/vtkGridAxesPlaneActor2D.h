// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkGridAxesPlaneActor2D
 * @brief   renders a 2D grid for vtkGridAxesActor2D.
 *
 * vtkGridAxesPlaneActor2D is designed for use by vtkGridAxesActor2D to render
 * the wireframe for the grid plane. It can also be used directly to render such
 * a wireframe in a renderer.
 */

#ifndef vtkGridAxesPlaneActor2D_h
#define vtkGridAxesPlaneActor2D_h

#include "vtkProp3D.h"
#include "vtkRenderingGridAxesModule.h" //needed for exports

#include "vtkGridAxesHelper.h" // For face enumeration
#include "vtkNew.h"            // For member variables
#include "vtkSmartPointer.h"   // For member variables
#include <deque>               // For keeping track of tick marks

VTK_ABI_NAMESPACE_BEGIN
class vtkActor;
class vtkCellArray;
class vtkDoubleArray;
class vtkPoints;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkProperty;

class VTKRENDERINGGRIDAXES_EXPORT vtkGridAxesPlaneActor2D : public vtkProp3D
{
public:
  static vtkGridAxesPlaneActor2D* New();
  vtkTypeMacro(vtkGridAxesPlaneActor2D, vtkProp3D);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set the bounding box defining the grid space. This, together with the
   * \c Face identify which planar surface this class is interested in. This
   * class is designed to work with a single planar surface.
   * Note: this is only needed/used when the vtkGridAxesHelper is not provided
   * when calling New(), otherwise the vtkGridAxesHelper is assumed to be
   * initialized externally.
   */
  vtkSetVector6Macro(GridBounds, double);
  vtkGetVector6Macro(GridBounds, double);
  ///@}

  ///@{
  /**
   * Indicate which face of the specified bounds is this class operating with.
   * Note: this is only needed/used when the vtkGridAxesHelper is not provided
   * when calling New(), otherwise the vtkGridAxesHelper is assumed to be
   * initialized externally.
   *
   * By default, Face is vtkGridAxesHelper::MIN_YZ.
   */
  vtkSetClampMacro(Face, int, vtkGridAxesHelper::MIN_YZ, vtkGridAxesHelper::MAX_XY);
  vtkGetMacro(Face, int);
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
   * Get/Set whether to generate lines for the plane's grid. Default is true.
   */
  vtkSetMacro(GenerateGrid, bool);
  vtkGetMacro(GenerateGrid, bool);
  vtkBooleanMacro(GenerateGrid, bool);
  ///@}

  ///@{
  /**
   * Get/Set whether to generate the polydata for the plane's edges. Default is
   * true.
   */
  vtkSetMacro(GenerateEdges, bool);
  vtkGetMacro(GenerateEdges, bool);
  vtkBooleanMacro(GenerateEdges, bool);
  ///@}

  ///@{
  /**
   * Get/Set whether to generate tick markers for the tick positions. Default is
   * true.
   */
  vtkSetMacro(GenerateTicks, bool);
  vtkGetMacro(GenerateTicks, bool);
  vtkBooleanMacro(GenerateTicks, bool);
  ///@}

  enum : unsigned char
  {
    TICK_DIRECTION_INWARDS = 0x1,
    TICK_DIRECTION_OUTWARDS = 0x2,
    TICK_DIRECTION_BOTH = TICK_DIRECTION_INWARDS | TICK_DIRECTION_OUTWARDS,
  };

  ///@{
  /**
   * Get/Set the tick direction.
   *
   * By defaule, it is TICK_DIRECTION_BOTH.
   */
  vtkSetClampMacro(TickDirection, unsigned int, static_cast<unsigned int>(TICK_DIRECTION_INWARDS),
    static_cast<unsigned int>(TICK_DIRECTION_BOTH));
  vtkGetMacro(TickDirection, unsigned int);
  ///@}

  ///@{
  /**
   * Set the tick positions for each of the coordinate axis. Which tick
   * positions get used depended on the face being rendered e.g. if Face is
   * MIN_XY, then the tick positions for Z-axis i.e. axis=2 will not be used
   * and hence need not be specified. Pass nullptr for data will clear the ticks
   * positions for that axis.
   * Note: This creates a deep-copy of the values in \c data and stores that.
   *
   * Default is an empty vector.
   */
  void SetTickPositions(int axis, vtkDoubleArray* data);
  const std::deque<double>& GetTickPositions(int axis)
  {
    return (axis >= 0 && axis < 3) ? this->TickPositions[axis] : this->EmptyVector;
  }
  ///@}

  ///@{
  /**
   * Get/Set the property used to control the appearance of the rendered grid.
   *
   * Internally, this property is set on the actor representing the grid.
   */
  void SetProperty(vtkProperty*);
  vtkProperty* GetProperty();
  ///@}

  //--------------------------------------------------------------------------
  // Methods for vtkProp3D API.
  //--------------------------------------------------------------------------

  ///@{
  /**
   * Returns the prop bounds.
   */
  double* GetBounds() override
  {
    this->GetGridBounds(this->Bounds);
    return this->Bounds;
  }
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

protected:
  vtkGridAxesPlaneActor2D(vtkGridAxesHelper* helper = nullptr);
  ~vtkGridAxesPlaneActor2D() override;

  ///@{
  /**
   * vtkGridAxesActor2D uses this method to create vtkGridAxesPlaneActor2D
   * instance. In that case, vtkGridAxesPlaneActor2D assumes that the
   * vtkGridAxesHelper will be updated and initialized externally. That avoids
   * unnecessary duplicate computations per render.
   */
  static vtkGridAxesPlaneActor2D* New(vtkGridAxesHelper* helper);
  friend class vtkGridAxesActor2D;
  ///@}

  ///@{
  /**
   * Update's the polydata.
   */
  void Update(vtkViewport* viewport);
  bool UpdateEdges(vtkViewport* viewport);
  bool UpdateGrid(vtkViewport* viewport);
  bool UpdateTicks(vtkViewport* viewport);
  ///@}

private:
  vtkGridAxesPlaneActor2D(const vtkGridAxesPlaneActor2D&) = delete;
  void operator=(const vtkGridAxesPlaneActor2D&) = delete;
  std::deque<double> EmptyVector;

  typedef std::pair<vtkVector3d, vtkVector3d> LineSegmentType;
  std::deque<LineSegmentType> LineSegments;

  double GridBounds[6];
  int Face = vtkGridAxesHelper::MIN_YZ;

  bool GenerateGrid = true;
  bool GenerateEdges = true;
  bool GenerateTicks = true;
  unsigned int TickDirection = TICK_DIRECTION_BOTH;
  std::deque<double> TickPositions[3];

  vtkNew<vtkPolyData> PolyData;
  vtkNew<vtkPoints> PolyDataPoints;
  vtkNew<vtkCellArray> PolyDataLines;
  vtkNew<vtkPolyDataMapper> Mapper;
  vtkNew<vtkActor> Actor;

  vtkSmartPointer<vtkGridAxesHelper> Helper;
  bool HelperManagedExternally;
};

VTK_ABI_NAMESPACE_END
#endif
