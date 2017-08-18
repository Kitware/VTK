/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridAxisCut.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkHyperTreeGridAxisCut
 * @brief   Axis aligned hyper tree grid cut
 *
 *
 * Cut along an axis aligned plane. Only works for 3D grids.
 * Produces disjoint (no point sharing) quads for now.
 * NB: If cut plane contains inter-cell boundaries, the output will contain
 * superimposed faces as a result.
 *
 * @sa
 * vtkHyperTreeGrid
 *
 * @par Thanks:
 * This class was written by Philippe Pebay and Charles Law, Kitware 2012
 * This work was supported in part by Commissariat a l'Energie Atomique (CEA/DIF)
*/

#ifndef vtkHyperTreeGridAxisCut_h
#define vtkHyperTreeGridAxisCut_h

#include "vtkFiltersHyperTreeModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkCellArray;
class vtkDataSetAttributes;
class vtkHyperTreeGrid;
class vtkPoints;

class VTKFILTERSHYPERTREE_EXPORT vtkHyperTreeGridAxisCut : public vtkPolyDataAlgorithm
{
public:
  static vtkHyperTreeGridAxisCut* New();
  vtkTypeMacro( vtkHyperTreeGridAxisCut, vtkPolyDataAlgorithm );
  void PrintSelf( ostream&, vtkIndent ) VTK_OVERRIDE;

  //@{
  /**
   * Normal axis: 0=X, 1=Y, 2=Z. Default is 0
   */
  vtkSetMacro(PlaneNormalAxis, int);
  vtkGetMacro(PlaneNormalAxis, int);
  //@}

  //@{
  /**
   * Position of plane: Axis constant. Default is 0.0
   */
  vtkSetMacro(PlanePosition, double);
  vtkGetMacro(PlanePosition, double);
  //@}

protected:
  vtkHyperTreeGridAxisCut();
  ~vtkHyperTreeGridAxisCut() VTK_OVERRIDE;

  int RequestData( vtkInformation*, vtkInformationVector**, vtkInformationVector* ) VTK_OVERRIDE;
  int FillInputPortInformation( int, vtkInformation* ) VTK_OVERRIDE;

  void ProcessTrees();
  void RecursiveProcessTree( void* );
  void ProcessLeaf3D( void* );
  void AddFace( vtkIdType inId, double* origin, double* size,
                double offset0, int axis0, int axis1, int axis2 );

  int PlaneNormalAxis;
  double PlanePosition;

  vtkHyperTreeGrid* Input;
  vtkPolyData* Output;

  vtkDataSetAttributes* InData;
  vtkDataSetAttributes* OutData;

  vtkPoints* Points;
  vtkCellArray* Cells;

private:
  vtkHyperTreeGridAxisCut(const vtkHyperTreeGridAxisCut&) VTK_DELETE_FUNCTION;
  void operator=(const vtkHyperTreeGridAxisCut&) VTK_DELETE_FUNCTION;
};

#endif
