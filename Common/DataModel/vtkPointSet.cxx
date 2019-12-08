/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointSet.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPointSet.h"

#include "vtkCell.h"
#include "vtkCellLocator.h"
#include "vtkClosestPointStrategy.h"
#include "vtkGarbageCollector.h"
#include "vtkGenericCell.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkPointLocator.h"
#include "vtkPointSetCellIterator.h"
#include "vtkStaticCellLocator.h"
#include "vtkStaticPointLocator.h"

#include "vtkSmartPointer.h"

#define VTK_CREATE(type, name) vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

vtkCxxSetObjectMacro(vtkPointSet, Points, vtkPoints);
vtkCxxSetObjectMacro(vtkPointSet, PointLocator, vtkAbstractPointLocator);
vtkCxxSetObjectMacro(vtkPointSet, CellLocator, vtkAbstractCellLocator);

//----------------------------------------------------------------------------
vtkPointSet::vtkPointSet()
{
  this->Editable = false;
  this->Points = nullptr;
  this->PointLocator = nullptr;
  this->CellLocator = nullptr;
}

//----------------------------------------------------------------------------
vtkPointSet::~vtkPointSet()
{
  this->Cleanup();

  if (this->PointLocator != nullptr)
  {
    cout << "DELETING LOCATOR: PointSet: " << this << " locator: " << this->PointLocator << "\n";
  }
  this->SetPointLocator(nullptr);
  this->SetCellLocator(nullptr);
}

//----------------------------------------------------------------------------
// Copy the geometric structure of an input point set object.
void vtkPointSet::CopyStructure(vtkDataSet* ds)
{
  vtkPointSet* ps = static_cast<vtkPointSet*>(ds);

  if (this->Points != ps->Points)
  {
    if (this->PointLocator)
    {
      this->PointLocator->Initialize();
    }
    this->SetPoints(ps->Points);

    if (this->CellLocator)
    {
      this->CellLocator->Initialize();
    }
  }
}

//----------------------------------------------------------------------------
void vtkPointSet::Cleanup()
{
  if (this->Points)
  {
    this->Points->UnRegister(this);
    this->Points = nullptr;
  }
}

//----------------------------------------------------------------------------
void vtkPointSet::Initialize()
{
  vtkDataSet::Initialize();

  this->Cleanup();

  if (this->PointLocator)
  {
    this->PointLocator->Initialize();
  }
  if (this->CellLocator)
  {
    this->CellLocator->Initialize();
  }
}

//----------------------------------------------------------------------------
void vtkPointSet::ComputeBounds()
{
  if (this->Points)
  {
    // only depends on tyhis->Points so only check this->Points mtime
    // The generic mtime check includes Field/Cell/PointData also
    // which has no impact on the bounds
    if (this->Points->GetMTime() >= this->ComputeTime)
    {
      const double* bounds = this->Points->GetBounds();
      for (int i = 0; i < 6; i++)
      {
        this->Bounds[i] = bounds[i];
      }
      this->ComputeTime.Modified();
    }
  }
}

//----------------------------------------------------------------------------
vtkMTimeType vtkPointSet::GetMTime()
{
  vtkMTimeType dsTime = vtkDataSet::GetMTime();

  if (this->Points)
  {
    if (this->Points->GetMTime() > dsTime)
    {
      dsTime = this->Points->GetMTime();
    }
  }

  // don't get locator's mtime because its an internal object that cannot be
  // modified directly from outside. Causes problems due to FindCell()
  // SetPoints() method.

  return dsTime;
}

//----------------------------------------------------------------------------
void vtkPointSet::BuildPointLocator()
{
  if (!this->Points)
  {
    return;
  }

  if (!this->PointLocator)
  {
    if (this->Editable || !this->Points->GetData()->HasStandardMemoryLayout())
    {
      this->PointLocator = vtkPointLocator::New();
    }
    else
    {
      this->PointLocator = vtkStaticPointLocator::New();
    }
    this->PointLocator->SetDataSet(this);
  }
  else if (this->Points->GetMTime() > this->PointLocator->GetMTime())
  {
    cout << "Building supplied point locator\n";
    this->PointLocator->SetDataSet(this);
  }

  this->PointLocator->BuildLocator();
}

//----------------------------------------------------------------------------
// Build the cell locator (if needed)
void vtkPointSet::BuildCellLocator()
{
  if (!this->Points)
  {
    return;
  }

  if (!this->CellLocator)
  {
    if (this->Editable || !this->Points->GetData()->HasStandardMemoryLayout())
    {
      this->CellLocator = vtkCellLocator::New();
    }
    else
    {
      this->CellLocator = vtkStaticCellLocator::New();
    }
    this->CellLocator->Register(this);
    this->CellLocator->Delete();
    this->CellLocator->SetDataSet(this);
  }
  else if (this->Points->GetMTime() > this->CellLocator->GetMTime())
  {
    this->CellLocator->SetDataSet(this);
  }

  this->CellLocator->BuildLocator();
}

//----------------------------------------------------------------------------
vtkIdType vtkPointSet::FindPoint(double x[3])
{
  if (!this->Points)
  {
    return -1;
  }

  if (!this->PointLocator)
  {
    this->BuildPointLocator();
  }

  return this->PointLocator->FindClosestPoint(x);
}

//----------------------------------------------------------------------------
// This FindCell() method is based on using a locator (either point or
// cell). In this application, point locators are typically faster to build
// and operate on than cell locator, yet do not always produce the correct
// result. The basic idea is that we find one or more close points to the
// query point, and we assume that one of the cells attached to one of the
// close points contains the query point. However this approach is not 100%
// reliable, in which case a slower cell locator must be used. The algorithm
// below (based on a point locator) uses progressively more complex (and
// expensive) approaches to identify close points near the query point (and
// connected cells). If a point locator approach proves unreliable, then a
// cell locator strategy should be used. Use subclasses of
// vtkFindCellStrategy to control the strategies.
vtkIdType vtkPointSet::FindCell(double x[3], vtkCell* cell, vtkGenericCell* gencell,
  vtkIdType cellId, double tol2, int& subId, double pcoords[3], double* weights)
{
  VTK_CREATE(vtkClosestPointStrategy, strategy);
  strategy->Initialize(this);
  return strategy->FindCell(x, cell, gencell, cellId, tol2, subId, pcoords, weights);
}

//----------------------------------------------------------------------------
vtkIdType vtkPointSet::FindCell(double x[3], vtkCell* cell, vtkIdType cellId, double tol2,
  int& subId, double pcoords[3], double* weights)
{
  return this->FindCell(x, cell, nullptr, cellId, tol2, subId, pcoords, weights);
}

//----------------------------------------------------------------------------
vtkCellIterator* vtkPointSet::NewCellIterator()
{
  vtkPointSetCellIterator* iter = vtkPointSetCellIterator::New();
  iter->SetPointSet(this);
  return iter;
}

//----------------------------------------------------------------------------
void vtkPointSet::Squeeze()
{
  if (this->Points)
  {
    this->Points->Squeeze();
  }
  vtkDataSet::Squeeze();
}

//----------------------------------------------------------------------------
void vtkPointSet::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  vtkGarbageCollectorReport(collector, this->PointLocator, "PointLocator");
  vtkGarbageCollectorReport(collector, this->CellLocator, "CellLocator");
}

//----------------------------------------------------------------------------
unsigned long vtkPointSet::GetActualMemorySize()
{
  unsigned long size = this->vtkDataSet::GetActualMemorySize();
  if (this->Points)
  {
    size += this->Points->GetActualMemorySize();
  }
  return size;
}

//----------------------------------------------------------------------------
void vtkPointSet::ShallowCopy(vtkDataObject* dataObject)
{
  vtkPointSet* pointSet = vtkPointSet::SafeDownCast(dataObject);

  if (pointSet != nullptr)
  {
    this->SetEditable(pointSet->GetEditable());
    this->SetPoints(pointSet->GetPoints());
  }

  // Do superclass
  this->vtkDataSet::ShallowCopy(dataObject);
}

//----------------------------------------------------------------------------
void vtkPointSet::DeepCopy(vtkDataObject* dataObject)
{
  vtkPointSet* pointSet = vtkPointSet::SafeDownCast(dataObject);

  if (pointSet != nullptr)
  {
    this->SetEditable(pointSet->GetEditable());
    vtkPoints* newPoints;
    vtkPoints* pointsToCopy = pointSet->GetPoints();
    if (pointsToCopy)
    {
      newPoints = pointsToCopy->NewInstance();
      newPoints->SetDataType(pointsToCopy->GetDataType());
      newPoints->DeepCopy(pointsToCopy);
    }
    else
    {
      newPoints = vtkPoints::New();
    }
    this->SetPoints(newPoints);
    newPoints->Delete();
  }

  // Do superclass
  this->vtkDataSet::DeepCopy(dataObject);
}

//----------------------------------------------------------------------------
vtkPointSet* vtkPointSet::GetData(vtkInformation* info)
{
  return info ? vtkPointSet::SafeDownCast(info->Get(DATA_OBJECT())) : nullptr;
}

//----------------------------------------------------------------------------
vtkPointSet* vtkPointSet::GetData(vtkInformationVector* v, int i)
{
  return vtkPointSet::GetData(v->GetInformationObject(i));
}

//----------------------------------------------------------------------------
void vtkPointSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Editable: " << (this->Editable ? "true\n" : "false\n");
  os << indent << "Number Of Points: " << this->GetNumberOfPoints() << "\n";
  os << indent << "Point Coordinates: " << this->Points << "\n";
  os << indent << "PointLocator: " << this->PointLocator << "\n";
  os << indent << "CellLocator: " << this->CellLocator << "\n";
}

//----------------------------------------------------------------------------
void vtkPointSet::Register(vtkObjectBase* o)
{
  this->RegisterInternal(o, 1);
}

//----------------------------------------------------------------------------
void vtkPointSet::UnRegister(vtkObjectBase* o)
{
  this->UnRegisterInternal(o, 1);
}
