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
#include "vtkMultiGroupDataInformation.h"

vtkCxxRevisionMacro(vtkHierarchicalDataIterator, "1.7");
vtkStandardNewMacro(vtkHierarchicalDataIterator);

//----------------------------------------------------------------------------
class vtkHierarchicalDataIterator::vtkInternal
{
public:
  unsigned int CurIndex;
  unsigned int CurLevel;
  bool AscendingLevels;
  vtkInternal()
    {
    this->Clear();
    }

  void Clear()
    {
    this->CurLevel = VTK_UNSIGNED_INT_MAX;
    this->CurIndex = VTK_UNSIGNED_INT_MAX;
    }

  // NOTE: Not safe to call this method when IsDoneWithTraversal returns true.
  bool IsCurrentDataSetEmpty(vtkHierarchicalDataSet* hDS)
    {
    return (hDS->GetDataSet(this->CurLevel, this->CurIndex) ==0);
    }

  // NOTE: Not safe to call this method when IsDoneWithTraversal returns true.
  void Next(vtkHierarchicalDataSet* hDS)
    {
    do
      {
      this->CurIndex++;
      if (this->CurIndex >= hDS->GetNumberOfDataSets(this->CurLevel))
        {
        // Reached end of current level. Go to next level.
        this->NextLevel();
        }
      } while (!this->Done(hDS) && this->IsCurrentDataSetEmpty(hDS));
    if (this->Done(hDS))
      {
      this->Clear();
      }
    }

  bool Done(vtkHierarchicalDataSet* hDS)
    {
    unsigned int numLevels = hDS->GetNumberOfLevels();
    return (this->CurLevel >= numLevels);
    }

private:
  void NextLevel()
    {
    this->CurIndex = 0;
    if (this->AscendingLevels)
      {
      this->CurLevel++;
      }
    else
      {
      this->CurLevel = this->CurLevel==0? 
        VTK_UNSIGNED_INT_MAX: (this->CurLevel-1);
      }
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
  this->Internal->AscendingLevels = (this->AscendingLevels ==1);

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

  // Now until we reach the first non-empty dataset, we keep on going to the
  // next item.
  if (!this->IsDoneWithTraversal() && 
    this->Internal->IsCurrentDataSetEmpty(hDS))
    {
    this->Internal->Next(hDS);
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
    this->Internal->Next(hDS);
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

  return this->Internal->Done(hDS)? 1: 0;
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
  if (!hDS || this->IsDoneWithTraversal())
    {
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

