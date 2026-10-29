// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Checks that an amrmesh whose domains carry `state/nghosts` layers of ghost cells converts to the
// same AMR structure as the one built from the valid boxes alone, with the ghost layers placed at
// negative and past-the-end extent indices and flagged as DUPLICATECELL.

#include "vtkAMRBox.h"
#include "vtkCellData.h"
#include "vtkConduitSource.h"
#include "vtkDataArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkDummyController.h"
#include "vtkLogger.h"
#include "vtkMathUtilities.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkOverlappingAMR.h"
#include "vtkSmartPointer.h"
#include "vtkUniformGrid.h"
#include "vtkUnsignedCharArray.h"

#include <catalyst_conduit.hpp>

#include <array>
#include <string>
#include <vector>

#define VERIFY(x, ...)                                                                             \
  if ((x) == false)                                                                                \
  {                                                                                                \
    vtkLogF(ERROR, __VA_ARGS__);                                                                   \
    return false;                                                                                  \
  }

namespace
{

struct Domain
{
  int Level;
  std::array<int, 3> Lo;    // first valid cell, in the index space of the level
  std::array<int, 3> Cells; // number of valid cells
};

// Level 0 is an 8^d grid split in two boxes along x. Level 1 refines the coarse cells [2, 5]^d.
std::vector<Domain> GetDomains(int dim)
{
  const int nz = dim == 3 ? 8 : 0;
  std::vector<Domain> domains = { { 0, { 0, 0, 0 }, { 4, 8, nz } },
    { 0, { 4, 0, 0 }, { 4, 8, nz } },
    { 1, { 4, 4, dim == 3 ? 4 : 0 }, { 8, 8, dim == 3 ? 8 : 0 } } };
  return domains;
}

double GetSpacing(int level)
{
  return 1.0 / (8 << level);
}

// Builds the amrmesh node. With nghosts > 0, each coordset describes the grid grown by nghosts
// layers on every side, as AMReX lays out a MultiFab with grow cells.
void CreateAMRMesh(int dim, int nghosts, conduit_cpp::Node& mesh)
{
  const auto domains = GetDomains(dim);
  for (size_t id = 0; id < domains.size(); ++id)
  {
    const Domain& dom = domains[id];
    const double h = GetSpacing(dom.Level);
    auto node = mesh["domain" + std::to_string(id)];
    node["state/domain_id"] = static_cast<int>(id);
    node["state/cycle"] = 0;
    node["state/time"] = 0.0;
    node["state/level"] = dom.Level;
    if (nghosts > 0)
    {
      node["state/nghosts"] = nghosts;
    }

    std::array<int, 3> pdims;
    std::array<double, 3> origin;
    for (int d = 0; d < 3; ++d)
    {
      const bool hasCells = dom.Cells[d] > 0;
      const int ng = hasCells ? nghosts : 0;
      pdims[d] = hasCells ? dom.Cells[d] + 1 + 2 * ng : 1;
      origin[d] = (dom.Lo[d] - ng) * h;
    }

    auto coords = node["coordsets/coords"];
    coords["type"] = "uniform";
    coords["dims/i"] = pdims[0];
    coords["dims/j"] = pdims[1];
    coords["dims/k"] = pdims[2];
    coords["origin/x"] = origin[0];
    coords["origin/y"] = origin[1];
    coords["origin/z"] = origin[2];
    coords["spacing/dx"] = h;
    coords["spacing/dy"] = h;
    coords["spacing/dz"] = h;

    auto topo = node["topologies/topo"];
    topo["type"] = "uniform";
    topo["coordset"] = "coords";

    if (dom.Level == 0)
    {
      auto window = node["nestsets/nest/windows/window_000"];
      window["domain_id"] = 2;
      window["domain_type"] = "child";
      window["ratio/i"] = 2;
      window["ratio/j"] = 2;
      window["ratio/k"] = 2;
    }

    // Store the x coordinate of each cell center, so the test can check that the field values line
    // up with the cells VTK builds.
    std::vector<double> xc;
    const int ncx = pdims[0] - 1;
    const int ncy = std::max(pdims[1] - 1, 1);
    const int ncz = std::max(pdims[2] - 1, 1);
    for (int k = 0; k < ncz; ++k)
    {
      for (int j = 0; j < ncy; ++j)
      {
        for (int i = 0; i < ncx; ++i)
        {
          xc.push_back(origin[0] + (i + 0.5) * h);
        }
      }
    }
    auto field = node["fields/xc"];
    field["association"] = "element";
    field["topology"] = "topo";
    field["values"] = xc;
  }
}

// The output arrays point into the node's memory, so the node must outlive the output.
vtkSmartPointer<vtkOverlappingAMR> Convert(const conduit_cpp::Node& mesh)
{
  vtkNew<vtkConduitSource> source;
  source->SetUseAMRMeshProtocol(true);
  source->SetNode(conduit_cpp::c_node(&mesh));
  source->Update();
  return vtkOverlappingAMR::SafeDownCast(source->GetOutputDataObject(0));
}

struct GhostCounts
{
  vtkIdType Duplicate = 0;
  vtkIdType Refined = 0;
  vtkIdType Both = 0;
};

GhostCounts CountGhosts(vtkUniformGrid* grid)
{
  GhostCounts counts;
  vtkUnsignedCharArray* ghosts = grid->GetCellGhostArray();
  if (!ghosts)
  {
    return counts;
  }
  for (vtkIdType i = 0; i < ghosts->GetNumberOfValues(); ++i)
  {
    const unsigned char g = ghosts->GetValue(i);
    const bool dup = (g & vtkDataSetAttributes::DUPLICATECELL) != 0;
    const bool refined = (g & vtkDataSetAttributes::REFINEDCELL) != 0;
    counts.Duplicate += dup;
    counts.Refined += refined;
    counts.Both += dup && refined;
  }
  return counts;
}

bool ValidateGhosts(int dim, int nghosts)
{
  conduit_cpp::Node refMesh;
  CreateAMRMesh(dim, 0, refMesh);
  conduit_cpp::Node mesh;
  CreateAMRMesh(dim, nghosts, mesh);
  auto ref = Convert(refMesh);
  auto amr = Convert(mesh);
  VERIFY(ref && amr, "Expected vtkOverlappingAMR outputs");
  VERIFY(amr->CheckValidity(), "Invalid AMR with nghosts=%d", nghosts);
  VERIFY(amr->GetNumberOfLevels() == ref->GetNumberOfLevels(), "Incorrect number of levels");

  const double* refOrigin = ref->GetOrigin();
  const double* origin = amr->GetOrigin();
  for (int d = 0; d < 3; ++d)
  {
    VERIFY(vtkMathUtilities::FuzzyCompare(origin[d], refOrigin[d]),
      "Incorrect AMR origin along %d with nghosts=%d: %g instead of %g", d, nghosts, origin[d],
      refOrigin[d]);
  }

  for (unsigned int level = 0; level < amr->GetNumberOfLevels(); ++level)
  {
    VERIFY(amr->GetNumberOfBlocks(level) == ref->GetNumberOfBlocks(level),
      "Incorrect number of blocks at level %u", level);
    for (unsigned int idx = 0; idx < amr->GetNumberOfBlocks(level); ++idx)
    {
      VERIFY(amr->GetAMRBox(level, idx) == ref->GetAMRBox(level, idx),
        "Incorrect AMR box (%u, %u) with nghosts=%d", level, idx, nghosts);

      auto refGrid = vtkUniformGrid::SafeDownCast(ref->GetDataSetAsImageData(level, idx));
      auto grid = vtkUniformGrid::SafeDownCast(amr->GetDataSetAsImageData(level, idx));
      VERIFY(refGrid && grid, "Missing grid (%u, %u)", level, idx);

      int refExtent[6], extent[6];
      refGrid->GetExtent(refExtent);
      grid->GetExtent(extent);
      for (int d = 0; d < 3; ++d)
      {
        const int ng = refExtent[2 * d + 1] > refExtent[2 * d] ? nghosts : 0;
        VERIFY(
          extent[2 * d] == refExtent[2 * d] - ng && extent[2 * d + 1] == refExtent[2 * d + 1] + ng,
          "Incorrect extent of grid (%u, %u) along %d with nghosts=%d", level, idx, d, nghosts);
        VERIFY(vtkMathUtilities::FuzzyCompare(grid->GetOrigin()[d], refGrid->GetOrigin()[d]),
          "Incorrect origin of grid (%u, %u) along %d with nghosts=%d", level, idx, d, nghosts);
      }

      vtkDataArray* xc = grid->GetCellData()->GetArray("xc");
      VERIFY(xc && xc->GetNumberOfTuples() == grid->GetNumberOfCells(),
        "Missing or wrongly sized cell field in grid (%u, %u)", level, idx);
      double bounds[6];
      for (vtkIdType c = 0; c < grid->GetNumberOfCells(); ++c)
      {
        grid->GetCellBounds(c, bounds);
        const double center = 0.5 * (bounds[0] + bounds[1]);
        VERIFY(vtkMathUtilities::FuzzyCompare(xc->GetComponent(c, 0), center, 1e-12),
          "Cell field does not match cell %lld of grid (%u, %u) with nghosts=%d",
          static_cast<long long>(c), level, idx, nghosts);
      }

      const GhostCounts refCounts = CountGhosts(refGrid);
      const GhostCounts counts = CountGhosts(grid);
      const vtkIdType expectedDuplicate = grid->GetNumberOfCells() - refGrid->GetNumberOfCells();
      VERIFY(counts.Duplicate == expectedDuplicate,
        "Grid (%u, %u) has %lld duplicate cells instead of %lld with nghosts=%d", level, idx,
        static_cast<long long>(counts.Duplicate), static_cast<long long>(expectedDuplicate),
        nghosts);
      VERIFY(counts.Refined == refCounts.Refined && (refCounts.Refined > 0) == (level == 0),
        "Grid (%u, %u) has %lld refined cells instead of %lld with nghosts=%d", level, idx,
        static_cast<long long>(counts.Refined), static_cast<long long>(refCounts.Refined), nghosts);
      VERIFY(counts.Both == 0, "Grid (%u, %u) has cells both refined and duplicate", level, idx);
    }
  }
  return true;
}

}

int TestConduitSourceAMRGhosts(int, char*[])
{
  vtkNew<vtkDummyController> controller;
  vtkMultiProcessController::SetGlobalController(controller);

  bool ok = true;
  for (int dim : { 2, 3 })
  {
    for (int nghosts : { 1, 2 })
    {
      ok = ValidateGhosts(dim, nghosts) && ok;
    }
  }
  vtkMultiProcessController::SetGlobalController(nullptr);
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
