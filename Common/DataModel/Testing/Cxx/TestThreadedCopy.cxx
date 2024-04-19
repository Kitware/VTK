// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// This Test validates that we can do a threaded copy of an imagedata
// and helps to evaluate the performance of doing so.

#include "vtkDataSetWriter.h"
#include "vtkImageData.h"
#include "vtkSMPTools.h"
#include "vtkSmartPointer.h"

#include <cmath>

int TestThreadedCopy(int ac, char* av[])
{
  double GB = 0.01;
  bool write = false;

  for (int i = 0; i < ac; ++i)
  {
    if (i < ac - 1 && !strcmp(av[i], "--numThreads"))
    {
      vtkSMPTools::Initialize(atoi(av[i + 1]));
    }
    if (i < ac - 1 && !strcmp(av[i], "--GB"))
    {
      GB = atof(av[i + 1]);
    }
    if (!strcmp(av[i], "--write"))
    {
      write = true;
    }
  }

  vtkImageData* hugeImage = vtkImageData::New();
  int edge = static_cast<int>(pow((1024l * 1024 * 1024 * GB) / (3 * VTK_SIZEOF_INT), 1 / 3.));
  hugeImage->SetDimensions(edge, edge, edge);
  hugeImage->AllocateScalars(VTK_INT, 3);

  if (write)
  {
    cout << "Populate it." << endl;
    int* ptr = static_cast<int*>(hugeImage->GetScalarPointer());
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
          *ptr = 42;
          ++ptr;
          *ptr = (int)((x * y * z + 0.125) * 4.0 * VTK_INT_MAX);
          ++ptr;
          *ptr = (int)x;
          ++ptr;
        }
      }
    }
  }

  vtkImageData* d = vtkImageData::New();
  d->DeepCopy(hugeImage);

  vtkSmartPointer<vtkDataSetWriter> dsw = vtkSmartPointer<vtkDataSetWriter>::New();
  dsw->SetInputData(hugeImage);
  dsw->SetFileName("source.vtk");
  dsw->SetInputData(d);
  dsw->SetFileName("dest.vtk");
  if (write)
  {
    cout << "Write them." << endl;
    dsw->Write();
    dsw->Write();
  }

  hugeImage->Delete();
  d->Delete();

  return EXIT_SUCCESS;
}
