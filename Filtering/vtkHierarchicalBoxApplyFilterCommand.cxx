/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalBoxApplyFilterCommand.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkHierarchicalBoxApplyFilterCommand.h"

#include "vtkAMRBox.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkSource.h"
#include "vtkUniformGrid.h"

#include "vtkApplyFilterCommandInternal.h"

vtkCxxRevisionMacro(vtkHierarchicalBoxApplyFilterCommand, "1.3");
vtkStandardNewMacro(vtkHierarchicalBoxApplyFilterCommand);

vtkCxxSetObjectMacro(vtkHierarchicalBoxApplyFilterCommand,
                     Output, 
                     vtkHierarchicalBoxDataSet);

//----------------------------------------------------------------
vtkHierarchicalBoxApplyFilterCommand::vtkHierarchicalBoxApplyFilterCommand() 
{ 
  this->Output = vtkHierarchicalBoxDataSet::New();

  this->Internal->FilterTypes.clear();
  vtkApplyFilterCommandInternal::FilterTypesVector ugfilters;
  ugfilters.push_back("vtkDataSetToDataSetFilter");
  this->Internal->FilterTypes["vtkUniformGrid"] = ugfilters;

}

//----------------------------------------------------------------
vtkHierarchicalBoxApplyFilterCommand::~vtkHierarchicalBoxApplyFilterCommand() 
{ 
  this->SetOutput(0);
}

//----------------------------------------------------------------
void vtkHierarchicalBoxApplyFilterCommand::Initialize()
{
  if (this->Output)
    {
    this->Output->Initialize();
    }
}

//----------------------------------------------------------------
void vtkHierarchicalBoxApplyFilterCommand::Execute(
  vtkCompositeDataVisitor *, vtkDataObject *input, void* callData)
{
  if (!this->Output)
    {
    vtkErrorMacro("Output is not set. Aborting");
    return;
    }

  if (!this->Filter)
    {
    vtkErrorMacro("Filter is not set. Aborting");
    return;
    }

  vtkAMRLevelInformation* info
    = reinterpret_cast<vtkAMRLevelInformation*>(callData);

  if (this->CheckFilterInputMatch(input))
    {
    this->SetFilterInput(this->Filter, input);
    this->Filter->Update();
    vtkUniformGrid* output = 
      vtkUniformGrid::SafeDownCast(this->Filter->GetOutputs()[0]);
    if (output)
      {
      vtkUniformGrid* outputsc = output->NewInstance();
      outputsc->ShallowCopy(output);
      this->Output->SetDataSet(info->Level, 
                               info->DataSetId, 
                               info->Box, 
                               outputsc);
      outputsc->Delete();
      }
    }
  else
    {
    vtkErrorMacro("The input and filter do not match. Aborting.");
    return;
    }
}

//----------------------------------------------------------------------------
void vtkHierarchicalBoxApplyFilterCommand::PrintSelf(ostream& os, 
                                                     vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Output: ";
  if (this->Output)
    {
    os << endl;
    this->Output->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
}
