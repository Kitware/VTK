/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkApplyFilterCommand.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkApplyFilterCommand - command responsible for executing a filter on datasets
// .SECTION Description
// vtkApplyFilterCommand is an abstract superclass for commands that
// apply a filter to a dataset each time Execute() is called.
// Currently, the filter has to be a subclass of one of the following:
// vtkDataSetToDataSetFilter, vtkDataSetToImageFilter, 
// vtkDataSetToPolyDataFilter, vtkDataSetToStructuredGridFilter
// vtkDataSetToStructuredPointsFilter", vtkDataSetToUnstructuredGridFilter
// and the datasets have to be subclasses of vtkDataSet.

// .SECTION See Also
// vtkMultiBlockApplyFilterCommand vtkHierarchicalBoxApplyFilterCommand

#ifndef __vtkApplyFilterCommand_h
#define __vtkApplyFilterCommand_h

#include "vtkCompositeDataCommand.h"

class vtkApplyFilterCommandInternal;
class vtkMultiBlockDataSet;
class vtkSource;
class vtkDataObject;

class VTK_FILTERING_EXPORT vtkApplyFilterCommand : public vtkCompositeDataCommand
{
public:
  vtkTypeRevisionMacro(vtkApplyFilterCommand, vtkCompositeDataCommand);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The filter to be executed in Execute()
  void SetFilter(vtkSource* filter);
  vtkGetObjectMacro(Filter, vtkSource)

protected:

  vtkApplyFilterCommandInternal* Internal;

  vtkSource* Filter;

  int CheckFilterInputMatch(vtkDataObject* inp);
  void SetFilterInput(vtkSource* source, vtkDataObject* input);
  
  vtkApplyFilterCommand();
  ~vtkApplyFilterCommand();

private:
  vtkApplyFilterCommand(const vtkApplyFilterCommand&); // Not implemented
  void operator=(const vtkApplyFilterCommand&); // Not implemented
};



#endif /* __vtkApplyFilterCommand_h */
 
