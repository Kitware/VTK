/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAdaptiveResampleToImage.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAdaptiveResampleToImage.h"

#include "vtkCellData.h"
#include "vtkDIYKdTreeUtilities.h"
#include "vtkDIYUtilities.h"
#include "vtkIdList.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPResampleToImage.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkUnsignedCharArray.h"

// clang-format off
#include "vtk_diy2.h"
#include VTK_DIY2(diy/mpi.hpp)
#include VTK_DIY2(diy/master.hpp)
#include VTK_DIY2(diy/link.hpp)
#include VTK_DIY2(diy/reduce.hpp)
#include VTK_DIY2(diy/reduce-operations.hpp)
#include VTK_DIY2(diy/partners/swap.hpp)
#include VTK_DIY2(diy/assigner.hpp)
#include VTK_DIY2(diy/algorithms.hpp)
// clang-format on

namespace impl
{
vtkUnsignedCharArray* get_mask_array(vtkDataSetAttributes* dsa)
{
  return vtkUnsignedCharArray::SafeDownCast(dsa->GetArray(vtkDataSetAttributes::GhostArrayName()));
}

// Resamples the input dataset to an image dataset spanning the requested
// boundingbox. This method returns null if a non-empty image cannot be
// generated for the given input and bounds.
vtkSmartPointer<vtkImageData> resample(
  const vtkBoundingBox& bbox, vtkDataObject* input, vtkAdaptiveResampleToImage* self)
{
  assert(bbox.IsValid());

  double bds[6];
  bbox.GetBounds(bds);

  vtkNew<vtkPResampleToImage> resampler;
  resampler->SetController(nullptr);
  resampler->SetUseInputBounds(false);
  resampler->SetSamplingDimensions(self->GetSamplingDimensions());
  resampler->SetSamplingBounds(bds);
  resampler->SetInputDataObject(input);
  resampler->Update();
  auto image = resampler->GetOutput();

  auto cellmask = get_mask_array(image->GetCellData());
  auto pointmask = get_mask_array(image->GetPointData());
  if ((static_cast<int>(cellmask->GetRange(0)[0]) & vtkDataSetAttributes::HIDDENCELL) != 0 &&
    (static_cast<int>(pointmask->GetRange(0)[0]) & vtkDataSetAttributes::HIDDENPOINT) != 0)
  {
    // if image has nothing valid, return empty.
    return nullptr;
  }

  return image;
}

vtkSmartPointer<vtkIdList> get_ids(vtkDataSetAttributes* source, unsigned char ghostFlag)
{
  auto mask = get_mask_array(source);
  if (!mask)
  {
    return nullptr;
  }

  vtkNew<vtkIdList> ids;
  ids->Allocate(mask->GetNumberOfTuples());
  for (vtkIdType cc = 0; cc < mask->GetNumberOfTuples(); ++cc)
  {
    if ((mask->GetTypedComponent(cc, 0) & ghostFlag) != ghostFlag)
    {
      ids->InsertNextId(cc);
    }
  }

  return ids->GetNumberOfIds() > 0 ? ids.GetPointer() : nullptr;
}

bool merge(vtkImageData* target, std::vector<vtkSmartPointer<vtkImageData> >& sources)
{
  if (sources.size() == 0)
  {
    return false;
  }

  if (sources.size() == 1)
  {
    target->ShallowCopy(sources[0]);
    return true;
  }

  vtkDataSetAttributesFieldList ptList;
  vtkDataSetAttributesFieldList cellList;
  for (auto& image : sources)
  {
    ptList.IntersectFieldList(image->GetPointData());
    cellList.IntersectFieldList(image->GetCellData());
  }

  target->Initialize();
  target->CopyStructure(sources[0]);

  auto opd = target->GetPointData();
  opd->CopyAllOn();
  opd->CopyAllocate(ptList, target->GetNumberOfPoints());
  opd->SetNumberOfTuples(target->GetNumberOfPoints());
  opd->CopyData(ptList, sources[0]->GetPointData(), 0, 0, target->GetNumberOfPoints(), 0);

  auto ocd = target->GetCellData();
  ocd->CopyAllOn();
  ocd->CopyAllocate(cellList, target->GetNumberOfCells());
  ocd->SetNumberOfTuples(target->GetNumberOfCells());
  ocd->CopyData(cellList, sources[0]->GetCellData(), 0, 0, target->GetNumberOfCells(), 0);

  for (int idx = 1; idx < static_cast<int>(sources.size()); ++idx)
  {
    auto inPD = sources[idx]->GetPointData();
    if (auto ptids = get_ids(inPD, vtkDataSetAttributes::HIDDENPOINT))
    {
      ptList.TransformData(idx, inPD, opd, [&ptids](vtkAbstractArray* in, vtkAbstractArray* out) {
        out->InsertTuples(ptids, ptids, in);
      });
    }

    auto inCD = sources[idx]->GetCellData();
    if (auto cellids = get_ids(inCD, vtkDataSetAttributes::HIDDENCELL))
    {
      cellList.TransformData(
        idx, inCD, ocd, [&cellids](vtkAbstractArray* in, vtkAbstractArray* out) {
          out->InsertTuples(cellids, cellids, in);
        });
    }
  }
  return true;
}
}

vtkStandardNewMacro(vtkAdaptiveResampleToImage);
vtkCxxSetObjectMacro(vtkAdaptiveResampleToImage, Controller, vtkMultiProcessController);
//----------------------------------------------------------------------------
vtkAdaptiveResampleToImage::vtkAdaptiveResampleToImage()
  : Controller(nullptr)
  , NumberOfImages(0)
  , SamplingDimensions{ 64, 64, 64 }
{
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
vtkAdaptiveResampleToImage::~vtkAdaptiveResampleToImage()
{
  this->SetController(nullptr);
}

//----------------------------------------------------------------------------
int vtkAdaptiveResampleToImage::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPartitionedDataSet");
  return 1;
}

//----------------------------------------------------------------------------
int vtkAdaptiveResampleToImage::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  auto inputDO = vtkDataObject::GetData(inputVector[0], 0);

  // get resampling bounds.
  auto controller = this->GetController();
  const int num_partitions = (controller && this->GetNumberOfImages() == 0)
    ? controller->GetNumberOfProcesses()
    : this->GetNumberOfImages();

  vtkLogStartScope(TRACE, "generate-kdtree");
  auto boxes = vtkDIYKdTreeUtilities::GenerateCuts(
    inputDO, std::max(1, num_partitions), /*use_cell_centers=*/false, controller);
  vtkLogEndScope("generate-kdtree");
  // vtkLogF(TRACE, "num-boxes: %d", (int)(boxes.size()));
  // for (const auto& box : boxes)
  // {
  //   double bds[6];
  //   box.GetBounds(bds);
  //   vtkLogF(TRACE, "box: {%f:%f, %f:%f, %f:%f}", bds[0], bds[1], bds[2], bds[3], bds[4], bds[5]);
  // }

  diy::mpi::communicator comm = vtkDIYUtilities::GetCommunicator(this->Controller);
  diy::Master master(
    comm, 1, -1, []() { return static_cast<void*>(vtkImageData::New()); },
    [](void* b) { reinterpret_cast<vtkImageData*>(b)->Delete(); });

  auto assigner = vtkDIYKdTreeUtilities::CreateAssigner(comm, static_cast<int>(boxes.size()));
  diy::RegularDecomposer<diy::DiscreteBounds> decomposer(
    /*dim*/ 1, diy::interval(0, assigner.nblocks() - 1), assigner.nblocks());
  decomposer.decompose(comm.rank(), assigner, master);

  std::vector<std::vector<vtkSmartPointer<vtkImageData> > > resamples(boxes.size());
  vtkLogStartScope(TRACE, "local resample");

  const auto localBounds = vtkDIYUtilities::GetLocalBounds(inputDO);
  std::transform(boxes.begin(), boxes.end(), resamples.begin(),
    [&inputDO, &localBounds, this](const vtkBoundingBox& bbox) {
      std::vector<vtkSmartPointer<vtkImageData> > retval;
      vtkSmartPointer<vtkImageData> img =
        localBounds.Intersects(bbox) ? impl::resample(bbox, inputDO, this) : nullptr;
      if (img)
      {
        retval.push_back(img);
      }
      return retval;
    });
  vtkLogEndScope("local resample");

  vtkLogStartScope(TRACE, "global exchange");
  diy::all_to_all(master, assigner, [&resamples, &comm](vtkImageData*, const diy::ReduceProxy& rp) {
    if (rp.in_link().size() == 0)
    {
      // 1. enqueue
      const auto& out_link = rp.out_link();
      for (int cc = 0, max = out_link.size(); cc < max; ++cc)
      {
        // resample input to image.
        const auto target = out_link.target(cc);
        auto& image_vector = resamples[target.gid];
        if (image_vector.size() > 0 && target.proc != comm.rank())
        {
          // send non-empty data to non-local block only.
          assert(image_vector.size() == 1);
          auto image = image_vector[0];
          rp.enqueue<vtkDataSet*>(target, image);
          // vtkLogF(TRACE, "enqueue for %d", target.gid);
          image_vector.clear(); // free up memory
        }
      }
    }
    else
    {
      // 2. dequeue
      const auto& in_link = rp.in_link();
      for (int cc = 0, max = in_link.size(); cc < max; ++cc)
      {
        const auto source = in_link.target(cc);
        if (rp.incoming(source.gid).size() == 0)
        {
          continue;
        }

        vtkDataSet* ptr = nullptr;
        rp.dequeue<vtkDataSet*>(source, ptr);
        if (ptr)
        {
          // vtkLogF(TRACE, "dequeue from %d", source.gid);
          auto img = vtkImageData::SafeDownCast(ptr);
          assert(img);
          resamples[rp.gid()].push_back(img);
          ptr->Delete();
        }
      }
    }
  });
  vtkLogEndScope("global exchange");

  // remove null images.
  for (auto& image_vector : resamples)
  {
    auto last = std::remove(image_vector.begin(), image_vector.end(), nullptr);
    image_vector.erase(last, image_vector.end());
  }

  auto outputPD = vtkPartitionedDataSet::GetData(outputVector, 0);
  master.foreach (
    [&outputPD, &resamples](vtkImageData* block, const diy::Master::ProxyWithLink& ln) {
      if (impl::merge(block, resamples[ln.gid()]))
      {
        outputPD->SetPartition(outputPD->GetNumberOfPartitions(), block);
      }
    });

  return 1;
}

//----------------------------------------------------------------------------
void vtkAdaptiveResampleToImage::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NumberOfImages: " << this->NumberOfImages << endl;
  os << indent << "SamplingDimensions: " << this->SamplingDimensions[0] << ", "
     << this->SamplingDimensions[1] << ", " << this->SamplingDimensions[2] << endl;
}
