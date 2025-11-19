// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Description:
// This tests vtkXdmfWriter and vtkXdmfReader
// It manufactures/reads a bunch of test data objects, writes them to disk
// via the xdmfwriter, reads the files back in with the xdmfreader, and
// compares the output of the reader against the input to the reader. If
// differences are found it fails and stops immediately, leaving any failed
// files around for inspection. Otherwise it deletes the temporary files it
// creates and returns success.

#include "vtkCellData.h"
#include "vtkDataObject.h"
#include "vtkDataObjectGenerator.h"
#include "vtkDataSet.h"
#include "vtkDataSetReader.h"
#include "vtkDataSetWriter.h"
#include "vtkPointData.h"
#include "vtkStringFormatter.h"
#include "vtkTimeSourceExample.h"
#include "vtkXdmfReader.h"
#include "vtkXdmfWriter.h"
#include "vtksys/SystemTools.hxx"

#ifndef _MSC_VER
#include <unistd.h>
#endif

#include <iostream>

#define NUMTESTS 20

const char testobject[NUMTESTS][40] = {
  "ID1",
  "ID2",
  "UF1",
  "RG1",
  "SG1",
  "PD1",
  "PD2",
  "UG1",
  "UG2",
  "UG3",
  "UG4",
  "MB{}",
  "MB{ID1}",
  "MB{UF1}",
  "MB{RG1}",
  "MB{SG1}",
  "MB{PD1}",
  "MB{UG1}",
  "MB{ ID1 UF1 RG1 SG1 PD1 UG1 }",
  "HB[ (UF1)(UF1)(UF1) ]",
};

bool CleanUpGood = true;

bool DoFilesExist(const char* xdmffile, const char* hdf5file, bool deleteIfSo)
{
  bool xexists = (xdmffile ? vtksys::SystemTools::FileExists(xdmffile) : true);
  bool hexists = (hdf5file ? vtksys::SystemTools::FileExists(hdf5file) : true);
  bool xlenOK = (xdmffile ? vtksys::SystemTools::FileLength(xdmffile) != 0 : true);
  bool hlenOK = (hdf5file ? vtksys::SystemTools::FileLength(hdf5file) != 0 : true);

  bool theyDo = xexists && xlenOK && hexists && hlenOK;
  if (theyDo && deleteIfSo && CleanUpGood)
  {
    if (xdmffile)
    {
      unlink(xdmffile);
    }
    if (hdf5file)
    {
      unlink(hdf5file);
    }
  }

  return theyDo;
}

bool DoDataObjectsDiffer(vtkDataObject* dobj1, vtkDataObject* dobj2)
{
  if (strcmp(dobj1->GetClassName(), dobj2->GetClassName()) != 0)
  {
    std::cerr << "Class name test failed " << dobj1->GetClassName()
              << " != " << dobj2->GetClassName() << std::endl;
    // return true; //disable for now
  }
  if (dobj1->GetFieldData()->GetNumberOfArrays() != dobj2->GetFieldData()->GetNumberOfArrays())
  {
    std::cerr << "Number of field arrays test failed" << std::endl;
    return true;
  }
  if (!dobj1->IsA("vtkPolyData") && !dobj1->IsA("vtkMultiBlockDataSet") &&
    dobj1->GetActualMemorySize() != dobj2->GetActualMemorySize())
  {
    std::cerr << "Mem size test failed" << std::endl;
    return true;
  }
  vtkDataSet* ds1 = vtkDataSet::SafeDownCast(dobj1);
  vtkDataSet* ds2 = vtkDataSet::SafeDownCast(dobj2);
  if (ds1 && ds2)
  {
    if ((ds1->GetNumberOfCells() != ds2->GetNumberOfCells()) ||
      (ds1->GetNumberOfPoints() != ds2->GetNumberOfPoints()))
    {
      std::cerr << "Number of Cells/Points test failed" << std::endl;
      return true;
    }
    const double* bds1 = ds1->GetBounds();
    const double* bds2 = ds2->GetBounds();
    if ((bds1[0] != bds2[0]) || (bds1[1] != bds2[1]) || (bds1[2] != bds2[2]) ||
      (bds1[3] != bds2[3]) || (bds1[4] != bds2[4]) || (bds1[5] != bds2[5]))
    {
      std::cerr << "Bounds test failed" << std::endl;
      return true;
    }
    if ((ds1->GetPointData()->GetNumberOfArrays() != ds2->GetPointData()->GetNumberOfArrays()) ||
      (ds1->GetCellData()->GetNumberOfArrays() != ds2->GetCellData()->GetNumberOfArrays()))
    {
      std::cerr << "Number of data arrays test failed" << std::endl;
      return true;
    }
    // TODO:Check array names, types, widths and ranges
  }
  return false;
}

bool TestXDMFConversion(vtkDataObject* input, char* prefix)
{
  auto xdmffile = vtk::format("{:s}.xmf", prefix);
  auto hdf5file = vtk::format("{:s}.h5", prefix);
  auto vtkfile = vtk::format("{:s}.vtk", prefix);

  vtkXdmfWriter* xwriter = vtkXdmfWriter::New();
  xwriter->SetLightDataLimit(10000);
  xwriter->WriteAllTimeStepsOn();
  xwriter->SetFileName(xdmffile.c_str());
  xwriter->SetInputData(input);
  xwriter->Write();

  xwriter->Delete();
  vtkDataSet* ds = vtkDataSet::SafeDownCast(input);
  if (ds)
  {
    vtkDataSetWriter* dsw = vtkDataSetWriter::New();
    dsw->SetFileName(vtkfile.c_str());
    dsw->SetInputData(ds);
    dsw->Write();
    dsw->Delete();
  }

  if (!DoFilesExist(xdmffile.c_str(), nullptr, false))
  {
    std::cerr << "Writer did not create " << xdmffile << std::endl;
    return true;
  }

  // TODO: Once it works, enable this
  vtkXdmfReader* xreader = vtkXdmfReader::New();
  xreader->SetFileName(xdmffile.c_str());
  xreader->Update();
  vtkDataObject* rOutput = xreader->GetOutputDataObject(0);

  bool fail = DoDataObjectsDiffer(input, rOutput);
  if (!fail && CleanUpGood)
  {
    // test passed!
    unlink(xdmffile.c_str());
    unlink(hdf5file.c_str());
    unlink(vtkfile.c_str());
  }

  xreader->Delete();
  return fail;
}

int XdmfTestVTKIO(int ac, char* av[])
{

  for (int i = 1; i < ac; i++)
  {
    if (!strcmp(av[i], "--dont-clean"))
    {
      CleanUpGood = false;
    }
  }

  bool fail = false;

  // TEST SET 1
  vtkDataObjectGenerator* dog = vtkDataObjectGenerator::New();
  int i = 0;
  while (!fail && i < NUMTESTS)
  {
    auto filename = vtk::format("xdmfIOtest_{:d}", i);
    std::cerr << "Test vtk object " << testobject[i] << std::endl;
    dog->SetProgram(testobject[i]);
    dog->Update();
    fail = TestXDMFConversion(dog->GetOutput(), filename.data());
    i++;
  }

  dog->Delete();

  if (fail)
  {
    return VTK_ERROR;
  }

  // TEST SET 2
  std::cerr << "Test temporal data" << std::endl;
  vtkTimeSourceExample* tsrc = vtkTimeSourceExample::New();
  tsrc->GrowingOn();
  tsrc->SetXAmplitude(2.0);

  vtkXdmfWriter* xwriter = vtkXdmfWriter::New();
  xwriter->SetLightDataLimit(10000);
  xwriter->WriteAllTimeStepsOn();
  xwriter->SetFileName("xdmfIOtest_temporal_1.xmf");
  xwriter->SetInputConnection(0, tsrc->GetOutputPort(0));
  xwriter->Write();

  xwriter->Delete();
  tsrc->Delete();

  fail = !DoFilesExist("xdmfIOtest_temporal_1.xmf", nullptr, true);
  if (fail)
  {
    std::cerr << "Failed Temporal Test 1" << std::endl;
    return VTK_ERROR;
  }

  // ETC.
  return 0;
}
