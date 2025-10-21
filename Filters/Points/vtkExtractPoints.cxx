// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkExtractPoints.h"

#include "vtkArrayDispatch.h"
#include "vtkArrayDispatchDataSetArrayList.h"
#include "vtkDataArrayRange.h"
#include "vtkImplicitFunction.h"
#include "vtkObjectFactory.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkSMPTools.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkExtractPoints);
vtkCxxSetObjectMacro(vtkExtractPoints, ImplicitFunction, vtkImplicitFunction);

//------------------------------------------------------------------------------
// Helper classes to support efficient computing, and threaded execution.
namespace
{

//------------------------------------------------------------------------------
// The threaded core of the algorithm
template <typename TArray>
struct ExtractPointsFunctor
{
  TArray* Points;
  vtkImplicitFunction* Function;
  bool ExtractInside;
  vtkIdType* PointMap;

  ExtractPointsFunctor(TArray* points, vtkImplicitFunction* f, bool inside, vtkIdType* map)
    : Points(points)
    , Function(f)
    , ExtractInside(inside)
    , PointMap(map)
  {
  }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    auto p = vtk::DataArrayTupleRange<3>(this->Points, ptId).begin();
    vtkIdType* map = this->PointMap + ptId;
    vtkImplicitFunction* f = this->Function;
    double x[3];
    double inside = (this->ExtractInside ? 1.0 : -1.0);

    for (; ptId < endPtId; ++ptId, ++p)
    {
      p->GetTuple(x);
      *map++ = ((f->FunctionValue(x) * inside) <= 0.0 ? 1 : -1);
    }
  }
}; // ExtractPoints

struct ExtractPointsWorker
{
  template <typename TArray>
  void operator()(TArray* inPts, vtkExtractPoints* self, vtkIdType* pointMap)
  {
    ExtractPointsFunctor<TArray> functor(
      inPts, self->GetImplicitFunction(), self->GetExtractInside(), pointMap);
    vtkSMPTools::For(0, inPts->GetNumberOfTuples(), functor);
  }
};
} // anonymous namespace

//================= Begin class proper =======================================
//------------------------------------------------------------------------------
vtkExtractPoints::vtkExtractPoints()
{
  this->ImplicitFunction = nullptr;
  this->ExtractInside = true;
}

//------------------------------------------------------------------------------
vtkExtractPoints::~vtkExtractPoints()
{
  this->SetImplicitFunction(nullptr);
}

//------------------------------------------------------------------------------
// Overload standard modified time function. If implicit function is modified,
// then this object is modified as well.
vtkMTimeType vtkExtractPoints::GetMTime()
{
  vtkMTimeType mTime = this->MTime.GetMTime();
  vtkMTimeType impFuncMTime;

  if (this->ImplicitFunction != nullptr)
  {
    impFuncMTime = this->ImplicitFunction->GetMTime();
    mTime = (impFuncMTime > mTime ? impFuncMTime : mTime);
  }

  return mTime;
}

//------------------------------------------------------------------------------
// Traverse all the input points and extract points that are contained within
// and implicit function.
int vtkExtractPoints::FilterPoints(vtkPointSet* input)
{
  // Check the input.
  if (!this->ImplicitFunction)
  {
    vtkErrorMacro(<< "Implicit function required\n");
    return 0;
  }

  // Determine which points, if any, should be removed. We create a map
  // to keep track. The bulk of the algorithmic work is done in this pass.
  ExtractPointsWorker worker;
  if (!vtkArrayDispatch::DispatchByArray<vtkArrayDispatch::PointArrays>::Execute(
        input->GetPoints()->GetData(), worker, this, this->PointMap))
  {
    worker(input->GetPoints()->GetData(), this, this->PointMap);
  }

  return 1;
}

//------------------------------------------------------------------------------
void vtkExtractPoints::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Implicit Function: " << static_cast<void*>(this->ImplicitFunction) << "\n";
  os << indent << "Extract Inside: " << (this->ExtractInside ? "On\n" : "Off\n");
}
VTK_ABI_NAMESPACE_END
