/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiBlockDataVisitor.cxx
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
#include "vtkMultiBlockDataVisitor.h"

#include "vtkCompositeDataSet.h"
#include "vtkDataSet.h"
#include "vtkMultiBlockDataIterator.h"
#include "vtkCompositeDataCommand.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkMultiBlockDataVisitor, "1.2");
vtkStandardNewMacro(vtkMultiBlockDataVisitor);

vtkCxxSetObjectMacro(vtkMultiBlockDataVisitor,
                     DataIterator, 
                     vtkMultiBlockDataIterator);

//----------------------------------------------------------------------------
vtkMultiBlockDataVisitor::vtkMultiBlockDataVisitor()
{
  this->DataIterator = 0;
}

//----------------------------------------------------------------------------
vtkMultiBlockDataVisitor::~vtkMultiBlockDataVisitor()
{
  this->SetDataIterator(0);
}

//----------------------------------------------------------------------------
void vtkMultiBlockDataVisitor::ExecuteDataSet(vtkDataSet* ds)
{
  this->Command->Execute(this, ds, 0);
}

//----------------------------------------------------------------------------
void vtkMultiBlockDataVisitor::ExecuteCompositeDataSet(
  vtkCompositeDataIterator* iter)
{
  iter->GoToFirstItem();
  while (!iter->IsDoneWithTraversal())
    {
    vtkDataObject* dobj = iter->GetCurrentDataObject();
    // If the data object is a primitive dataset, simply
    // apply the command to it.
    if (dobj->IsA("vtkDataSet"))
      {
      vtkDataSet* curDataSet = vtkDataSet::SafeDownCast(dobj);
      if (curDataSet)
        {
        this->ExecuteDataSet(curDataSet);
        }
      }
    // if the data object is a composite dataset, recursively
    // call  ExecuteCompositeDataSet() until a leaf node is reached
    else if (dobj->IsA("vtkCompositeDataSet"))
      {
      vtkCompositeDataSet* curDataSet = vtkCompositeDataSet::SafeDownCast(dobj);
      if (dobj)
        {
        vtkCompositeDataIterator* it = curDataSet->NewIterator();
        this->ExecuteCompositeDataSet(it);
        it->Delete();
        }
      }
    iter->GoToNextItem();
    }
}

//----------------------------------------------------------------------------
void vtkMultiBlockDataVisitor::Execute()
{
  if (!this->DataIterator)
    {
    vtkErrorMacro("No iterator has been specified. Aborting.");
    return;
    }

  if (!this->Command)
    {
    vtkErrorMacro("No command has been specified. Aborting.");
    return;
    }

  this->Command->Initialize();
  this->ExecuteCompositeDataSet(this->DataIterator);
}

//----------------------------------------------------------------------------
void vtkMultiBlockDataVisitor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "DataIterator: ";
  if (this->DataIterator)
    {
    os << endl;
    this->DataIterator->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
}

