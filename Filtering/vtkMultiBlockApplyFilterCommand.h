/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiBlockApplyFilterCommand.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMultiBlockApplyFilterCommand - command responsible for executing a filter on datasets
// .SECTION Description
// vtkMultiBlockApplyFilterCommand is a concrete implemetation of
// vtkApplyFilterCommand. It executes the filter on each dataset
// and collects the output in a vtkMultiBlockDataSet.

#ifndef __vtkMultiBlockApplyFilterCommand_h
#define __vtkMultiBlockApplyFilterCommand_h

#include "vtkApplyFilterCommand.h"

class vtkMultiBlockDataSet;

class VTK_FILTERING_EXPORT vtkMultiBlockApplyFilterCommand : public vtkApplyFilterCommand
{
public:
  static vtkMultiBlockApplyFilterCommand *New(); 

  vtkTypeRevisionMacro(vtkMultiBlockApplyFilterCommand, vtkApplyFilterCommand);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Called by the visitor. The caller should pass itself, the
  // current dataset. The last argument is ignored.
  virtual void Execute(vtkCompositeDataVisitor *caller, 
                       vtkDataObject *input,
                       void*);

  // Description:
  // The output to be used to store the results. A default output
  // is created during construction.
  void SetOutput(vtkMultiBlockDataSet* output);
  vtkGetObjectMacro(Output, vtkMultiBlockDataSet);

  // Description:
  // Initialize should be called before iteration starts. It initializes
  // the output.
  void Initialize();

protected:

  vtkMultiBlockDataSet* Output;

  vtkMultiBlockApplyFilterCommand();
  ~vtkMultiBlockApplyFilterCommand();

private:
  vtkMultiBlockApplyFilterCommand(const vtkMultiBlockApplyFilterCommand&); // Not implemented
  void operator=(const vtkMultiBlockApplyFilterCommand&); // Not implemented
};



#endif /* __vtkMultiBlockApplyFilterCommand_h */
 
