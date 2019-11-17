/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAdaptiveDataSetSurfaceFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
  void PrintSelf(ostream&, vtkIndent) override;

  //@{
  /**
   * Set/Get the renderer attached to this adaptive surface extractor
   */
  void SetRenderer(vtkRenderer* ren);
  vtkGetObjectMacro(Renderer, vtkRenderer);
  //@}

  /**
   * Get the mtime of this object.
   */
  vtkMTimeType GetMTime() override;

  //@{
  /**
   * Set/Get for active the circle selection viewport (defaut true)
   */
  vtkSetMacro(CircleSelection, bool);
  vtkGetMacro(CircleSelection, bool);
  //@}

  //@{
  /**
   * Set/Get for active the bounding box selection viewport (defaut false)
   * JB C'est un facteur supplementaire d'acceleration possible
   * JB uniquement si l'on ne peut faire de rotation dans la vue.
   */
  vtkSetMacro(BBSelection, bool);
  vtkGetMacro(BBSelection, bool);
  //@}

  //@{
  /**
   * JB Activation de la dependance au point de vue. Par defaut a True.
   */
  vtkSetMacro(ViewPointDepend, bool);
  vtkGetMacro(ViewPointDepend, bool);
  //@}

  //@{
  /**
   * Set/Get for forced a fixed the level max (lost dynamicity) (defaut -1)
   */
  vtkSetMacro(FixedLevelMax, int);
  vtkGetMacro(FixedLevelMax, int);
  //@}

  //@{
  /**
   * JB Set/Get the scale factor influence le calcul de l'adaptive view.
   * JB Pour un raffinement de 2, donner Scale=2*X revient a faire un
   * JB appel a DynamicDecimateLevelMax avec la valeur X. (defaut 1)
   */
  vtkSetMacro(Scale, double);
  vtkGetMacro(Scale, double);
  //@}

  //@{
  /**
   * JB Set/Get reduit de autant le niveau max de profondeur, calcule
   * JB dynamiquement a parcourir dans la
   * JB representation HTG. (defaut 0)
   */
  vtkSetMacro(DynamicDecimateLevelMax, int);
  vtkGetMacro(DynamicDecimateLevelMax, int);
  //@}

protected:
  vtkAdaptiveDataSetSurfaceFilter();
  ~vtkAdaptiveDataSetSurfaceFilter() override;

  int RequestData(vtkInformation* vtkNotUsed(request), vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int DataSetExecute(vtkDataObject* input, vtkPolyData* output) /*override*/;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  /**
   * Main routine to generate external boundary
   */
  void ProcessTrees(vtkHyperTreeGrid* input, vtkPolyData* output);

  /**
   * Recursively descend into tree down to leaves
   */
  void RecursivelyProcessTreeNot3D(vtkHyperTreeGridNonOrientedGeometryCursor*, int);
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
   * JB Activation de la dependance au point de vue
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
   * Product cell when in nounding box selection
   */
  bool BBSelection;

#ifndef NDEBUG
  /**
   * Effect of options selection
   */
  long int NbRejectByCircle;
  long int NbRejectByBB;
#endif

  /**
   * JB Forced, fixed the level depth, ignored automatic determination
   */
  int FixedLevelMax;

  /**
   * Scale factor for adaptive view
   */
  double Scale;

  /**
   * JB Decimate level max after automatic determination
   */
  int DynamicDecimateLevelMax;

private:
  vtkAdaptiveDataSetSurfaceFilter(const vtkAdaptiveDataSetSurfaceFilter&) = delete;
  void operator=(const vtkAdaptiveDataSetSurfaceFilter&) = delete;
};

#endif // vtkAdaptiveDataSetSurfaceFilter_h
