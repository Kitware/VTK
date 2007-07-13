/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiBlockMergeFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMultiBlockMergeFilter - merges multiple inputs into one multi-group dataset. 
// .SECTION Description
// vtkMultiBlockMergeFilter is an M to 1 filter similar to vtkMultiGroupDataGroupFilters. However, when all N inputs are multiblock datasets, rather than producing a dataset with N groups, this will produce a multiblock dataset with 1 or more groups (depending on how many are in the input) where the output groups contain one dataset from the same numbered group in each of its inputs. 

#ifndef __vtkMultiBlockMergeFilter_h
#define __vtkMultiBlockMergeFilter_h

#include "vtkMultiGroupDataGroupFilter.h"

class VTK_GRAPHICS_EXPORT vtkMultiBlockMergeFilter : public vtkMultiGroupDataGroupFilter
{
public:
  vtkTypeRevisionMacro(vtkMultiBlockMergeFilter,vtkMultiGroupDataGroupFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object with PointIds and CellIds on; and ids being generated
  // as scalars.
  static vtkMultiBlockMergeFilter *New();

protected:
  vtkMultiBlockMergeFilter();
  ~vtkMultiBlockMergeFilter();

  int RequestData(vtkInformation *, 
                  vtkInformationVector **, 
                  vtkInformationVector *);

private:
  vtkMultiBlockMergeFilter(const vtkMultiBlockMergeFilter&);  // Not implemented.
  void operator=(const vtkMultiBlockMergeFilter&);  // Not implemented.
};

#endif


