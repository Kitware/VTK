// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDeflectNormals.h"

#include "vtkArrayDispatch.h"
#include "vtkDataArrayAccessor.h"
#include "vtkDataSet.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSMPTools.h"
#include "vtkVector.h"

VTK_ABI_NAMESPACE_BEGIN
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
  void operator()(VectorArrayT* vectorArray)
  {
    typedef vtkDataArrayAccessor<VectorArrayT> VectorAccessorT;
    VectorAccessorT vectors(vectorArray);
    const double* normal = this->Self->GetUserNormal();
    vtkSMPTools::For(0, vectorArray->GetNumberOfTuples(),
      [this, vectors, normal](vtkIdType begin, vtkIdType end)
      {
        bool isFirst = vtkSMPTools::GetSingleThread();
        for (vtkIdType t = begin; t < end; ++t)
        {
          if (isFirst)
          {
            this->Self->CheckAbort();
          }
          if (this->Self->GetAbortOutput())
          {
            break;
          }
          typename VectorAccessorT::APIType vec[3];
          vectors.Get(t, vec);
          this->ComputeTuple(t, vec, normal);
        }
      });
  }

  template <typename VectorArrayT, typename NormalArrayT>
  void operator()(VectorArrayT* vectorArray, NormalArrayT* normalArray)
  {
    typedef vtkDataArrayAccessor<VectorArrayT> VectorAccessorT;
    VectorAccessorT vectors(vectorArray);
    typedef vtkDataArrayAccessor<NormalArrayT> NormalAccessorT;
    NormalAccessorT normals(normalArray);
    vtkSMPTools::For(0, vectorArray->GetNumberOfTuples(),
      [this, vectors, normals](vtkIdType begin, vtkIdType end)
      {
        bool isFirst = !vtkSMPTools::GetSingleThread();
        for (vtkIdType t = begin; t < end; ++t)
        {
          if (isFirst)
          {
            this->Self->CheckAbort();
          }
          if (this->Self->GetAbortOutput())
          {
            break;
          }
          typename VectorAccessorT::APIType vec[3];
          typename NormalAccessorT::APIType normal[3];
          vectors.Get(t, vec);
          normals.Get(t, normal);
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
      worker(vectors);
    }
  }
  else
  {
    if (!vtkArrayDispatch::Dispatch2ByValueType<R, R>::Execute(vectors, normals, worker))
    {
      worker(vectors, normals);
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
VTK_ABI_NAMESPACE_END
