// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPartitionedDataSetCollectionSource.h"

#include "vtkAssume.h"
#include "vtkDataArrayRange.h"
#include "vtkDataAssembly.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkParametricBoy.h"
#include "vtkParametricConicSpiral.h"
#include "vtkParametricCrossCap.h"
#include "vtkParametricDini.h"
#include "vtkParametricEllipsoid.h"
#include "vtkParametricEnneper.h"
#include "vtkParametricFigure8Klein.h"
#include "vtkParametricFunctionSource.h"
#include "vtkParametricKlein.h"
#include "vtkParametricMobius.h"
#include "vtkParametricRoman.h"
#include "vtkParametricSuperToroid.h"
#include "vtkParametricTorus.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSMPTools.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"

#include <algorithm>
#include <numeric>
#include <vector>
#include <vtksys/SystemTools.hxx>

VTK_ABI_NAMESPACE_BEGIN
namespace
{
std::vector<int> GenerateAssignments(int numRanks, int& count)
{
  std::vector<int> partsPerRank(numRanks);
  std::iota(partsPerRank.begin(), partsPerRank.end(), 1);
  count = std::accumulate(partsPerRank.begin(), partsPerRank.end(), 0);
  return partsPerRank;
}

// returns [start, end].
std::pair<int, int> GetRange(int rank, const std::vector<int>& counts)
{
  std::pair<int, int> result(0, counts[0]);
  if (rank == 0)
  {
    return result;
  }
  for (int cc = 1; cc <= rank; ++cc)
  {
    result.first = result.second;
    result.second += counts[cc];
  }
  return result;
}

template <typename ArrayType>
void OffsetPoints(ArrayType* array, const vtkVector3d& delta)
{
  VTK_ASSUME(array->GetNumberOfComponents() == 3);

  using ValueType = vtk::GetAPIType<ArrayType>;

  vtkSMPTools::For(0, array->GetNumberOfTuples(), [&](vtkIdType start, vtkIdType end) {
    ValueType tuple[3];
    for (vtkIdType tidx = start; tidx < end; ++tidx)
    {
      array->GetTypedTuple(tidx, tuple);
      tuple[0] += delta[0];
      tuple[1] += delta[1];
      tuple[2] += delta[2];
      array->SetTypedTuple(tidx, tuple);
    }
  });
}

}

vtkStandardNewMacro(vtkPartitionedDataSetCollectionSource);
//----------------------------------------------------------------------------
vtkPartitionedDataSetCollectionSource::vtkPartitionedDataSetCollectionSource()
  : NumberOfShapes(7)
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkPartitionedDataSetCollectionSource::~vtkPartitionedDataSetCollectionSource() = default;

//------------------------------------------------------------------------------
int vtkPartitionedDataSetCollectionSource::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  auto outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);
  return 1;
}

//----------------------------------------------------------------------------
int vtkPartitionedDataSetCollectionSource::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  auto outInfo = outputVector->GetInformationObject(0);
  auto output = vtkPartitionedDataSetCollection::GetData(outInfo);
  const int piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  const int numPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  vtkNew<vtkParametricFunctionSource> source;
  source->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);

  std::vector<vtkSmartPointer<vtkParametricFunction>> functions;
  functions.emplace_back(vtkSmartPointer<vtkParametricBoy>::New());
  functions.emplace_back(vtkSmartPointer<vtkParametricCrossCap>::New());
  functions.emplace_back(vtkSmartPointer<vtkParametricFigure8Klein>::New());
  functions.emplace_back(vtkSmartPointer<vtkParametricKlein>::New());
  functions.emplace_back(vtkSmartPointer<vtkParametricMobius>::New());
  functions.emplace_back(vtkSmartPointer<vtkParametricRoman>::New());

  const auto firstOrientableSurface = static_cast<int>(functions.size());

  functions.emplace_back(vtkSmartPointer<vtkParametricConicSpiral>::New());
  functions.emplace_back(vtkSmartPointer<vtkParametricDini>::New());
  functions.emplace_back(vtkSmartPointer<vtkParametricEllipsoid>::New());
  functions.emplace_back(vtkSmartPointer<vtkParametricEnneper>::New());
  functions.emplace_back(vtkSmartPointer<vtkParametricSuperToroid>::New());
  functions.emplace_back(vtkSmartPointer<vtkParametricTorus>::New());

  vtkNew<vtkDataAssembly> assembly;
  assembly->SetRootNodeName("Assembly");
  output->SetDataAssembly(assembly);

  auto nonOrientableSurfaces = assembly->AddNode("NonOrientableSurfaces");
  auto orientableSurfaces = assembly->AddNode("OrientableSurfaces");

  for (int idx = 0; idx < this->NumberOfShapes; ++idx)
  {
    auto& function = functions.at(idx);
    function->JoinVOff();
    function->JoinUOff();
    source->SetParametricFunction(function);
    source->SetScalarModeToV();

    const double maxV = function->GetMaximumV();

    int totalParts;
    std::vector<int> counts = ::GenerateAssignments(numPieces, totalParts);
    const double deltaV = maxV / totalParts;
    const auto range = ::GetRange(piece, counts);
    for (int partition = range.first; partition < range.second; ++partition)
    {
      function->SetMinimumV(partition * deltaV);
      function->SetMaximumV((partition + 1) * deltaV);
      vtkLogF(TRACE, "min=%f max=%f", function->GetMinimumV(), function->GetMaximumV());
      source->Update();

      vtkNew<vtkPolyData> clone;
      clone->ShallowCopy(source->GetOutputDataObject(0));

      vtkNew<vtkIntArray> partId;
      partId->SetName("PartitionId");
      partId->SetNumberOfTuples(clone->GetNumberOfPoints());
      partId->FillValue(partition);
      clone->GetPointData()->AddArray(partId);

      vtkNew<vtkIntArray> objectId;
      objectId->SetName("ObjectId");
      objectId->SetNumberOfTuples(clone->GetNumberOfPoints());
      objectId->FillValue(idx);
      clone->GetPointData()->AddArray(objectId);

      ::OffsetPoints(
        vtkDoubleArray::SafeDownCast(clone->GetPoints()->GetData()), vtkVector3d(2.5 * idx, 0, 0));

      output->SetPartition(idx, output->GetNumberOfPartitions(idx), clone);
    }

    output->GetMetaData(idx)->Set(vtkCompositeDataSet::NAME(),
      // offset to remove vtkParametric prefix from the class name.
      vtksys::SystemTools::AddSpaceBetweenCapitalizedWords(
        function->GetClassName() + strlen("vtkParametric"))
        .c_str());
    if (idx < firstOrientableSurface)
    {
      assembly->AddDataSetIndex(nonOrientableSurfaces, idx);
    }
    else
    {
      assembly->AddDataSetIndex(orientableSurfaces, idx);
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkPartitionedDataSetCollectionSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NumberOfShapes: " << this->NumberOfShapes << endl;
}
VTK_ABI_NAMESPACE_END
