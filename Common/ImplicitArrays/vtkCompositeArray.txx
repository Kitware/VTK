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
  vtkIdType nComps = arrays[0]->GetNumberOfComponents();
  for (auto arr : arrays)
  {
    if (arr->GetNumberOfComponents() != nComps)
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
    [&nTuples](vtkDataArray* arr) { nTuples += arr->GetNumberOfTuples(); });
  composite->SetNumberOfTuples(nTuples);
  return composite;
}
VTK_ABI_NAMESPACE_END
}
