/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalBoxCellDataToPointData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHierarchicalBoxCellDataToPointData.h"

#include "vtkCellDataToPointData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkHierarchicalBoxApplyFilterCommand.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkHierarchicalBoxVisitor.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"



vtkCxxRevisionMacro(vtkHierarchicalBoxCellDataToPointData, "1.3");
vtkStandardNewMacro(vtkHierarchicalBoxCellDataToPointData);

//----------------------------------------------------------------------------
vtkHierarchicalBoxCellDataToPointData::vtkHierarchicalBoxCellDataToPointData()
{
  this->PassCellData = 0;
}

//----------------------------------------------------------------------------
vtkHierarchicalBoxCellDataToPointData::~vtkHierarchicalBoxCellDataToPointData()
{
}

//----------------------------------------------------------------------------
void vtkHierarchicalBoxCellDataToPointData::ExecuteData(vtkDataObject* dooutput)
{
  vtkHierarchicalBoxDataSet* input = this->GetInput();

  vtkHierarchicalBoxApplyFilterCommand* comm = 
    vtkHierarchicalBoxApplyFilterCommand::New();

  vtkCellDataToPointData* filter = vtkCellDataToPointData::New();
  filter->SetPassCellData(this->PassCellData);
  comm->SetFilter(filter);
  filter->Delete();

  vtkCompositeDataVisitor* visitor = input->NewVisitor();
  visitor->SetCommand(comm);

  visitor->Execute();

  vtkHierarchicalBoxDataSet* output = comm->GetOutput();

  dooutput->ShallowCopy(output);

  comm->Delete();
  visitor->Delete();
}


//----------------------------------------------------------------------------
void vtkHierarchicalBoxCellDataToPointData::PrintSelf(
  ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "PassCellData: " << this->PassCellData << endl;
}

