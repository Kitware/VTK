/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalBoxVisitor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHierarchicalBoxVisitor.h"

#include "vtkAMRBox.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataCommand.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkUniformGrid.h"

vtkCxxRevisionMacro(vtkHierarchicalBoxVisitor, "1.3");
vtkStandardNewMacro(vtkHierarchicalBoxVisitor);

vtkCxxSetObjectMacro(vtkHierarchicalBoxVisitor, 
                     DataSet,
                     vtkHierarchicalBoxDataSet);

//----------------------------------------------------------------------------
vtkHierarchicalBoxVisitor::vtkHierarchicalBoxVisitor()
{
  this->DataSet = 0;
}

//----------------------------------------------------------------------------
vtkHierarchicalBoxVisitor::~vtkHierarchicalBoxVisitor()
{
  this->SetDataSet(0);
}

//----------------------------------------------------------------------------
void vtkHierarchicalBoxVisitor::Execute()
{
  if (!this->DataSet)
    {
    vtkErrorMacro("No dataset has been specified. Aborting.");
    return;
    }

  if (!this->Command)
    {
    vtkErrorMacro("No command has been specified. Aborting.");
    return;
    }

  vtkAMRLevelInformation info;
  int numLevels = this->DataSet->GetNumberOfLevels();
  for (int levelId=0; levelId<numLevels; levelId++)
    {
    int numDataSets = this->DataSet->GetNumberOfDataSets(levelId);
    for (int dsId=0; dsId<numDataSets; dsId++)
      {
      vtkAMRBox box;
      vtkUniformGrid* grid = this->DataSet->GetDataSet(levelId, dsId, box);
      info.Level = levelId;
      info.DataSetId = dsId;
      info.Box = box;
      if (grid)
        {
        this->Command->Execute(this, grid, &info);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkHierarchicalBoxVisitor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "DataSet: ";
  if (this->DataSet)
    {
    os << endl;
    this->DataSet->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
}


