// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkJogglePoints.h"

#include "vtkArrayDispatch.h"
#include "vtkArrayDispatchDataSetArrayList.h"
#include "vtkCellData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkSMPThreadLocal.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkVoronoiCore.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkJogglePoints);

namespace // anonymous
{

// Perform the actual point joggleing depending on mode.
struct JoggleWorker
{
  template <typename InPT, typename OutPT>
  void operator()(
    InPT* inPts, OutPT* outPts, vtkJogglePoints* self, double radius, unsigned char* selection)
  {
    vtkIdType numPts = inPts->GetNumberOfTuples();
    const auto ipts = vtk::DataArrayTupleRange<3>(inPts);
    auto opts = vtk::DataArrayTupleRange<3>(outPts);
    int joggleMode = self->GetJoggle();

    // We use a threshold to test if the data size is small enough
    // to execute the functor serially.
    vtkSMPThreadLocal<vtkVoronoiRandom01Range> LocalGenerator;
    vtkSMPTools::For(0, numPts, 10000,
      [self, joggleMode, radius, selection, &ipts, &opts, &LocalGenerator](
        vtkIdType ptId, vtkIdType endPtId)
      {
        auto& localGen = LocalGenerator.Local();
        bool isFirst = vtkSMPTools::GetSingleThread();
        for (; ptId < endPtId; ++ptId)
        {
          if (isFirst)
          {
            self->CheckAbort();
          }
          if (self->GetAbortOutput())
          {
            break;
          }

          if (!selection || selection[ptId] > 0)
          {
            const auto xi = ipts[ptId];
            auto xo = opts[ptId];

            double x[3];
            x[0] = static_cast<double>(xi[0]);
            x[1] = static_cast<double>(xi[1]);
            x[2] = static_cast<double>(xi[2]);

            localGen.Seed(ptId); // produce invariant output
            switch (joggleMode)
            {
              case vtkJogglePoints::UNCONSTRAINED:
                vtkVoronoiJoggle::JoggleXYZ(x, x, radius, localGen);
                break;
              case vtkJogglePoints::XY_PLANE:
                vtkVoronoiJoggle::JoggleXY(x, x, radius, localGen);
                break;
              case vtkJogglePoints::XZ_PLANE:
                vtkVoronoiJoggle::JoggleXZ(x, x, radius, localGen);
                break;
              case vtkJogglePoints::YZ_PLANE:
                vtkVoronoiJoggle::JoggleYZ(x, x, radius, localGen);
                break;
            }

            xo[0] = x[0];
            xo[1] = x[1];
            xo[2] = x[2];
          } // if selected
        }   // for all points in this batch
      });   // lambda
  }
}; // JoggleWorker

} // anonymous namespace

//------------------------------------------------------------------------------
vtkJogglePoints::vtkJogglePoints()
{
  this->Radius = 0.0001;
  this->RadiusIsAbsolute = false;
  this->Joggle = vtkJogglePoints::UNCONSTRAINED;

  // By default process active point scalars to obtain region ids
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS);
}

//------------------------------------------------------------------------------
vtkJogglePoints::~vtkJogglePoints() = default;

//------------------------------------------------------------------------------
int vtkJogglePoints::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPointSet* input = vtkPointSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPointSet* output = vtkPointSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Set the joggle radius.
  double radius = this->Radius;
  if (!this->RadiusIsAbsolute)
  {
    radius = input->GetLength() * this->Radius;
  }

  // Shallow copy the input to the output. We'll replace the joggleed points
  // as well.
  output->CopyStructure(input);
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());

  vtkPoints* inPts = input->GetPoints();
  vtkNew<vtkPoints> newPts;
  newPts->SetDataType(inPts->GetDataType());
  newPts->SetNumberOfPoints(inPts->GetNumberOfPoints());
  output->SetPoints(newPts);

  // See if the optional joggling selection array is provided.
  unsigned char* selection = nullptr;
  vtkDataArray* s = this->GetInputArrayToProcess(0, inputVector);
  vtkUnsignedCharArray* sArray = vtkUnsignedCharArray::FastDownCast(s);
  if (sArray)
  {
    selection = sArray->GetPointer(0);
  }

  // Dispatch the joggle process. Fastpath for real types, fallback to slower
  // path for non-real types. Note that for Voronoi and Delaunay tessellation,
  // the point data type is expected to be double - but this filter has been
  // generalized in case it is used for other purposes (e.g., randomizing
  // probe points).
  using vtkArrayDispatch::Reals;
  using JoggleDispatch = vtkArrayDispatch::Dispatch2ByValueType<Reals, Reals>;
  JoggleWorker joggleWorker;

  // Finally execute the joggling operation.
  if (!JoggleDispatch::Execute(
        inPts->GetData(), newPts->GetData(), joggleWorker, this, radius, selection))
  { // fallback to slowpath
    joggleWorker(inPts->GetData(), newPts->GetData(), this, radius, selection);
  }

  return 1;
}

//------------------------------------------------------------------------------
void vtkJogglePoints::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Joggle: " << this->Joggle << "\n";
  os << indent << "Radius: " << this->Radius << "\n";
  os << indent << "Radius Is Absolute: " << (this->RadiusIsAbsolute ? "True\n" : "False\n");
}
VTK_ABI_NAMESPACE_END
