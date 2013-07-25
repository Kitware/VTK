#include "vtkNew.h"
#include "vtkScalarsToColors.h"
#include "vtkStringArray.h"
#include "vtkVariant.h"
#include "vtkVariantArray.h"

int TestScalarsToColors(int, char*[])
{
  vtkNew<vtkScalarsToColors> lut;
  vtkNew<vtkStringArray> ann;
  vtkNew<vtkVariantArray> val;

  lut->SetAnnotations(NULL, NULL);
  vtkStringArray* ann2 = lut->GetAnnotations();
  vtkAbstractArray* val2 = lut->GetAnnotatedValues();
  if (ann2 || val2)
    {
    cerr << "Annotations set to NULL but didn't return NULL\n";
    return EXIT_FAILURE;
    }

  ann->InsertNextValue("Foo");
  val->InsertNextValue(vtkVariant(10.3));
  lut->SetAnnotations(val.GetPointer(), ann.GetPointer());
  ann2 = lut->GetAnnotations();
  val2 = lut->GetAnnotatedValues();
  if (!ann2 || !val2)
    {
    cerr << "Annotations set to non-NULL but returned NULL\n";
    return EXIT_FAILURE;
    }

  int idx = lut->GetAnnotatedValueIndex(10.3);
  if (idx != 0)
    {
    cerr << "Could not find annotated value 10.3.\n";
    return EXIT_FAILURE;
    }

  idx = lut->GetAnnotatedValueIndex("Narf");
  if (idx >= 0)
    {
    cerr << "Found unexpected annotated value \"Narf\".\n";
    return EXIT_FAILURE;
    }

  ann->InsertNextValue("Not hardly!");
  val->InsertNextValue("Narf");
  ann->InsertNextValue("Fezzik");
  val->InsertNextValue(vtkVariant(20));
  lut->SetAnnotations(val.GetPointer(), ann.GetPointer());

  idx = lut->GetAnnotatedValueIndex("Narf");
  if (idx != 1)
    {
    cerr << "Couldn't find newly-annotated value (\"Narf\").\n";
    return EXIT_FAILURE;
    }

  lut->SetAnnotations(NULL, NULL);
  ann2 = lut->GetAnnotations();
  val2 = lut->GetAnnotatedValues();
  if (ann2 || val2)
    {
    cerr << "Annotations again set to NULL but didn't return NULL\n";
    return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}
