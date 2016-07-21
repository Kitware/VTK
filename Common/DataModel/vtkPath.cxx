/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPath.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPath.h"

#include "vtkGenericCell.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkIntArray.h"

#include <cassert>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPath)

//----------------------------------------------------------------------------
vtkPath::vtkPath()
{
  vtkNew<vtkPoints> points;
  this->SetPoints(points.GetPointer());

  vtkNew<vtkIntArray> controlPointCodes;
  controlPointCodes->SetNumberOfComponents(1);
  this->PointData->SetScalars(controlPointCodes.GetPointer());
}

//----------------------------------------------------------------------------
vtkPath::~vtkPath()
{
}

//----------------------------------------------------------------------------
void vtkPath::Allocate(vtkIdType size, int extSize)
{
  this->Points->Allocate(size, extSize);
  this->PointData->Allocate(size, extSize);
}

//----------------------------------------------------------------------------
void vtkPath::GetCell(vtkIdType, vtkGenericCell *cell)
{
  cell->SetCellTypeToEmptyCell();
}

//----------------------------------------------------------------------------
void vtkPath::GetCellPoints(vtkIdType, vtkIdList *ptIds)
{
  ptIds->Reset();
}

//----------------------------------------------------------------------------
void vtkPath::GetPointCells(vtkIdType, vtkIdList *cellIds)
{
  cellIds->Reset();
}

//----------------------------------------------------------------------------
void vtkPath::Reset()
{
  this->Points->Reset();
  this->PointData->Reset();
}

//----------------------------------------------------------------------------
vtkPath* vtkPath::GetData(vtkInformation* info)
{
  return info ? vtkPath::SafeDownCast(info->Get(DATA_OBJECT())) : 0;
}

//----------------------------------------------------------------------------
vtkPath* vtkPath::GetData(vtkInformationVector* v, int i)
{
  return vtkPath::GetData(v->GetInformationObject(i));
}

//----------------------------------------------------------------------------
void vtkPath::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkPath::InsertNextPoint(float pts[], int code)
{
  this->Points->InsertNextPoint(pts);

  vtkIntArray *codes = vtkArrayDownCast<vtkIntArray>(
        this->PointData->GetScalars());
  assert("control point code array is int type" && codes);
  codes->InsertNextValue(code);
}

//----------------------------------------------------------------------------
void vtkPath::InsertNextPoint(double pts[], int code)
{
  this->InsertNextPoint(pts[0], pts[1], pts[2], code);
}

//----------------------------------------------------------------------------
void vtkPath::InsertNextPoint(double x, double y, double z, int code)
{
  this->Points->InsertNextPoint(x, y, z);

  vtkIntArray *codes = vtkArrayDownCast<vtkIntArray>(
        this->PointData->GetScalars());
  assert("control point code array is int type" && codes);
  codes->InsertNextValue(code);
}

//----------------------------------------------------------------------------
void vtkPath::SetCodes(vtkIntArray *codes)
{
  this->PointData->SetScalars(codes);
}

//----------------------------------------------------------------------------
vtkIntArray *vtkPath::GetCodes()
{
  return vtkArrayDownCast<vtkIntArray>(this->PointData->GetScalars());
}
