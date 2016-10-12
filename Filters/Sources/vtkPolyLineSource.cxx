/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyLineSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPolyLineSource.h"

#include "vtkCellArray.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPolyLineSource);

//----------------------------------------------------------------------------
vtkPolyLineSource::vtkPolyLineSource()
{
  this->Points = NULL;
  this->Closed = 0;

  this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
vtkPolyLineSource::~vtkPolyLineSource()
{
  if (this->Points)
  {
    this->Points->Delete();
  }
}

//----------------------------------------------------------------------------
void vtkPolyLineSource::SetNumberOfPoints(vtkIdType numPoints)
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
vtkIdType vtkPolyLineSource::GetNumberOfPoints()
{
  if (this->Points)
  {
    return this->Points->GetNumberOfPoints();
  }

  return 0;
}

//----------------------------------------------------------------------------
void vtkPolyLineSource::Resize(vtkIdType numPoints)
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
void vtkPolyLineSource::SetPoint(vtkIdType id, double x, double y, double z)
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
void vtkPolyLineSource::SetPoints(vtkPoints* points)
{
  if ( points != this->Points )
  {
    if ( this->Points != NULL )
    {
      this->Points->Delete();
    }
    this->Points = points;
    if ( this->Points != NULL )
    {
      this->Points->Register(this);
    }
    this->Modified();
  }
}

//----------------------------------------------------------------------------
int vtkPolyLineSource::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the ouptut
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkIdType numPoints = this->GetNumberOfPoints();
  vtkSmartPointer<vtkIdList> pointIds = vtkSmartPointer<vtkIdList>::New();
  pointIds->SetNumberOfIds(this->Closed ? numPoints + 1 : numPoints);
  for (vtkIdType i = 0; i < numPoints; ++i)
  {
    pointIds->SetId(i, i);
  }
  if (this->Closed)
  {
    pointIds->SetId(numPoints, 0);
  }

  vtkSmartPointer<vtkCellArray> polyLine = vtkSmartPointer<vtkCellArray>::New();
  polyLine->InsertNextCell(pointIds.GetPointer());

  output->SetPoints(this->Points);
  output->SetLines(polyLine);

  return 1;
}

//----------------------------------------------------------------------------
void vtkPolyLineSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Points: " << this->Points << "\n";
  os << indent << "Closed: " << this->Closed << "\n";
}
