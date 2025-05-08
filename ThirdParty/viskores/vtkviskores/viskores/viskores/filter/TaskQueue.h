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
#ifndef viskores_filter_TaskQueue_h
#define viskores_filter_TaskQueue_h

#include <queue>

namespace viskores
{
namespace filter
{

template <typename T>
class TaskQueue
{
public:
  TaskQueue() = default;

  //Add a task to the Queue.
  void Push(T&& item)
  {
    std::unique_lock<std::mutex> lock(this->Lock);
    this->Queue.push(item);
  }

  bool HasTasks()
  {
    std::unique_lock<std::mutex> lock(this->Lock);
    return !(this->Queue.empty());
  }

  bool GetTask(T& item)
  {
    std::unique_lock<std::mutex> lock(this->Lock);
    if (this->Queue.empty())
      return false;

    item = this->Queue.front();
    this->Queue.pop();
    return true;
  }

  T Pop()
  {
    T item;
    std::unique_lock<std::mutex> lock(this->Lock);
    if (!this->Queue.empty())
    {
      item = this->Queue.front();
      this->Queue.pop();
    }

    return item;
  }

protected:
  viskores::Id Length()
  {
    std::unique_lock<std::mutex> lock(this->Lock);
    return static_cast<viskores::Id>(this->Queue.size());
  }

private:
  std::mutex Lock;
  std::queue<T> Queue;

  //don't want copies of this
  TaskQueue(const TaskQueue& rhs) = delete;
  TaskQueue& operator=(const TaskQueue& rhs) = delete;
  TaskQueue(TaskQueue&& rhs) = delete;
  TaskQueue& operator=(TaskQueue&& rhs) = delete;
};


class DataSetQueue : public TaskQueue<std::pair<viskores::Id, viskores::cont::DataSet>>
{
public:
  DataSetQueue(const viskores::cont::PartitionedDataSet& input)
  {
    viskores::Id idx = 0;
    for (auto ds : input)
      this->Push(std::make_pair(idx++, std::move(ds)));
  }

  DataSetQueue() {}

  viskores::cont::PartitionedDataSet Get()
  {
    viskores::cont::PartitionedDataSet pds;
    viskores::Id num = this->Length();

    if (num > 0)
    {
      std::vector<viskores::cont::DataSet> dataSets(static_cast<std::size_t>(num));

      //Insert them back in the same order.
      std::pair<viskores::Id, viskores::cont::DataSet> task;
      while (this->GetTask(task))
      {
        dataSets[static_cast<std::size_t>(task.first)] = std::move(task.second);
      }

      pds.AppendPartitions(dataSets);
    }

    return pds;
  }

private:
};

}
}

#endif
