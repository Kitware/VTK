/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVectorDot.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkVectorDot.h"

#include "vtkArrayDispatch.h"
#include "vtkDataArrayRange.h"
#include "vtkDataSet.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSMPTools.h"

#include <algorithm>
#include <limits>

vtkStandardNewMacro(vtkVectorDot);

namespace {

template <typename NormArrayT, typename VecArrayT>
struct DotWorker
{
  NormArrayT *Normals;
  VecArrayT *Vectors;
  vtkFloatArray *Scalars;

  vtkSMPThreadLocal<float> LocalMin;
  vtkSMPThreadLocal<float> LocalMax;

  DotWorker(NormArrayT *normals,
            VecArrayT *vectors,
            vtkFloatArray *scalars)
    : Normals{normals}
    , Vectors{vectors}
    , Scalars{scalars}
    , LocalMin{std::numeric_limits<float>::max()}
    , LocalMax{std::numeric_limits<float>::lowest()}
  {
  }

  void operator()(vtkIdType begin, vtkIdType end)
  {
    float &min = this->LocalMin.Local();
    float &max = this->LocalMax.Local();

    // Restrict the iterator ranges to [begin,end)
    const auto normals = vtk::DataArrayTupleRange<3>(this->Normals, begin, end);
    const auto vectors = vtk::DataArrayTupleRange<3>(this->Vectors, begin, end);
    auto scalars = vtk::DataArrayValueRange<1>(this->Scalars, begin, end);

    using NormalConstRef = typename decltype(normals)::ConstTupleReferenceType;
    using VectorConstRef = typename decltype(vectors)::ConstTupleReferenceType;

    auto computeScalars = [&](NormalConstRef n, VectorConstRef v) -> float
    {
      const float s = static_cast<float>(n[0]*v[0] + n[1]*v[1] + n[2]*v[2]);

      min = std::min(min, s);
      max = std::max(max, s);

      return s;
    };

    std::transform(normals.cbegin(), normals.cend(),
                   vectors.cbegin(),
                   scalars.begin(),
                   computeScalars);
  }
};

// Dispatcher entry point:
struct LaunchDotWorker
{
  template <typename NormArrayT, typename VecArrayT>
  void operator()(NormArrayT *normals, VecArrayT *vectors,
                  vtkFloatArray *scalars, float scalarRange[2])
  {
    const vtkIdType numPts = normals->GetNumberOfTuples();

    using Worker = DotWorker<NormArrayT, VecArrayT>;
    Worker worker{normals, vectors, scalars};

    vtkSMPTools::For(0, numPts, worker);

    // Reduce the scalar ranges:
    auto minElem = std::min_element(worker.LocalMin.begin(),
                                    worker.LocalMin.end());
    auto maxElem = std::max_element(worker.LocalMax.begin(),
                                    worker.LocalMax.end());

    // There should be at least one element in the range from worker
    // initialization:
    assert(minElem != worker.LocalMin.end());
    assert(maxElem != worker.LocalMax.end());

    scalarRange[0] = *minElem;
    scalarRange[1] = *maxElem;
  }
};

struct MapWorker
{
  vtkFloatArray *Scalars;
  float InMin;
  float InRange;
  float OutMin;
  float OutRange;

  void operator()(vtkIdType begin, vtkIdType end)
  {
    // Restrict the iterator range to [begin,end)
    auto scalars = vtk::DataArrayValueRange<1>(this->Scalars, begin, end);

    using ScalarRef = typename decltype(scalars)::ReferenceType;

    for (ScalarRef s : scalars)
    {
      // Map from inRange to outRange:
      s = this->OutMin + ((s - this->InMin) / this->InRange) * this->OutRange;
    }
  }
};

} // end anon namespace

//=================================Begin class proper=========================
//----------------------------------------------------------------------------
// Construct object with scalar range (-1,1).
vtkVectorDot::vtkVectorDot()
{
  this->MapScalars = 1;

  this->ScalarRange[0] = -1.0;
  this->ScalarRange[1] = 1.0;

  this->ActualRange[0] = -1.0;
  this->ActualRange[1] = 1.0;
}

//----------------------------------------------------------------------------
// Compute dot product.
//
int vtkVectorDot::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet* output = vtkDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkIdType numPts;
  vtkFloatArray* newScalars;
  vtkDataArray* inNormals;
  vtkDataArray* inVectors;
  vtkPointData *pd = input->GetPointData(), *outPD = output->GetPointData();

  // Initialize
  //
  vtkDebugMacro(<< "Generating vector/normal dot product!");

  // First, copy the input to the output as a starting point
  output->CopyStructure(input);

  if ((numPts = input->GetNumberOfPoints()) < 1)
  {
    vtkErrorMacro(<< "No points!");
    return 1;
  }
  if ((inNormals = pd->GetNormals()) == nullptr)
  {
    vtkErrorMacro(<< "No normals defined!");
    return 1;
  }
  if ((inVectors = pd->GetVectors()) == nullptr)
  {
    vtkErrorMacro(<< "No vectors defined!");
    return 1;
  }

  // Allocate
  //
  newScalars = vtkFloatArray::New();
  newScalars->SetNumberOfTuples(numPts);

  // This is potentiall a two pass algorithm. The first pass computes the dot
  // product and keeps track of min/max scalar values; and the second
  // (optional pass) maps the output into a specified range. Passes two and
  // three are optional.

  // Compute dot product. Use a fast path for double/float:
  using vtkArrayDispatch::Reals;
  using Dispatcher = vtkArrayDispatch::Dispatch2ByValueType<Reals, Reals>;
  LaunchDotWorker dotWorker;

  float aRange[2];
  if (!Dispatcher::Execute(inNormals, inVectors, dotWorker, newScalars, aRange))
  { // fallback to slow path:
    dotWorker(inNormals, inVectors, newScalars, aRange);
  }

  // Update ivars:
  this->ActualRange[0] = static_cast<double>(aRange[0]);
  this->ActualRange[1] = static_cast<double>(aRange[1]);

  // Map if requested:
  if (this->GetMapScalars())
  {
    MapWorker mapWorker{newScalars,
                        aRange[1] - aRange[0],
                        aRange[0],
                        static_cast<float>(this->ScalarRange[1] -
                                           this->ScalarRange[0]),
                        static_cast<float>(this->ScalarRange[0])};

    vtkSMPTools::For(0, newScalars->GetNumberOfValues(), mapWorker);
  }

  // Update self and release memory
  //
  outPD->PassData(input->GetPointData());

  int idx = outPD->AddArray(newScalars);
  outPD->SetActiveAttribute(idx, vtkDataSetAttributes::SCALARS);
  newScalars->Delete();

  return 1;
}

//----------------------------------------------------------------------------
void vtkVectorDot::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "MapScalars: " << (this->MapScalars ? "On\n" : "Off\n");

  os << indent << "Scalar Range: (" << this->ScalarRange[0] << ", " << this->ScalarRange[1]
     << ")\n";

  os << indent << "Actual Range: (" << this->ActualRange[0] << ", " << this->ActualRange[1]
     << ")\n";
}
