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

VTK_ABI_NAMESPACE_BEGIN
class vtkBitArray;
class vtkCamera;
class vtkHyperTreeGrid;
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
  vtkSetMacro(CircleSelection, bool);
  vtkGetMacro(CircleSelection, bool);
  ///@}

  ///@{
  /**
   * Set/Get activate the bounding box selection viewport
   * This is a possible acceleration factor only if the view cannot rotate.
   *
   * Default is false.
   */
  vtkSetMacro(BBSelection, bool);
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
  vtkSetMacro(Scale, double);
  vtkGetMacro(Scale, double);
  ///@}

  ///@{
  /**
   * Set/Get the dynamic decimate level max.
   * This value is subtracted to the max depth level (LevelMax).
   *
   * Default is 0.
   */
  vtkSetMacro(DynamicDecimateLevelMax, int);
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
   * Maximum depth parameter for adaptive view
   */
  int LevelMax;

  /**
   * Parallel projection parameter for adaptive view
   */
  bool ParallelProjection;

  /**
   * Last renderer size parameters for adaptive view
   */
  int LastRendererSize[2];

  /**
   * Depend on point of view
   */
  bool ViewPointDepend;

  /**
   * Last camera focal point coordinates for adaptive view
   */
  double LastCameraFocalPoint[3];

  /**
   * Last camera parallel scale for adaptive view
   */
  double LastCameraParallelScale;

  /**
   * Bounds windows in the real coordinates
   */
  double WindowBounds[4];

  /**
   * Product cell when in circle selection
   */
  bool CircleSelection;

  /**
   * Radius parameter for adaptive view
   */
  double Radius;

  /**
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
  vtkAdaptiveDataSetSurfaceFilter(const vtkAdaptiveDataSetSurfaceFilter&) = delete;
  void operator=(const vtkAdaptiveDataSetSurfaceFilter&) = delete;

  /**
   * Compute the max cell level.
   * If the input in 2 dimensions, dynamically compute the value.
   * Otherwise, select all levels (max level is 65536)
   * @param input hyper tree grid input
   * @return maximum cell level
   */
  int ComputeMaxLevel(vtkHyperTreeGrid* input);
};

VTK_ABI_NAMESPACE_END
#endif // vtkAdaptiveDataSetSurfaceFilter_h
