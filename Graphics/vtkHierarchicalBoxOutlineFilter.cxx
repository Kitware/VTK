/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalBoxOutlineFilter.cxx
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
#include "vtkHierarchicalBoxOutlineFilter.h"

#include "vtkAppendPolyData.h"
#include "vtkMultiBlockApplyFilterCommand.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataVisitor.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkOutlineFilter.h"
#include "vtkPolyData.h"



vtkCxxRevisionMacro(vtkHierarchicalBoxOutlineFilter, "1.1");
vtkStandardNewMacro(vtkHierarchicalBoxOutlineFilter);

//----------------------------------------------------------------------------
vtkHierarchicalBoxOutlineFilter::vtkHierarchicalBoxOutlineFilter()
{
}

//----------------------------------------------------------------------------
vtkHierarchicalBoxOutlineFilter::~vtkHierarchicalBoxOutlineFilter()
{
}

//----------------------------------------------------------------------------
void vtkHierarchicalBoxOutlineFilter::ExecuteData(vtkDataObject* dooutput)
{
  vtkHierarchicalBoxDataSet* input = this->GetInput();

  vtkMultiBlockApplyFilterCommand* comm = vtkMultiBlockApplyFilterCommand::New();

  vtkOutlineFilter* filter = vtkOutlineFilter::New();
  comm->SetFilter(filter);
  filter->Delete();

  vtkCompositeDataVisitor* visitor = input->NewVisitor();
  visitor->SetCommand(comm);

  visitor->Execute();

  vtkMultiBlockDataSet* output = comm->GetOutput();

  vtkAppendPolyData* append = vtkAppendPolyData::New();

  vtkCompositeDataIterator* iter = output->NewIterator();
  iter->GoToFirstItem();
  while (!iter->IsDoneWithTraversal())
    {
    append->AddInput(vtkPolyData::SafeDownCast(iter->GetCurrentDataObject()));
    iter->GoToNextItem();
    }
  iter->Delete();
  
  append->Update();
  vtkPolyData::SafeDownCast(dooutput)->ShallowCopy(append->GetOutput());
  append->Delete();

  comm->Delete();
  visitor->Delete();
}


//----------------------------------------------------------------------------
void vtkHierarchicalBoxOutlineFilter::PrintSelf(
  ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

