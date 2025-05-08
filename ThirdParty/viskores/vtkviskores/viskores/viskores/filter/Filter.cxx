//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <viskores/cont/Algorithm.h>
#include <viskores/cont/Logging.h>
#include <viskores/cont/RuntimeDeviceTracker.h>

#include <viskores/filter/Filter.h>
#include <viskores/filter/TaskQueue.h>

#include <future>

namespace viskores
{
namespace filter
{

namespace
{
void RunFilter(Filter* self,
               viskores::filter::DataSetQueue& input,
               viskores::filter::DataSetQueue& output)
{
  auto& tracker = viskores::cont::GetRuntimeDeviceTracker();
  bool prevVal = tracker.GetThreadFriendlyMemAlloc();
  tracker.SetThreadFriendlyMemAlloc(true);

  std::pair<viskores::Id, viskores::cont::DataSet> task;
  while (input.GetTask(task))
  {
    auto outDS = self->Execute(task.second);
    output.Push(std::make_pair(task.first, std::move(outDS)));
  }

  viskores::cont::Algorithm::Synchronize();
  tracker.SetThreadFriendlyMemAlloc(prevVal);
}

} // anonymous namespace

Filter::Filter()
{
  this->SetActiveCoordinateSystem(0);
}

Filter::~Filter() = default;

bool Filter::CanThread() const
{
  return true;
}

//----------------------------------------------------------------------------
void Filter::SetFieldsToPass(const viskores::filter::FieldSelection& fieldsToPass)
{
  this->FieldsToPass = fieldsToPass;
}

void Filter::SetFieldsToPass(viskores::filter::FieldSelection&& fieldsToPass)
{
  this->FieldsToPass = std::move(fieldsToPass);
}

void Filter::SetFieldsToPass(const viskores::filter::FieldSelection& fieldsToPass,
                             viskores::filter::FieldSelection::Mode mode)
{
  this->FieldsToPass = fieldsToPass;
  this->FieldsToPass.SetMode(mode);
}

VISKORES_CONT void Filter::SetFieldsToPass(std::initializer_list<std::string> fields,
                                           viskores::filter::FieldSelection::Mode mode)
{
  this->SetFieldsToPass(viskores::filter::FieldSelection{ fields, mode });
}

void Filter::SetFieldsToPass(
  std::initializer_list<std::pair<std::string, viskores::cont::Field::Association>> fields,
  viskores::filter::FieldSelection::Mode mode)
{
  this->SetFieldsToPass(viskores::filter::FieldSelection{ fields, mode });
}

void Filter::SetFieldsToPass(const std::string& fieldname,
                             viskores::cont::Field::Association association,
                             viskores::filter::FieldSelection::Mode mode)
{
  this->SetFieldsToPass(viskores::filter::FieldSelection{ fieldname, association, mode });
}


//----------------------------------------------------------------------------
viskores::cont::PartitionedDataSet Filter::DoExecutePartitions(
  const viskores::cont::PartitionedDataSet& input)
{
  viskores::cont::PartitionedDataSet output;

  if (this->GetRunMultiThreadedFilter())
  {
    viskores::filter::DataSetQueue inputQueue(input);
    viskores::filter::DataSetQueue outputQueue;

    viskores::Id numThreads = this->DetermineNumberOfThreads(input);

    //Run 'numThreads' filters.
    std::vector<std::future<void>> futures(static_cast<std::size_t>(numThreads));
    for (std::size_t i = 0; i < static_cast<std::size_t>(numThreads); i++)
    {
      auto f = std::async(
        std::launch::async, RunFilter, this, std::ref(inputQueue), std::ref(outputQueue));
      futures[i] = std::move(f);
    }

    for (auto& f : futures)
      f.get();

    //Get results from the outputQueue.
    output = outputQueue.Get();
  }
  else
  {
    for (const auto& inBlock : input)
    {
      viskores::cont::DataSet outBlock = this->Execute(inBlock);
      output.AppendPartition(outBlock);
    }
  }

  return this->CreateResult(input, output);
}

viskores::cont::DataSet Filter::Execute(const viskores::cont::DataSet& input)
{
  return this->DoExecute(input);
}

viskores::cont::PartitionedDataSet Filter::Execute(const viskores::cont::PartitionedDataSet& input)
{
  VISKORES_LOG_SCOPE(viskores::cont::LogLevel::Perf,
                     "Filter (%d partitions): '%s'",
                     (int)input.GetNumberOfPartitions(),
                     viskores::cont::TypeToString<decltype(*this)>().c_str());

  return this->DoExecutePartitions(input);
}

viskores::cont::DataSet Filter::CreateResult(const viskores::cont::DataSet& inDataSet) const
{
  auto fieldMapper = [](viskores::cont::DataSet& out, const viskores::cont::Field& fieldToPass)
  { out.AddField(fieldToPass); };
  return this->CreateResult(inDataSet, inDataSet.GetCellSet(), fieldMapper);
}

viskores::cont::PartitionedDataSet Filter::CreateResult(
  const viskores::cont::PartitionedDataSet& input,
  const viskores::cont::PartitionedDataSet& resultPartitions) const
{
  auto fieldMapper = [](viskores::cont::PartitionedDataSet& out,
                        const viskores::cont::Field& fieldToPass) { out.AddField(fieldToPass); };
  return this->CreateResult(input, resultPartitions, fieldMapper);
}

viskores::cont::DataSet Filter::CreateResultField(const viskores::cont::DataSet& inDataSet,
                                                  const viskores::cont::Field& resultField) const
{
  viskores::cont::DataSet outDataSet = this->CreateResult(inDataSet);
  outDataSet.AddField(resultField);
  VISKORES_ASSERT(!resultField.GetName().empty());
  VISKORES_ASSERT(outDataSet.HasField(resultField.GetName(), resultField.GetAssociation()));
  return outDataSet;
}

viskores::Id Filter::DetermineNumberOfThreads(const viskores::cont::PartitionedDataSet& input)
{
  viskores::Id numDS = input.GetNumberOfPartitions();

  viskores::Id availThreads = 1;

  auto& tracker = viskores::cont::GetRuntimeDeviceTracker();

  if (tracker.CanRunOn(viskores::cont::DeviceAdapterTagCuda{}))
    availThreads = this->NumThreadsPerGPU;
  else if (tracker.CanRunOn(viskores::cont::DeviceAdapterTagKokkos{}))
  {
    //Kokkos doesn't support threading on the CPU.
#ifdef VISKORES_KOKKOS_CUDA
    availThreads = this->NumThreadsPerGPU;
#else
    availThreads = 1;
#endif
  }
  else if (tracker.CanRunOn(viskores::cont::DeviceAdapterTagSerial{}))
    availThreads = 1;
  else
    availThreads = this->NumThreadsPerCPU;

  viskores::Id numThreads = std::min<viskores::Id>(numDS, availThreads);
  return numThreads;
}

void Filter::ResizeIfNeeded(size_t index_st)
{
  if (this->ActiveFieldNames.size() <= index_st)
  {
    auto oldSize = this->ActiveFieldNames.size();
    this->ActiveFieldNames.resize(index_st + 1);
    this->ActiveFieldAssociation.resize(index_st + 1);
    this->UseCoordinateSystemAsField.resize(index_st + 1);
    this->ActiveCoordinateSystemIndices.resize(index_st + 1);
    for (std::size_t i = oldSize; i <= index_st; ++i)
    {
      this->ActiveFieldAssociation[i] = cont::Field::Association::Any;
      this->UseCoordinateSystemAsField[i] = false;
      this->ActiveCoordinateSystemIndices[i] = 0;
    }
  }
}

} // namespace filter
} // namespace viskores
