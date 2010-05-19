/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMergeTables.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkMergeTables.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMergeColumns.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkTable.h"

vtkStandardNewMacro(vtkMergeTables);
//---------------------------------------------------------------------------
vtkMergeTables::vtkMergeTables()
{
  this->FirstTablePrefix = 0;
  this->SecondTablePrefix = 0;
  this->MergeColumnsByName = true;
  this->PrefixAllButMerged = false;
  this->SetFirstTablePrefix("Table1.");
  this->SetSecondTablePrefix("Table2.");
  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(1);
}

//---------------------------------------------------------------------------
vtkMergeTables::~vtkMergeTables()
{
  this->SetFirstTablePrefix(0);
  this->SetSecondTablePrefix(0);
}

//---------------------------------------------------------------------------
int vtkMergeTables::RequestData(
  vtkInformation*, 
  vtkInformationVector** inputVector, 
  vtkInformationVector* outputVector)
{
  // Get input tables
  vtkInformation* table1Info = inputVector[0]->GetInformationObject(0);
  vtkTable* table1 = vtkTable::SafeDownCast(
    table1Info->Get(vtkDataObject::DATA_OBJECT()));
  vtkInformation* table2Info = inputVector[1]->GetInformationObject(0);
  vtkTable* table2 = vtkTable::SafeDownCast(
    table2Info->Get(vtkDataObject::DATA_OBJECT()));

  // Get output table
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkTable* output = vtkTable::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  
  if (!this->FirstTablePrefix || !this->SecondTablePrefix)
    {
    vtkErrorMacro("FirstTablePrefix and/or SecondTablePrefix must be non-null.");
    return 0;
    }
  if (!strcmp(this->FirstTablePrefix, this->SecondTablePrefix))
    {
    vtkErrorMacro("FirstTablePrefix and SecondTablePrefix must be different.");
    return 0;
    }

  // Add columns from table 1
  for (int c = 0; c < table1->GetNumberOfColumns(); c++)
    {
    vtkAbstractArray* col = table1->GetColumn(c);
    char* name = col->GetName();
    char* newName = name;
    if (this->PrefixAllButMerged)
      {
      int len = static_cast<int>(strlen(name));
      int prefixLen = static_cast<int>(strlen(this->FirstTablePrefix));
      newName = new char[prefixLen + len + 1];
      strcpy(newName, this->FirstTablePrefix);
      strcat(newName, name);
      }
    vtkAbstractArray* newCol = vtkAbstractArray::CreateArray(col->GetDataType());
    newCol->DeepCopy(col);
    newCol->SetName(newName);
    if(newName!=name)
      {
      delete[] newName;
      }
    //vtkWarningMacro("adding column " << newCol->GetName() << " of size " << newCol->GetNumberOfTuples());
    output->AddColumn(newCol);
    newCol->Delete();
    }
  
  // Add empty values
  for (int r = 0; r < table2->GetNumberOfRows(); r++)
    {
    output->InsertNextBlankRow();
    }
  
  // Add columns from table 2
  vtkStringArray* toMerge = vtkStringArray::New();
  vtkTable* tempTable = vtkTable::New();
  for (int c = 0; c < table2->GetNumberOfColumns(); c++)
    {
    vtkAbstractArray* col = table2->GetColumn(c);
    char* name = col->GetName();
    vtkAbstractArray* newCol = vtkAbstractArray::CreateArray(col->GetDataType());
    if (table1->GetColumnByName(name) != 0)
      {
      // We have a naming conflict.
      // Rename both columns using the prefixes.
      int len = static_cast<int>(strlen(name));
      char* newName1 = new char[len + strlen(this->FirstTablePrefix) + 1];
      strcpy(newName1, this->FirstTablePrefix);
      strcat(newName1, name);
      if (!this->PrefixAllButMerged)
        {
        vtkAbstractArray* col1 = output->GetColumnByName(name);
        col1->SetName(newName1);
        }
      char* newName2 = new char[len + strlen(this->SecondTablePrefix) + 1];
      strcpy(newName2, this->SecondTablePrefix);
      strcat(newName2, name);
      newCol->SetName(newName2);
      toMerge->InsertNextValue(newName1);
      toMerge->InsertNextValue(newName2);
      toMerge->InsertNextValue(name);
      delete[] newName1;
      delete[] newName2;
      }
    else
      {
      char* newName = name;
      if (this->PrefixAllButMerged)
        {
        int len = static_cast<int>(strlen(name));
        int prefixLen = static_cast<int>(strlen(this->SecondTablePrefix));
        newName = new char[prefixLen + len + 1];
        strcpy(newName, this->SecondTablePrefix);
        strcat(newName, name);
        }
      newCol->SetName(newName);
      if(newName!=name)
        {
        delete[] newName;
        }
      }
    tempTable->AddColumn(newCol);
    newCol->Delete();
    }
  
  // Add empty values
  for (int r = 0; r < table1->GetNumberOfRows(); r++)
    {
    tempTable->InsertNextBlankRow();
    }
  
  // Add values from table 2
  for (int r = 0; r < table2->GetNumberOfRows(); r++)
    {
    for (int c = 0; c < tempTable->GetNumberOfColumns(); c++)
      {
      vtkAbstractArray* tempCol = tempTable->GetColumn(c);
      vtkAbstractArray* col = table2->GetColumn(c);
      tempCol->InsertNextTuple(r, col);
      }
    }
    
  // Move the columns from the temp table to the output table
  for (int c = 0; c < tempTable->GetNumberOfColumns(); c++)
    {
    vtkAbstractArray* col = tempTable->GetColumn(c);
    //vtkWarningMacro("adding column " << col->GetName() << " of size " << col->GetNumberOfTuples());
    output->AddColumn(col);
    }
  tempTable->Delete();

  // Merge any arrays that have the same name
  vtkMergeColumns* mergeColumns = vtkMergeColumns::New();
  vtkTable* temp = vtkTable::New();
  temp->ShallowCopy(output);
  mergeColumns->SetInput(temp);
  if (this->MergeColumnsByName)
    {
    for (vtkIdType i = 0; i < toMerge->GetNumberOfValues(); i += 3)
      {
      mergeColumns->SetInputArrayToProcess(
        0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_ROWS,
        toMerge->GetValue(i).c_str());
      mergeColumns->SetInputArrayToProcess(
        1, 0, 0, vtkDataObject::FIELD_ASSOCIATION_ROWS,
        toMerge->GetValue(i+1).c_str());
      mergeColumns->SetMergedColumnName(toMerge->GetValue(i+2).c_str());
      mergeColumns->Update();
      temp->ShallowCopy(mergeColumns->GetOutput());
      }
    }
  mergeColumns->Delete();
  toMerge->Delete();
  
  output->ShallowCopy(temp);
  temp->Delete();

  // Clean up pipeline information
  int piece = -1;
  int npieces = -1;
  if (outInfo->Has(
        vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()))
    {
    piece = outInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
    npieces = outInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
    }
  output->GetInformation()->Set(vtkDataObject::DATA_NUMBER_OF_PIECES(), npieces);
  output->GetInformation()->Set(vtkDataObject::DATA_PIECE_NUMBER(), piece);  
  
  return 1;
}

//---------------------------------------------------------------------------
void vtkMergeTables::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FirstTablePrefix: "
     << (this->FirstTablePrefix ? this->FirstTablePrefix : "(null)") << endl;
  os << indent << "SecondTablePrefix: "
     << (this->SecondTablePrefix ? this->SecondTablePrefix : "(null)") << endl;
  os << indent << "MergeColumnsByName: "
     << (this->MergeColumnsByName ? "on" : "off") << endl;
  os << indent << "PrefixAllButMerged: "
     << (this->PrefixAllButMerged ? "on" : "off") << endl;
}
