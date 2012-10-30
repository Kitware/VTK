#include "vtkInformation.h"
#include "vtkInformationDoubleKey.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformationStringKey.h"
#include "vtkInformationStringVectorKey.h"
#include "vtkInformationVariantKey.h"
#include "vtkInformationVariantVectorKey.h"
#include "vtkNew.h"
#include "vtkStdString.h"
#include "vtkVariant.h"

template<typename T, typename V>
int UnitTestScalarValueKey(vtkInformation* info, T* key, const V& val)
{
  key->Set(info, val);
  int ok_setget = (val == key->Get(info));
  if (!ok_setget)
    {
    cerr << "Set + Get not reflexive.\n";
    }

  vtkNew<vtkInformation> shinyNew;
  key->ShallowCopy(info, shinyNew.GetPointer());
  int ok_copyget = (val == key->Get(shinyNew.GetPointer()));
  if (!ok_copyget)
    {
    cerr << "Copy + Get not reflexive.\n";
    }

  return ok_setget & ok_copyget;
}

template<typename T, typename V>
int UnitTestVectorValueKey(vtkInformation* info, T* key, const V& val)
{
  key->Set(info, const_cast<V*>(&val), 1);
  int ok_setget = (val == key->Get(info, 0));
  if (!ok_setget)
    {
    cerr << "Set + get not reflexive.\n";
    }
  int ok_setgetcomp = (val == *key->Get(info));
  if (!ok_setgetcomp)
    {
    cerr << "Set + component-wise-get not reflexive.\n";
    }

  vtkNew<vtkInformation> shinyNew;
  key->ShallowCopy(info, shinyNew.GetPointer());
  int ok_copyget = (val == *key->Get(shinyNew.GetPointer()));
  if (!ok_copyget)
    {
    cerr << "Copy + get not reflexive.\n";
    }

  int ok_length = (key->Length(info) == 1);
  if (!ok_length)
    {
    cerr << "Length was " << key->Length(info) << " not 1.\n";
    }
  key->Append(info, val);
  int ok_appendedlength = (key->Length(info) == 2);
  if (!ok_appendedlength)
    {
    cerr << "Appended length was " << key->Length(info) << " not 2.\n";
    }

  return
    ok_setget && ok_setgetcomp && ok_copyget &&
    ok_length && ok_appendedlength;
}

// === String adaptations of tests above ===
// Note these are not specializations.

int UnitTestScalarValueKey(
  vtkInformation* info, vtkInformationStringKey* key,
  const vtkStdString& val)
{
  key->Set(info, val.c_str());
  int ok_setget = (val == key->Get(info));
  if (!ok_setget)
    {
    cerr << "Set + Get not reflexive.\n";
    }

  vtkNew<vtkInformation> shinyNew;
  key->ShallowCopy(info, shinyNew.GetPointer());
  int ok_copyget = (val == key->Get(shinyNew.GetPointer()));
  if (!ok_copyget)
    {
    cerr << "Copy + Get not reflexive.\n";
    }

  return ok_setget & ok_copyget;
}

int UnitTestVectorValueKey(
  vtkInformation* info, vtkInformationStringVectorKey* key,
  const vtkStdString& val)
{
  key->Set(info, val.c_str(), 0);
  int ok_setgetcomp = (val == key->Get(info, 0));
  if (!ok_setgetcomp)
    {
    cerr << "Set + get not reflexive.\n";
    }

  vtkNew<vtkInformation> shinyNew;
  key->ShallowCopy(info, shinyNew.GetPointer());
  int ok_copyget = (val == key->Get(shinyNew.GetPointer(), 0));
  if (!ok_copyget)
    {
    cerr << "Copy + get not reflexive.\n";
    }

  int ok_length = (key->Length(info) == 1);
  if (!ok_length)
    {
    cerr << "Length was " << key->Length(info) << " not 1.\n";
    }
  key->Append(info, val.c_str());
  int ok_appendedlength = (key->Length(info) == 2);
  if (!ok_appendedlength)
    {
    cerr << "Appended length was " << key->Length(info) << " not 2.\n";
    }

  return ok_setgetcomp && ok_copyget && ok_length && ok_appendedlength;
}

int UnitTestInformationKeys(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  int ok = 1;
  vtkNew<vtkInformation> info;
  vtkVariant tvval("foo");
  double tdval = 3.14159;
  vtkStdString tsval = "bar";

  vtkInformationVariantKey* tvskey =
    new vtkInformationVariantKey("Test", "vtkTest");
  ok &= UnitTestScalarValueKey(info.GetPointer(), tvskey, tvval);

  vtkInformationVariantVectorKey* tvvkey =
    new vtkInformationVariantVectorKey("Test", "vtkTest");
  ok &= UnitTestVectorValueKey(info.GetPointer(), tvvkey, tvval);

  vtkInformationDoubleKey* tdskey =
    new vtkInformationDoubleKey("Test", "vtkTest");
  ok &= UnitTestScalarValueKey(info.GetPointer(), tdskey, tdval);

  vtkInformationDoubleVectorKey* tdvkey =
    new vtkInformationDoubleVectorKey("Test", "vtkTest");
  ok &= UnitTestVectorValueKey(info.GetPointer(), tdvkey, tdval);

  vtkInformationStringKey* tsskey =
    new vtkInformationStringKey("Test", "vtkTest");
  ok &= UnitTestScalarValueKey(info.GetPointer(), tsskey, tsval);

  vtkInformationStringVectorKey* tsvkey =
    new vtkInformationStringVectorKey("Test", "vtkTest");
  ok &= UnitTestVectorValueKey(info.GetPointer(), tsvkey, tsval);

  return ! ok;
}
