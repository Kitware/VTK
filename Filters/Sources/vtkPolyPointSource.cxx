/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyPointSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPolyPointSource.h"

#include "vtkCellArray.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPolyPointSource);

vtkCxxSetObjectMacro(vtkPolyPointSource, Points, vtkPoints);

//----------------------------------------------------------------------------
vtkPolyPointSource::vtkPolyPointSource()
{
  this->Points = nullptr;

  this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
vtkPolyPointSource::~vtkPolyPointSource()
{
  if (this->Points)
  {
    this->Points->Delete();
  }
}

//----------------------------------------------------------------------------
vtkMTimeType vtkPolyPointSource::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();
  vtkMTimeType time;

  if (this->Points != nullptr)
  {
    time = this->Points->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }

  return mTime;
}

//----------------------------------------------------------------------------
void vtkPolyPointSource::SetNumberOfPoints(vtkIdType numPoints)
{
  if (!this->Points)
  {
    vtkPoints* pts = vtkPoints::New(VTK_DOUBLE);
    this->SetPoints(pts);
    this->Points = pts;
    pts->Delete();
  }

  if (numPoints != this->GetNumberOfPoints())
  {
    this->Points->SetNumberOfPoints(numPoints);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
vtkIdType vtkPolyPointSource::GetNumberOfPoints()
{
  if (this->Points)
  {
    return this->Points->GetNumberOfPoints();
  }

  return 0;
}

//----------------------------------------------------------------------------
void vtkPolyPointSource::Resize(vtkIdType numPoints)
{
  if (!this->Points)
  {
    this->SetNumberOfPoints(numPoints);
  }

  if (numPoints != this->GetNumberOfPoints())
  {
    this->Points->Resize(numPoints);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkPolyPointSource::SetPoint(vtkIdType id, double x, double y, double z)
{
  if (!this->Points)
  {
    return;
  }

  if (id >= this->Points->GetNumberOfPoints())
  {
    vtkErrorMacro(<< "point id " << id << " is larger than the number of points");
    return;
  }

  this->Points->SetPoint(id, x, y, z);
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkPolyPointSource::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  // get the info object
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the output
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkIdType numPoints = this->GetNumberOfPoints();
  vtkSmartPointer<vtkIdList> pointIds = vtkSmartPointer<vtkIdList>::New();
  pointIds->SetNumberOfIds(numPoints);
  for (vtkIdType i = 0; i < numPoints; ++i)
  {
    pointIds->SetId(i, i);
  }

  vtkSmartPointer<vtkCellArray> PolyPoint = vtkSmartPointer<vtkCellArray>::New();
  PolyPoint->InsertNextCell(pointIds);

  output->SetPoints(this->Points);
  output->SetVerts(PolyPoint);

  return 1;
}

//----------------------------------------------------------------------------
void vtkPolyPointSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Points: " << this->Points << "\n";
}
