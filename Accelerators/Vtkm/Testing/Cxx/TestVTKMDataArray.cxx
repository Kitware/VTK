#include "vtkSmartPointer.h"
#include "vtkmDataArray.h"

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>

#include <vector>

namespace
{

//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
template <typename ArrayHandleType>
void TestWithArrayHandle(const ArrayHandleType& vtkmArray)
{
  vtkSmartPointer<vtkDataArray> vtkArray;
  vtkArray.TakeReference(make_vtkmDataArray(vtkmArray));

  auto vtkmPortal = vtkmArray.GetPortalConstControl();

  vtkIdType length = vtkArray->GetNumberOfTuples();
  std::cout << "Length: " << length << "\n";
  TEST_VERIFY(length == vtkmArray.GetNumberOfValues(), "Array lengths don't match");

  int numberOfComponents = vtkArray->GetNumberOfComponents();
  std::cout << "Number of components: " << numberOfComponents << "\n";
  TEST_VERIFY(numberOfComponents ==
      internal::FlattenVec<typename ArrayHandleType::ValueType>::GetNumberOfComponents(
        vtkmPortal.Get(0)),
    "Number of components don't match");

  for (vtkIdType i = 0; i < length; ++i)
  {
    double tuple[9];
    vtkArray->GetTuple(i, tuple);
    auto val = vtkmPortal.Get(i);
    for (int j = 0; j < numberOfComponents; ++j)
    {
      auto comp = internal::FlattenVec<typename ArrayHandleType::ValueType>::GetComponent(val, j);
      TEST_VERIFY(IsEqualFloat(tuple[j], static_cast<double>(comp)), "values don't match");
      TEST_VERIFY(IsEqualFloat(vtkArray->GetComponent(i, j), static_cast<double>(comp)),
        "values don't match");
    }
  }
}

} // anonymous namespace

//-----------------------------------------------------------------------------
int TestVTKMDataArray(int, char*[])
try
{
  static const std::vector<double> testData = { 3.0, 6.0, 2.0, 5.0, 1.0, 0.0, 4.0 };

  std::cout << "Testing with Basic ArrayHandle\n";
  TestWithArrayHandle(vtkm::cont::make_ArrayHandle(testData));
  std::cout << "Passed\n";

  std::cout << "Testing with ArrayHandleConstant\n";
  TestWithArrayHandle(vtkm::cont::make_ArrayHandleConstant(
    vtkm::Vec<vtkm::Vec<float, 3>, 3>{ { 1.0f, 2.0f, 3.0f } }, 10));
  std::cout << "Passed\n";

  std::cout << "Testing with ArrayHandleUniformPointCoordinates\n";
  TestWithArrayHandle(vtkm::cont::ArrayHandleUniformPointCoordinates(vtkm::Id3{ 3 }));
  std::cout << "Passed\n";

  return EXIT_SUCCESS;
}
catch (const TestError& e)
{
  e.PrintMessage(std::cout);
  return 1;
}
