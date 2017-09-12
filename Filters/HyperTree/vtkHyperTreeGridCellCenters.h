/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridCellCenters.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkHyperTreeGridCellCenters
 * @brief   generate points at center of hyper
 * tree grid leaf cell centers.
 *
 *
 * vtkHyperTreeGridCellCenters is a filter that takes as input an hyper
 * tree grid and generates on output points at the center of the leaf
 * cells in the hyper tree grid.
 * These points can be used for placing glyphs (vtkGlyph3D) or labeling
 * (vtkLabeledDataMapper).
 * The cell attributes will be associated with the points on output.
 *
 * @warning
 * You can choose to generate just points or points and vertex cells.
 * Vertex cells are drawn during rendering; points are not. Use the ivar
 * VertexCells to generate cells.
 *
 * @sa
 * vtkCellCenters vtkHyperTreeGrid vtkGlyph3D
 *
 * @par Thanks:
 * This class was written by Guenole Harel and Jacques-Bernard Lekien 2014
 * This class was modified by Philippe Pebay, 2016
 * This work was supported by Commissariat a l'Energie Atomique (CEA/DIF)
*/

#ifndef vtkHyperTreeGridCellCenters_h
#define vtkHyperTreeGridCellCenters_h

#include "vtkFiltersHyperTreeModule.h" // For export macro
#include "vtkCellCenters.h"

class vtkBitArray;
class vtkDataSetAttributes;
class vtkHyperTreeGrid;
class vtkHyperTreeGridCursor;
class vtkPolyData;

class VTKFILTERSHYPERTREE_EXPORT vtkHyperTreeGridCellCenters : public vtkCellCenters
{
public:
  static vtkHyperTreeGridCellCenters* New();
  vtkTypeMacro( vtkHyperTreeGridCellCenters, vtkCellCenters );
  void PrintSelf( ostream&, vtkIndent ) VTK_OVERRIDE;

protected:
  vtkHyperTreeGridCellCenters();
  ~vtkHyperTreeGridCellCenters();

  int RequestData( vtkInformation*, vtkInformationVector**, vtkInformationVector* ) VTK_OVERRIDE;
  int FillInputPortInformation( int, vtkInformation* ) VTK_OVERRIDE;

  /**
   * Main routine to process individual trees in the grid
   */
  virtual void ProcessTrees();

  /**
   * Recursively descend into tree down to leaves
   */
  void RecursivelyProcessTree( vtkHyperTreeGridCursor*, vtkBitArray* );

  vtkHyperTreeGrid* Input;
  vtkPolyData* Output;

  vtkDataSetAttributes* InData;
  vtkDataSetAttributes* OutData;

  vtkPoints* Points;

  vtkPointData* InPointData;
  vtkPointData* OutPointData;

private:
  vtkHyperTreeGridCellCenters(const vtkHyperTreeGridCellCenters&) VTK_DELETE_FUNCTION;
  void operator=(const vtkHyperTreeGridCellCenters&) VTK_DELETE_FUNCTION;
};


#endif /* vtkHyperTreeGridCellCenters_h */
