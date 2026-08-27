// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Tests the node and element user ids read by vtkLSDynaReader against
// the ids parsed directly from the LS-DYNA input deck.

#include "vtkLSDynaReader.h"

#include "vtkCellData.h"
#include "vtkCompositeDataSet.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkStringScanner.h"
#include "vtkTestUtilities.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <system_error>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace
{

using ElemIds = std::vector<std::size_t>;
using NodeIds = std::unordered_set<std::size_t>;

constexpr char SPACE = ' ';

// Convert a string to std::size_t. Returns the maximum std::size_t
// value when the string is not a valid number.
std::size_t ToSizeT(const std::string& s)
{
  std::size_t value = 0;
  auto result = vtk::from_chars(s, value);
  if (result.ec != std::errc())
  {
    std::cerr << "failed to parse an id from: " << s << std::endl;
    return std::numeric_limits<std::size_t>::max();
  }
  return value;
}

// Parse the *NODE and *ELEMENT_ blocks of a LS-DYNA input deck and
// return the element and node ids per part id. Returns an empty map
// when the input can not be parsed.
std::unordered_map<std::size_t, std::tuple<ElemIds, NodeIds>> ReadIdsFromDynaInput(
  const std::string& fpath)
{
  auto split = [](const std::string& s) -> std::vector<std::string>
  {
    std::vector<std::string> result;
    std::size_t start = 0, end = 0;
    while ((start = s.find_first_not_of(SPACE, end)) != std::string::npos)
    {
      end = s.find_first_of(SPACE, start);
      result.push_back(s.substr(start, end - start));
    }
    return result;
  };

  std::unordered_map<std::size_t, std::tuple<ElemIds, NodeIds>> results;

  std::ifstream ifs(fpath);
  if (!ifs.is_open())
  {
    std::cerr << "failed to open LS-DYNA input: " << fpath << std::endl;
    return {};
  }

  std::string line;
  std::string block;
  std::vector<std::size_t> nodeid_glob;

  while (std::getline(ifs, line))
  {
    line.erase(0, line.find_first_not_of(SPACE));
    line.erase(line.find_last_not_of(SPACE) + 1);

    if (line[0] == '$')
    {
      // ignore comments
      continue;
    }
    if (line[0] == '*')
    {
      block = line.substr(1);
      continue;
    }
    if (block == "NODE")
    {
      auto s_nid = line.substr(0, line.find_first_of(SPACE));
      nodeid_glob.push_back(ToSizeT(s_nid));
    }
    else if (block.substr(0, 8) == "ELEMENT_")
    {
      auto splitted = split(line);
      if (splitted.size() <= 2)
      {
        std::cerr << "wrong input: eid, pid and connectivities are expected" << std::endl;
        return {};
      }

      const std::size_t eid = ToSizeT(splitted[0]);
      const std::size_t pid = ToSizeT(splitted[1]);
      if (results.find(pid) == results.end())
      {
        // append new pid
        results.emplace(pid, std::make_tuple<ElemIds, NodeIds>({}, {}));
      }
      auto& res = results.at(pid);
      std::get<0>(res).push_back(eid);
      for (std::size_t i = 2; i < splitted.size(); i++)
      {
        auto nid = ToSizeT(splitted[i]);
        if (std::find(nodeid_glob.begin(), nodeid_glob.end(), nid) == nodeid_glob.end())
        {
          std::cerr << "wrong input: nid must be in the global nodes" << std::endl;
          return {};
        }
        std::get<1>(res).emplace(nid);
      }
    }
  }
  return results;
}

// Extract the part id from a part name such as "Part5". Returns the
// maximum std::size_t value when the name has an unexpected format.
std::size_t GetPartId(const std::string& pname)
{
  if (pname.substr(0, 4) != "Part")
  {
    std::cerr << "unexpected part name: " << pname << std::endl;
    return std::numeric_limits<std::size_t>::max();
  }
  return ToSizeT(pname.substr(4));
}

}

int TestLSDynaReaderUserIds(int argc, char* argv[])
{
  constexpr char name_elem_id[] = "UserIds";
  constexpr char name_node_id[] = "UserID";

  // Read elem/node IDs from LS-DYNA input directly
  char* fname_dynainp =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/LSDyna/hemi.draw/hemi_draw.k");
  const auto expected_ids = ReadIdsFromDynaInput(fname_dynainp);
  delete[] fname_dynainp;
  if (expected_ids.empty())
  {
    std::cerr << "no expected ids could be read from the LS-DYNA input" << std::endl;
    return EXIT_FAILURE;
  }

  // Read file name.
  char* fname =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/LSDyna/hemi.draw/hemi_draw.d3plot");

  // Create the reader.
  vtkNew<vtkLSDynaReader> reader;
  reader->SetFileName(fname);
  reader->Update();
  delete[] fname;

  auto mesh_all = reader->GetOutput();
  auto num_blocks = mesh_all->GetNumberOfBlocks();

  // NOTE: Part5 is not contained in LS-DYNA input file explicitly
  //       because it is rigid plane.
  const std::unordered_set<std::size_t> skipped_pids{ 5 };

  for (decltype(num_blocks) iblock = 0; iblock < num_blocks; iblock++)
  {
    auto info = mesh_all->GetMetaData(iblock);
    auto pid = GetPartId(info->Get(vtkCompositeDataSet::NAME()));
    if (pid == std::numeric_limits<std::size_t>::max())
    {
      return EXIT_FAILURE;
    }

    auto block = vtkPointSet::SafeDownCast(mesh_all->GetBlock(iblock));
    auto arr_elem_id =
      vtkArrayDownCast<vtkIdTypeArray>(block->GetCellData()->GetAbstractArray(name_elem_id));
    auto arr_node_id =
      vtkArrayDownCast<vtkIdTypeArray>(block->GetPointData()->GetAbstractArray(name_node_id));

    if (!arr_elem_id)
    {
      std::cerr << "user elem ID (" << name_elem_id << ") not found or not a vtkIdTypeArray"
                << std::endl;
      return EXIT_FAILURE;
    }
    if (!arr_node_id)
    {
      std::cerr << "user node ID (" << name_node_id << ") not found or not a vtkIdTypeArray"
                << std::endl;
      return EXIT_FAILURE;
    }

    if (skipped_pids.find(pid) != skipped_pids.end())
    {
      continue;
    }

    const auto item = expected_ids.find(pid);
    if (item == expected_ids.end())
    {
      std::cerr << "part id " << pid << " not found in the LS-DYNA input" << std::endl;
      return EXIT_FAILURE;
    }
    const auto& expected_eid = std::get<0>(item->second);
    const auto& expected_nid = std::get<1>(item->second);

    if (expected_eid.size() != static_cast<std::size_t>(arr_elem_id->GetNumberOfTuples()))
    {
      std::cerr << "length mismatch for elem IDs: " << expected_eid.size() << ", "
                << arr_elem_id->GetNumberOfTuples() << std::endl;
      return EXIT_FAILURE;
    }
    if (expected_nid.size() != static_cast<std::size_t>(arr_node_id->GetNumberOfTuples()))
    {
      std::cerr << "length mismatch for node IDs: " << expected_nid.size() << ", "
                << arr_node_id->GetNumberOfTuples() << std::endl;
      return EXIT_FAILURE;
    }

    for (vtkIdType i = 0; i < arr_elem_id->GetNumberOfTuples(); i++)
    {
      const vtkIdType expected =
        static_cast<vtkIdType>(expected_eid.at(static_cast<std::size_t>(i)));
      if (arr_elem_id->GetValue(i) != expected)
      {
        std::cerr << "value mismatch for user elem ID (" << i << "): " << arr_elem_id->GetValue(i)
                  << ", " << expected << std::endl;
        return EXIT_FAILURE;
      }
    }

    for (vtkIdType i = 0; i < arr_node_id->GetNumberOfTuples(); i++)
    {
      // TODO: check the order of node IDs
      const auto nid = static_cast<std::size_t>(arr_node_id->GetValue(i));
      if (expected_nid.find(nid) == expected_nid.end())
      {
        std::cerr << "unexpected value for user node ID (" << i << "): " << arr_node_id->GetValue(i)
                  << std::endl;
        return EXIT_FAILURE;
      }
    }
  }

  return EXIT_SUCCESS;
}
