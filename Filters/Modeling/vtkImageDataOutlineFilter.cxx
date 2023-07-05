// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkImageDataOutlineFilter.h"

#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkDataSet.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkPolyData.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkImageDataOutlineFilter);

//------------------------------------------------------------------------------
vtkImageDataOutlineFilter::vtkImageDataOutlineFilter()
{
  this->GenerateFaces = 0;
  this->OutputPointsPrecision = SINGLE_PRECISION;
}

//------------------------------------------------------------------------------
vtkImageDataOutlineFilter::~vtkImageDataOutlineFilter() = default;

//------------------------------------------------------------------------------
// Core method to produce an oriented vtkImageData outline.
namespace
{

void ProduceOutline(vtkImageData* input, vtkTypeBool genFaces, vtkPoints* points,
  vtkCellArray* lines, vtkCellArray* faces)
{
  // Keep track of inserted point ids
  vtkIdType ptIds[8];

  // Produce the eight vtkImageData points, possibly oriented. Use the
  // corner indices of the volume.
  double x[3];
  int ext[6];
  input->GetExtent(ext);

  input->TransformIndexToPhysicalPoint(ext[0], ext[2], ext[4], x);
  ptIds[0] = points->InsertNextPoint(x);

  input->TransformIndexToPhysicalPoint(ext[1], ext[2], ext[4], x);
  ptIds[1] = points->InsertNextPoint(x);

  input->TransformIndexToPhysicalPoint(ext[0], ext[3], ext[4], x);
  ptIds[2] = points->InsertNextPoint(x);

  input->TransformIndexToPhysicalPoint(ext[1], ext[3], ext[4], x);
  ptIds[3] = points->InsertNextPoint(x);

  input->TransformIndexToPhysicalPoint(ext[0], ext[2], ext[5], x);
  ptIds[4] = points->InsertNextPoint(x);

  input->TransformIndexToPhysicalPoint(ext[1], ext[2], ext[5], x);
  ptIds[5] = points->InsertNextPoint(x);

  input->TransformIndexToPhysicalPoint(ext[0], ext[3], ext[5], x);
  ptIds[6] = points->InsertNextPoint(x);

  input->TransformIndexToPhysicalPoint(ext[1], ext[3], ext[5], x);
  ptIds[7] = points->InsertNextPoint(x);

  // Produce topology, lines plus optional quad faces
  vtkIdType pts[4];

  // Always generate wire edges. This is a historical behavior.
  pts[0] = ptIds[0];
  pts[1] = ptIds[1];
  lines->InsertNextCell(2, pts);
  pts[0] = ptIds[2];
  pts[1] = ptIds[3];
  lines->InsertNextCell(2, pts);
  pts[0] = ptIds[4];
  pts[1] = ptIds[5];
  lines->InsertNextCell(2, pts);
  pts[0] = ptIds[6];
  pts[1] = ptIds[7];
  lines->InsertNextCell(2, pts);
  pts[0] = ptIds[0];
  pts[1] = ptIds[2];
  lines->InsertNextCell(2, pts);
  pts[0] = ptIds[1];
  pts[1] = ptIds[3];
  lines->InsertNextCell(2, pts);
  pts[0] = ptIds[4];
  pts[1] = ptIds[6];
  lines->InsertNextCell(2, pts);
  pts[0] = ptIds[5];
  pts[1] = ptIds[7];
  lines->InsertNextCell(2, pts);
  pts[0] = ptIds[0];
  pts[1] = ptIds[4];
  lines->InsertNextCell(2, pts);
  pts[0] = ptIds[1];
  pts[1] = ptIds[5];
  lines->InsertNextCell(2, pts);
  pts[0] = ptIds[2];
  pts[1] = ptIds[6];
  lines->InsertNextCell(2, pts);
  pts[0] = ptIds[3];
  pts[1] = ptIds[7];
  lines->InsertNextCell(2, pts);

  if (genFaces)
  {
    pts[0] = ptIds[1];
    pts[1] = ptIds[0];
    pts[2] = ptIds[2];
    pts[3] = ptIds[3];
    faces->InsertNextCell(4, pts);
    pts[0] = ptIds[0];
    pts[1] = ptIds[1];
    pts[2] = ptIds[5];
    pts[3] = ptIds[4];
    faces->InsertNextCell(4, pts);
    pts[0] = ptIds[2];
    pts[1] = ptIds[0];
    pts[2] = ptIds[4];
    pts[3] = ptIds[6];
    faces->InsertNextCell(4, pts);
    pts[0] = ptIds[3];
    pts[1] = ptIds[2];
    pts[2] = ptIds[6];
    pts[3] = ptIds[7];
    faces->InsertNextCell(4, pts);
    pts[0] = ptIds[1];
    pts[1] = ptIds[3];
    pts[2] = ptIds[7];
    pts[3] = ptIds[5];
    faces->InsertNextCell(4, pts);
    pts[0] = ptIds[7];
    pts[1] = ptIds[6];
    pts[2] = ptIds[4];
    pts[3] = ptIds[5];
    faces->InsertNextCell(4, pts);
  }
}

} // anonymous

//------------------------------------------------------------------------------
int vtkImageDataOutlineFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output. Differentiate between composite and typical datasets.
  vtkImageData* input = vtkImageData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (input == nullptr)
  {
    vtkErrorMacro(<< "Invalid or missing input");
    return 0;
  }
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDebugMacro(<< "Creating outline");

  // Set the desired precision for the points in the output.
  vtkNew<vtkPoints> pts;
  if (this->OutputPointsPrecision == vtkAlgorithm::DOUBLE_PRECISION)
  {
    pts->SetDataType(VTK_DOUBLE);
  }
  else
  {
    pts->SetDataType(VTK_FLOAT);
  }

  vtkNew<vtkCellArray> lines;
  vtkNew<vtkCellArray> faces;

  // Generate the outline
  ProduceOutline(input, this->GenerateFaces, pts, lines, faces);

  // Define output
  output->SetPoints(pts);
  output->SetLines(lines);

  if (this->GenerateFaces)
  {
    output->SetPolys(faces);
  }

  this->CheckAbort();

  return 1;
}

//------------------------------------------------------------------------------
int vtkImageDataOutlineFilter::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  return 1;
}

//------------------------------------------------------------------------------
void vtkImageDataOutlineFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Generate Faces: " << (this->GenerateFaces ? "On\n" : "Off\n");
  os << indent << "Output Points Precision: " << this->OutputPointsPrecision << "\n";
}
VTK_ABI_NAMESPACE_END
