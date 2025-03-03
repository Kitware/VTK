// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAdaptiveDataSetSurfaceFilter
 * @brief   Adaptively extract dataset surface
 *
 * vtkAdaptiveDataSetSurfaceFilter uses view and dataset properties to
 * create the outside surface mesh with the minimum minimorum of facets
 * @warning
 * Only implemented currently for 2-dimensional vtkHyperTreeGrid objects
 * @sa
 * vtkHyperTreeGrid vtkDataSetSurfaceFilter
 * @par Thanks:
 * This class was written by Guenole Harel and Jacques-Bernard Lekien, 2014
 * This class was rewritten by Philippe Pebay, 2016
 * This class was modified by Rogeli Grima, 2016
 * This work was supported by Commissariat a l'Energie Atomique (CEA/DIF)
 * CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef vtkAdaptiveDataSetSurfaceFilter_h
#define vtkAdaptiveDataSetSurfaceFilter_h

#include "vtkFiltersHybridModule.h" // For export macro
#include "vtkGeometryFilter.h"

#include <set>

VTK_ABI_NAMESPACE_BEGIN
class vtkBitArray;
class vtkCamera;
class vtkHyperTreeGrid;
class vtkMatrix4x4;
class vtkRenderer;

class vtkHyperTreeGridNonOrientedGeometryCursor;
class vtkHyperTreeGridNonOrientedVonNeumannSuperCursorLight;

class VTKFILTERSHYBRID_EXPORT vtkAdaptiveDataSetSurfaceFilter : public vtkGeometryFilter
{
public:
  static vtkAdaptiveDataSetSurfaceFilter* New();
  vtkTypeMacro(vtkAdaptiveDataSetSurfaceFilter, vtkGeometryFilter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get the renderer attached to this adaptive surface extractor
   */
  void SetRenderer(vtkRenderer* ren);
  vtkGetObjectMacro(Renderer, vtkRenderer);
  ///@}

  /**
   * Get the mtime of this object.
   */
  vtkMTimeType GetMTime() override;

  ///@{
  /**
   * Set/Get for active the circle selection viewport (default true)
   */
  VTK_DEPRECATED_IN_9_5_0(
    "CircleSelection is deprecated. A single new selection method has been implemented.")
  vtkSetMacro(CircleSelection, bool);
  VTK_DEPRECATED_IN_9_5_0(
    "CircleSelection is deprecated. A single new selection method has been implemented.")
  vtkGetMacro(CircleSelection, bool);
  ///@}

  ///@{
  /**
   * Set/Get activate the bounding box selection viewport
   * This is a possible acceleration factor only if the view cannot rotate.
   *
   * Default is false.
   */
  VTK_DEPRECATED_IN_9_5_0(
    "BBSelection is deprecated. A single new selection method has been implemented.")
  vtkSetMacro(BBSelection, bool);
  VTK_DEPRECATED_IN_9_5_0(
    "BBSelection is deprecated. A single new selection method has been implemented.")
  vtkGetMacro(BBSelection, bool);
  ///@}

  ///@{
  /**
   * Set/Get the dependence to the point of view.
   *
   * Default is true.
   */
  vtkSetMacro(ViewPointDepend, bool);
  vtkGetMacro(ViewPointDepend, bool);
  ///@}

  ///@{
  /**
   * Set/Get for forced a fixed the level max (lost dynamicity) (default -1)
   */
  vtkSetMacro(FixedLevelMax, int);
  vtkGetMacro(FixedLevelMax, int);
  ///@}

  ///@{
  /**
   * Set/Get the scale factor that influences the adaptive view computation.
   * For a refinement of 2, giving Scale=2*X amounts to calling DynamicDecimateLevelMax with the
   * value X.
   *
   * Default is 1.0.
   */
  VTK_DEPRECATED_IN_9_5_0("Scale is deprecated. Please use ForcedLevelMax if needed.")
  vtkSetMacro(Scale, double);
  VTK_DEPRECATED_IN_9_5_0("Scale is deprecated. Please use ForcedLevelMax if needed.")
  vtkGetMacro(Scale, double);
  ///@}

  ///@{
  /**
   * Set/Get the dynamic decimate level max.
   * This value is subtracted to the max depth level (LevelMax).
   *
   * Default is 0.
   */
  VTK_DEPRECATED_IN_9_5_0(
    "DynamicDecimateLevelMax is deprecated. Please use ForcedLevelMax if needed.")
  vtkSetMacro(DynamicDecimateLevelMax, int);
  VTK_DEPRECATED_IN_9_5_0(
    "DynamicDecimateLevelMax is deprecated. Please use ForcedLevelMax if needed.")
  vtkGetMacro(DynamicDecimateLevelMax, int);
  ///@}

protected:
  vtkAdaptiveDataSetSurfaceFilter();
  ~vtkAdaptiveDataSetSurfaceFilter() override;

  int RequestData(vtkInformation* vtkNotUsed(request), vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int DataObjectExecute(vtkDataObject* input, vtkPolyData* output);
  int FillInputPortInformation(int port, vtkInformation* info) override;

  /**
   * Main routine to generate external boundary
   */
  void ProcessTrees(vtkHyperTreeGrid* input, vtkPolyData* output);

  /**
   * Recursively descend into tree down to leaves
   */
  void RecursivelyProcessTree1DAnd2D(vtkHyperTreeGridNonOrientedGeometryCursor*, int);
  void RecursivelyProcessTree3D(vtkHyperTreeGridNonOrientedVonNeumannSuperCursorLight*, int);

  /**
   * Process 1D leaves and issue corresponding edges (lines)
   */
  void ProcessLeaf1D(vtkHyperTreeGridNonOrientedGeometryCursor*);

  /**
   * Process 2D leaves and issue corresponding faces (quads)
   */
  void ProcessLeaf2D(vtkHyperTreeGridNonOrientedGeometryCursor*);

  /**
   * Process 3D leaves and issue corresponding cells (voxels)
   */
  void ProcessLeaf3D(vtkHyperTreeGridNonOrientedVonNeumannSuperCursorLight*);

  /**
   * Helper method to generate a face based on its normal and offset from cursor origin
   */
  void AddFace(vtkIdType, const double*, const double*, int, unsigned int);

  vtkDataSetAttributes* InData;
  vtkDataSetAttributes* OutData;

  /**
   * Dimension of input grid
   */
  unsigned int Dimension;

  /**
   * Orientation of input grid when dimension < 3
   */
  unsigned int Orientation;

  /**
   * Visibility Mask
   */
  vtkBitArray* Mask;

  /**
   * Storage for points of output unstructured mesh
   */
  vtkPoints* Points;

  /**
   * Storage for cells of output unstructured mesh
   */
  vtkCellArray* Cells;

  /**
   * Pointer to the renderer in use
   */
  vtkRenderer* Renderer;

  /**
   * First axis parameter for adaptive view
   */
  unsigned int Axis1;

  /**
   * Second axis parameter for adaptive view
   */
  unsigned int Axis2;

  /**
   * Last renderer size parameters for adaptive view
   */
  int LastRendererSize[2];

  /**
   * Depend on point of view
   */
  bool ViewPointDepend;

  /**
   * DEPRECATED
   * Product cell when in circle selection
   */
  bool CircleSelection;

  /**
   * DEPRECATED
   * Product cell when in bounding box selection
   */
  bool BBSelection;

  /**
   * Forced, fixed the level depth, ignored automatic determination
   */
  int FixedLevelMax;

  /**
   * Scale factor for adaptive view
   */
  double Scale;

  /**
   * Decimate level max after automatic determination
   */
  int DynamicDecimateLevelMax;

private:
  int MaxLevel = VTK_INT_MAX;
  bool IsParallel = false;

  enum class ShapeState : uint8_t;

  vtkAdaptiveDataSetSurfaceFilter(const vtkAdaptiveDataSetSurfaceFilter&) = delete;
  void operator=(const vtkAdaptiveDataSetSurfaceFilter&) = delete;

  /**
   * Check whether a shape is visible on the screen.
   * @param points Points of the shape
   * @param nbPoints Number of points to be read in `points` (not all 8 are necessarily defined).
   * @param level The current depth level of the cell
   * @return Whether the shape is visible on the screen (fully or partially).
   */
  ShapeState IsShapeVisible(
    const std::array<std::array<double, 3>, 8>& points, int nbPoints, int level);

  vtkSmartPointer<vtkMatrix4x4> ModelViewMatrix;
  vtkSmartPointer<vtkMatrix4x4> ProjectionMatrix;
};

VTK_ABI_NAMESPACE_END
#endif // vtkAdaptiveDataSetSurfaceFilter_h
