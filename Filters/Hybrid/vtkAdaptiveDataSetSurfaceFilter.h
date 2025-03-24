// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAdaptiveDataSetSurfaceFilter
 * @brief   Adaptively extract dataset surface
 *
 * vtkAdaptiveDataSetSurfaceFilter uses view and dataset properties to
 * create the outside surface mesh with the minimum number of faces.
 * This reduces the memory usage at the expense of compute time.
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
   * Set/Get the dependence to the point of view.
   *
   * Default is true.
   */
  vtkSetMacro(ViewPointDepend, bool);
  vtkGetMacro(ViewPointDepend, bool);
  ///@}

  ///@{
  /**
   * Set/Get for forced a fixed the level max (lost dynamicity)
   *
   * Default is -1
   */
  vtkSetMacro(FixedLevelMax, int);
  vtkGetMacro(FixedLevelMax, int);
  ///@}

  VTK_DEPRECATED_IN_9_5_0("CircleSelection has been removed. Do not use.")
  virtual void SetCircleSelection(bool _arg);
  VTK_DEPRECATED_IN_9_5_0("CircleSelection has been removed. Do not use.")
  virtual bool GetCircleSelection();

  VTK_DEPRECATED_IN_9_5_0("BBSelection has been removed. Do not use.")
  virtual void SetBBSelection(bool _arg);
  VTK_DEPRECATED_IN_9_5_0("BBSelection has been removed. Do not use.")
  virtual bool GetBBSelection();

  VTK_DEPRECATED_IN_9_5_0("DynamicDecimateLevelMax has been removed. Do not use.")
  virtual void SetDynamicDecimateLevelMax(int _arg);
  VTK_DEPRECATED_IN_9_5_0("DynamicDecimateLevelMax has been removed. Do not use.")
  virtual int GetDynamicDecimateLevelMax();

  VTK_DEPRECATED_IN_9_5_0("Scale has been removed. Do not use.")
  virtual void SetScale(double _arg);
  VTK_DEPRECATED_IN_9_5_0("Scale has been removed. Do not use.")
  virtual int GetScale();

protected:
  vtkAdaptiveDataSetSurfaceFilter();
  ~vtkAdaptiveDataSetSurfaceFilter() override;

  int RequestData(vtkInformation* vtkNotUsed(request), vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int DataObjectExecute(vtkDataObject* input, vtkPolyData* output);
  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkAdaptiveDataSetSurfaceFilter(const vtkAdaptiveDataSetSurfaceFilter&) = delete;
  void operator=(const vtkAdaptiveDataSetSurfaceFilter&) = delete;

  enum class ShapeState : uint8_t;

  /**
   * Check whether a shape is visible on the screen.
   * @param points Points of the shape
   * @param level The current depth level of the cell
   * @return Whether the shape is visible on the screen (fully or partially).
   */
  template <int N>
  ShapeState IsShapeVisible(const std::array<std::array<double, 3>, N>& points, int level);

  /**
   * Main routine to generate external boundary
   */
  void ProcessTrees(vtkHyperTreeGrid* input, vtkPolyData* output);

  /**
   * Recursively descend into tree down to leaves
   */
  void RecursivelyProcessTree1D(vtkHyperTreeGridNonOrientedGeometryCursor*, int);
  void RecursivelyProcessTree2D(vtkHyperTreeGridNonOrientedGeometryCursor*, int);
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

  vtkDataSetAttributes* InData = nullptr;
  vtkDataSetAttributes* OutData = nullptr;

  /**
   * Dimension of input grid
   */
  unsigned int Dimension = 0;

  /**
   * Orientation of input grid when dimension < 3
   */
  unsigned int Orientation = 0;

  /**
   * Visibility Mask
   */
  vtkBitArray* Mask;

  /**
   * Storage for points of output unstructured mesh
   */
  vtkPoints* Points = nullptr;

  /**
   * Storage for cells of output unstructured mesh
   */
  vtkCellArray* Cells = nullptr;

  /**
   * Pointer to the renderer in use
   */
  vtkRenderer* Renderer = nullptr;

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
  int LastRendererSize[2] = { 0, 0 };

  /**
   * Whether to use the camera frustum to decimate cells.
   */
  bool ViewPointDepend = true;

  /**
   * Forced, fixed the level depth, ignored automatic determination
   */
  int FixedLevelMax = -1;

  /**
   * Whether ParallelProjection is enabled on the renderer's camera
   */
  bool IsParallel = false;

  /**
   * Max depth to be rendered, any deeper is smaller than one pixel.
   */
  int MaxLevel = VTK_INT_MAX;

  vtkSmartPointer<vtkMatrix4x4> ModelViewMatrix;
  vtkSmartPointer<vtkMatrix4x4> ProjectionMatrix;
};

VTK_ABI_NAMESPACE_END
#endif // vtkAdaptiveDataSetSurfaceFilter_h
