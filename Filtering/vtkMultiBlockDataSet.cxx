/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiBlockDataSet.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMultiBlockDataSet.h"

#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"

#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkMultiBlockDataSet, "1.6");
vtkStandardNewMacro(vtkMultiBlockDataSet);

vtkInformationKeyMacro(vtkMultiBlockDataSet,BLOCK,Integer);

//----------------------------------------------------------------------------
vtkMultiBlockDataSet::vtkMultiBlockDataSet()
{
}

//----------------------------------------------------------------------------
vtkMultiBlockDataSet::~vtkMultiBlockDataSet()
{
}

//----------------------------------------------------------------------------
void vtkMultiBlockDataSet::AddDataSet(vtkInformation* index, vtkDataObject* dobj)
{
  if (index->Has(INDEX()) && index->Has(BLOCK()))
    {
    this->SetDataSet(index->Get(BLOCK()), index->Get(INDEX()), dobj);
    }
  else
    {
    this->Superclass::AddDataSet(index, dobj);
    }
}

//----------------------------------------------------------------------------
vtkDataObject* vtkMultiBlockDataSet::GetDataSet(vtkInformation* index)
{
  if (index->Has(INDEX()) && index->Has(BLOCK()))
    {
    return this->GetDataSet(index->Get(BLOCK()), index->Get(INDEX()));
    }
  return this->Superclass::GetDataSet(index);
}

//----------------------------------------------------------------------------
void vtkMultiBlockDataSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

