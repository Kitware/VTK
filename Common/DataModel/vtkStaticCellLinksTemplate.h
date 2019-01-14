/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStaticCellLinksTemplate.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkStaticCellLinksTemplate
 * @brief   object represents upward pointers from points
 * to list of cells using each point (template implementation)
 *
 *
 * vtkStaticCellLinksTemplate is a supplemental object to vtkCellArray and
 * vtkCellTypes, enabling access from points to the cells using the
 * points. vtkStaticCellLinksTemplate is an array of links, each link represents a
 * list of cell ids using a particular point. The information provided by
 * this object can be used to determine neighbors and construct other local
 * topological information. This class is a faster implementation of
 * vtkCellLinks. However, it cannot be incrementally constructed; it is meant
 * to be constructed once (statically) and must be rebuilt if the cells
 * change.
 *
 * This is a templated implementation for vtkStaticCellLinks. The reason for
 * the templating is to gain performance and reduce memory by using smaller
 * integral types to represent ids. For example, if the maximum id can be
 * represented by an int (as compared to a vtkIdType), it is possible to
 * reduce memory requirements by half and increase performance up to
 * 30%. This templated class can be used directly; alternatively the
 * non-templated class vtkStaticCellLinks can be used for convenience;
 * although it uses vtkIdType and thereby loses some speed and memory
 * advantage.
 *
 * @sa
 * vtkCellLinks vtkStaticCellLinks
*/

#ifndef vtkStaticCellLinksTemplate_h
#define vtkStaticCellLinksTemplate_h

class vtkDataSet;
class vtkPolyData;
class vtkUnstructuredGrid;
class vtkCellArray;


template <typename TIds>
class vtkStaticCellLinksTemplate
{
public:
  /**
   * Default constructor. BuildLinks() does most of the work.
   */
  vtkStaticCellLinksTemplate() :
    LinksSize(0), NumPts(0), NumCells(0), Links(nullptr), Offsets(nullptr)
  {
  }

  /**
   * Virtual destructor, anticipating future subclassing.
   */
  virtual ~vtkStaticCellLinksTemplate()
    {this->Initialize();}

  /**
   * Make sure any previously created links are cleaned up.
   */
  virtual void Initialize();

  /**
   * Build the link list array. Satisfy superclass' API.
   */
  virtual void BuildLinks(vtkDataSet *ds);

  /**
   * Build the link list array for vtkPolyData.
   */
  void BuildLinks(vtkPolyData *pd);

  /**
   * Build the link list array for vtkUnstructuredGrid.
   */
  void BuildLinks(vtkUnstructuredGrid *ugrid);

  /**
   * Get the number of cells using the point specified by ptId.
   */
  TIds GetNumberOfCells(vtkIdType ptId)
  {
      return (this->Offsets[ptId+1] - this->Offsets[ptId]);
  }

  /**
   * Return a list of cell ids using the point.
   */
  const TIds *GetCells(vtkIdType ptId)
  {
      return this->Links + this->Offsets[ptId];
  }

protected:
  // The various templated data members
  TIds LinksSize;
  TIds NumPts;
  TIds NumCells;

  // These point to the core data structures
  TIds *Links; //contiguous runs of cell ids
  TIds *Offsets; //offsets for each point into the link array

private:
  vtkStaticCellLinksTemplate(const vtkStaticCellLinksTemplate&) = delete;
  void operator=(const vtkStaticCellLinksTemplate&) = delete;

};

#include "vtkStaticCellLinksTemplate.txx"

#endif
// VTK-HeaderTest-Exclude: vtkStaticCellLinksTemplate.h
