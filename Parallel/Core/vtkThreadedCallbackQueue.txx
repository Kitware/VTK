/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThreadedCallbackQueue.txx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <tuple>
#include <type_traits>

VTK_ABI_NAMESPACE_BEGIN

//=============================================================================
struct vtkThreadedCallbackQueue::BaseWorker
{
  virtual ~BaseWorker() = default;
  virtual void operator()() = 0;
};

//=============================================================================
template <class FT, class... ArgsT>
class vtkThreadedCallbackQueue::WorkerWrapper : public vtkThreadedCallbackQueue::BaseWorker
{
public:
  template <class FTT, class... ArgsTT>
  WorkerWrapper(FTT&& f, ArgsTT&&... args)
    : Functor(std::forward<FT>(f))
    , Args(std::make_tuple(std::forward<ArgsT>(args)...))
  {
  }

  ~WorkerWrapper() override = default;

  void operator()() override { this->Execute(vtkMakeIntegerSequence<sizeof...(ArgsT)>()); }

private:
  template <std::size_t... Is>
  struct vtkIntegerSequence
  {
  };

  template <std::size_t N, std::size_t... Is>
  struct vtkMakeIntegerSequence : vtkMakeIntegerSequence<N - 1, N - 1, Is...>
  {
  };

  template <std::size_t... Is>
  struct vtkMakeIntegerSequence<0, Is...> : vtkIntegerSequence<Is...>
  {
  };

  template <std::size_t... Is>
  void Execute(vtkIntegerSequence<Is...>)
  {
    this->Functor(std::get<Is>(this->Args)...);
  }

  FT Functor;
  std::tuple<ArgsT...> Args;
};

//-----------------------------------------------------------------------------
template <class FT, class... ArgsT>
void vtkThreadedCallbackQueue::Push(FT&& f, ArgsT&&... args)
{
  {
    std::lock_guard<std::mutex> lock(this->Mutex);

    // We remove referenceness so the tuple holding the arguments is valid
    this->Workers.emplace(new WorkerWrapper<FT, typename std::remove_reference<ArgsT>::type...>(
      std::forward<FT>(f), std::forward<ArgsT>(args)...));
    this->Empty = false;
  }

  this->ConditionVariable.notify_one();
}

VTK_ABI_NAMESPACE_END
