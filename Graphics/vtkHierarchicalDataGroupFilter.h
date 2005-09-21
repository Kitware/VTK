/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalDataGroupFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkHierarchicalDataGroupFilter - collects multiple inputs into one hierarchical dataset
// .SECTION Description
// vtkHierarchicalDataGroupFilter is an M to 1 filter that merges multiple
// input into one hierarchical dataset. It will assign each input to
// one level of the hierarchical dataset and will assign each update piece
// as a sub-block. For example, if there are two inputs and four update
// pieces, the output contains two levels with four datasets each.

#ifndef __vtkHierarchicalDataGroupFilter_h
#define __vtkHierarchicalDataGroupFilter_h

#include "vtkHierarchicalDataSetAlgorithm.h"

class VTK_GRAPHICS_EXPORT vtkHierarchicalDataGroupFilter : public vtkHierarchicalDataSetAlgorithm 
{
public:
  vtkTypeRevisionMacro(vtkHierarchicalDataGroupFilter,vtkHierarchicalDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object with PointIds and CellIds on; and ids being generated
  // as scalars.
  static vtkHierarchicalDataGroupFilter *New();

  // Description:
  // Add an input of this algorithm.  Note that these methods support
  // old-style pipeline connections.  When writing new code you should
  // use the more general vtkAlgorithm::AddInputConnection().  See
  // SetInput() for details.
  void AddInput(vtkDataObject *);
  void AddInput(int, vtkDataObject*);

protected:
  vtkHierarchicalDataGroupFilter();
  ~vtkHierarchicalDataGroupFilter();

  int RequestData(vtkInformation *, 
                  vtkInformationVector **, 
                  vtkInformationVector *);

  virtual int FillInputPortInformation(int port, vtkInformation *info);

private:
  vtkHierarchicalDataGroupFilter(const vtkHierarchicalDataGroupFilter&);  // Not implemented.
  void operator=(const vtkHierarchicalDataGroupFilter&);  // Not implemented.
};

#endif


