/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNonLinearCell.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkNonLinearCell - abstract superclass for non-linear cells
// .SECTION Description
// vtkNonLinearCell is an abstract superclass for non-linear cell types.
// Cells that are a direct subclass of vtkCell or vtkCell3D are linear;
// cells that are a subclass of vtkNonLinearCell have non-linear interpolation
// functions. Non-linear cells require special treatment when tessellating
// or converting to graphics primitives. Note that the linearity of the cell
// is a function of whether the cell needs tessellation, which does not 
// strictly correlate with interpolation order (e.g., vtkHexahedron has
// non-linear interpolation functions (a product of three linear functions
// in r-s-t) even thought vtkHexahedron is considered linear.)
//
// The Error instance variable is used to control the tessellation of the
// cell. Error is normalized between (0.001,1) and typically measures the
// chordal deviation of linear (tessellated) primitives from the actual
// cell boundary. Each cell may have its own interpretation of this error
// measure.

#ifndef __vtkNonLinearCell_h
#define __vtkNonLinearCell_h

#include "vtkCell.h"
#include "vtkPointLocator.h"

class vtkPolyData;
class vtkDataSet;
class vtkUnstructuredGrid;


class VTK_COMMON_EXPORT vtkNonLinearCell : public vtkCell
{
public:
  vtkTypeRevisionMacro(vtkNonLinearCell,vtkCell);  
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the normalized error measure used to control the 
  // tessellation of the cell.
  vtkSetClampMacro(Error,float,0.001,1.0);
  vtkGetMacro(Error,float);

  // Description:
  // Non-linear cells require special treatment (tessellation) when 
  // converting to graphics primitives (during mapping). The vtkCell 
  // API IsLinear() is modified to indicate this requirement.
  virtual int IsLinear() {return 0;}
  
  // Description:
  // This method tessellates the cell returning polydata. (The Error ivar
  // controls the tessellation depth.) The new dataset will contain polydata
  // primitives, possibly new points as well as interpolated point and cell
  // data.  The user must provide (the output) polydata which is filled in by
  // the method. If the optional PointLocator is supplied, then any new
  // points that are created are inserted through the vtkPointLocator, rather
  // than directly in the vtkPolyData.  (Note: the input dataset and cellId
  // are used if the cell requires access to its owning dataset.) This method
  // is called when the topological dimension of the cell is 2D or less.
  virtual void Tesselate(vtkIdType cellId, 
                         vtkDataSet *input, vtkPolyData *output, 
                         vtkPointLocator *locator=NULL);
  
  // Description:
  // This method tessellates the cell returning unstructured grid. (The Error
  // ivar controls the tessellation depth.) The new dataset will contain
  // unstructured grid primitives, possibly new points as well as
  // interpolated point and cell data.  The user must provide (the output)
  // unstructured grid which is filled in by the method. If the optional
  // PointLocator is supplied, then any new points that are created are
  // inserted through the vtkPointLocator, rather than directly in the
  // vtkUnstructuredGrid.  (Note: the input dataset and cellId are used if
  // the cell requires access to its owning dataset.) This method is called
  // when the topological dimension of the cell is 3D.
  virtual void Tesselate(vtkIdType cellId, 
                         vtkDataSet *input, vtkUnstructuredGrid *output, 
                         vtkPointLocator *locator=NULL);
  

protected:
  vtkNonLinearCell();
  ~vtkNonLinearCell() {}

  float Error;

  // inline helper for tesselation- used by subclasses
  vtkIdType InsertPoint(vtkPointLocator *locator, vtkPoints *pts, float *x)
    {
      if ( locator != NULL ) 
        {
        vtkIdType p;
        locator->InsertUniquePoint(x,p);
        return p;
        }
      else
        {
        return pts->InsertNextPoint(x);
        }
    }
  
private:
  vtkNonLinearCell(const vtkNonLinearCell&);  // Not implemented.
  void operator=(const vtkNonLinearCell&);  // Not implemented.
};

#endif


