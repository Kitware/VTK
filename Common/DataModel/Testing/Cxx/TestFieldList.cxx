#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkIntArray.h"
#include "vtkNew.h"
#include "vtkSmartPointer.h"

#include <string>

namespace
{
template <typename T>
vtkSmartPointer<T> CreateArray(const char* aname, int num_comps, vtkIdType numTuples)
{
  auto array = vtkSmartPointer<T>::New();
  array->SetName(aname);
  array->SetNumberOfComponents(num_comps);
  array->SetNumberOfTuples(numTuples);
  array->FillValue(typename T::ValueType());
  return array;
}

#define EXPECT_THAT(v, m)                                                                          \
  if ((v) != (m))                                                                                  \
  {                                                                                                \
    cerr << "FAILED at line " << __LINE__ << ": \n     " << #v << " must match " << #m << endl;    \
    return EXIT_FAILURE;                                                                           \
  }
}

int TestFieldList(int, char*[])
{

  {
    // For arrays without names, ensure we are doing a order-dependent matching.
    // For attributes, the attribute flag is preserved if the same arrays is
    // consistently flagged as an attribute on all DSA instances.
    vtkNew<vtkDataSetAttributes> dsa0;
    dsa0->SetScalars(CreateArray<vtkDoubleArray>(nullptr, 1, 20));
    dsa0->AddArray(CreateArray<vtkDoubleArray>(nullptr, 2, 20));
    dsa0->SetVectors(CreateArray<vtkDoubleArray>(nullptr, 3, 20));
    EXPECT_THAT(dsa0->GetNumberOfArrays(), 3);

    vtkNew<vtkDataSetAttributes> dsa1;
    dsa1->SetScalars(CreateArray<vtkDoubleArray>(nullptr, 1, 20));
    dsa1->AddArray(CreateArray<vtkDoubleArray>(nullptr, 3, 20));
    dsa1->AddArray(CreateArray<vtkDoubleArray>(nullptr, 3, 20));
    EXPECT_THAT(dsa1->GetNumberOfArrays(), 3);

    vtkDataSetAttributes::FieldList fl;
    fl.InitializeFieldList(dsa0);
    fl.IntersectFieldList(dsa1);

    vtkNew<vtkDataSetAttributes> output;
    fl.CopyAllocate(output, vtkDataSetAttributes::COPYTUPLE, 0, 0);
    EXPECT_THAT(output->GetNumberOfArrays(), 2);
    EXPECT_THAT(output->GetArray(0)->GetNumberOfComponents(), 1);
    EXPECT_THAT(output->GetArray(1)->GetNumberOfComponents(), 3);
    EXPECT_THAT(output->GetVectors(), nullptr);
    EXPECT_THAT(output->GetScalars() != nullptr, true);

    fl.InitializeFieldList(dsa0);
    fl.UnionFieldList(dsa1);
    output->Initialize();
    fl.CopyAllocate(output, vtkDataSetAttributes::COPYTUPLE, 0, 0);
    EXPECT_THAT(output->GetNumberOfArrays(), 4);
    EXPECT_THAT(output->GetArray(0)->GetNumberOfComponents(), 1);
    EXPECT_THAT(output->GetArray(1)->GetNumberOfComponents(), 2);
    EXPECT_THAT(output->GetArray(2)->GetNumberOfComponents(), 3);
    EXPECT_THAT(output->GetArray(3)->GetNumberOfComponents(), 3);
    EXPECT_THAT(output->GetVectors(), nullptr);
    EXPECT_THAT(output->GetScalars() != nullptr, true);

    // just to increase coverage.
    fl.PrintSelf(cout, vtkIndent());
  }

  {
    // If inputs arrays with different names for attributes,
    // make sure output doesn't have either of the arrays flagged as attributes.
    vtkNew<vtkDataSetAttributes> dsa0;
    dsa0->SetScalars(CreateArray<vtkDoubleArray>("scalars", 1, 20));
    dsa0->AddArray(CreateArray<vtkDoubleArray>("vectors", 3, 20));
    dsa0->AddArray(CreateArray<vtkDoubleArray>("common", 1, 20));
    dsa0->AddArray(CreateArray<vtkDoubleArray>("uncommon0", 1, 20));

    vtkNew<vtkDataSetAttributes> dsa1;
    dsa1->AddArray(CreateArray<vtkDoubleArray>("scalars", 1, 20));
    dsa1->SetVectors(CreateArray<vtkDoubleArray>("vectors", 3, 20));
    dsa1->AddArray(CreateArray<vtkDoubleArray>("common", 1, 20));
    dsa0->AddArray(CreateArray<vtkDoubleArray>("uncommon1", 1, 20));

    vtkDataSetAttributes::FieldList fl;
    fl.InitializeFieldList(dsa0);
    fl.IntersectFieldList(dsa1);

    vtkNew<vtkDataSetAttributes> output;
    fl.CopyAllocate(output, vtkDataSetAttributes::COPYTUPLE, 0, 0);
    EXPECT_THAT(output->GetNumberOfArrays(), 3);
    EXPECT_THAT(output->GetArray("uncommon0"), nullptr);
    EXPECT_THAT(output->GetArray("uncommon1"), nullptr);
    EXPECT_THAT(output->GetScalars(), nullptr);
    EXPECT_THAT(output->GetVectors(), nullptr);
    EXPECT_THAT(output->GetArray("scalars") != nullptr, true);
    EXPECT_THAT(output->GetArray("vectors") != nullptr, true);

    fl.InitializeFieldList(dsa0);
    fl.UnionFieldList(dsa1);
    output->Initialize();
    fl.CopyAllocate(output, vtkDataSetAttributes::COPYTUPLE, 0, 0);
    EXPECT_THAT(output->GetNumberOfArrays(), 5);
    EXPECT_THAT(output->GetScalars(), nullptr);
    EXPECT_THAT(output->GetVectors(), nullptr);
  }

  return EXIT_SUCCESS;
}
