/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestBigData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// This Test validates that we can create arrays and datasets in
// an extended memory space and that operations like copy and filter
// work as we expect them too.

#include "vtkDataSetWriter.h"
#include "vtkDoubleArray.h"
#include "vtkExtractVOI.h"
#include "vtkFloatArray.h"
#include "vtkImageCast.h"
#include "vtkImageData.h"
#include "vtkImageGaussianSource.h"
#include "vtkImageResize.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkShrinkFilter.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkTable.h"
#include "vtkUnstructuredGrid.h"

int TestMemkindData(int ac, char* av[])
{
  std::string home = ".";
  int GB = 1;
  for (int a = 0; a < ac; a++)
  {
    if (!strcmp(av[a], "-home") && a < ac - 1)
    {
      // a directory for the extended memory space, ideal mounted -o dax
      home = std::string(av[a + 1]);
    }
    if (!strcmp(av[a], "-GB") && a < ac - 1)
    {
      GB = std::stoi(std::string(av[a + 1]));
    }
  }
  cout << "Extended memory is backed by " << home << endl;
  vtkObjectBase::SetMemkindDirectory(home.c_str());

  cout << "*****************************************" << endl;
  cout << "Test allocation of " << GB << " gigabytes." << endl;
  // This is expected to succeed as long as the file system has at least this much free space.
  vtkFloatArray* extArray = vtkFloatArray::ExtendedNew();
  cout << "In extended memory ... " << endl;
  extArray->Allocate(1024l * 1024 * 1024 * GB / sizeof(float));
  cout << "OK!" << endl;

  // This is expected to succeed as long as the RAM has at least this much capacity.
  vtkFloatArray* normalArray = vtkFloatArray::New();
  cout << "In standard memory ... " << endl;
  normalArray->Allocate(1024l * 1024 * 1024 * GB / sizeof(float));
  cout << "OK!" << endl;

  cout << "Delete extended memory ... " << endl;
  extArray->Delete();
  cout << "Delete standard memory ... " << endl;
  normalArray->Delete();

  // Demonstrate that it works with smart pointers too.
  vtkSmartPointer<vtkFloatArray> extArray2 = vtkSmartPointer<vtkFloatArray>::ExtendedNew();
  cout << "Another in extended memory ... " << endl;
  extArray->Allocate(1024l * 1024 * 1024 * GB / sizeof(float));
  cout << "OK!" << endl;

  cout << "*****************************************" << endl;
  cout << "Make a big ImageData in extended memory. " << endl;
  vtkImageData* hugeImage = vtkImageData::ExtendedNew();
  assert(hugeImage->GetIsInMemkind() == true);
  int edge = static_cast<int>(std::cbrt(1024l * 1024 * 1024 * GB / VTK_SIZEOF_SHORT));
  cout << "Each edge is " << edge << endl;
  hugeImage->SetDimensions(edge, edge, edge);
  hugeImage->AllocateScalars(VTK_UNSIGNED_SHORT, 1);
  cout << "Populate it." << endl;
  unsigned short* ptr = static_cast<unsigned short*>(hugeImage->GetScalarPointer());
  for (int k = 0; k < edge; ++k)
  {
    double z = (double)k / edge - 0.5;
    if (k % (edge / 10) == 0)
    {
      cout << (z + 0.5) * 100 << "% done" << endl;
    }
    for (int j = 0; j < edge; ++j)
    {
      double y = (double)j / edge - 0.5;
      for (int i = 0; i < edge; ++i)
      {
        double x = (double)i / edge - 0.5;
        *ptr = (unsigned short)((x * y * z + 0.125) * 4.0 * VTK_UNSIGNED_SHORT_MAX);
        ++ptr;
      }
    }
  }
  assert(hugeImage->GetPointData()->GetArray(0)->GetIsInMemkind() == true);

  cout << "Apply a filter." << endl;
  vtkSmartPointer<vtkExtractVOI> slice = vtkSmartPointer<vtkExtractVOI>::New();
  slice->SetVOI(0, edge - 1, 0, edge - 1, edge / 4.0, edge / 4.0);
  slice->SetInputData(hugeImage);
  vtkSmartPointer<vtkDataSetWriter> dsw = vtkSmartPointer<vtkDataSetWriter>::New();
  dsw->SetInputConnection(slice->GetOutputPort());
  dsw->SetFileName("slice.vtk");
  dsw->Write();
  // Extendedness does not necessarily flow down the pipeline
  assert(slice->GetOutput()->GetIsInMemkind() == false);
  hugeImage->Delete();

  cout << "*****************************************" << endl;
  cout << "array tests" << endl;
  // make an extended array
  vtkDoubleArray* da = vtkDoubleArray::ExtendedNew();
  assert(da->GetIsInMemkind() == true);
  // make a normal array
  vtkDoubleArray* db = vtkDoubleArray::New();
  assert(db->GetIsInMemkind() == false);

  // try out the extended array
  da->SetNumberOfComponents(3);
  da->SetNumberOfTuples(3000);
  for (int i = 0; i < 3000; i++)
  {
    da->SetTuple3(i, i, i, i);
  }
  for (int i = 0; i < 3000; i += 100)
  {
    assert(da->GetTypedComponent(i, 0) == i);
  }
  da->Delete();
  db->Delete();

  cout << "*****************************************" << endl;
  cout << "field tests" << endl;
  // make a extended set of arrays
  vtkFieldData* fda = vtkFieldData::ExtendedNew();
  assert(fda->GetIsInMemkind() == true);
  vtkIntArray* ia = vtkIntArray::ExtendedNew();
  assert(ia->GetIsInMemkind() == true);
  ia->SetNumberOfComponents(3);
  ia->SetNumberOfTuples(10);
  ia->SetName("Extended Array");
  fda->AddArray(ia);
  ia->Delete();

  // make a normal set of arrays
  vtkFieldData* fdb = vtkFieldData::New();
  assert(fdb->GetIsInMemkind() == false);
  db = vtkDoubleArray::New();
  assert(db->GetIsInMemkind() == false);
  db->SetNumberOfComponents(1);
  db->SetNumberOfTuples(10);
  db->SetName("Normal Array");
  fdb->AddArray(db);
  db->Delete();

  // shouldn't crash on delete despite container holding mixed contents
  fda->AddArray(db);
  fdb->AddArray(ia);
  fda->PrintSelf(cout, vtkIndent(0));
  fdb->PrintSelf(cout, vtkIndent(0));

  fda->Delete();
  fdb->Delete();

  cout << "*****************************************" << endl;
  cout << "table tests" << endl;
  vtkTable* ta = vtkTable::ExtendedNew();
  assert(ta->GetIsInMemkind() == true);
  vtkTable* tb = vtkTable::New();
  assert(tb->GetIsInMemkind() == false);
  ta->Delete();
  tb->Delete();

  cout << "*****************************************" << endl;
  cout << "imagedata tests" << endl;
  vtkSmartPointer<vtkImageData> ida = vtkSmartPointer<vtkImageData>::ExtendedNew();
  assert(ida->GetIsInMemkind() == true);

  // try some more filtering operations to ensure things work when input is in the extended space
  // first let's make something real
  int size[3] = { 128, 128, 128 };
  vtkSmartPointer<vtkImageGaussianSource> source = vtkSmartPointer<vtkImageGaussianSource>::New();
  assert(source->GetIsInMemkind() == false);
  source->SetWholeExtent(0, size[0] - 1, 0, size[1] - 1, 0, size[2] - 1);
  source->SetCenter(0.5 * (size[0] - 1), 0.5 * (size[1] - 1), 0.5 * (size[2] - 1));
  int maxdim = (size[0] > size[1] ? size[0] : size[1]);
  maxdim = (maxdim > size[2] ? maxdim : size[2]);
  source->SetStandardDeviation(0.25 * (maxdim - 1));
  source->SetMaximum(255.0);
  vtkSmartPointer<vtkImageCast> cast = vtkSmartPointer<vtkImageCast>::New();
  assert(cast->GetIsInMemkind() == false);
  cast->SetInputConnection(source->GetOutputPort());
  cast->SetOutputScalarType(VTK_SHORT);
  cast->Update();
  vtkImageData* castout = cast->GetOutput();
  assert(castout->GetIsInMemkind() == false);

  ida->DeepCopy(castout); // DeepCopy into an extendedspace object will make extended objects
  // ida->ShallowCopy(castout); //ShallowCopy will not
  assert(ida->GetPointData()->GetIsInMemkind() == true);
  assert(ida->GetPointData()->GetArray(0)->GetIsInMemkind() == true);

  vtkSmartPointer<vtkImageResize> resize = vtkSmartPointer<vtkImageResize>::New();
  resize->SetInputData(ida);
  resize->SetMagnificationFactors(0.1, 0.15, 0.2);
  resize->SetResizeMethodToMagnificationFactors();
  resize->Update();
  assert(resize->GetOutput()->GetIsInMemkind() ==
    false); // resize doesn't shallow copy input, so container should be normal
  assert(resize->GetOutput()->GetPointData()->GetArray(0)->GetIsInMemkind() == false);

  cout << "*****************************************" << endl;
  cout << "unstructuredgrid tests" << endl;
  vtkUnstructuredGrid* uga = vtkUnstructuredGrid::ExtendedNew();
  assert(uga->GetIsInMemkind() == true);
  vtkUnstructuredGrid* ugb = vtkUnstructuredGrid::New();
  assert(ugb->GetIsInMemkind() == false);
  uga->Delete();
  ugb->Delete();

  cout << "*****************************************" << endl;
  cout << "polydata tests" << endl;
  vtkPolyData* pda = vtkPolyData::ExtendedNew();
  assert(pda->GetIsInMemkind() == true);
  vtkPolyData* pdb = vtkPolyData::New();
  assert(pdb->GetIsInMemkind() == false);

  // do some filtering to test unstructured types
  vtkSmartPointer<vtkSphereSource> ss = vtkSmartPointer<vtkSphereSource>::New();
  ss->Update();
  // should all be normal memory so far
  assert(ss->GetOutput()->GetIsInMemkind() == false);
  assert(ss->GetOutput()->GetPoints()->GetIsInMemkind() == false);
  assert(ss->GetOutput()->GetPointData()->GetNormals()->GetIsInMemkind() == false);
  pda->DeepCopy(
    ss->GetOutput()); // DeepCopy into an extendedspace object will make extended objects
  // pda->ShallowCopy(ss->GetOutput()); //ShallowCopy will not
  assert(pda->GetIsInMemkind() == true);
  assert(pda->GetPoints()->GetIsInMemkind() == true);
  assert(pda->GetPointData()->GetNormals()->GetIsInMemkind() == true);

  vtkSmartPointer<vtkShrinkFilter> sf = vtkSmartPointer<vtkShrinkFilter>::New();
  sf->SetInputData(pda);
  sf->SetShrinkFactor(0.5);
  sf->Update();
  assert(sf->GetOutput()->GetIsInMemkind() == false); // output of a filter should be normal
  assert(
    sf->GetOutput()->GetPoints()->GetIsInMemkind() == false); // changed results should be normal
  assert(sf->GetOutput()->GetPointData()->GetNormals()->GetIsInMemkind() == false);

  pda->Delete();
  pdb->Delete();

  return EXIT_SUCCESS;
}
