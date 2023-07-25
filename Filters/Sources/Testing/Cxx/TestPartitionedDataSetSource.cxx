// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkParametricKlein.h"
#include <vtkLogger.h>
#include <vtkPartitionedDataSet.h>
#include <vtkPartitionedDataSetSource.h>
#include <vtkSmartPointer.h>

int TestPartitionedDataSetSource(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  {
    vtkNew<vtkPartitionedDataSetSource> pdsSource;
    pdsSource->SetParametricFunction(vtkSmartPointer<vtkParametricKlein>::New());

    if (!pdsSource->IsEnabledRank(1))
    {
      vtkLogF(ERROR, "vtkPartitionedDataSetSource::IsEnabledRank(1) must be true");
      return EXIT_FAILURE;
    }

    pdsSource->DisableRank(1);
    if (pdsSource->IsEnabledRank(1))
    {
      vtkLogF(ERROR, "vtkPartitionedDataSetSource::IsEnabledRank(1) must be false");
      return EXIT_FAILURE;
    }

    pdsSource->DisableRank(0);
    pdsSource->DisableRank(2);
    pdsSource->DisableRank(4);
    pdsSource->EnableRank(4);
    pdsSource->Update();

    // Rank 0 is enabled
    auto pds = pdsSource->GetOutput();
    if (pds->GetNumberOfPartitions() != 0)
    {
      vtkLogF(ERROR, "vtkPartitionedDataSetSource::GetNumberOfPartitions() must be 0");
      return EXIT_FAILURE;
    }

    // Rank 2 is disabled
    pdsSource->UpdatePiece(2, 5, 0);
    pds = pdsSource->GetOutput();
    if (pds->GetNumberOfPartitions() != 0)
    {
      vtkLogF(ERROR, "GetOutput(rank2) returns empty PartitionedDataSet");
      return EXIT_FAILURE;
    }

    // Rank 4 is enabled
    pdsSource->UpdatePiece(4, 5, 0);
    pds = pdsSource->GetOutput();
    if (pds->GetNumberOfPartitions() != 1)
    {
      vtkLogF(ERROR, "vtkPartitionedDataSetSource::GetNumberOfPartitions() (rank4) must be 1");
      return EXIT_FAILURE;
    }
  }

  // Now we specify the number of partitions
  {
    vtkNew<vtkPartitionedDataSetSource> pdsSource;
    pdsSource->SetParametricFunction(vtkSmartPointer<vtkParametricKlein>::New());

    pdsSource->SetNumberOfPartitions(6);
    pdsSource->DisableRank(1);
    pdsSource->DisableRank(2);

    // Rank 0 is enabled
    pdsSource->UpdatePiece(0, 5, 0);
    auto pds = pdsSource->GetOutput();
    if (pds->GetNumberOfPartitions() != 2)
    {
      vtkLogF(ERROR, "vtkPartitionedDataSetSource::GetNumberOfPartitions() must be 2");
      return EXIT_FAILURE;
    }

    // Rank 4 is enabled
    pdsSource->UpdatePiece(4, 5, 0);
    pds = pdsSource->GetOutput();
    if (pds->GetNumberOfPartitions() != 2)
    {
      vtkLogF(ERROR, "vtkPartitionedDataSetSource::GetNumberOfPartitions() must be 2");
      return EXIT_FAILURE;
    }
  }

  // Now we specified odd number of partitions
  {
    vtkNew<vtkPartitionedDataSetSource> pdsSource;
    pdsSource->SetNumberOfPartitions(5);

    // Rank 1 must have 2 partition
    pdsSource->UpdatePiece(1, 3, 0);
    auto pds = pdsSource->GetOutput();
    if (pds->GetNumberOfPartitions() != 2)
    {
      vtkLogF(ERROR, "vtkPartitionedDataSetSource::GetNumberOfPartitions() must be 2");
      return EXIT_FAILURE;
    }

    // Rank 2 must have only one partition
    pdsSource->UpdatePiece(2, 3, 0);
    pds = pdsSource->GetOutput();
    if (pds->GetNumberOfPartitions() != 1)
    {
      vtkLogF(ERROR, "vtkPartitionedDataSetSource::GetNumberOfPartitions() must be 1");
      return EXIT_FAILURE;
    }

    // Rank 0 must have only one partition
    pdsSource->UpdatePiece(0, 2, 0);
    pds = pdsSource->GetOutput();
    if (pds->GetNumberOfPartitions() != 3)
    {
      vtkLogF(ERROR, "vtkPartitionedDataSetSource::GetNumberOfPartitions() must be 3");
      return EXIT_FAILURE;
    }
  }

  // Now we user DisableAllRanks
  {
    vtkNew<vtkPartitionedDataSetSource> pdsSource;
    pdsSource->SetParametricFunction(vtkSmartPointer<vtkParametricKlein>::New());

    pdsSource->SetNumberOfPartitions(6);
    pdsSource->DisableAllRanks();

    // Rank 1 is disabled
    pdsSource->UpdatePiece(1, 5, 0);
    auto pds = pdsSource->GetOutput();
    if (pds->GetNumberOfPartitions() != 0)
    {
      vtkLogF(ERROR, "vtkPartitionedDataSetSource::GetNumberOfPartitions() must be 0");
      return EXIT_FAILURE;
    }

    pdsSource->EnableRank(1);
    pdsSource->EnableRank(2);

    // Rank 1 is enabled now
    pdsSource->UpdatePiece(1, 5, 0);
    pds = pdsSource->GetOutput();
    if (pds->GetNumberOfPartitions() != 3)
    {
      vtkLogF(ERROR, "vtkPartitionedDataSetSource::GetNumberOfPartitions() must be 3");
      return EXIT_FAILURE;
    }

    // Rank 3 is enabled
    pdsSource->UpdatePiece(4, 5, 0);
    pds = pdsSource->GetOutput();
    if (pds->GetNumberOfPartitions() != 0)
    {
      vtkLogF(ERROR, "vtkPartitionedDataSetSource::GetNumberOfPartitions() must be 0");
      return EXIT_FAILURE;
    }
  }

  // Now we pass a nullptr for ParametricFunction
  {
    vtkNew<vtkPartitionedDataSetSource> pdsSource;
    pdsSource->SetParametricFunction(nullptr);

    // Rank 0 is disabled
    pdsSource->UpdatePiece(0, 5, 0);
    auto pds = pdsSource->GetOutput();
    if (pds->GetNumberOfPartitions() != 0)
    {
      vtkLogF(ERROR, "vtkPartitionedDataSetSource::GetNumberOfPartitions() must be 0");
      return EXIT_FAILURE;
    }
  }

  // Now we set zero partitions (auto partitions)
  {
    vtkNew<vtkPartitionedDataSetSource> pdsSource;
    pdsSource->SetNumberOfPartitions(0);

    // Rank 0 is disabled
    pdsSource->UpdatePiece(0, 5, 0);
    auto pds = pdsSource->GetOutput();
    if (pds->GetNumberOfPartitions() != 1)
    {
      vtkLogF(ERROR, "vtkPartitionedDataSetSource::GetNumberOfPartitions() must be 1");
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}
