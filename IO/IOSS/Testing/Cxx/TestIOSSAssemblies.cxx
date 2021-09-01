/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestIOSSAssemblies.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <vtkDataArraySelection.h>
#include <vtkDataAssembly.h>
#include <vtkIOSSReader.h>
#include <vtkInformation.h>
#include <vtkLogger.h>
#include <vtkPartitionedDataSet.h>
#include <vtkPartitionedDataSetCollection.h>
#include <vtkTestUtilities.h>

#include <set>
#include <string>
#include <vector>

static std::string GetFileName(int argc, char* argv[], const char* fnameC)
{
  char* fileNameC = vtkTestUtilities::ExpandDataFileName(argc, argv, fnameC);
  std::string fname(fileNameC);
  delete[] fileNameC;
  return fname;
}

static bool Validate(vtkPartitionedDataSetCollection* pdc, const std::string& query,
  const std::set<std::string>& blocknames)
{
  auto assembly = pdc->GetDataAssembly();

  std::set<unsigned int> dataset_indices;
  for (const auto& node : assembly->SelectNodes({ query }))
  {
    auto indices = assembly->GetDataSetIndices(node);
    dataset_indices.insert(indices.begin(), indices.end());
  }

  std::set<std::string> chosen_names;
  for (auto& idx : dataset_indices)
  {
    auto info = pdc->GetMetaData(idx);
    if (info->Has(vtkCompositeDataSet::NAME()))
    {
      chosen_names.insert(info->Get(vtkCompositeDataSet::NAME()));
    }
  }

  if (blocknames != chosen_names)
  {
    vtkLogF(ERROR, "Check failed for query '%s'", query.c_str());
    return false;
  }

  return true;
}

int TestIOSSAssemblies(int argc, char* argv[])
{
  auto fname = ::GetFileName(argc, argv, "Data/Exodus/Assembly-Example.g");
  vtkNew<vtkIOSSReader> reader;
  reader->SetFileName(fname.c_str());
  reader->UpdateInformation();

  // disable all blocks.
  reader->GetElementBlockSelection()->DisableAllArrays();

  // enable only "Low".
  reader->SetSelector("//Low");

  reader->Update();

  auto pdc = vtkPartitionedDataSetCollection::SafeDownCast(reader->GetOutputDataObject(0));

  if (!::Validate(pdc, "//assemblies/Low", { "block_1", "block_2", "block_3", "block_4" }) ||
    !::Validate(
      pdc, "//assemblies/Conglomerate/Top/Odd", { "block_1", "block_3", "block_5", "block_7" }) ||
    !::Validate(
      pdc, "//assemblies/Conglomerate/Top/Even", { "block_2", "block_4", "block_6", "block_8" }) ||
    !::Validate(
      pdc, "//assemblies/Conglomerate/Top/Prime", { "block_2", "block_3", "block_5", "block_7" }) ||
    !::Validate(
      pdc, "//assemblies/Conglomerate/Mid", { "block_3", "block_4", "block_5", "block_6" }) ||
    !::Validate(
      pdc, "//assemblies/Conglomerate/High", { "block_5", "block_6", "block_7", "block_8" }))
  {
    return EXIT_FAILURE;
  }

  // ensure only "Low" blocks were read in.
  for (unsigned int cc = 0; cc < pdc->GetNumberOfPartitionedDataSets(); ++cc)
  {
    if (cc < 4)
    {
      if (pdc->GetNumberOfPartitions(cc) == 0)
      {
        vtkLogF(ERROR, "'Low' block wasn't read!");
        return EXIT_FAILURE;
      }
    }
    else if (pdc->GetNumberOfPartitions(cc) != 0)
    {
      vtkLogF(ERROR, "Non-'Low' block was read!");
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}
