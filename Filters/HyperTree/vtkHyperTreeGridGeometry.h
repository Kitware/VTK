/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridGeometry.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkHyperTreeGridGeometry
 * @brief   Hyper tree grid outer surface
 *
 * @sa
 * vtkHyperTreeGrid
 *
 * @par Thanks:
 * This class was written by Philippe Pebay, Joachim Pouderoux and Charles Law,
 * Kitware 2013
 * This work was supported in part by Commissariat a l'Energie Atomique (CEA/DIF)
*/

#ifndef vtkHyperTreeGridGeometry_h
#define vtkHyperTreeGridGeometry_h

#include "vtkFiltersHyperTreeModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkCellArray;
class vtkDataSetAttributes;
class vtkHyperTreeGrid;
class vtkPoints;

class VTKFILTERSHYPERTREE_EXPORT vtkHyperTreeGridGeometry : public vtkPolyDataAlgorithm
{
public:
  static vtkHyperTreeGridGeometry* New();
  vtkTypeMacro( vtkHyperTreeGridGeometry, vtkPolyDataAlgorithm );
  void PrintSelf( ostream&, vtkIndent );

protected:
  vtkHyperTreeGridGeometry();
  ~vtkHyperTreeGridGeometry();

  virtual int RequestData( vtkInformation*,
    vtkInformationVector**, vtkInformationVector* );
  virtual int FillInputPortInformation( int, vtkInformation* );

  void ProcessTrees();
  void RecursiveProcessTree( void* );
  void ProcessLeaf1D( void* );
  void ProcessLeaf2D( void* );
  void ProcessLeaf3D( void* );
  void AddFace( vtkIdType inId, double* origin, double* size,
                int offset, int orientation );

  vtkHyperTreeGrid* Input;
  vtkPolyData* Output;

  vtkDataSetAttributes* InData;
  vtkDataSetAttributes* OutData;

  vtkPoints* Points;
  vtkCellArray* Cells;

private:
  vtkHyperTreeGridGeometry(const vtkHyperTreeGridGeometry&) VTK_DELETE_FUNCTION;
  void operator=(const vtkHyperTreeGridGeometry&) VTK_DELETE_FUNCTION;
};

#endif
