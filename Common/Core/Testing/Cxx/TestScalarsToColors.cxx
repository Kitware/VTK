#include "vtkNew.h"
#include "vtkScalarsToColors.h"
#include "vtkStringArray.h"
#include "vtkVariant.h"
#include "vtkVariantArray.h"

//----------------------------------------------------------------------------
static bool TestRange()
{
  bool success = true;

  vtkNew<vtkScalarsToColors> lut;

  // Check default range.
  const double *range = lut->GetRange();
  if (range[0] != 0.0 || range[1] != 255.0)
    {
    cerr << "Default range wrong\n";
    success = false;
    }

  // nop range change.
  lut->SetRange(0.0, 255.0);
  range = lut->GetRange();
  if (range[0] != 0.0 || range[1] != 255.0)
    {
    cerr << "nop range change failed\n";
    success = false;
    }

  // actual range change.
  lut->SetRange(100.0, 200.0);
  range = lut->GetRange();
  if (range[0] != 100.0 || range[1] != 200.0)
    {
    cerr << "range change failed\n";
    success = false;
    }

  return success;
}

//----------------------------------------------------------------------------
static bool TestAlpha()
{
  bool success = true;

  vtkNew<vtkScalarsToColors> lut;

  // Check default alpha.
  double alpha = lut->GetAlpha();
  if (alpha != 1.0)
    {
    cerr << "Default alpha wrong\n";
    success = false;
    }

  // Set out of range.
  lut->SetAlpha(-12345.6);
  alpha = lut->GetAlpha();
  if (alpha != 0.0)
    {
    cerr << "Alpha clamp fail\n";
    success = false;
    }

  lut->SetAlpha(45657.8);
  alpha = lut->GetAlpha();
  if (alpha != 1.0)
    {
    cerr << "Alpha clamp fail\n";
    success = false;
    }

  return success;
}

//----------------------------------------------------------------------------
static bool TestGetColorAndMapValue()
{
  bool success = true;

  vtkNew<vtkScalarsToColors> lut;

  double rgb[3] = {0.1, 0.2, 0.3};
  const unsigned char * rgba = NULL;

  // Sane range.
  lut->SetRange(0.0, 1.0);
  lut->GetColor(0.5, rgb);
  rgba = lut->MapValue(0.5);

  if (rgb[0] != 0.5 || rgb[1] != 0.5 || rgb[2] != 0.5)
    {
    cerr << "GetColor result wrong\n";
    success = false;
    }
  if (rgba[0] != 128 || rgba[1] != 128 || rgba[2] != 128 || rgba[3] != 255)
    {
    cerr << "MapValue result wrong\n";
    success = false;
    }


  // Tiny range.
  lut->SetRange(0.0, 1e-80);
  lut->GetColor(1e-79, rgb);
  rgba = lut->MapValue(1e-79);

  if (rgb[0] != 1e-62 || rgb[1] != 1e-62 || rgb[2] != 1e-62)
    {
    cerr << "GetColor result wrong\n";
    success = false;
    }
  if (rgba[0] != 0 || rgba[1] != 0 || rgba[2] != 0 || rgba[3] != 255)
    {
    cerr << "MapValue result wrong\n";
    success = false;
    }

  return success;
}

//----------------------------------------------------------------------------
static bool TestDeepCopy()
{
  bool success = true;

  vtkNew<vtkScalarsToColors> lut;

  vtkNew<vtkStringArray> ann;
  ann->InsertNextValue("HelloWorld");
  vtkNew<vtkVariantArray> val;
  val->InsertNextValue(vtkVariant(123.4));
  lut->SetAnnotations(val.Get(), ann.Get());

  // Test nop DeepCopy.
  vtkNew<vtkScalarsToColors> copy1;
  copy1->DeepCopy(NULL);

  // Test actual copy.
  vtkNew<vtkScalarsToColors> copy2;
  copy2->DeepCopy(lut.Get());

  vtkStringArray* ann2 = copy2->GetAnnotations();
  vtkAbstractArray* val2 = copy2->GetAnnotatedValues();
  if (!ann2 || !val2)
    {
    cerr << "Annotations not copied\n";
    success = false;
    }
  if (ann.Get() == ann2 || val.Get() == val2)
    {
    cerr << "Annotations only shallow copied\n";
    success = false;
    }
  int idx = lut->GetAnnotatedValueIndex(123.4);
  if (idx != 0)
    {
    cerr << "Could not find annotated value 123.4.\n";
    success = false;
    }

  return success;
}

//----------------------------------------------------------------------------
static bool TestGeneral()
{
  bool success = true;

  vtkNew<vtkScalarsToColors> lut;

  lut->SetAnnotations(NULL, NULL);
  vtkStringArray* ann2 = lut->GetAnnotations();
  vtkAbstractArray* val2 = lut->GetAnnotatedValues();
  if (ann2 || val2)
    {
    cerr << "Annotations set to NULL but didn't return NULL\n";
    success = false;
    }

  vtkNew<vtkStringArray> ann;
  ann->InsertNextValue("Foo");
  vtkNew<vtkVariantArray> val;
  val->InsertNextValue(vtkVariant(10.3));
  lut->SetAnnotations(val.Get(), ann.Get());
  ann2 = lut->GetAnnotations();
  val2 = lut->GetAnnotatedValues();
  if (!ann2 || !val2)
    {
    cerr << "Annotations set to non-NULL but returned NULL\n";
    success = false;
    }

  int idx = lut->GetAnnotatedValueIndex(10.3);
  if (idx != 0)
    {
    cerr << "Could not find annotated value 10.3.\n";
    success = false;
    }

  idx = lut->GetAnnotatedValueIndex("Narf");
  if (idx >= 0)
    {
    cerr << "Found unexpected annotated value \"Narf\".\n";
    success = false;
    }

  ann->InsertNextValue("Not hardly!");
  val->InsertNextValue("Narf");
  ann->InsertNextValue("Fezzik");
  val->InsertNextValue(vtkVariant(20));
  lut->SetAnnotations(val.Get(), ann.Get());

  idx = lut->GetAnnotatedValueIndex("Narf");
  if (idx != 1)
    {
    cerr << "Couldn't find newly-annotated value (\"Narf\").\n";
    success = false;
    }

  lut->SetAnnotations(NULL, NULL);
  ann2 = lut->GetAnnotations();
  val2 = lut->GetAnnotatedValues();
  if (ann2 || val2)
    {
    cerr << "Annotations again set to NULL but didn't return NULL\n";
    success = false;
    }

  return success;
}

//----------------------------------------------------------------------------
int TestScalarsToColors(int, char*[])
{
  bool success1 = TestRange();
  bool success2 = TestAlpha();
  bool success3 = TestGetColorAndMapValue();
  bool success4 = TestDeepCopy();
  bool success5 = TestGeneral();

  if (success1 && success2 && success3 && success4 && success5)
    {
    return EXIT_SUCCESS;
    }
  else
    {
    return EXIT_FAILURE;
    }
}
