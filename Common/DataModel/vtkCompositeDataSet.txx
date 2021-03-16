/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeDataSet.txx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCompositeDataSet.h"

#include "vtkCompositeDataIterator.h"
#include "vtkDataObject.h"

#include <vector>

#ifndef vtkCompositeDataSet_txx
#define vtkCompositeDataSet_txx

//------------------------------------------------------------------------------
template <class DataSetT>
std::vector<DataSetT*> vtkCompositeDataSet::GetDataSets(vtkDataObject* dobj)
{
  std::vector<DataSetT*> datasets;
  if (auto cd = vtkCompositeDataSet::SafeDownCast(dobj))
  {
    auto iter = cd->NewIterator();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      if (auto ds = DataSetT::SafeDownCast(iter->GetCurrentDataObject()))
      {
        datasets.push_back(ds);
      }
    }
    iter->Delete();
  }
  else if (auto ds = DataSetT::SafeDownCast(dobj))
  {
    datasets.push_back(ds);
  }

  return datasets;
}

#endif
