/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExplicitCell.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkExplicitCell - abstract superclass for cells requiring an explicit representation
// .SECTION Description
// vtkExplicitCell is an abstract superclass for cells that cannot be
// represented implicitly. An implicit representation requires only a
// cell type and connectivity list (e.g., triangle). Explicit cells
// require information beyond this; e.g., a NURBS surface or cells that
// require explicit face/edge descriptions. Most cells in VTK are
// implicitly represented.

#ifndef __vtkExplicitCell_h
#define __vtkExplicitCell_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkNonLinearCell.h"

class vtkDataSet;

class VTKCOMMONDATAMODEL_EXPORT vtkExplicitCell : public vtkNonLinearCell
{
public:
  vtkTypeMacro(vtkExplicitCell,vtkNonLinearCell);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Explicit cells require additional representational information
  // beyond the usual cell type and connectivity list information.
  // Most cells in VTK are implicit cells. The vtkCell::IsImplicitCell()
  // virtual function is overloaded to reflect this requirement.
  virtual int IsExplicitCell() {return 1;}

  // Description:
  // Set/Get the cell id. This is necessary for explicit cells because they
  // often need to keep extra information (typically contained in the
  // cell data of a point set). This information might be things like
  // knot points/weights, boundaries, etc.
  vtkSetMacro(CellId,vtkIdType);
  vtkGetMacro(CellId,vtkIdType);

  // Description:
  // Set/Get the mesh that owns this cell. This is necessary for explicit
  // cells because they often need to keep extra information (typically
  // contained in the cell data of a point set). This information might be
  // things like knot points/weights, boundaries, etc.
  virtual void SetDataSet(vtkDataSet*);
  vtkGetObjectMacro(DataSet,vtkDataSet);

protected:
  vtkExplicitCell();
  ~vtkExplicitCell() {}

  vtkIdType  CellId; //used to index into other arrays
  vtkDataSet *DataSet; //dataset from which this cell came

private:
  vtkExplicitCell(const vtkExplicitCell&);  // Not implemented.
  void operator=(const vtkExplicitCell&);  // Not implemented.
};

#endif


