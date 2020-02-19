#include "vtkAdaptiveResampleToImage.h"
#include "vtkBoundingBox.h"
#include "vtkClipDataSet.h"
#include "vtkDataSet.h"
#include "vtkLogger.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSet.h"
#include "vtkRTAnalyticSource.h"

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkMPIController.h"
#else
#include "vtkDummyController.h"
#endif

#include <numeric>
#include <vector>

namespace
{
bool ValidateDataset(vtkPartitionedDataSet* pds, vtkMultiProcessController* controller,
  int numLeaves, const vtkBoundingBox& gbox)
{
  numLeaves = vtkMath::NearestPowerOfTwo(numLeaves);

  const int numParts = static_cast<int>(pds->GetNumberOfPartitions());
  int allParts = numParts;
  controller->AllReduce(&numParts, &allParts, 1, vtkCommunicator::SUM_OP);

  if (allParts != numLeaves)
  {
    vtkLogF(ERROR, "Error: mismatched leaves. expected: %d, got %d", numLeaves, allParts);
    return false;
  }

  // validate all boxes same as the input dataset box.
  double bds[6];
  vtkMath::UninitializeBounds(bds);
  pds->GetBounds(bds);

  vtkBoundingBox bbox, allbbox;
  bbox.SetBounds(bds);
  controller->AllReduce(bbox, allbbox);

  if (allbbox != gbox)
  {
    vtkLogF(ERROR, "Error: mismatched bounds!");
    return false;
  }

  // validate no bounding boxes overlap.
  std::vector<int> parts(controller->GetNumberOfProcesses());
  parts[controller->GetLocalProcessId()] = numParts;

  controller->AllGather(&numParts, &parts[0], 1);

  std::vector<double> local_boxes(6 * numParts);
  for (int cc = 0; cc < numParts; ++cc)
  {
    vtkDataSet::SafeDownCast(pds->GetPartition(cc))->GetBounds(&local_boxes[6 * cc]);
  }
  std::vector<double> boxes(6 * std::accumulate(parts.begin(), parts.end(), 0));

  std::vector<vtkIdType> recvLengths(controller->GetNumberOfProcesses());
  std::vector<vtkIdType> offsets(controller->GetNumberOfProcesses());
  controller->AllGatherV(&local_boxes[0], &boxes[0], static_cast<vtkIdType>(local_boxes.size()),
    &recvLengths[0], &offsets[0]);
  if (controller->GetNumberOfProcesses() == 1)
  {
    boxes = local_boxes;
  }

  for (size_t i = 0; i < (boxes.size()) / 6; ++i)
  {
    const vtkBoundingBox boxA(&boxes[6 * i]);
    for (size_t j = i + 1; j < (boxes.size()) / 6; ++j)
    {
      const vtkBoundingBox boxB(&boxes[6 * j]);
      int overlap = 0;
      for (int dim = 0; dim < 3; ++dim)
      {
        if (boxB.GetMinPoint()[dim] > boxA.GetMinPoint()[dim] &&
          boxB.GetMinPoint()[dim] < boxA.GetMaxPoint()[dim])
        {
          overlap++;
        }
        else if (boxB.GetMaxPoint()[dim] > boxA.GetMinPoint()[dim] &&
          boxB.GetMaxPoint()[dim] < boxA.GetMaxPoint()[dim])
        {
          overlap++;
        }
      }
      if (overlap == 3)
      {
        vtkLogF(ERROR, "Error: boxes overlap!");
        abort();
      }
    }
  }

  return true;
}
}

int TestAdaptiveResampleToImage(int argc, char* argv[])
{
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  vtkMPIController* contr = vtkMPIController::New();
#else
  vtkDummyController* contr = vtkDummyController::New();
#endif
  contr->Initialize(&argc, &argv);
  vtkMultiProcessController::SetGlobalController(contr);

  int status = EXIT_SUCCESS;

  // Create Pipeline
  vtkNew<vtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(0, 63, 0, 63, 0, 63);
  wavelet->SetCenter(16, 16, 16);

  vtkNew<vtkClipDataSet> clip;
  clip->SetInputConnection(wavelet->GetOutputPort());
  clip->SetValue(157);

  vtkNew<vtkAdaptiveResampleToImage> resampler;
  resampler->SetNumberOfImages(4);
  resampler->SetInputConnection(clip->GetOutputPort());
  resampler->SetSamplingDimensions(8, 8, 8);
  resampler->UpdatePiece(contr->GetLocalProcessId(), contr->GetNumberOfProcesses(), 0);

  double bds[6];
  vtkDataSet::SafeDownCast(clip->GetOutputDataObject(0))->GetBounds(bds);
  vtkBoundingBox bbox(bds), allbbox;
  contr->AllReduce(bbox, allbbox);

  if (!ValidateDataset(
        vtkPartitionedDataSet::SafeDownCast(resampler->GetOutputDataObject(0)), contr, 4, allbbox))
  {
    status = EXIT_FAILURE;
  }

  resampler->SetNumberOfImages(6);
  resampler->UpdatePiece(contr->GetLocalProcessId(), contr->GetNumberOfProcesses(), 0);
  if (!ValidateDataset(
        vtkPartitionedDataSet::SafeDownCast(resampler->GetOutputDataObject(0)), contr, 6, allbbox))
  {
    status = EXIT_FAILURE;
  }

  resampler->SetNumberOfImages(3);
  resampler->UpdatePiece(contr->GetLocalProcessId(), contr->GetNumberOfProcesses(), 0);
  if (!ValidateDataset(
        vtkPartitionedDataSet::SafeDownCast(resampler->GetOutputDataObject(0)), contr, 3, allbbox))
  {
    status = EXIT_FAILURE;
  }

  vtkMultiProcessController::SetGlobalController(nullptr);
  contr->Finalize();
  contr->Delete();
  return status;
}
