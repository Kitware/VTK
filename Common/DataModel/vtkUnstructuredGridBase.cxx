/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridBase.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkUnstructuredGridBase.h"

#include "vtkCellIterator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkSmartPointer.h"

//----------------------------------------------------------------------------
vtkUnstructuredGridBase::vtkUnstructuredGridBase() = default;

//----------------------------------------------------------------------------
vtkUnstructuredGridBase::~vtkUnstructuredGridBase() = default;

//----------------------------------------------------------------------------
void vtkUnstructuredGridBase::DeepCopy(vtkDataObject* src)
{
  this->Superclass::DeepCopy(src);

  if (vtkDataSet* ds = vtkDataSet::SafeDownCast(src))
  {
    vtkSmartPointer<vtkCellIterator> cellIter =
      vtkSmartPointer<vtkCellIterator>::Take(ds->NewCellIterator());
    for (cellIter->InitTraversal(); !cellIter->IsDoneWithTraversal(); cellIter->GoToNextCell())
    {
      this->InsertNextCell(cellIter->GetCellType(), cellIter->GetNumberOfPoints(),
        cellIter->GetPointIds()->GetPointer(0), cellIter->GetNumberOfFaces(),
        cellIter->GetFaces()->GetPointer(1));
    }
  }
}

//----------------------------------------------------------------------------
vtkUnstructuredGridBase* vtkUnstructuredGridBase::GetData(vtkInformation* info)
{
  return vtkUnstructuredGridBase::SafeDownCast(info ? info->Get(DATA_OBJECT()) : nullptr);
}

//----------------------------------------------------------------------------
vtkUnstructuredGridBase* vtkUnstructuredGridBase::GetData(vtkInformationVector* v, int i)
{
  return vtkUnstructuredGridBase::GetData(v->GetInformationObject(i));
}

//----------------------------------------------------------------------------
vtkIdType vtkUnstructuredGridBase::InsertNextCell(int type, vtkIdType npts, const vtkIdType pts[])
{
  return this->InternalInsertNextCell(type, npts, pts);
}

//----------------------------------------------------------------------------
vtkIdType vtkUnstructuredGridBase::InsertNextCell(int type, vtkIdList* ptIds)
{
  return this->InternalInsertNextCell(type, ptIds);
}

//----------------------------------------------------------------------------
vtkIdType vtkUnstructuredGridBase::InsertNextCell(
  int type, vtkIdType npts, const vtkIdType pts[], vtkIdType nfaces, const vtkIdType faces[])
{
  return this->InternalInsertNextCell(type, npts, pts, nfaces, faces);
}

//----------------------------------------------------------------------------
void vtkUnstructuredGridBase::ReplaceCell(vtkIdType cellId, int npts, const vtkIdType pts[])
{
  this->InternalReplaceCell(cellId, npts, pts);
}
