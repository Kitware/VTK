/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractCellLinks.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkAbstractCellLinks
 * @brief   an abstract base class for classes that build
 * topological links from points to cells
 *
 * vtkAbstractCellLinks is a family of supplemental objects to vtkCellArray and
 * vtkCellTypes, enabling fast access from points to the cells using the
 * points. vtkAbstractCellLinks is an array of links, each link representing a
 * list of cell ids using a particular point. The information provided by
 * this object can be used to determine neighbors and construct other local
 * topological information.
 *
 *
 * @sa
 * vtkCellLinks vtkStaticCellLinks vtkStaticCellLinksTemplate
*/

#ifndef vtkAbstractCellLinks_h
#define vtkAbstractCellLinks_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"

class vtkDataSet;
class vtkCellArray;


class VTKCOMMONDATAMODEL_EXPORT vtkAbstractCellLinks : public vtkObject
{
public:
  //@{
  /**
   * Standard type and print methods.
   */
  vtkTypeMacro(vtkAbstractCellLinks,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  //@}

  /**
   * Build the link list array. All subclasses must implement this method.
   */
  virtual void BuildLinks(vtkDataSet *data) = 0;

  /**
   * Based on the input (i.e., number of points, number of cells, and length
   * of connectivity array) this helper method returns the integral type to
   * use when instantiating cell link-related classes in order to properly
   * represent the data.  The return value is one of the types
   * (VTK_ID_TYPE,VTK_INT,VTK_SHORT) defined in the file
   * vtkType.h. Subclasses may choose to instantiate themselves with
   * different integral types for performance and/or memory reasons.
   */
  static int GetIdType(vtkIdType maxPtId, vtkIdType maxCellId, vtkCellArray *ca);

protected:
  vtkAbstractCellLinks();
  ~vtkAbstractCellLinks() VTK_OVERRIDE;

private:
  vtkAbstractCellLinks(const vtkAbstractCellLinks&) VTK_DELETE_FUNCTION;
  void operator=(const vtkAbstractCellLinks&) VTK_DELETE_FUNCTION;
};

#endif
