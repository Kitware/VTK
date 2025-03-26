// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkmDataArray.h"

#include "vtkDoubleArray.h"
#include "vtkIntArray.h"
#include "vtkSmartPointer.h"
#include "vtkUnsignedCharArray.h"

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>

#include <chrono>
#include <limits>
#include <random>
#include <type_traits>
#include <vector>

namespace
{

//------------------------------------------------------------------------------
class TestError
{
public:
  TestError(const std::string& message, int line)
    : Message(message)
    , Line(line)
  {
  }

  void PrintMessage(std::ostream& out) const
  {
    out << "Error at line " << this->Line << ": " << this->Message << "\n";
  }

private:
  std::string Message;
  int Line;
};

#define RAISE_TEST_ERROR(msg) throw TestError((msg), __LINE__)

#define TEST_VERIFY(cond, msg)                                                                     \
  if (!(cond))                                                                                     \
  RAISE_TEST_ERROR((msg))

inline bool IsEqualFloat(double a, double b, double e = 1e-6f)
{
  return (std::abs(a - b) <= e);
}

template <typename BaseComponentType>
void FlattenVecImpl(const BaseComponentType& vec, std::vector<BaseComponentType>& cppvec)
{
  cppvec.push_back(vec);
}

template <typename VecType, typename BaseComponentType>
void FlattenVecImpl(const VecType& vec, std::vector<BaseComponentType>& cppvec)
{
  for (vtkm::IdComponent i = 0; i < vtkm::VecTraits<VecType>::GetNumberOfComponents(vec); ++i)
  {
    FlattenVecImpl(vec[i], cppvec);
  }
}

template <typename VecType>
auto FlattenVec(const VecType& vec)
{
  std::vector<typename vtkm::VecTraits<VecType>::BaseComponentType> cppvec;
  cppvec.reserve(9); // avoid clang-tidy warning about `push_back` inside a loop
  FlattenVecImpl(vec, cppvec);
  return cppvec;
}

//------------------------------------------------------------------------------
template <typename ArrayHandleType>
void TestWithArrayHandle(const ArrayHandleType& vtkmArray)
{
  vtkSmartPointer<vtkDataArray> vtkArray;
  vtkArray.TakeReference(make_vtkmDataArray(vtkmArray));

  auto vtkmPortal = vtkmArray.ReadPortal();

  vtkIdType length = vtkArray->GetNumberOfTuples();
  std::cout << "Length: " << length << "\n";
  TEST_VERIFY(length == vtkmArray.GetNumberOfValues(), "Array lengths don't match");

  int numberOfComponents = vtkArray->GetNumberOfComponents();
  std::cout << "Number of components: " << numberOfComponents << "\n";
  TEST_VERIFY(numberOfComponents == static_cast<int>(FlattenVec(vtkmPortal.Get(0)).size()),
    "Number of components don't match");

  for (vtkIdType i = 0; i < length; ++i)
  {
    std::vector<double> tuple(numberOfComponents);
    vtkArray->GetTuple(i, tuple.data());
    auto vec = FlattenVec(vtkmPortal.Get(i));
    for (int j = 0; j < numberOfComponents; ++j)
    {
      TEST_VERIFY(IsEqualFloat(tuple[j], static_cast<double>(vec[j])), "values don't match");
      TEST_VERIFY(IsEqualFloat(vtkArray->GetComponent(i, j), static_cast<double>(vec[j])),
        "values don't match");
    }
  }
}

//------------------------------------------------------------------------------
template <typename T, bool IsInteger = std::is_integral<T>::value>
struct UniformDistribution;

template <typename T>
struct UniformDistribution<T, true>
{
  using Type = std::uniform_int_distribution<T>;
};

template <typename T>
struct UniformDistribution<T, false>
{
  using Type = std::uniform_real_distribution<T>;
};

inline auto GetRandomEngine()
{
  static auto seed = std::chrono::steady_clock::now().time_since_epoch().count();
  static std::default_random_engine randomEngine(seed);
  static auto& os = std::cout << "random seed = " << seed << "\n";
  (void)os;

  return randomEngine;
}

template <typename T>
vtkSmartPointer<vtkDataArray> GenerateRandomVtkDataArray(
  int numberOfTuples, int numberOfComponents, const T between[2])
{
  auto randomEngine = GetRandomEngine();
  auto distribution = typename UniformDistribution<T>::Type{ between[0], between[1] };
  auto array = vtkSmartPointer<vtkAOSDataArrayTemplate<T>>::New();

  array->SetNumberOfComponents(numberOfComponents);
  array->SetNumberOfTuples(numberOfTuples);
  std::vector<T> tuple(numberOfComponents); // pre-allocate to avoid clang-tidy warning
  for (int i = 0; i < numberOfTuples; ++i)
  {
    tuple.clear();
    for (int j = 0; j < numberOfComponents; ++j)
    {
      tuple.push_back(distribution(randomEngine));
    }
    array->SetTypedTuple(i, tuple.data());
  }

  return array;
}

template <typename T>
void AddNonFinites(vtkAOSDataArrayTemplate<T>* array)
{
  auto randomEngine = GetRandomEngine();
  auto dt = std::uniform_int_distribution<int>(0, array->GetNumberOfTuples() - 1);
  auto dc = std::uniform_int_distribution<int>(0, array->GetNumberOfComponents() - 1);
  array->SetTypedComponent(dt(randomEngine), dc(randomEngine), std::numeric_limits<T>::infinity());
  array->SetTypedComponent(dt(randomEngine), dc(randomEngine), -std::numeric_limits<T>::infinity());
}

template <typename T>
void PrintArray(vtkAOSDataArrayTemplate<T>* array)
{
  array->Print(std::cout);
  std::cout << "Values: ";
  for (int i = 0; i < array->GetNumberOfTuples(); ++i)
  {
    std::cout << "(" << array->GetTypedComponent(i, 0);
    for (int j = 1; j < array->GetNumberOfComponents(); ++j)
    {
      std::cout << ", " << array->GetTypedComponent(i, j);
    }
    std::cout << ") ";
  }
  std::cout << "\n";
}

template <typename T>
void PrintArray(vtkmDataArray<T>* array)
{
  array->GetVtkmUnknownArrayHandle().PrintSummary(std::cout, true);
}

#define PRINT_RANGE_TYPE(index)                                                                    \
  if (index == -1)                                                                                 \
  {                                                                                                \
    std::cout << "\t\t\tVectorRange\n";                                                            \
  }                                                                                                \
  else                                                                                             \
  {                                                                                                \
    std::cout << "\t\t\tScalarRange, Component: " << index << "\n";                                \
  }

template <typename T>
void TestComputeRange(int numberOfTuples, int numberOfComponents, const T between[2])
{
  std::cout << "\tTesting array with " << numberOfTuples << " tuples and " << numberOfComponents
            << " components\n";

  auto vtkArray = GenerateRandomVtkDataArray(numberOfTuples, numberOfComponents, between);
  vtkSmartPointer<vtkDataArray> vtkmArray(vtkmDataArray<T>::New());
  vtkmArray->FastDelete();
  vtkmArray->DeepCopy(vtkArray); // internally allocates and performs a typed copy

  // `uniform_int_distribution` for char types is not supported by some compilers.
  // So, we generate random ghost values in `int` and then copy it to an `unsigned char` array
  auto ghosts = vtkSmartPointer<vtkUnsignedCharArray>::New();
  {
    int ghostsBetween[] = { 0, 1 };
    auto tmp = GenerateRandomVtkDataArray(numberOfTuples, 1, ghostsBetween);
    ghosts->DeepCopy(tmp);
  }
  const auto* ghostsPtr = ghosts->GetPointer(0);

  double vtkRange[2], vtkmRange[2];

  try
  {
    std::cout << "\t\tTesting GetRange w/o ghosts:\n";

    for (int i = -1; i < numberOfComponents; ++i)
    {
      PRINT_RANGE_TYPE(i)
      vtkArray->GetRange(vtkRange, i);
      vtkmArray->GetRange(vtkmRange, i);
      TEST_VERIFY(IsEqualFloat(vtkRange[0], vtkmRange[0]), "range min doesn't match");
      TEST_VERIFY(IsEqualFloat(vtkRange[1], vtkmRange[1]), "range max doesn't match");
    }

    std::cout << "\t\tTesting GetRange w/ ghosts:\n";
    for (int i = -1; i < numberOfComponents; ++i)
    {
      PRINT_RANGE_TYPE(i)
      vtkArray->GetRange(vtkRange, i, ghostsPtr, 1);
      vtkmArray->GetRange(vtkmRange, i, ghostsPtr, 1);
      TEST_VERIFY(IsEqualFloat(vtkRange[0], vtkmRange[0]), "range min doesn't match");
      TEST_VERIFY(IsEqualFloat(vtkRange[1], vtkmRange[1]), "range max doesn't match");
    }

    if (std::is_floating_point<T>::value)
    {
      AddNonFinites(vtkAOSDataArrayTemplate<T>::SafeDownCast(vtkArray.Get()));
      vtkmArray->DeepCopy(vtkArray);

      std::cout << "\t\tTesting GetFiniteRange w/o ghosts:\n";
      for (int i = -1; i < numberOfComponents; ++i)
      {
        PRINT_RANGE_TYPE(i)
        vtkArray->GetFiniteRange(vtkRange, i);
        vtkmArray->GetFiniteRange(vtkmRange, i);
        TEST_VERIFY(IsEqualFloat(vtkRange[0], vtkmRange[0]), "range min doesn't match");
        TEST_VERIFY(IsEqualFloat(vtkRange[1], vtkmRange[1]), "range max doesn't match");
      }

      std::cout << "\t\tTesting GetFiniteRange w/ ghosts:\n";
      for (int i = -1; i < numberOfComponents; ++i)
      {
        PRINT_RANGE_TYPE(i)
        vtkArray->GetFiniteRange(vtkRange, i, ghostsPtr, 1);
        vtkmArray->GetFiniteRange(vtkmRange, i, ghostsPtr, 1);
        TEST_VERIFY(IsEqualFloat(vtkRange[0], vtkmRange[0]), "range min doesn't match");
        TEST_VERIFY(IsEqualFloat(vtkRange[1], vtkmRange[1]), "range max doesn't match");
      }
    }
  }
  catch (const TestError&)
  {
    auto printableGhosts = vtkSmartPointer<vtkIntArray>::New();
    printableGhosts->DeepCopy(ghosts.Get());

    std::cout << "VTK Array: \n";
    PrintArray(vtkAOSDataArrayTemplate<T>::SafeDownCast(vtkArray.Get()));
    std::cout << "VTK-m Array: \n";
    PrintArray(vtkmDataArray<T>::SafeDownCast(vtkmArray.Get()));
    std::cout << "Ghosts: \n";
    PrintArray(printableGhosts.Get());
    std::cout << "VTK Range: " << vtkRange[0] << ", " << vtkRange[1] << "\n";
    std::cout << "VTK-m Range: " << vtkmRange[0] << ", " << vtkmRange[1] << "\n";
    throw;
  }
}

#undef PRINT_RANGE_TYPE

} // anonymous namespace

//------------------------------------------------------------------------------
int TestVTKMDataArray(int, char*[])
try
{
  static const std::vector<double> testData = { 3.0, 6.0, 2.0, 5.0, 1.0, 0.0, 4.0, 9.0, 8.0, 7.0,
    10.0, 11.0 };

  std::cout << "Testing with Basic ArrayHandle\n";
  TestWithArrayHandle(vtkm::cont::make_ArrayHandle(testData, vtkm::CopyFlag::Off));
  std::cout << "Passed\n";

  std::cout << "Testing with ArrayHandleConstant\n";
  TestWithArrayHandle(vtkm::cont::make_ArrayHandleConstant(
    vtkm::Vec<vtkm::Vec<float, 3>, 3>{ { 1.0f, 2.0f, 3.0f } }, 10));
  std::cout << "Passed\n";

  std::cout << "Testing with ArrayHandleUniformPointCoordinates\n";
  TestWithArrayHandle(vtkm::cont::ArrayHandleUniformPointCoordinates(vtkm::Id3{ 3 }));
  std::cout << "Passed\n";

  std::cout << "Testing with ArrayHandleGroupVecVariable\n";
  TestWithArrayHandle(vtkm::cont::make_ArrayHandleGroupVecVariable(
    vtkm::cont::make_ArrayHandle(testData, vtkm::CopyFlag::Off),
    vtkm::cont::ArrayHandleCounting<vtkm::Id>(0, 2, vtkm::Id(testData.size() / 2) + 1)));
  std::cout << "Passed\n";

  std::cout << "Testing Range with int\n";
  int intBetween[2] = { -10, 10 };
  TestComputeRange(10, 1, intBetween);
  TestComputeRange(10, 3, intBetween);
  TestComputeRange(10, 5, intBetween);
  std::cout << "Passed\n";

  std::cout << "Testing Range with double\n";
  double doubleBetween[2] = { -32, 32 };
  TestComputeRange(10, 1, doubleBetween);
  TestComputeRange(10, 3, doubleBetween);
  TestComputeRange(10, 5, doubleBetween);
  std::cout << "Passed\n";

  return EXIT_SUCCESS;
}
catch (const TestError& e)
{
  e.PrintMessage(std::cout);
  return 1;
}
