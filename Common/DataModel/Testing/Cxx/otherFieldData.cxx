// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellData.h"
#include "vtkDataSetAttributes.h"
#include "vtkDebugLeaks.h"
#include "vtkDoubleArray.h"
#include "vtkFieldData.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkLogger.h"
#include "vtkPointData.h"
#include "vtkUnsignedCharArray.h"

namespace
{
constexpr vtkIdType NUMBER_OF_VALS = 20;

constexpr double VALS[] = { 0, 1, 2, 3, 4, 5, 6, 7, 999, 25, 21, 1, 2, 4, 5, 6, 7, 3, 75, -10 };

constexpr unsigned char GHOSTS[] = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

//------------------------------------------------------------------------------
bool TestGhostAwareRange()
{
  bool retVal = true;

  vtkNew<vtkPointData> pd;
  if (pd->GetGhostsToSkip() != vtkDataSetAttributes::HIDDENPOINT)
  {
    vtkLog(ERROR, "GhostsToSkip has wrong default value in vtkPointData.");
    retVal = false;
  }

  vtkNew<vtkCellData> cd;
  if (cd->GetGhostsToSkip() !=
    (vtkDataSetAttributes::HIDDENCELL | vtkDataSetAttributes::REFINEDCELL))
  {
    vtkLog(ERROR, "GhostsToSkip has wrong default value in vtkCellData.");
    retVal = false;
  }

  vtkNew<vtkFieldData> fd;
  fd->SetNumberOfTuples(NUMBER_OF_VALS);
  fd->SetGhostsToSkip(0xff);

  vtkNew<vtkDoubleArray> values;
  values->SetName("Values");
  values->SetNumberOfValues(NUMBER_OF_VALS);

  vtkNew<vtkUnsignedCharArray> ghosts;
  ghosts->SetName(vtkDataSetAttributes::GhostArrayName());
  ghosts->SetNumberOfValues(NUMBER_OF_VALS);

  for (vtkIdType id = 0; id < NUMBER_OF_VALS; ++id)
  {
    values->SetValue(id, VALS[id]);
    ghosts->SetValue(id, GHOSTS[id] ? fd->GetGhostsToSkip() : 0);
  }

  fd->AddArray(values);

  double range[2];

  fd->GetRange(0, range);

  if (range[0] != -10 || range[1] != 999)
  {
    vtkLog(ERROR,
      "Wrong range when no ghosts are present in field data: [" << range[0] << ", " << range[1]
                                                                << "]");
    retVal = false;
  }

  values->SetValue(0, std::numeric_limits<double>::infinity());
  values->Modified();

  fd->GetFiniteRange(0, range);

  if (range[0] != -10 || range[1] != 999)
  {
    vtkLog(ERROR,
      "Wrong finite range when no ghosts are present in field data: [" << range[0] << ", "
                                                                       << range[1] << "]");
    retVal = false;
  }

  fd->GetRange(0, range);

  if (range[0] != -10 || range[1] != std::numeric_limits<double>::infinity())
  {
    vtkLog(ERROR,
      "Wrong finite range when no ghosts are present in field data: [" << range[0] << ", "
                                                                       << range[1] << "]");
    retVal = false;
  }

  fd->GetRange("foo", range);

  if (range[0] == range[0] || range[1] == range[1])
  {
    vtkLog(ERROR,
      "Field data should return NaN when querying a non-existing array [" << range[0] << ", "
                                                                          << range[1] << "]");
    retVal = false;
  }

  fd->AddArray(ghosts);

  fd->GetFiniteRange(0, range);

  if (range[0] != -10 || range[1] != 75)
  {
    vtkLog(ERROR,
      "Field data computed wrong finite range when ghosts are present. [" << range[0] << ", "
                                                                          << range[1] << "]");
    retVal = false;
  }

  values->SetValue(0, 0);
  values->Modified();

  fd->GetRange(0, range);

  if (range[0] != -10 || range[1] != 75)
  {
    vtkLog(ERROR,
      "Field data computed wrong range when ghosts are present. [" << range[0] << ", " << range[1]
                                                                   << "]");
    retVal = false;
  }

  ghosts->SetValue(NUMBER_OF_VALS - 1, fd->GetGhostsToSkip());
  ghosts->Modified();

  fd->GetRange(0, range);

  if (range[0] != 0 || range[1] != 75)
  {
    vtkLog(ERROR,
      "Field data computed wrong range when a value if ghost array was changed ["
        << range[0] << ", " << range[1] << "]");
    retVal = false;
  }

  fd->RemoveArray(ghosts->GetName());

  fd->GetRange(0, range);

  if (range[0] != -10 || range[1] != 999)
  {
    vtkLog(ERROR,
      "Field data computed wrong range when removing the ghost array [" << range[0] << ", "
                                                                        << range[1] << "]");
    retVal = false;
  }

  return retVal;
}
} // anonymous namespace

//------------------------------------------------------------------------------
int otherFieldData(int, char*[])
{
  int retVal = EXIT_SUCCESS;
  int i;
  vtkFieldData* fd = vtkFieldData::New();

  vtkFloatArray* fa;

  char name[128];
  for (i = 0; i < 5; i++)
  {
    snprintf(name, sizeof(name), "Array%d", i);
    fa = vtkFloatArray::New();
    fa->SetName(name);
    // the tuples must be set before being read to avoid a UMR
    // this must have been a UMR in the past that was suppressed
    fa->Allocate(20);
    fa->SetTuple1(0, 0.0);
    fa->SetTuple1(2, 0.0);
    fd->AddArray(fa);
    fa->Delete();
  }

  // Coverage
  vtkFieldData::Iterator it(fd);
  vtkFieldData::Iterator it2(it);
  (void)it;
  (void)it2;

  fd->Allocate(20);
  fd->CopyFieldOff("Array0");
  fd->CopyFieldOff("Array1");

  vtkFieldData* fd2 = fd->NewInstance();
  fd2->CopyStructure(fd);
  fd2->ShallowCopy(fd);
  fd2->DeepCopy(fd);

  vtkIdList* ptIds = vtkIdList::New();
  ptIds->InsertNextId(0);
  ptIds->InsertNextId(2);

  fd->GetField(ptIds, fd2);
  ptIds->Delete();

  int arrayComp;
  int a = fd->GetArrayContainingComponent(1, arrayComp);
  if (a != 1)
  {
    retVal = EXIT_FAILURE;
  }

  vtkLog(INFO, "Testing Ghost Aware Ranges...");
  if (!TestGhostAwareRange())
  {
    retVal = EXIT_FAILURE;
  }

  /* Obsolete API.
  double tuple[10];
  // initialize tuple before using it to set something
  for (i = 0; i < 10; i++)
    {
    tuple[i] = i;
    }
  fd->GetTuple(2);
  fd->SetTuple(2, tuple);
  fd->InsertTuple(2, tuple);
  fd->InsertNextTuple(tuple);
  fd->SetComponent(0,0, 1.0);
  fd->InsertComponent(0,0, 1.0);
  */
  fd2->Reset();

  fd->Delete();
  fd2->Delete();

  return retVal;
}
