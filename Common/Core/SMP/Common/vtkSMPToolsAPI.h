// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkSMPToolsAPI_h
#define vtkSMPToolsAPI_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkNew.h"
#include "vtkObject.h"
#include "vtkSMP.h"

#include <memory>

#include "SMP/Common/vtkSMPToolsImpl.h"
#if VTK_SMP_ENABLE_SEQUENTIAL
#include "SMP/Sequential/vtkSMPToolsImpl.txx"
#endif
#if VTK_SMP_ENABLE_STDTHREAD
#include "SMP/STDThread/vtkSMPToolsImpl.txx"
#endif
#if VTK_SMP_ENABLE_TBB
#include "SMP/TBB/vtkSMPToolsImpl.txx"
#endif
#if VTK_SMP_ENABLE_OPENMP
#include "SMP/OpenMP/vtkSMPToolsImpl.txx"
#endif

namespace vtk
{
namespace detail
{
namespace smp
{
VTK_ABI_NAMESPACE_BEGIN

using vtkSMPToolsDefaultImpl = vtkSMPToolsImpl<DefaultBackend>;

class VTKCOMMONCORE_EXPORT vtkSMPToolsAPI
{
public:
  //--------------------------------------------------------------------------------
  static vtkSMPToolsAPI& GetInstance();

  //--------------------------------------------------------------------------------
  BackendType GetBackendType();

  //--------------------------------------------------------------------------------
  const char* GetBackend();

  //--------------------------------------------------------------------------------
  bool SetBackend(const char* type);

  //--------------------------------------------------------------------------------
  void Initialize(int numThreads = 0);

  //--------------------------------------------------------------------------------
  int GetEstimatedNumberOfThreads();

  //--------------------------------------------------------------------------------
  int GetEstimatedDefaultNumberOfThreads();

  //------------------------------------------------------------------------------
  void SetNestedParallelism(bool isNested);

  //--------------------------------------------------------------------------------
  bool GetNestedParallelism();

  //--------------------------------------------------------------------------------
  bool IsParallelScope();

  //--------------------------------------------------------------------------------
  bool GetSingleThread();

  //--------------------------------------------------------------------------------
  int GetInternalDesiredNumberOfThread() { return this->DesiredNumberOfThread; }

  //------------------------------------------------------------------------------
  template <typename Config, typename T>
  void LocalScope(Config const& config, T&& lambda)
  {
    const Config oldConfig(*this);
    *this << config;
    try
    {
      lambda();
    }
    catch (...)
    {
      *this << oldConfig;
      throw;
    }
    *this << oldConfig;
  }

  //--------------------------------------------------------------------------------
  template <typename FunctorInternal>
  void For(vtkIdType first, vtkIdType last, vtkIdType grain, FunctorInternal& fi)
  {
    switch (this->ActivatedBackend)
    {
      case BackendType::Sequential:
        this->SequentialBackend->For(first, last, grain, fi);
        break;
      case BackendType::STDThread:
        this->STDThreadBackend->For(first, last, grain, fi);
        break;
      case BackendType::TBB:
        this->TBBBackend->For(first, last, grain, fi);
        break;
      case BackendType::OpenMP:
        this->OpenMPBackend->For(first, last, grain, fi);
        break;
    }
  }

  //--------------------------------------------------------------------------------
  template <typename InputIt, typename OutputIt, typename Functor>
  void Transform(InputIt inBegin, InputIt inEnd, OutputIt outBegin, Functor& transform)
  {
    switch (this->ActivatedBackend)
    {
      case BackendType::Sequential:
        this->SequentialBackend->Transform(inBegin, inEnd, outBegin, transform);
        break;
      case BackendType::STDThread:
        this->STDThreadBackend->Transform(inBegin, inEnd, outBegin, transform);
        break;
      case BackendType::TBB:
        this->TBBBackend->Transform(inBegin, inEnd, outBegin, transform);
        break;
      case BackendType::OpenMP:
        this->OpenMPBackend->Transform(inBegin, inEnd, outBegin, transform);
        break;
    }
  }

  //--------------------------------------------------------------------------------
  template <typename InputIt1, typename InputIt2, typename OutputIt, typename Functor>
  void Transform(
    InputIt1 inBegin1, InputIt1 inEnd, InputIt2 inBegin2, OutputIt outBegin, Functor& transform)
  {
    switch (this->ActivatedBackend)
    {
      case BackendType::Sequential:
        this->SequentialBackend->Transform(inBegin1, inEnd, inBegin2, outBegin, transform);
        break;
      case BackendType::STDThread:
        this->STDThreadBackend->Transform(inBegin1, inEnd, inBegin2, outBegin, transform);
        break;
      case BackendType::TBB:
        this->TBBBackend->Transform(inBegin1, inEnd, inBegin2, outBegin, transform);
        break;
      case BackendType::OpenMP:
        this->OpenMPBackend->Transform(inBegin1, inEnd, inBegin2, outBegin, transform);
        break;
    }
  }

  //--------------------------------------------------------------------------------
  template <typename Iterator, typename T>
  void Fill(Iterator begin, Iterator end, const T& value)
  {
    switch (this->ActivatedBackend)
    {
      case BackendType::Sequential:
        this->SequentialBackend->Fill(begin, end, value);
        break;
      case BackendType::STDThread:
        this->STDThreadBackend->Fill(begin, end, value);
        break;
      case BackendType::TBB:
        this->TBBBackend->Fill(begin, end, value);
        break;
      case BackendType::OpenMP:
        this->OpenMPBackend->Fill(begin, end, value);
        break;
    }
  }

  //--------------------------------------------------------------------------------
  template <typename RandomAccessIterator>
  void Sort(RandomAccessIterator begin, RandomAccessIterator end)
  {
    switch (this->ActivatedBackend)
    {
      case BackendType::Sequential:
        this->SequentialBackend->Sort(begin, end);
        break;
      case BackendType::STDThread:
        this->STDThreadBackend->Sort(begin, end);
        break;
      case BackendType::TBB:
        this->TBBBackend->Sort(begin, end);
        break;
      case BackendType::OpenMP:
        this->OpenMPBackend->Sort(begin, end);
        break;
    }
  }

  //--------------------------------------------------------------------------------
  template <typename RandomAccessIterator, typename Compare>
  void Sort(RandomAccessIterator begin, RandomAccessIterator end, Compare comp)
  {
    switch (this->ActivatedBackend)
    {
      case BackendType::Sequential:
        this->SequentialBackend->Sort(begin, end, comp);
        break;
      case BackendType::STDThread:
        this->STDThreadBackend->Sort(begin, end, comp);
        break;
      case BackendType::TBB:
        this->TBBBackend->Sort(begin, end, comp);
        break;
      case BackendType::OpenMP:
        this->OpenMPBackend->Sort(begin, end, comp);
        break;
    }
  }

  // disable copying
  vtkSMPToolsAPI(vtkSMPToolsAPI const&) = delete;
  void operator=(vtkSMPToolsAPI const&) = delete;

protected:
  //--------------------------------------------------------------------------------
  // Address the static initialization order 'fiasco' by implementing
  // the schwarz counter idiom.
  static void ClassInitialize();
  static void ClassFinalize();
  friend class vtkSMPToolsAPIInitialize;

private:
  //--------------------------------------------------------------------------------
  vtkSMPToolsAPI();

  //--------------------------------------------------------------------------------
  void RefreshNumberOfThread();

  //--------------------------------------------------------------------------------
  // This operator overload is used to unpack Config parameters and set them
  // in vtkSMPToolsAPI (e.g `*this << config;`)
  template <typename Config>
  vtkSMPToolsAPI& operator<<(Config const& config)
  {
    this->Initialize(config.MaxNumberOfThreads);
    this->SetBackend(config.Backend.c_str());
    this->SetNestedParallelism(config.NestedParallelism);
    return *this;
  }

  /**
   * Indicate which backend to use.
   */
  BackendType ActivatedBackend = DefaultBackend;

  /**
   * Max threads number
   */
  int DesiredNumberOfThread = 0;

  /**
   * Sequential backend
   */
#if VTK_SMP_ENABLE_SEQUENTIAL
  std::unique_ptr<vtkSMPToolsImpl<BackendType::Sequential>> SequentialBackend;
#else
  std::unique_ptr<vtkSMPToolsDefaultImpl> SequentialBackend;
#endif

  /**
   * STDThread backend
   */
#if VTK_SMP_ENABLE_STDTHREAD
  std::unique_ptr<vtkSMPToolsImpl<BackendType::STDThread>> STDThreadBackend;
#else
  std::unique_ptr<vtkSMPToolsDefaultImpl> STDThreadBackend;
#endif

  /**
   * TBB backend
   */
#if VTK_SMP_ENABLE_TBB
  std::unique_ptr<vtkSMPToolsImpl<BackendType::TBB>> TBBBackend;
#else
  std::unique_ptr<vtkSMPToolsDefaultImpl> TBBBackend;
#endif

  /**
   * TBB backend
   */
#if VTK_SMP_ENABLE_OPENMP
  std::unique_ptr<vtkSMPToolsImpl<BackendType::OpenMP>> OpenMPBackend;
#else
  std::unique_ptr<vtkSMPToolsDefaultImpl> OpenMPBackend;
#endif
};

//--------------------------------------------------------------------------------
class VTKCOMMONCORE_EXPORT vtkSMPToolsAPIInitialize
{
public:
  vtkSMPToolsAPIInitialize();
  ~vtkSMPToolsAPIInitialize();
};

//--------------------------------------------------------------------------------
// This instance will show up in any translation unit that uses vtkSMPToolsAPI singleton.
// It will make sure vtkSMPToolsAPI is initialized before it is used and finalized when it
// is done being used.
static vtkSMPToolsAPIInitialize vtkSMPToolsAPIInitializer;

VTK_ABI_NAMESPACE_END
} // namespace smp
} // namespace detail
} // namespace vtk

#endif
/* VTK-HeaderTest-Exclude: vtkSMPToolsAPI.h */
