/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPExtractArraysOverTime.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPExtractArraysOverTime.h"

#include "vtkCompositeDataIterator.h"
#include "vtkDataSetAttributes.h"
#include "vtkInformation.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiProcessStream.h"
#include "vtkObjectFactory.h"
#include "vtkSelectionNode.h"
#include "vtkTable.h"
#include "vtkUnsignedCharArray.h"

#include <string>
#include <cassert>

vtkStandardNewMacro(vtkPExtractArraysOverTime);
vtkCxxSetObjectMacro(vtkPExtractArraysOverTime, Controller, vtkMultiProcessController);
//----------------------------------------------------------------------------
vtkPExtractArraysOverTime::vtkPExtractArraysOverTime()
{
  this->Controller = 0;
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
vtkPExtractArraysOverTime::~vtkPExtractArraysOverTime()
{
  this->SetController(0);
}

//----------------------------------------------------------------------------
void vtkPExtractArraysOverTime::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Controller: " << this->Controller << endl;
}

//----------------------------------------------------------------------------
void vtkPExtractArraysOverTime::PostExecute(
  vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  this->Superclass::PostExecute(request, inputVector, outputVector);

  int procid = 0;
  int numProcs = 1;
  if ( this->Controller )
  {
    procid = this->Controller->GetLocalProcessId();
    numProcs = this->Controller->GetNumberOfProcesses();
  }

  if (numProcs <= 1)
  {
    // Trivial case.
    return;
  }

  vtkMultiBlockDataSet* output = vtkMultiBlockDataSet::GetData(outputVector, 0);
  if (procid == 0)
  {
    for (int cc=1; cc < numProcs; cc++)
    {
      vtkMultiBlockDataSet* remoteOutput = vtkMultiBlockDataSet::New();
      this->Controller->Receive(remoteOutput, cc, EXCHANGE_DATA);
      // Now receive the block names explicitly.
      vtkMultiProcessStream stream;
      this->Controller->Receive(stream, cc, EXCHANGE_DATA);
      vtkCompositeDataIterator* iter = remoteOutput->NewIterator();
      for (iter->InitTraversal(); !iter->IsDoneWithTraversal();
        iter->GoToNextItem())
      {
        unsigned int index;
        stream >> index;
        assert(iter->GetCurrentFlatIndex() == index);
        std::string name;
        stream >> name;
        iter->GetCurrentMetaData()->Set(vtkCompositeDataSet::NAME(),
          name.c_str());
      }
      iter->Delete();
      this->AddRemoteData(remoteOutput, output);
      remoteOutput->Delete();
    }
    int num_blocks = static_cast<int>(output->GetNumberOfBlocks());
    this->Controller->Broadcast(&num_blocks, 1, 0);
  }
  else
  {
    this->Controller->Send(output, 0, EXCHANGE_DATA);
    // Send the names explicitly since the vtkMultiProcessController cannot send
    // composite-data meta-data yet.
    vtkMultiProcessStream stream;
    vtkCompositeDataIterator* iter = output->NewIterator();
    for (iter->InitTraversal(); iter->IsDoneWithTraversal() != 1;
      iter->GoToNextItem())
    {
      stream << iter->GetCurrentFlatIndex()
        << std::string(iter->GetCurrentMetaData()->Get(vtkCompositeDataSet::NAME()));
    }
    iter->Delete();
    this->Controller->Send(stream, 0, EXCHANGE_DATA);
    output->Initialize();
    int num_blocks = 0;
    // ensures that all processes have the same structure.
    this->Controller->Broadcast(&num_blocks, 1, 0);
    output->SetNumberOfBlocks(static_cast<unsigned int>(num_blocks));
  }
}

//----------------------------------------------------------------------------
void vtkPExtractArraysOverTime::AddRemoteData(
  vtkMultiBlockDataSet* remoteOutput, vtkMultiBlockDataSet* output)
{
  vtkCompositeDataIterator* remoteIter = remoteOutput->NewIterator();
  vtkCompositeDataIterator* localIter = output->NewIterator();
  for (remoteIter->InitTraversal();
    !remoteIter->IsDoneWithTraversal(); remoteIter->GoToNextItem())
  {
    // We really need to think of merging blocks only in 2 cases: for global id
    // based selections or for location based selections
    if (this->ContentType != vtkSelectionNode::LOCATIONS &&
      this->ContentType != vtkSelectionNode::GLOBALIDS)
    {
      unsigned int index = output->GetNumberOfBlocks();
      output->SetBlock(index, remoteIter->GetCurrentDataObject());
      output->GetMetaData(index)->Copy(remoteIter->GetCurrentMetaData(),
        /*deep=*/0);
      continue;
    }

    if (!remoteIter->GetCurrentMetaData()->Has(vtkCompositeDataSet::NAME()))
    {
      vtkWarningMacro("Internal filter error: Missing NAME()");
      continue;
    }
    std::string name = remoteIter->GetCurrentMetaData()->Get(
      vtkCompositeDataSet::NAME());

    // We need to merge "coincident" tables.
    bool merged = false;
    for (localIter->InitTraversal(); !localIter->IsDoneWithTraversal();
      localIter->GoToNextItem())
    {
      if (name ==
        localIter->GetCurrentMetaData()->Get(vtkCompositeDataSet::NAME()))
      {
        this->MergeTables(vtkTable::SafeDownCast(remoteIter->GetCurrentDataObject()),
          vtkTable::SafeDownCast(localIter->GetCurrentDataObject()));
        merged = true;
        break;
      }
    }
    if (!merged)
    {
      unsigned int index = output->GetNumberOfBlocks();
      output->SetBlock(index, remoteIter->GetCurrentDataObject());
      output->GetMetaData(index)->Copy(remoteIter->GetCurrentMetaData(),
        /*deep=*/0);
    }
  }
  localIter->Delete();
  remoteIter->Delete();
}

//----------------------------------------------------------------------------
void vtkPExtractArraysOverTime::MergeTables(
  vtkTable* routput, vtkTable* output)
{
  if (!routput || !output)
  {
    return;
  }
  vtkIdType rDims = routput->GetNumberOfRows();
  vtkIdType dims = output->GetNumberOfRows();
  if (dims != rDims)
  {
    vtkWarningMacro("Tried to add remote dataset of different length. "
                    "Skipping");
    return;
  }

  vtkUnsignedCharArray* rValidPts = vtkArrayDownCast<vtkUnsignedCharArray>(
    routput->GetRowData()->GetArray("vtkValidPointMask"));

  // Copy the valid values
  if (rValidPts)
  {
    for (vtkIdType i=0; i<dims; i++)
    {
      if (rValidPts->GetValue(i))
      {
        vtkDataSetAttributes* outRowData = output->GetRowData();
        vtkDataSetAttributes* remoteRowData = routput->GetRowData();
        // Copy arrays from remote to current
        int numRArrays = remoteRowData->GetNumberOfArrays();
        for (int aidx=0; aidx<numRArrays; aidx++)
        {
          const char* name = 0;
          vtkAbstractArray* raa = remoteRowData->GetAbstractArray(aidx);
          if (raa)
          {
            name = raa->GetName();
          }
          if (name)
          {
            vtkAbstractArray* aa = outRowData->GetAbstractArray(name);
            // Create the output array if necessary
            if (!aa)
            {
              aa = raa->NewInstance();
              aa->DeepCopy(raa);
              aa->SetName(name);
              outRowData->AddArray(aa);
              aa->UnRegister(0);
            }
            if (raa->GetNumberOfTuples() > i)
            {
              aa->InsertTuple(i, i, raa);
            }
            else
            {
              //cerr << (raa->GetName()?raa->GetName():"NONAME")
              // <<" has only " << raa->GetNumberOfTuples()
              // << " and looking for value at " << i << endl;
            }
          }
        }
      }
    }
  }
}
