// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPolyLineSource.h"

#include "vtkCellArray.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPolyLineSource);

//------------------------------------------------------------------------------
vtkPolyLineSource::vtkPolyLineSource()
{
  this->Closed = 0;
}

//------------------------------------------------------------------------------
vtkPolyLineSource::~vtkPolyLineSource() = default;

//------------------------------------------------------------------------------
int vtkPolyLineSource::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  // get the info object
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the output
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

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
  polyLine->InsertNextCell(pointIds);

  output->SetPoints(this->Points);
  output->SetLines(polyLine);

  return 1;
}

//------------------------------------------------------------------------------
void vtkPolyLineSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Closed: " << this->Closed << "\n";
}
VTK_ABI_NAMESPACE_END
