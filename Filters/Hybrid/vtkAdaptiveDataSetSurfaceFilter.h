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
*/

#ifndef vtkAdaptiveDataSetSurfaceFilter_h
#define vtkAdaptiveDataSetSurfaceFilter_h

#include "vtkFiltersHybridModule.h" // For export macro
#include "vtkDataSetSurfaceFilter.h"

class vtkBitArray;
class vtkCamera;
class vtkHyperTreeGrid;
class vtkHyperTreeGridCursor;
class vtkRenderer;

class VTKFILTERSHYBRID_EXPORT vtkAdaptiveDataSetSurfaceFilter : public vtkDataSetSurfaceFilter
{
public:
  static vtkAdaptiveDataSetSurfaceFilter* New();
  vtkTypeMacro( vtkAdaptiveDataSetSurfaceFilter, vtkDataSetSurfaceFilter );
  void PrintSelf( ostream&, vtkIndent ) override;

  //@{
  /**
   * Set/Get the renderer attached to this adaptive surface extractor
   */
  void SetRenderer( vtkRenderer* ren );
  vtkGetObjectMacro(Renderer, vtkRenderer);
  //@}

  /**
   * Set the scale factor
   */
  vtkSetMacro(Scale,double);

  /**
   * Get the mtime of this object.
   */
  vtkMTimeType GetMTime() override;

protected:
  vtkAdaptiveDataSetSurfaceFilter();
  ~vtkAdaptiveDataSetSurfaceFilter() override;

  int RequestData( vtkInformation* vtkNotUsed(request),
                   vtkInformationVector** inputVector,
                   vtkInformationVector* outputVector ) override;
  int DataSetExecute( vtkDataSet* input, vtkPolyData* output ) override;

  /**
   * Main routine to generate external boundary
   */
  void ProcessTrees( vtkHyperTreeGrid* input, vtkPolyData* output );

  /**
   * Recursively descend into tree down to leaves
   */
  void RecursivelyProcessTree( vtkHyperTreeGridCursor*, vtkBitArray*, int );

  /**
   * Process 1D leaves and issue corresponding edges (lines)
   */
  void ProcessLeaf1D( vtkHyperTreeGridCursor* );

  /**
   * Process 2D leaves and issue corresponding faces (quads)
   */
  void ProcessLeaf2D( vtkHyperTreeGridCursor*, vtkBitArray* );

  /**
   * Process 3D leaves and issue corresponding cells (voxels)
   */
  void ProcessLeaf3D( vtkHyperTreeGridCursor*, vtkBitArray* );

  /**
   * Helper method to generate a face based on its normal and offset from cursor origin
   */
  void AddFace( vtkIdType, double*, double*, int, unsigned int );

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
  vtkRenderer *Renderer;

  /**
   * Radius parameter for adaptive view
   */
  double Radius;

  /**
   * First axis parameter for adaptive view
   */
  int Axis1;

  /**
   * Second axis parameter for adaptive view
   */
  int Axis2;

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
   * Last camera focal point coordinates for adaptive view
   */
  double LastCameraFocalPoint[3];

  /**
   * Last camera parallel scale for adaptive view
   */
  double LastCameraParallelScale;

  /**
   * Scale factor for adaptive view
   */
  double Scale;

private:
  vtkAdaptiveDataSetSurfaceFilter( const vtkAdaptiveDataSetSurfaceFilter& ) = delete;
  void operator = ( const vtkAdaptiveDataSetSurfaceFilter& ) = delete;
};

#endif
