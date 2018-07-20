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
 * vtkHyperTreeGridCellCenters is a filter that takes as input a hyper
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
 * vtkCellCenters vtkHyperTreeGrid vtkHyperTreeGridAlgorithm
 *
 * @par Thanks:
 * This class was written by Guenole Harel and Jacques-Bernard Lekien 2014
 * This class was rewritten by Philippe Pebay, NexGen Analytics 2017
 * This work was supported by Commissariat a l'Energie Atomique (CEA/DIF)
*/

#ifndef vtkHyperTreeGridCellCenters_h
#define vtkHyperTreeGridCellCenters_h

#include "vtkFiltersHyperTreeModule.h" // For export macro
#include "vtkHyperTreeGridAlgorithm.h"

class vtkBitArray;
class vtkHyperTreeGrid;
class vtkHyperTreeGridCursor;
class vtkPoints;

class VTKFILTERSHYPERTREE_EXPORT vtkHyperTreeGridCellCenters : public vtkHyperTreeGridAlgorithm
{
public:
  static vtkHyperTreeGridCellCenters* New();
  vtkTypeMacro( vtkHyperTreeGridCellCenters, vtkHyperTreeGridAlgorithm );
  void PrintSelf( ostream&, vtkIndent ) override;

  //@{
  /**
   * Enable/disable the generation of vertex cells. The default
   * is Off.
   */
  vtkSetMacro(VertexCells,int);
  vtkGetMacro(VertexCells,int);
  vtkBooleanMacro(VertexCells,int);
  //@}

protected:
  vtkHyperTreeGridCellCenters();
  ~vtkHyperTreeGridCellCenters() override;

  /**
   * For this algorithm the output is a vtkPolyData instance
   */
  int FillOutputPortInformation( int, vtkInformation* ) override;

  /**
   * Main routine to generate cell centers
   */
  int ProcessTrees( vtkHyperTreeGrid*, vtkDataObject* ) override;

  /**
   * Recursively descend into tree down to leaves
   */
  void RecursivelyProcessTree( vtkHyperTreeGridCursor*, vtkBitArray* );

  /**
   * Storage for points of output unstructured mesh
   */
  vtkPoints* Points;

  /**
   * Keep track as to whether vertex cells shall be generated.
   */
  int VertexCells;

private:
  vtkHyperTreeGridCellCenters(const vtkHyperTreeGridCellCenters&) = delete;
  void operator=(const vtkHyperTreeGridCellCenters&) = delete;
};


#endif /* vtkHyperTreeGridCellCenters_h */
