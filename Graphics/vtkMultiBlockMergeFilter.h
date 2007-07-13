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
// .NAME vtkMultiBlockMergeFilter - merges multiple inputs into one multi-group dataset with special treatment for multiblock inputs. 
// .SECTION Description
// vtkMultiBlockMergeFilter is an M to 1 filter similar to vtkMultiGroupDataGroupFilters. However where as the parent class creates N groups in the output for N inputs, this creates (when all N inputs are multiblock datasets) 1 group in the output with N datasets inside it. In actuality if the inputs have M groups, this will produce M groups, each of which has N datasets. 

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


