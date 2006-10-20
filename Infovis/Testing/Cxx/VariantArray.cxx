
#include "vtkIntArray.h"
#include "vtkStringArray.h"
#include "vtkDoubleArray.h"
#include "vtkVariantArray.h"
#include "vtkArrayIterator.h"
#include "vtkArrayIteratorTemplate.h"
#include "vtkMath.h"

#include <time.h>
#include <vtkstd/vector>
using vtkstd::vector;

int VariantArray(int, char*[])
{
  long seed = time(NULL);
  cout << "Seed: " << seed << endl;
  vtkMath::RandomSeed(seed);

  int size = 100;
  double prob = 1.0 - 1.0 / size;

  vtkVariantArray* arr = vtkVariantArray::New();
  vector<double> vec;

  // Resizing
  // * int Allocate(vtkIdType sz);
  // * void Initialize();
  // * void SetNumberOfTuples(vtkIdType number);
  // * void Squeeze(); 
  // * int Resize(vtkIdType numTuples);
  // * void SetNumberOfValues(vtkIdType number);
  // * void SetVoidArray(void *arr, vtkIdType size, int save);
  // * void SetArray(vtkVariant* arr, vtkIdType size, int save);

  arr->Allocate(1000);
  if (arr->GetSize() != 1000 || arr->GetNumberOfTuples() != 0)
    {
    cout << "size (" << arr->GetSize() << ") should be 1000, "
         << "tuples (" << arr->GetNumberOfTuples() << ") should be 0." << endl;
    exit(1);
    }

  arr->SetNumberOfValues(2000);
  if (arr->GetSize() != 2000 || arr->GetNumberOfTuples() != 2000)
    {
    cout << "size (" << arr->GetSize() << ") should be 2000, "
         << "tuples (" << arr->GetNumberOfTuples() << ") should be 2000." << endl;
    exit(1);
    }

  arr->Initialize();
  if (arr->GetSize() != 0 || arr->GetNumberOfTuples() != 0)
    {
    cout << "size (" << arr->GetSize() << ") should be 0, "
         << "tuples (" << arr->GetNumberOfTuples() << ") should be 0." << endl;
    exit(1);
    }

  arr->SetNumberOfComponents(3);

  arr->SetNumberOfTuples(1000);
  if (arr->GetSize() != 3000 || arr->GetNumberOfTuples() != 1000)
    {
    cout << "size (" << arr->GetSize() << ") should be 3000, "
         << "tuples (" << arr->GetNumberOfTuples() << ") should be 1000." << endl;
    exit(1);
    }

  arr->SetNumberOfTuples(500);
  if (arr->GetSize() != 3000 || arr->GetNumberOfTuples() != 500)
    {
    cout << "size (" << arr->GetSize() << ") should be 3000, "
         << "tuples (" << arr->GetNumberOfTuples() << ") should be 500." << endl;
    exit(1);
    }

  arr->Squeeze();
  if (arr->GetSize() != 1500 || arr->GetNumberOfTuples() != 500)
    {
    cout << "size (" << arr->GetSize() << ") should be 1500, "
         << "tuples (" << arr->GetNumberOfTuples() << ") should be 500." << endl;
    exit(1);
    }

  arr->SetNumberOfTuples(1000);
  if (arr->GetSize() != 3000 || arr->GetNumberOfTuples() != 1000)
    {
    cout << "size=" << arr->GetSize() << ", should be 3000, "
         << "tuples (" << arr->GetNumberOfTuples() << ", should be 1000." << endl;
    exit(1);
    }

  arr->Resize(500);
  if (arr->GetSize() != 1500 || arr->GetNumberOfTuples() != 500)
    {
    cout << "size=" << arr->GetSize() << ", should be 1500, "
         << "tuples=" << arr->GetNumberOfTuples() << ", should be 500." << endl;
    exit(1);
    }

  vtkVariant* userArray = new vtkVariant[3000];
  arr->SetVoidArray(reinterpret_cast<void*>(userArray), 3000, 0);
  if (arr->GetSize() != 3000 || arr->GetNumberOfTuples() != 1000)
    {
    cout << "size=" << arr->GetSize() << ", should be 3000, "
         << "tuples=" << arr->GetNumberOfTuples() << ", should be 1000." << endl;
    exit(1);
    }

  arr->SetNumberOfComponents(1);
  arr->Initialize();

  // Writing to the array
  // * void InsertValue(vtkIdType id, vtkVariant value);
  // * vtkIdType InsertNextValue(vtkVariant value);
  // * void InsertTuple(vtkIdType i, vtkIdType j, vtkAbstractArray* source);
  // * vtkIdType InsertNextTuple(vtkIdType j, vtkAbstractArray* source);
  // * void SetValue(vtkIdType id, vtkVariant value);
  // * void SetTuple(vtkIdType i, vtkIdType j, vtkAbstractArray* source);

  cout << "Performing insert operations." << endl;
  vtkIdType id = 0;
  while (vtkMath::Random() < prob)
    {
    if (vtkMath::Random() < 0.5)
      {
      arr->InsertValue(id, vtkVariant(id));
      }
    else
      {
      vtkIdType index = arr->InsertNextValue(vtkVariant(id));
      if (index != id)
        {
        cout << "index=" << index << ", id=" << id << endl;
        exit(1);
        }
      }
    vec.push_back(id);
    id++;
    }

  vtkStringArray* stringArr = vtkStringArray::New();
  vtkIdType strId = id;
  while (vtkMath::Random() < prob)
    {
    stringArr->InsertNextValue(vtkVariant(strId).ToString());
    strId++;
    }

  for (int i = 0; i < stringArr->GetNumberOfValues(); i++)
    {
    if (vtkMath::Random() < 0.5)
      {
      arr->InsertTuple(id, i, stringArr);
      }
    else
      {
      vtkIdType index = arr->InsertNextTuple(i, stringArr);
      if (index != id)
        {
        cout << "index=" << index << ", id=" << id << endl;
        exit(1);
        }
      }
    vec.push_back(id);
    id++;
    }

  cout << "Performing set operations." << endl;
  while (vtkMath::Random() < prob)
    {
    int index = static_cast<int>(vtkMath::Random(0, arr->GetNumberOfValues()));
    if (vtkMath::Random() < 0.5)
      {
      arr->SetValue(index, vtkVariant(id));
      vec[index] = id;
      }
    else
      {
      int index2 = static_cast<int>(vtkMath::Random(0, stringArr->GetNumberOfValues()));
      arr->SetTuple(index, index2, stringArr);
      vec[index] = vtkVariant(stringArr->GetValue(index2)).ToDouble();
      }
    id++;
    }

  stringArr->Delete();

  // Reading from the array
  // * unsigned long GetActualMemorySize();
  // * int IsNumeric();
  // * int GetDataType();
  // * int GetDataTypeSize();
  // * int GetElementComponentSize();
  // * vtkArrayIterator* NewIterator();
  // * vtkVariant & GetValue(vtkIdType id) const;
  // * vtkVariant* GetPointer(vtkIdType id);
  // * void *GetVoidPointer(vtkIdType id);
  // * vtkIdType GetNumberOfValues();
  // * void DeepCopy(vtkAbstractArray *da);
  //   void InterpolateTuple(vtkIdType i, vtkIdList *ptIndices,
  //     vtkAbstractArray* source,  double* weights);
  //   void InterpolateTuple(vtkIdType i, 
  //     vtkIdType id1, vtkAbstractArray* source1, 
  //     vtkIdType id2, vtkAbstractArray* source2, double t);

  if (arr->IsNumeric())
    {
    cout << "The variant array is reported to be numeric, but should not be." << endl;
    exit(1);
    }

  if (arr->GetDataType() != VTK_VARIANT)
    {
    cout << "The type of the array should be VTK_VARIANT." << endl;
    exit(1);
    }

  if (arr->GetActualMemorySize() == 0 
    || arr->GetDataTypeSize() == 0 
    || arr->GetElementComponentSize() == 0)
    {
    cout << "One of the size functions returned zero." << endl;
    exit(1);
    }

  if (arr->GetNumberOfValues() != static_cast<vtkIdType>(vec.size()))
    {
    cout << "Sizes do not match (" 
         << arr->GetNumberOfValues() << " != " << vec.size() << ")" << endl;
    exit(1);
    }

  cout << "Checking by index." << endl;
  for (vtkIdType i = 0; i < arr->GetNumberOfValues(); i++)
    {
    double arrVal = arr->GetValue(i).ToDouble();
    if (arrVal != vec[i])
      {
      cout << "values do not match (" << arrVal << " != " << vec[i] << ")" << endl;
      exit(1);
      }
    }

  cout << "Check using an iterator." << endl;
  vtkArrayIteratorTemplate<vtkVariant>* iter 
    = dynamic_cast<vtkArrayIteratorTemplate<vtkVariant>*>(arr->NewIterator());
  for (vtkIdType i = 0; i < iter->GetNumberOfValues(); i++)
    {
    double arrVal = iter->GetValue(i).ToDouble();
    if (arrVal != vec[i])
      {
      cout << "values do not match (" << arrVal << " != " << vec[i] << ")" << endl;
      exit(1);
      }
    }
  iter->Delete();

  cout << "Check using array pointer." << endl;
  vtkVariant* pointer = reinterpret_cast<vtkVariant*>(arr->GetVoidPointer(0));
  for (vtkIdType i = 0; i < arr->GetNumberOfValues(); i++)
    {
    double arrVal = pointer[i].ToDouble();
    if (arrVal != vec[i])
      {
      cout << "values do not match (" << arrVal << " != " << vec[i] << ")" << endl;
      exit(1);
      }
    }

  cout << "Perform a deep copy and check it." << endl;
  vtkVariantArray* copy = vtkVariantArray::New();
  arr->DeepCopy(copy);
  for (vtkIdType i = 0; i < arr->GetNumberOfValues(); i++)
    {
    double arrVal = copy->GetValue(i).ToDouble();
    if (arrVal != vec[i])
      {
      cout << "values do not match (" << arrVal << " != " << vec[i] << ")" << endl;
      exit(1);
      }
    }
  copy->Delete();

  arr->Delete();

  return 0;
}

