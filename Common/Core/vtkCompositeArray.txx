#include "vtkCompositeArray.h"

#include "vtkDataArray.h"

//-----------------------------------------------------------------------
namespace vtk
{
VTK_ABI_NAMESPACE_BEGIN
template <typename T>
vtkSmartPointer<vtkCompositeArray<T>> ConcatenateDataArrays(
  const std::vector<vtkDataArray*>& arrays)
{
  if (arrays.empty())
  {
    return nullptr;
  }
  vtkIdType nComps = -1;
  for (auto arr : arrays)
  {
    if (arr == nullptr)
    {
      continue;
    }
    if (nComps == -1)
    {
      nComps = arr->GetNumberOfComponents();
    }
    else if (arr->GetNumberOfComponents() != nComps)
    {
      vtkErrorWithObjectMacro(nullptr, "Number of components of all the arrays are not equal");
      return nullptr;
    }
  }
  vtkNew<vtkCompositeArray<T>> composite;
  composite->SetBackend(std::make_shared<vtkCompositeImplicitBackend<T>>(arrays));
  composite->SetNumberOfComponents(nComps);
  int nTuples = 0;
  std::for_each(arrays.begin(), arrays.end(),
    [&nTuples](vtkDataArray* arr) { nTuples += arr ? arr->GetNumberOfTuples() : 0; });
  composite->SetNumberOfTuples(nTuples);
  return composite;
}
VTK_ABI_NAMESPACE_END
}
