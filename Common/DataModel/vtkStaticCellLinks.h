/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStaticCellLinks.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkStaticCellLinks - object represents upward pointers from points
// to list of cells using each point

// .SECTION Description
// vtkStaticCellLinks is a supplemental object to vtkCellArray and
// vtkCellTypes, enabling access from points to the cells using the
// points. vtkStaticCellLinks is an array of links, each link represents a
// list of cell ids using a particular point. The information provided by
// this object can be used to determine neighbors and construct other local
// topological information. This class is a faster implementation of
// vtkCellLinks. However, it cannot be incrementally constructed; it is meant
// to be constructed once (statically) and must be rebuilt if the cells
// change.

// .SECTION Caveats
// This is a drop-in replacement for vtkCellLinks using static link
// construction. It uses the templated vtkStaticCellLinksTemplate class,
// instantiating vtkStaticCellLinksTemplate with a vtkIdType template
// parameter. Note that for best performance, the vtkStaticCellLinksTemplate
// class may be used directly, instantiating it with the appropriate id
// type. This class is also wrappable and can be used from an interpreted
// language such as Python.

// .SECTION See Also
// vtkCellLinks vtkStaticCellLinksTemplate

#ifndef vtkStaticCellLinks_h
#define vtkStaticCellLinks_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkAbstractCellLinks.h"
#include "vtkStaticCellLinksTemplate.h" // For implementations

class vtkDataSet;
class vtkCellArray;


class VTKCOMMONDATAMODEL_EXPORT vtkStaticCellLinks : public vtkAbstractCellLinks
{
public:
  // Description:
  // Standard methods for instantiation, type manipulation and printing.
  static vtkStaticCellLinks *New();
  vtkTypeMacro(vtkStaticCellLinks,vtkAbstractCellLinks);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Build the link list array. Satisfy the superclass API.
  virtual void BuildLinks(vtkDataSet *ds)
    {this->Impl->BuildLinks(ds);}

  // Description:
  // Get the number of cells using the point specified by ptId.
  vtkIdType GetNumberOfCells(vtkIdType ptId)
    {return this->Impl->GetNumberOfCells(ptId);}

  // Description:
  // Get the number of cells using the point specified by ptId. This is an
  // alias for GetNumberOfCells(); consistent with the vtkCellLinks API.
  unsigned short GetNcells(vtkIdType ptId)
    { return static_cast<unsigned short>(this->GetNumberOfCells(ptId)); }

  // Description:
  // Return a list of cell ids using the specified point.
  const vtkIdType *GetCells(vtkIdType ptId)
    {return this->Impl->GetCells(ptId);}

  // Description:
  // Make sure any previously created links are cleaned up.
  void Initialize()
    {this->Impl->Initialize();}

protected:
  vtkStaticCellLinks();
  virtual ~vtkStaticCellLinks();

  vtkStaticCellLinksTemplate<vtkIdType> *Impl;

private:
  vtkStaticCellLinks(const vtkStaticCellLinks&);  // Not implemented.
  void operator=(const vtkStaticCellLinks&);  // Not implemented.

};


#endif
