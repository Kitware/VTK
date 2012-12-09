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
// .NAME vtkHyperTreeGridGeometry - Hyper tree grid outer surface
// .SECTION Description

#ifndef __vtkHyperTreeGridGeometry_h
#define __vtkHyperTreeGridGeometry_h

#include "vtkFiltersHyperTreeModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"
#include "vtkHyperTreeGrid.h" // We need this because of supercursor

class vtkCellArray;
class vtkPoints;

class VTKFILTERSHYPERTREE_EXPORT vtkHyperTreeGridGeometry : public vtkPolyDataAlgorithm
{
public:
  static vtkHyperTreeGridGeometry* New();
  vtkTypeMacro(vtkHyperTreeGridGeometry,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkHyperTreeGridGeometry();
  ~vtkHyperTreeGridGeometry();

  virtual int RequestData( vtkInformation*, vtkInformationVector**, vtkInformationVector* );
  virtual int FillInputPortInformation( int, vtkInformation* );

  void ProcessTrees();
  void RecursiveProcessTree( vtkHyperTreeGridSuperCursor* );
  void AddFace( vtkIdType inId, double* origin, double* size,
                int offset, int orientation );

  vtkHyperTreeGrid* Input;
  vtkPolyData* Output;
  vtkPoints* Points;
  vtkCellArray* Cells;
  
private:
  vtkHyperTreeGridGeometry(const vtkHyperTreeGridGeometry&);  // Not implemented.
  void operator=(const vtkHyperTreeGridGeometry&);  // Not implemented.
};

#endif
