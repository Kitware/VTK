/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkHierarchicalBoxApplyFilterCommand#include "vtkCompositeDataCommand.h"
.h,v $
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkHierarchicalBoxApplyFilterCommand - command responsible for executing a filter on datasets
// .SECTION Description
// vtkMultiBlockApplyFilterCommand is a concrete implemetation of
// vtkApplyFilterCommand. It executes the filter on each dataset
// and collects the output in a vtkHierarchicalBoxDataSet.
// Currently, the filter has to be a subclass of vtkDataSetToDataSetFilter
// and the datasets have to be vtkUniformGrid.

#ifndef __vtkHierarchicalBoxApplyFilterCommand_h
#define __vtkHierarchicalBoxApplyFilterCommand_h

#include "vtkApplyFilterCommand.h"
#include "vtkAMRBox.h" // Needed for LevelInformation

class vtkHierarchicalBoxDataSet;

class VTK_FILTERING_EXPORT vtkHierarchicalBoxApplyFilterCommand : public vtkApplyFilterCommand
{
public:
  static vtkHierarchicalBoxApplyFilterCommand *New(); 

  vtkTypeRevisionMacro(vtkHierarchicalBoxApplyFilterCommand, 
                       vtkApplyFilterCommand);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Called by the visitor. The caller should pass itself, the
  // current dataset. The last argument should be a pointer to
  // an instance of LevelInformation.
  virtual void Execute(vtkCompositeDataVisitor *caller, 
                       vtkDataObject *input,
                       void* callData);

  // Description:
  // The output to be used to store the results. A default output
  // is created during construction.
  void SetOutput(vtkHierarchicalBoxDataSet* output);
  vtkGetObjectMacro(Output, vtkHierarchicalBoxDataSet);

  // Description:
  // Initialize should be called before iteration starts. It initializes
  // the output.
  void Initialize();

protected:

  vtkHierarchicalBoxDataSet* Output;

  vtkHierarchicalBoxApplyFilterCommand();
  ~vtkHierarchicalBoxApplyFilterCommand();

  vtkHierarchicalBoxApplyFilterCommand(const vtkHierarchicalBoxApplyFilterCommand&); // Not implemented
  void operator=(const vtkHierarchicalBoxApplyFilterCommand&); // Not implemented
};

#endif /* __vtkHierarchicalBoxApplyFilterCommand_h */
 
