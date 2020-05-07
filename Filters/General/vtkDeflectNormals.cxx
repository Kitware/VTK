/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDeflectNormals.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDeflectNormals.h"

#include "vtkArrayDispatch.h"
#include "vtkDataSet.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSMPTools.h"
#include "vtkVector.h"

vtkStandardNewMacro(vtkDeflectNormals);

//------------------------------------------------------------------------------
vtkDeflectNormals::vtkDeflectNormals()
{
  // by default process active point vectors
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::VECTORS);
}

//------------------------------------------------------------------------------
vtkDeflectNormals::~vtkDeflectNormals() = default;

//------------------------------------------------------------------------------
namespace
{

struct vtkDeflectNormalsWorker
{
  vtkDeflectNormals* Self;
  vtkFloatArray* OutNormals;

  vtkDeflectNormalsWorker(vtkDeflectNormals* self, vtkFloatArray* out)
    : Self(self)
    , OutNormals(out)
  {
  }

  template <typename VectorT, typename NormalT>
  VTK_ALWAYS_INLINE void ComputeTuple(
    const vtkIdType index, const VectorT* vec, const NormalT* normal)
  {
    vtkVector3f result;
    for (int c = 0; c < 3; ++c)
    {
      result[c] = static_cast<float>(vec[c] * this->Self->GetScaleFactor() + normal[c]);
    }
    result.Normalize();
    this->OutNormals->SetTypedTuple(index, result.GetData());
  }

  template <typename VectorArrayT>
  void operator()(VectorArrayT* vectors)
  {
    const double* normal = this->Self->GetUserNormal();
    vtkSMPTools::For(0, vectors->GetNumberOfTuples(), [=](vtkIdType begin, vtkIdType end) {
      for (vtkIdType t = begin; t < end; ++t)
      {
        typename VectorArrayT::ValueType vec[3];
        vectors->GetTypedTuple(t, vec);
        this->ComputeTuple(t, vec, normal);
      }
    });
  }

  template <typename VectorArrayT, typename NormalArrayT>
  void operator()(VectorArrayT* vectors, NormalArrayT* normals)
  {
    vtkSMPTools::For(0, vectors->GetNumberOfTuples(), [=](vtkIdType begin, vtkIdType end) {
      for (vtkIdType t = begin; t < end; ++t)
      {
        typename VectorArrayT::ValueType vec[3];
        typename NormalArrayT::ValueType normal[3];
        vectors->GetTypedTuple(t, vec);
        normals->GetTypedTuple(t, normal);
        this->ComputeTuple(t, vec, normal);
      }
    });
  }
};
} // end anon namespace

//------------------------------------------------------------------------------
int vtkDeflectNormals::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataSet* input = vtkDataSet::GetData(inputVector[0]);
  vtkDataSet* output = vtkDataSet::GetData(outputVector);

  if (!input)
  {
    vtkErrorMacro("Invalid or missing input");
    return 0;
  }

  // First, copy the input to the output as a starting point
  output->ShallowCopy(input);

  vtkDataArray* vectors = this->GetInputArrayToProcess(0, inputVector);

  if (!vectors)
  {
    vtkErrorMacro("No array to process");
    return 0;
  }

  vtkNew<vtkFloatArray> deflectedNormals;
  deflectedNormals->SetName("DeflectedNormals");
  deflectedNormals->SetNumberOfComponents(3);
  deflectedNormals->SetNumberOfTuples(vectors->GetNumberOfTuples());

  bool userNormals = this->UseUserNormal;

  vtkDataArray* normals = input->GetPointData()->GetNormals();
  if (!normals)
  {
    vtkWarningMacro("No normals on the dataset, falling to user defined normal: "
      << this->UserNormal[0] << ", " << this->UserNormal[1] << ", " << this->UserNormal[2]);
    userNormals = true;
  }

  vtkDeflectNormalsWorker worker(this, deflectedNormals);
  using R = vtkArrayDispatch::Reals;

  if (userNormals)
  {
    if (!vtkArrayDispatch::DispatchByValueType<R>::Execute(vectors, worker))
    {
      vtkErrorMacro("Dispatch failed using user normal.");
      return 0;
    }
  }
  else
  {
    if (!vtkArrayDispatch::Dispatch2ByValueType<R, R>::Execute(vectors, normals, worker))
    {
      vtkErrorMacro("Dispatch failed using dataset normals.");
      return 0;
    }
  }

  // now set the new normals.
  output->GetPointData()->SetNormals(deflectedNormals);

  return 1;
}

//------------------------------------------------------------------------------
void vtkDeflectNormals::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Scale Factor: " << this->ScaleFactor << "\n";
  os << indent << "Use User Normal: " << this->UseUserNormal << "\n";
  os << indent << "User Normal: " << this->UserNormal[0] << ", " << this->UserNormal[1] << ", "
     << this->UserNormal[2] << "\n";
}
