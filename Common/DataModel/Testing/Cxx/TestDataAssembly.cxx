// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkLogger.h"

#include <algorithm>
#include <iterator>
#include <map>
#include <unordered_map>
#include <vector>

#include "vtkDataAssembly.h"
#include "vtkNew.h"

#include <sstream>

namespace
{

void Assemble(vtkDataAssembly* assembly, const std::vector<int>& children, int& count,
  int parent = 0, int depth = 1)
{
  if (children.empty())
  {
    return;
  }
  std::vector<int> subset(children.size() - 1);
  std::copy(std::next(children.begin(), 1), children.end(), subset.begin());
  for (auto cc = 0; cc < children.front(); ++cc)
  {
    auto name = ("Child[" + std::to_string(depth) + "]#" + std::to_string(cc));
    auto child =
      assembly->AddNode(vtkDataAssembly::MakeValidNodeName(name.c_str()).c_str(), parent);
    ++count;
    Assemble(assembly, subset, count, child, (depth + 1));
  }
}

struct TestFailed : public std::exception
{
};
}

#define VERIFY(x)                                                                                  \
  do                                                                                               \
  {                                                                                                \
    if (!(x))                                                                                      \
    {                                                                                              \
      vtkLogF(ERROR, "Failed test '%s'", #x);                                                      \
      throw TestFailed{};                                                                          \
    }                                                                                              \
  } while (false)

int TestDataAssembly(int, char*[])
{
  vtkNew<vtkDataAssembly> assembly;
  {
    vtkLogScopeF(INFO, "Assemble");
    int count;
    Assemble(assembly, { 10, 10, 1000 }, count);
    vtkLogF(INFO, "count=%d", count);
  }

  vtkObject::GlobalWarningDisplayOff();
  VERIFY(assembly->InitializeFromXML("<node  id='0' />") == false);
  vtkObject::GlobalWarningDisplayOn();

  assembly->Initialize();
  assembly->SetNodeName(vtkDataAssembly::GetRootNode(), "exodus");
  auto groups = assembly->AddNodes({ "blocks", "sets" });
  auto blocks = assembly->AddNodes({ "element", "face", "edge" }, groups[0]);
  auto sets = assembly->AddNodes({ "element", "face", "edge" }, groups[1]);
  auto elem_blocks =
    assembly->AddNodes({ "b_one", "b_two", "b_three", "b_four", "b_five" }, blocks[0]);
  auto elem_sets = assembly->AddNodes({ "s_one", "s_two", "s_three", "s_four", "s_five" }, sets[0]);
  assembly->AddDataSetIndices(elem_blocks[0], { 0, 1, 2, 3, 4, 5 });
  assembly->AddDataSetIndices(elem_sets[0], { 8, 9, 10 });
  assembly->AddDataSetIndices(elem_sets[1], { 6, 7, 8 });
  // add dataset on a non-leaf node.
  assembly->AddDataSetIndices(groups[1], { 11 });
  assembly->Print(cout);

  try
  {
    vtkLogF(INFO, "path= %s", assembly->GetNodePath(sets[0]).c_str());
    VERIFY(strcmp(assembly->GetRootNodeName(), "exodus") == 0);
    VERIFY(strcmp(assembly->GetNodeName(elem_sets[4]), "s_five") == 0);
    VERIFY(assembly->FindFirstNodeWithName("s_five") == elem_sets[4]);
    VERIFY(assembly->GetChildNodes(groups[0], /*traverse_subtree*/ false) == blocks);

    // test remove node works
    VERIFY(assembly->RemoveNode(elem_sets[4]));
    VERIFY(assembly->FindFirstNodeWithName("s_five") == -1);

    // re-add the removed node.
    elem_sets[4] = assembly->AddNode("s_five", sets[0]);
    VERIFY(elem_sets[4] != -1);

    // let get all datasets under 'sets'
    VERIFY(
      (assembly->GetDataSetIndices(groups[1]) == std::vector<unsigned int>{ 11, 8, 9, 10, 6, 7 }));

    // all datasets, breadth first.
    VERIFY((assembly->GetDataSetIndices(0, true, vtkDataAssembly::TraversalOrder::BreadthFirst) ==
      std::vector<unsigned int>{ 11, 0, 1, 2, 3, 4, 5, 8, 9, 10, 6, 7 }));

    VERIFY((assembly->SelectNodes({ "/" }) == std::vector<int>{ 0 }));
    VERIFY((assembly->SelectNodes({ "//sets" }) == std::vector<int>{ 2 }));
    VERIFY(assembly->SelectNodes({ "/sets" }).empty());
    VERIFY((assembly->SelectNodes({ "//sets/*" }) == std::vector<int>{ 6, 7, 8 }));
    assembly->Print(cout);

    vtkNew<vtkDataAssembly> subset;
    subset->SubsetCopy(assembly, { 6 });
    subset->Print(cout);

    subset->RemapDataSetIndices({ { 11, 0 }, { 6, 1 } }, /*remove_unmapped=*/true);
    subset->Print(cout);
    VERIFY((subset->GetDataSetIndices(0) == std::vector<unsigned int>{ 0, 1 }));
    VERIFY(subset->GetDataSetIndices(14).empty());
    VERIFY((subset->GetDataSetIndices(15) == std::vector<unsigned int>{ 1 }));

    VERIFY(vtkDataAssembly::IsNodeNameValid("Ying-Yang"));
  }
  catch (const TestFailed&)
  {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
