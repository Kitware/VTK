/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiBlockApplyFilterCommand.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMultiBlockApplyFilterCommand.h"

#include "vtkDataSet.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkSource.h"

vtkCxxRevisionMacro(vtkMultiBlockApplyFilterCommand, "1.3");
vtkStandardNewMacro(vtkMultiBlockApplyFilterCommand);

vtkCxxSetObjectMacro(vtkMultiBlockApplyFilterCommand,
                     Output, 
                     vtkMultiBlockDataSet);

//----------------------------------------------------------------
vtkMultiBlockApplyFilterCommand::vtkMultiBlockApplyFilterCommand() 
{ 
  this->Output = vtkMultiBlockDataSet::New();
}

//----------------------------------------------------------------
vtkMultiBlockApplyFilterCommand::~vtkMultiBlockApplyFilterCommand() 
{ 
  this->SetOutput(0);
}

//----------------------------------------------------------------
void vtkMultiBlockApplyFilterCommand::Initialize()
{
  if (this->Output)
    {
    this->Output->Initialize();
    }
}

//----------------------------------------------------------------
void vtkMultiBlockApplyFilterCommand::Execute(vtkCompositeDataVisitor *, 
                                              vtkDataObject *input,
                                              void*)
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

  if (this->CheckFilterInputMatch(input))
    {
    this->SetFilterInput(this->Filter, input);
    this->Filter->Update();
    vtkDataSet* output = 
      vtkDataSet::SafeDownCast(this->Filter->GetOutputs()[0]);
    vtkDataSet* outputsc = output->NewInstance();
    outputsc->ShallowCopy(output);
    this->Output->AddDataSet(outputsc);
    outputsc->Delete();
    }
  else
    {
    vtkErrorMacro("The input and filter do not match. Aborting.");
    return;
    }
}

//----------------------------------------------------------------------------
void vtkMultiBlockApplyFilterCommand::PrintSelf(ostream& os, vtkIndent indent)
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
