/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalDataIterator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHierarchicalDataIterator.h"

#include "vtkHierarchicalDataSet.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkHierarchicalDataIterator, "1.5");
vtkStandardNewMacro(vtkHierarchicalDataIterator);

//----------------------------------------------------------------------------
class vtkHierarchicalDataIterator::vtkInternal
{
public:
  unsigned int CurIndex;
  unsigned int CurLevel;
  vtkInternal()
    {
    this->Clear();
    }

  void Clear()
    {
    this->CurLevel = VTK_UNSIGNED_INT_MAX;
    this->CurIndex = VTK_UNSIGNED_INT_MAX;
    }
};

//----------------------------------------------------------------------------
vtkHierarchicalDataIterator::vtkHierarchicalDataIterator()
{
  this->Internal = new vtkInternal();
  this->AscendingLevels = 1;
}

//----------------------------------------------------------------------------
vtkHierarchicalDataIterator::~vtkHierarchicalDataIterator()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
vtkHierarchicalDataSet* vtkHierarchicalDataIterator::GetDataSet()
{
  return vtkHierarchicalDataSet::SafeDownCast(
    this->Superclass::GetDataSet());
}

//----------------------------------------------------------------------------
void vtkHierarchicalDataIterator::GoToFirstItem()
{
  this->Internal->Clear();

  vtkHierarchicalDataSet* hDS = this->GetDataSet();
  if (!hDS)
    {
    vtkErrorMacro("No data object has been set.");
    return;
    }

  if (this->AscendingLevels)
    {
    this->Internal->CurLevel = 0;
    }
  else
    {
    unsigned int numLevels = hDS->GetNumberOfLevels();
    this->Internal->CurLevel = numLevels>0? numLevels-1:
      VTK_UNSIGNED_INT_MAX;
    }
  this->Internal->CurIndex = 0;

  // Now take the CurLevel to the first non-empty level.
  this->GoToNonEmptyLevel();
}

//----------------------------------------------------------------------------
void vtkHierarchicalDataIterator::GoToNonEmptyLevel()
{
  if (this->Internal->CurLevel == VTK_UNSIGNED_INT_MAX)
    {
    return;
    }

  vtkHierarchicalDataSet* hDS = this->GetDataSet();
  unsigned int numLevels = hDS->GetNumberOfLevels();
  while (hDS->GetNumberOfDataSets(this->Internal->CurLevel) ==0)
    {
    if (this->AscendingLevels)
      {
      this->Internal->CurLevel++;
      if (this->Internal->CurLevel >= numLevels)
        {
        // Done with traversal
        this->Internal->Clear();
        break;
        }
      }
    else
      {
      if (this->Internal->CurLevel == 0)
        {
        // Done with traversal.
        this->Internal->Clear();
        break;
        }
      this->Internal->CurLevel--;
      }
    }
}

//----------------------------------------------------------------------------
void vtkHierarchicalDataIterator::GoToNextItem()
{
  vtkHierarchicalDataSet* hDS = this->GetDataSet();
  if (!hDS)
    {
    vtkErrorMacro("No data object has been set.");
    return;
    }

  if (!this->IsDoneWithTraversal())
    {
    unsigned int numDS = hDS->GetNumberOfDataSets(this->Internal->CurLevel);
    this->Internal->CurIndex++;

    while (this->Internal->CurIndex < numDS &&
      !hDS->GetDataSet(this->Internal->CurLevel, this->Internal->CurIndex))
      {
      // skip empty pieces.
      this->Internal->CurIndex++;
      }

    if (this->Internal->CurIndex < numDS)
      {
      return;
      }

    // End of current level reached, go to "next" level.
    this->Internal->CurIndex=0;
    if (this->AscendingLevels)
      {
      this->Internal->CurLevel++;
      }
    else
      {
      // Descending levels.
      if (this->Internal->CurLevel != 0)
        {
        this->Internal->CurLevel--;
        }
      else
        {
        this->Internal->Clear();
        }
      }

    if (!this->IsDoneWithTraversal())
      {
      this->GoToNonEmptyLevel();
      }
    else
      {
      this->Internal->Clear();
      }
    }
}

//----------------------------------------------------------------------------
int vtkHierarchicalDataIterator::IsDoneWithTraversal()
{
  vtkHierarchicalDataSet* hDS = this->GetDataSet();
  if (!hDS)
    {
    vtkErrorMacro("No data object has been set.");
    return 1;
    }

  unsigned int numLevels = hDS->GetNumberOfLevels();
  return (this->Internal->CurLevel >= numLevels)? 1 : 0;
}

//----------------------------------------------------------------------------
vtkDataObject* vtkHierarchicalDataIterator::GetCurrentDataObject()
{
  vtkHierarchicalDataSet* hDS = this->GetDataSet();
  if (!hDS || this->IsDoneWithTraversal())
    {
    return 0;
    }

  return hDS->GetDataSet(this->Internal->CurLevel, this->Internal->CurIndex);
}

//----------------------------------------------------------------------------
unsigned int vtkHierarchicalDataIterator::GetCurrentLevel()
{
  return this->Internal->CurLevel;
}

//----------------------------------------------------------------------------
unsigned int vtkHierarchicalDataIterator::GetCurrentIndex()
{
  return this->Internal->CurIndex;
}

//----------------------------------------------------------------------------
vtkInformation* vtkHierarchicalDataIterator::GetCurrentInformationObject()
{
  vtkHierarchicalDataSet* hDS = this->GetDataSet();
  if (!hDS)
    {
    vtkErrorMacro("No data object has been set.");
    return 0;
    }
  vtkMultiGroupDataInformation* mgInfo =
    this->DataSet->GetMultiGroupDataInformation();
  if (!mgInfo)
    {
    return 0;
    }
  return mgInfo->GetInformation(this->Internal->CurLevel, this->Internal->CurIndex);
}

//----------------------------------------------------------------------------
void vtkHierarchicalDataIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "AscendingLevels: " << this->AscendingLevels << endl;
}

