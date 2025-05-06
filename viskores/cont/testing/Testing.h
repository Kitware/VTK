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
#ifndef viskores_cont_testing_Testing_h
#define viskores_cont_testing_Testing_h

#include <viskores/cont/EnvironmentTracker.h>
#include <viskores/cont/Error.h>
#include <viskores/cont/Initialize.h>
#include <viskores/cont/internal/OptionParser.h>
#include <viskores/testing/Testing.h>
#include <viskores/thirdparty/diy/Configure.h>

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/CellSetExplicit.h>
#include <viskores/cont/CellSetStructured.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/UncertainCellSet.h>
#include <viskores/cont/UnknownArrayHandle.h>

// Because the testing directory is reserved for test executables and not
// libraries, the viskores_cont_testing module has to put this file in
// viskores/cont/testlib instead of viskores/cont/testing where you normally would
// expect it.
#include <viskores/cont/testlib/viskores_cont_testing_export.h>

#include <sstream>
#include <viskores/thirdparty/diy/diy.h>

// We could, conceivably, use CUDA or Kokkos specific print statements here.
// But we cannot use std::stringstream on device, so for now, we'll just accept
// that on CUDA and Kokkos we print less actionable information.
#if defined(VISKORES_ENABLE_CUDA) || defined(VISKORES_ENABLE_KOKKOS)
#define VISKORES_MATH_ASSERT(condition, message) \
  {                                              \
    if (!(condition))                            \
    {                                            \
      this->RaiseError(message);                 \
    }                                            \
  }
#else
#define VISKORES_MATH_ASSERT(condition, message)                                                   \
  {                                                                                                \
    if (!(condition))                                                                              \
    {                                                                                              \
      std::stringstream ss;                                                                        \
      ss << "\n\tError at " << __FILE__ << ":" << __LINE__ << ":" << __func__ << "\n\t" << message \
         << "\n";                                                                                  \
      this->RaiseError(ss.str().c_str());                                                          \
    }                                                                                              \
  }
#endif

namespace opt = viskores::cont::internal::option;

namespace viskores
{
namespace cont
{
namespace testing
{

struct VISKORES_CONT_TESTING_EXPORT Testing
{
public:
  static VISKORES_CONT std::string GetTestDataBasePath();

  static VISKORES_CONT std::string DataPath(const std::string& filename);

  static VISKORES_CONT std::string GetRegressionTestImageBasePath();

  static VISKORES_CONT std::string RegressionImagePath(const std::string& filename);

  static VISKORES_CONT std::string GetWriteDirBasePath();

  static VISKORES_CONT std::string WriteDirPath(const std::string& filename);

  template <class Func>
  static VISKORES_CONT int ExecuteFunction(Func function)
  {
    try
    {
      function();
    }
    catch (viskores::testing::Testing::TestFailure const& error)
    {
      std::cerr << "Error at " << error.GetFile() << ":" << error.GetLine() << ":"
                << error.GetFunction() << "\n\t" << error.GetMessage() << "\n";
      return 1;
    }
    catch (viskores::cont::Error const& error)
    {
      std::cerr << "Uncaught Viskores exception thrown.\n" << error.GetMessage() << "\n";
      std::cerr << "Stacktrace:\n" << error.GetStackTrace() << "\n";
      return 1;
    }
    catch (std::exception const& error)
    {
      std::cerr << "STL exception throw.\n" << error.what() << "\n";
      return 1;
    }
    catch (...)
    {
      std::cerr << "Unidentified exception thrown.\n";
      return 1;
    }
    return 0;
  }

  template <class Func>
  static VISKORES_CONT int Run(Func function, int& argc, char* argv[])
  {
    std::unique_ptr<viskoresdiy::mpi::environment> env_diy = nullptr;
    if (!viskoresdiy::mpi::environment::initialized())
    {
      env_diy.reset(new viskoresdiy::mpi::environment(argc, argv));
    }

    viskores::cont::Initialize(argc, argv);
    ParseAdditionalTestArgs(argc, argv);

    // Turn on floating point exception trapping where available
    viskores::testing::FloatingPointExceptionTrapEnable();
    return ExecuteFunction(function);
  }

  template <class Func>
  static VISKORES_CONT int RunOnDevice(Func function, int argc, char* argv[])
  {
    auto opts = viskores::cont::InitializeOptions::RequireDevice;
    auto config = viskores::cont::Initialize(argc, argv, opts);
    ParseAdditionalTestArgs(argc, argv);

    return ExecuteFunction([&]() { function(config.Device); });
  }

  template <typename... T>
  static VISKORES_CONT void MakeArgs(int& argc, char**& argv, T&&... args)
  {
    constexpr std::size_t numArgs = sizeof...(args);

    std::array<std::string, numArgs> stringArgs = { { args... } };

    // These static variables are declared as static so that the memory will stick around but won't
    // be reported as a leak.
    static std::array<std::vector<char>, numArgs> vecArgs;
    static std::array<char*, numArgs + 1> finalArgs;
    std::cout << "  starting args:";
    for (std::size_t i = 0; i < numArgs; ++i)
    {
      std::cout << " " << stringArgs[i];
      // Safely copying a C-style string is a PITA
      vecArgs[i].resize(0);
      vecArgs[i].reserve(stringArgs[i].size() + 1);
      for (auto&& c : stringArgs[i])
      {
        vecArgs[i].push_back(c);
      }
      vecArgs[i].push_back('\0');

      finalArgs[i] = vecArgs[i].data();
    }
    finalArgs[numArgs] = nullptr;
    std::cout << std::endl;

    argc = static_cast<int>(numArgs);
    argv = finalArgs.data();
  }

  template <typename... T>
  static VISKORES_CONT void MakeArgsAddProgramName(int& argc, char**& argv, T&&... args)
  {
    MakeArgs(argc, argv, "program-name", args...);
  }

  static void SetEnv(const std::string& var, const std::string& value);

  static void UnsetEnv(const std::string& var);

private:
  static std::string& SetAndGetTestDataBasePath(std::string path = "");

  static std::string& SetAndGetRegressionImageBasePath(std::string path = "");

  static std::string& SetAndGetWriteDirBasePath(std::string path = "");

  // Method to parse the extra arguments given to unit tests
  static VISKORES_CONT void ParseAdditionalTestArgs(int& argc, char* argv[]);
};

} // namespace viskores::cont::testing
} // namespace viskores::cont
} // namespace viskores

//============================================================================
template <typename T1, typename T2, typename StorageTag1, typename StorageTag2>
VISKORES_CONT TestEqualResult
test_equal_ArrayHandles(const viskores::cont::ArrayHandle<T1, StorageTag1>& array1,
                        const viskores::cont::ArrayHandle<T2, StorageTag2>& array2)
{
  TestEqualResult result;

  if (array1.GetNumberOfValues() != array2.GetNumberOfValues())
  {
    result.PushMessage("Arrays have different sizes.");
    return result;
  }

  auto portal1 = array1.ReadPortal();
  auto portal2 = array2.ReadPortal();
  for (viskores::Id i = 0; i < portal1.GetNumberOfValues(); ++i)
  {
    if (!test_equal(portal1.Get(i), portal2.Get(i)))
    {
      result.PushMessage("Values don't match at index " + std::to_string(i));
      break;
    }
  }

  return result;
}

VISKORES_CONT_TESTING_EXPORT TestEqualResult
test_equal_ArrayHandles(const viskores::cont::UnknownArrayHandle& array1,
                        const viskores::cont::UnknownArrayHandle& array2);

namespace detail
{

struct TestEqualCellSet
{
  template <typename CellSetType1, typename CellSetType2>
  void operator()(const CellSetType1& cs1, const CellSetType2& cs2, TestEqualResult& result) const
  {
    // Avoid ambiguous overloads by specifying whether each cell type is known or unknown.
    this->Run(cs1,
              typename viskores::cont::internal::CellSetCheck<CellSetType1>::type{},
              cs2,
              typename viskores::cont::internal::CellSetCheck<CellSetType2>::type{},
              result);
  }

private:
  template <typename ShapeST, typename ConnectivityST, typename OffsetST>
  void Run(const viskores::cont::CellSetExplicit<ShapeST, ConnectivityST, OffsetST>& cs1,
           std::true_type,
           const viskores::cont::CellSetExplicit<ShapeST, ConnectivityST, OffsetST>& cs2,
           std::true_type,
           TestEqualResult& result) const
  {
    viskores::TopologyElementTagCell visitTopo{};
    viskores::TopologyElementTagPoint incidentTopo{};

    if (cs1.GetNumberOfPoints() != cs2.GetNumberOfPoints())
    {
      result.PushMessage("number of points don't match");
      return;
    }

    result = test_equal_ArrayHandles(cs1.GetShapesArray(visitTopo, incidentTopo),
                                     cs2.GetShapesArray(visitTopo, incidentTopo));
    if (!result)
    {
      result.PushMessage("shapes arrays don't match");
      return;
    }

    result = test_equal_ArrayHandles(cs1.GetNumIndicesArray(visitTopo, incidentTopo),
                                     cs2.GetNumIndicesArray(visitTopo, incidentTopo));
    if (!result)
    {
      result.PushMessage("counts arrays don't match");
      return;
    }
    result = test_equal_ArrayHandles(cs1.GetConnectivityArray(visitTopo, incidentTopo),
                                     cs2.GetConnectivityArray(visitTopo, incidentTopo));
    if (!result)
    {
      result.PushMessage("connectivity arrays don't match");
      return;
    }
    result = test_equal_ArrayHandles(cs1.GetOffsetsArray(visitTopo, incidentTopo),
                                     cs2.GetOffsetsArray(visitTopo, incidentTopo));
    if (!result)
    {
      result.PushMessage("offsets arrays don't match");
      return;
    }
  }

  template <viskores::IdComponent DIMENSION>
  void Run(const viskores::cont::CellSetStructured<DIMENSION>& cs1,
           std::true_type,
           const viskores::cont::CellSetStructured<DIMENSION>& cs2,
           std::true_type,
           TestEqualResult& result) const
  {
    if (cs1.GetPointDimensions() != cs2.GetPointDimensions())
    {
      result.PushMessage("point dimensions don't match");
      return;
    }
  }

  template <typename CellSetType>
  void Run(const CellSetType& cs1,
           std::true_type,
           const viskores::cont::UnknownCellSet& cs2,
           std::false_type,
           TestEqualResult& result) const
  {
    if (!cs2.CanConvert<CellSetType>())
    {
      result.PushMessage("types don't match");
      return;
    }
    this->Run(cs1, std::true_type{}, cs2.AsCellSet<CellSetType>(), std::true_type{}, result);
  }

  template <typename CellSetType>
  void Run(const viskores::cont::UnknownCellSet& cs1,
           std::false_type,
           const CellSetType& cs2,
           std::true_type,
           TestEqualResult& result) const
  {
    if (!cs1.CanConvert<CellSetType>())
    {
      result.PushMessage("types don't match");
      return;
    }
    this->Run(cs1.AsCellSet<CellSetType>(), std::true_type{}, cs2, std::true_type{}, result);
  }

  template <typename UnknownCellSetType>
  void Run(const UnknownCellSetType& cs1,
           std::false_type,
           const viskores::cont::UnknownCellSet& cs2,
           std::false_type,
           TestEqualResult& result) const
  {
    viskores::cont::CastAndCall(cs1, *this, cs2, result);
  }
};

} // detail

template <typename CellSet1, typename CellSet2>
inline VISKORES_CONT TestEqualResult test_equal_CellSets(const CellSet1& cellset1,
                                                         const CellSet2& cellset2)
{
  TestEqualResult result;
  detail::TestEqualCellSet{}(cellset1, cellset2, result);
  return result;
}

inline VISKORES_CONT TestEqualResult test_equal_Fields(const viskores::cont::Field& f1,
                                                       const viskores::cont::Field& f2)
{
  TestEqualResult result;

  if (f1.GetName() != f2.GetName())
  {
    result.PushMessage("names don't match");
    return result;
  }

  if (f1.GetAssociation() != f2.GetAssociation())
  {
    result.PushMessage("associations don't match");
    return result;
  }

  result = test_equal_ArrayHandles(f1.GetData(), f2.GetData());
  if (!result)
  {
    result.PushMessage("data doesn't match");
  }

  return result;
}

template <typename CellSetTypes = VISKORES_DEFAULT_CELL_SET_LIST>
inline VISKORES_CONT TestEqualResult test_equal_DataSets(const viskores::cont::DataSet& ds1,
                                                         const viskores::cont::DataSet& ds2,
                                                         CellSetTypes ctypes = CellSetTypes())
{
  TestEqualResult result;
  if (ds1.GetNumberOfCoordinateSystems() != ds2.GetNumberOfCoordinateSystems())
  {
    result.PushMessage("number of coordinate systems don't match");
    return result;
  }
  for (viskores::IdComponent i = 0; i < ds1.GetNumberOfCoordinateSystems(); ++i)
  {
    result = test_equal_ArrayHandles(ds1.GetCoordinateSystem(i).GetData(),
                                     ds2.GetCoordinateSystem(i).GetData());
    if (!result)
    {
      result.PushMessage(std::string("coordinate systems don't match at index ") +
                         std::to_string(i));
      return result;
    }
  }

  result = test_equal_CellSets(ds1.GetCellSet().ResetCellSetList(ctypes),
                               ds2.GetCellSet().ResetCellSetList(ctypes));
  if (!result)
  {
    result.PushMessage(std::string("cellsets don't match"));
    return result;
  }

  if (ds1.GetNumberOfFields() != ds2.GetNumberOfFields())
  {
    result.PushMessage("number of fields don't match");
    return result;
  }
  for (viskores::IdComponent i = 0; i < ds1.GetNumberOfFields(); ++i)
  {
    result = test_equal_Fields(ds1.GetField(i), ds2.GetField(i));
    if (!result)
    {
      result.PushMessage(std::string("fields don't match at index ") + std::to_string(i));
      return result;
    }
  }

  return result;
}

#endif //viskores_cont_internal_Testing_h
