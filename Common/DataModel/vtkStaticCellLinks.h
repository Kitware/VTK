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
// list of cell id's using a particular point. The information provided by
// this object can be used to determine neighbors and construct other local
// topological information. This class is a faster implementation of
// vtkCellLinks. However, it cannot be incrementally constructed; it is meant
// to be constructed once (statically) and must be rebuilt if the cells
// change.

// .SECTION See Also
// vtkCellLinks

#ifndef vtkStaticCellLinks_h
#define vtkStaticCellLinks_h

#include "vtkObject.h"
#include "vtkSmartPointer.h" // For vtkSmartPointer
#include "vtkTypeTemplate.h" // For vtkTypeTemplate

class vtkPolyData;
class vtkUnstructuredGrid;

template <typename TIds>
class vtkStaticCellLinks
{
public:
  // Description:
  // Default constructor. BuildLinks() does most of the work.
  vtkStaticCellLinks() :
    LinksSize(0), NumPts(0), NumCells(0), Links(NULL), Offsets(NULL)
    {
    }

  // Description:
  // Release memory if necessary.
  ~vtkStaticCellLinks()
    {
      if ( this->Links )
        {
        delete [] this->Links;
        }
      if ( this->Offsets )
        {
        delete [] this->Offsets;
        }
    }

  // Description:
  // Build the link list array.
  void BuildLinks(vtkPolyData *pd);

  // Description:
  // Build the link list array.
  void BuildLinks(vtkUnstructuredGrid *ugrid);

  // Description:
  // Get the number of cells using the point specified by ptId.
  TIds GetNumberOfCells(vtkIdType ptId)
    {
      return (this->Offsets[ptId+1] - this->Offsets[ptId]);
    }

  // Description:
  // Return a list of cell ids using the point.
  const TIds *GetCells(vtkIdType ptId)
    {
      return this->Links + this->Offsets[ptId];
    }

protected:
  // Okay the various ivars
  TIds LinksSize;
  TIds NumPts;
  TIds NumCells;

  TIds *Links; //contiguous runs of cells
  TIds *Offsets; //offsets for each point into the link array

private:
  vtkStaticCellLinks(const vtkStaticCellLinks&);  // Not implemented.
  void operator=(const vtkStaticCellLinks&);  // Not implemented.

};

#include "vtkStaticCellLinks.txx"

#endif
// VTK-HeaderTest-Exclude: vtkStaticCellLinks.h
