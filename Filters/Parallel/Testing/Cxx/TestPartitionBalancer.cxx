/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPartitionBalancer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

#include "vtkPartitionBalancer.h"

#include "vtkFieldData.h"
#include "vtkImageData.h"
#include "vtkLogger.h"
#include "vtkMPIController.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkStringArray.h"

namespace
{
constexpr const char* names[2][4] = { { "r0_PD0_DS0", "r0_PD0_DS1", "r0_PD0_DS2", "r0_PD1_DS0" },
  { "r1_PD0_DS0", "r2_PD0_DS1", nullptr, nullptr } };

//----------------------------------------------------------------------------
vtkNew<vtkImageData> GenerateDataSet(int rank, int id)
{
  vtkNew<vtkStringArray> array;
  array->SetName(names[rank][id]);
  vtkNew<vtkImageData> ds;
  ds->GetFieldData()->AddArray(array);
  return ds;
}

//----------------------------------------------------------------------------
bool TestExpandPDS0(vtkPartitionedDataSet* outPDS0, int rank)
{
  bool retVal = true;

  if (outPDS0->GetNumberOfPartitions() != 5)
  {
    vtkLog(ERROR,
      "Wrong number of generated partitions in PD0 in rank "
        << rank << ". There are " << outPDS0->GetNumberOfPartitions() << " instead of 5.");
    retVal = false;
  }

  vtkDataSet* outDS0 = outPDS0->GetPartition(0);
  vtkDataSet* outDS1 = outPDS0->GetPartition(1);
  vtkDataSet* outDS2 = outPDS0->GetPartition(2);
  vtkDataSet* outDS3 = outPDS0->GetPartition(3);
  vtkDataSet* outDS4 = outPDS0->GetPartition(4);

  if (rank == 0)
  {
    if (!outDS0 || !outDS1 || !outDS2)
    {
      vtkLog(ERROR,
        "Output partitioned data set r0 - PD0 has nullptr at wrong places."
          << " All those pointers should be non nullptr: DS0 == " << outDS0 << ", DS1 == " << outDS1
          << ", DS2 == " << outDS2);
      retVal = false;
    }
    if (retVal != EXIT_FAILURE &&
      (!outDS0->GetFieldData()->GetAbstractArray(names[0][0]) ||
        !outDS1->GetFieldData()->GetAbstractArray(names[0][1]) ||
        !outDS2->GetFieldData()->GetAbstractArray(names[0][2])))
    {
      vtkLog(ERROR, "Output partitioned data set r0 - PD0 have been wrongly copied.");
      retVal = false;
    }
    if (outDS3 || outDS4)
    {
      vtkLog(ERROR,
        "Output partitioned data set r0 - PD0"
          << " should have nullptr at partition 3 and 4");
      retVal = false;
    }
  }
  if (rank == 1)
  {
    if (!outDS3 || !outDS4)
    {
      vtkLog(ERROR,
        "Output partitioned data set r1 - PD0 has nullptr at wrong places."
          << " All those pointers should be non nullptr: DS3 == " << outDS3
          << ", DS4 == " << outDS4);
      retVal = false;
    }
    if (retVal != EXIT_FAILURE &&
      (!outDS3->GetFieldData()->GetAbstractArray(names[1][0]) ||
        !outDS4->GetFieldData()->GetAbstractArray(names[1][1])))
    {
      vtkLog(ERROR, "Output partitioned data set r1 - PD0 have been wrongly copied.");
      retVal = false;
    }
    if (outDS0 || outDS1 || outDS2)
    {
      vtkLog(ERROR,
        "Output partitioned data set r1 - PD0"
          << " should have nullptr at partition 3 and 4");
      retVal = false;
    }
  }

  return retVal;
}

//----------------------------------------------------------------------------
bool TestExpandPDS1(vtkPartitionedDataSet* outPDS1, int rank)
{
  bool retVal = true;

  if (outPDS1->GetNumberOfPartitions() != 1)
  {
    vtkLog(ERROR,
      "Wrong number of generated partitions in PD0 in rank "
        << rank << ". There are " << outPDS1->GetNumberOfPartitions() << " instead of 1.");
    retVal = false;
  }

  vtkDataSet* outDS100 = outPDS1->GetPartition(0);

  if (rank == 0)
  {
    if (!outDS100)
    {
      vtkLog(ERROR, "Output partitioned data set r0 - PD1 has a nullptr partition.") retVal = false;
    }
    else if (!outDS100->GetFieldData()->GetAbstractArray(names[0][3]))
    {
      vtkLog(ERROR, "DS100 has been wrongly copied in rank 0.");
    }
  }
  if (rank == 1)
  {
    if (outDS100)
    {
      vtkLog(ERROR, "Output partitioned data set r1 - PD1 should have a nullptr partition.")
        retVal = false;
    }
  }

  return retVal;
}

//----------------------------------------------------------------------------
bool TestSquashPDS0(vtkPartitionedDataSet* outPDS0, int rank)
{
  bool retVal = true;

  if (outPDS0->GetNumberOfPartitions() != 3)
  {
    vtkLog(ERROR,
      "Wrong number of generated partitions in PD0 in rank "
        << rank << ". There are " << outPDS0->GetNumberOfPartitions() << " instead of 3.");
    retVal = false;
  }

  if (rank == 0)
  {
    vtkDataSet* outDS0 = outPDS0->GetPartition(0);
    vtkDataSet* outDS1 = outPDS0->GetPartition(1);
    vtkDataSet* outDS2 = outPDS0->GetPartition(2);

    if (!outDS0 || !outDS1 || !outDS2)
    {
      vtkLog(ERROR,
        "Output partitioned data set r0 - PD0 has nullptr at wrong places."
          << " All those pointers should be non nullptr: DS0 == " << outDS0 << ", DS1 == " << outDS1
          << ", DS2 == " << outDS2);
      retVal = false;
    }
    if (retVal != false &&
      (!outDS0->GetFieldData()->GetAbstractArray(names[0][0]) ||
        !outDS1->GetFieldData()->GetAbstractArray(names[0][1]) ||
        !outDS2->GetFieldData()->GetAbstractArray(names[0][2])))
    {
      vtkLog(ERROR, "Output partitioned data set r0 - PD0 have been wrongly copied.");
      retVal = false;
    }
  }
  if (rank == 1)
  {
    vtkDataSet* outDS3 = outPDS0->GetPartition(0);
    vtkDataSet* outDS4 = outPDS0->GetPartition(1);
    vtkDataSet* outDS5 = outPDS0->GetPartition(2);

    if (!outDS3 || !outDS4)
    {
      vtkLog(ERROR,
        "Output partitioned data set r1 - PD0 has nullptr at wrong places."
          << " All those pointers should be non nullptr: DS3 == " << outDS3
          << ", DS4 == " << outDS4);
      retVal = false;
    }
    if (retVal != false &&
      (!outDS3->GetFieldData()->GetAbstractArray(names[1][0]) ||
        !outDS4->GetFieldData()->GetAbstractArray(names[1][1])))
    {
      vtkLog(ERROR, "Output partitioned data set r1 - PD0 have been wrongly copied.");
      retVal = false;
    }
    if (outDS5)
    {
      vtkLog(ERROR,
        "Output partitioned data set r1 - PD0"
          << " should have nullptr at partition 2");
      retVal = false;
    }
  }

  return retVal;
}

//----------------------------------------------------------------------------
bool TestSquashPDS1(vtkPartitionedDataSet* outPDS1, int rank)
{
  bool retVal = true;

  if (outPDS1->GetNumberOfPartitions() != 1)
  {
    vtkLog(ERROR,
      "Wrong number of generated partitions in PD1 in rank "
        << rank << ". There are " << outPDS1->GetNumberOfPartitions() << " instead of 1.");
    retVal = false;
  }

  vtkDataSet* outDS100 = outPDS1->GetPartition(0);

  if (rank == 0)
  {
    if (!outDS100)
    {
      vtkLog(ERROR, "Output partitioned data set r0 - PD1 has a nullptr partition.") retVal = false;
    }
    else if (!outDS100->GetFieldData()->GetAbstractArray(names[0][3]))
    {
      vtkLog(ERROR, "DS100 has been wrongly copied in rank 0.");
    }
  }
  if (rank == 1)
  {
    if (outDS100)
    {
      vtkLog(ERROR, "Output partitioned data set r1 - PD1 should have a nullptr partition.")
        retVal = false;
    }
  }

  return retVal;
}
} // anonmymous namespace

//----------------------------------------------------------------------------
int TestPartitionBalancer(int argc, char* argv[])
{
  int retVal = EXIT_SUCCESS;

  vtkNew<vtkMPIController> controller;

  controller->Initialize(&argc, &argv);
  vtkMultiProcessController::SetGlobalController(controller);

  int rank = controller->GetLocalProcessId();

  vtkNew<vtkPartitionedDataSetCollection> pdsc;
  pdsc->SetNumberOfPartitionedDataSets(2);

  // rank 0: PDC [ PD (DS0, DS1,     DS2) ] [PD (nullptr, DS100) ]
  // rank 1: PDC [ PD (DS3, nullptr, DS4) ] [PD () ]

  if (rank == 0)
  {
    vtkNew<vtkPartitionedDataSet> pds0;
    pds0->SetNumberOfPartitions(3);
    pds0->SetPartition(0, GenerateDataSet(0, 0));
    pds0->SetPartition(1, GenerateDataSet(0, 1));
    pds0->SetPartition(2, GenerateDataSet(0, 2));
    pdsc->SetPartitionedDataSet(0, pds0);

    vtkNew<vtkPartitionedDataSet> pds1;
    pds1->SetNumberOfPartitions(2);
    pds1->SetPartition(0, nullptr);
    pds1->SetPartition(0, GenerateDataSet(0, 3));
    pdsc->SetPartitionedDataSet(1, pds1);
  }
  else if (rank == 1)
  {
    vtkNew<vtkPartitionedDataSet> pds0;
    pds0->SetNumberOfPartitions(3);
    pds0->SetPartition(0, GenerateDataSet(1, 0));
    pds0->SetPartition(1, nullptr);
    pds0->SetPartition(2, GenerateDataSet(1, 1));
    pdsc->SetPartitionedDataSet(0, pds0);

    pdsc->SetPartitionedDataSet(1, vtkNew<vtkPartitionedDataSet>());
  }

  vtkNew<vtkPartitionBalancer> balancer;
  balancer->SetInputDataObject(pdsc);
  balancer->SetController(controller);

  if (rank == 0)
  {
    vtkLog(INFO, "Testing vtkPartitionBalancer for vtkPartitionedDataSetCollection input");
  }

  if (rank == 0)
  {
    vtkLog(INFO, "*** Expand mode");
  }

  balancer->SetModeToExpand();
  balancer->Update();

  {
    // Looking if output looks like that:
    // rank 0: PDC [ PD (DS0,     DS1,     DS2,     nullptr, nullptr) ] [PD (DS100)   ]
    // rank 1: PDC [ PD (nullptr, nullptr, nullptr, DS3,     DS4)     ] [PD (nullptr) ]
    auto outPDSC = vtkPartitionedDataSetCollection::SafeDownCast(balancer->GetOutputDataObject(0));

    if (outPDSC->GetNumberOfPartitionedDataSets() != 2)
    {
      vtkLog(ERROR,
        "Wrong number of generated partitioned data sets in rank "
          << rank << ". There are " << outPDSC->GetNumberOfPartitionedDataSets()
          << " instead of 2");
      retVal = EXIT_FAILURE;
    }

    retVal = !TestExpandPDS0(outPDSC->GetPartitionedDataSet(0), rank) ||
        !TestExpandPDS1(outPDSC->GetPartitionedDataSet(1), rank)
      ? EXIT_FAILURE
      : retVal;
  }

  if (rank == 0)
  {
    vtkLog(INFO, "*** Squash mode");
  }

  balancer->SetModeToSquash();
  balancer->Update();

  {
    // Looking if output looks like that:
    //  rank 0: PDC [ PD (DS0, DS1, DS2)     ] [PD (DS100)   ]
    //  rank 1: PDC [ PD (DS3, DS4, nullptr) ] [PD (nullptr) ]
    auto outPDSC = vtkPartitionedDataSetCollection::SafeDownCast(balancer->GetOutputDataObject(0));

    if (outPDSC->GetNumberOfPartitionedDataSets() != 2)
    {
      vtkLog(ERROR,
        "Wrong number of generated partitioned data sets in rank "
          << rank << ". There are " << outPDSC->GetNumberOfPartitionedDataSets()
          << " instead of 2");
      retVal = EXIT_FAILURE;
    }

    retVal = !TestSquashPDS0(outPDSC->GetPartitionedDataSet(0), rank) ||
        !TestSquashPDS1(outPDSC->GetPartitionedDataSet(1), rank)
      ? EXIT_FAILURE
      : retVal;
  }

  if (rank == 0)
  {
    vtkLog(INFO, "Testing vtkPartitionBalancer for vtkPartitionedDataSet input");
  }

  // Here, we will test the same input as before, but directly feed in partitioned data sets.

  if (rank == 0)
  {
    vtkLog(INFO, "*** Expand mode");
  }

  balancer->SetModeToExpand();
  balancer->SetInputDataObject(pdsc->GetPartitionedDataSet(0));
  balancer->Update();

  retVal =
    !TestExpandPDS0(vtkPartitionedDataSet::SafeDownCast(balancer->GetOutputDataObject(0)), rank)
    ? EXIT_FAILURE
    : retVal;

  balancer->SetInputDataObject(pdsc->GetPartitionedDataSet(1));
  balancer->Update();

  retVal =
    !TestExpandPDS1(vtkPartitionedDataSet::SafeDownCast(balancer->GetOutputDataObject(0)), rank)
    ? EXIT_FAILURE
    : retVal;

  if (rank == 0)
  {
    vtkLog(INFO, "*** Squash mode");
  }

  balancer->SetModeToSquash();
  balancer->SetInputDataObject(pdsc->GetPartitionedDataSet(0));
  balancer->Update();

  retVal =
    !TestSquashPDS0(vtkPartitionedDataSet::SafeDownCast(balancer->GetOutputDataObject(0)), rank)
    ? EXIT_FAILURE
    : retVal;

  balancer->SetInputDataObject(pdsc->GetPartitionedDataSet(1));
  balancer->Update();

  retVal =
    !TestSquashPDS1(vtkPartitionedDataSet::SafeDownCast(balancer->GetOutputDataObject(0)), rank)
    ? EXIT_FAILURE
    : retVal;

  controller->Finalize();

  return retVal;
}
