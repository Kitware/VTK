/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiBlockDataGroupFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkMultiBlockDataGroupFilter
 * @brief   collects multiple inputs into one multi-group dataset
 *
 * vtkMultiBlockDataGroupFilter is an M to 1 filter that merges multiple
 * input into one multi-group dataset. It will assign each input to
 * one group of the multi-group dataset and will assign each update piece
 * as a sub-block. For example, if there are two inputs and four update
 * pieces, the output contains two groups with four datasets each.
*/

#ifndef vtkMultiBlockDataGroupFilter_h
#define vtkMultiBlockDataGroupFilter_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkMultiBlockDataSetAlgorithm.h"

class VTKFILTERSGENERAL_EXPORT vtkMultiBlockDataGroupFilter : public vtkMultiBlockDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkMultiBlockDataGroupFilter,vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Construct object with PointIds and CellIds on; and ids being generated
   * as scalars.
   */
  static vtkMultiBlockDataGroupFilter *New();

  //@{
  /**
   * Assign a data object as input. Note that this method does not
   * establish a pipeline connection. Use AddInputConnection() to
   * setup a pipeline connection.
   */
  void AddInputData(vtkDataObject *);
  void AddInputData(int, vtkDataObject*);
  //@}

protected:
  vtkMultiBlockDataGroupFilter();
  ~vtkMultiBlockDataGroupFilter() VTK_OVERRIDE;

  int RequestInformation(vtkInformation *,
                  vtkInformationVector **,
                  vtkInformationVector *) VTK_OVERRIDE;
  int RequestUpdateExtent(vtkInformation *,
                  vtkInformationVector **,
                  vtkInformationVector *) VTK_OVERRIDE;
  int RequestData(vtkInformation *,
                  vtkInformationVector **,
                  vtkInformationVector *) VTK_OVERRIDE;

  int FillInputPortInformation(int port, vtkInformation *info) VTK_OVERRIDE;

private:
  vtkMultiBlockDataGroupFilter(const vtkMultiBlockDataGroupFilter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkMultiBlockDataGroupFilter&) VTK_DELETE_FUNCTION;
};

#endif


