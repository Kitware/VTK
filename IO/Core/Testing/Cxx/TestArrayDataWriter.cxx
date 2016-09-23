
#include "vtkDenseArray.h"
#include "vtkSparseArray.h"
#include "vtkArrayData.h"
#include "vtkArrayDataReader.h"
#include "vtkArrayDataWriter.h"
#include "vtkNew.h"
#include "vtkStdString.h"

#include <iostream>

int TestArrayDataWriter(int, char*[])
{
  std::cerr << "Testing dense first..." << std::endl;

  vtkNew<vtkDenseArray<double> > da;
  da->Resize(10, 10);
  da->SetName("dense");
  vtkNew<vtkSparseArray<double> > sa;
  sa->Resize(10, 10);
  sa->SetName("sparse");
  for(int i = 0; i < 10; ++i)
  {
    sa->SetValue(i, 0, i);
    for(int j = 0; j < 10; ++j)
    {
      da->SetValue(i, j, i*j);
    }
  }

  vtkNew<vtkArrayData> d;
  d->AddArray(da.GetPointer());
  d->AddArray(sa.GetPointer());

  vtkNew<vtkArrayDataWriter> w;
  w->SetInputData(d.GetPointer());
  w->WriteToOutputStringOn();
  w->Write();
  vtkStdString s = w->GetOutputString();

  vtkNew<vtkArrayDataReader> r;
  r->ReadFromInputStringOn();
  r->SetInputString(s);
  r->Update();
  vtkArrayData* d_out = r->GetOutput();

  if(d_out->GetNumberOfArrays() != 2)
  {
    std::cerr << "Wrong number of arrays" << std::endl;
    return 1;
  }

  vtkArray* da_out = d_out->GetArray(0);
  if(da_out->GetDimensions() != 2)
  {
    std::cerr << "da wrong number of dimensions" << std::endl;
    return 1;
  }
  if(!da_out->IsDense())
  {
    std::cerr << "da wrong array type" << std::endl;
    return 1;
  }
  if(da_out->GetSize() != 100)
  {
    std::cerr << "da wrong array size" << std::endl;
    return 1;
  }

  vtkArray* sa_out = d_out->GetArray(1);
  if(sa_out->GetDimensions() != 2)
  {
    std::cerr << "sa wrong number of dimensions" << std::endl;
    return 1;
  }
  if(sa_out->IsDense())
  {
    std::cerr << "sa wrong array type" << std::endl;
    return 1;
  }
  if(sa_out->GetSize() != 100)
  {
    std::cerr << "sa wrong array size" << std::endl;
    return 1;
  }

  std::cerr << "Testing sparse first..." << std::endl;
  d->ClearArrays();
  d->AddArray(sa.GetPointer());
  d->AddArray(da.GetPointer());
  w->Update();
  s = w->GetOutputString();
  r->SetInputString(s);
  r->Update();
  d_out = r->GetOutput();
  if(d_out->GetNumberOfArrays() != 2)
  {
    std::cerr << "Wrong number of arrays" << std::endl;
    return 1;
  }

  std::cerr << "Testing binary writer..." << std::endl;
  w->BinaryOn();
  w->Update();
  s = w->GetOutputString();
  r->SetInputString(s);
  r->Update();
  d_out = r->GetOutput();
  if(d_out->GetNumberOfArrays() != 2)
  {
    std::cerr << "Wrong number of arrays" << std::endl;
    return 1;
  }


  return 0;
}
