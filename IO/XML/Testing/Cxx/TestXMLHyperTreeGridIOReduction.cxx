// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// .NAME Test of vtkSimplePointsReader and vtkSimplePointsWriter
// .SECTION Description

#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridDepthLimiter.h"
#include "vtkHyperTreeGridNonOrientedCursor.h"
#include "vtkHyperTreeGridOrientedGeometryCursor.h"
#include "vtkNew.h"
#include "vtkRandomHyperTreeGridSource.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkXMLHyperTreeGridReader.h"
#include "vtkXMLHyperTreeGridWriter.h"

#include <array>
#include <string>
namespace
{
template <class M_HTG>
size_t count_trees(M_HTG* htg)
{
  size_t nbTrees = 0;
  vtkHyperTreeGrid::vtkHyperTreeGridIterator itHTG;
  vtkIdType iHT = 0;
  htg->InitializeTreeIterator(itHTG);
  while (itHTG.GetNextTree(iHT))
  {
    ++nbTrees;
  }
  return nbTrees;
}
}
int TestXMLHyperTreeGridIOReduction(int argc, char* argv[])
{
  const char* tmpstr =
    vtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary");
  std::string tdir = tmpstr ? tmpstr : std::string();
  delete[] tmpstr;

  std::string fname = tdir + std::string("/TestXMLHyperTreeGridIOReduction.htg");
  // using default source: 5 levels, 5x5x2 HT grid, [-10,10] for x, y, z
  vtkNew<vtkRandomHyperTreeGridSource> source;
  source->SetMaxDepth(5);
  source->Update();
  vtkHyperTreeGrid* htg = source->GetHyperTreeGridOutput();

  std::cout << "Writing " << fname << std::endl;
  vtkNew<vtkXMLHyperTreeGridWriter> writer;
  writer->SetFileName(fname.c_str());
  writer->SetDataModeToAppended();
  writer->SetInputData(htg);
  writer->SetDataSetMajorVersion(1);
  writer->Write();

  const unsigned int defaultSourceLevel = htg->GetNumberOfLevels();

  vtkNew<vtkXMLHyperTreeGridReader> reader;
  // Testing Fixed Level == 1 to 9
  std::cout << "Testing Level limitation from 1 to 9" << std::endl;
  for (unsigned int fixedLevel = 1; fixedLevel < 10; ++fixedLevel)
  {
    reader->SetFileName(fname.c_str());
    reader->SetFixedLevel(fixedLevel);
    reader->Update();
    const size_t expectedLevel = std::min(fixedLevel, defaultSourceLevel);
    vtkHyperTreeGrid* htgRead = vtkHyperTreeGrid::SafeDownCast(reader->GetOutputDataObject(0));
    const size_t actualLevel = htgRead->GetNumberOfLevels();
    if (actualLevel != expectedLevel)
    {
      std::cerr << "Expected level " << expectedLevel << " and got " << actualLevel << std::endl;
      return EXIT_FAILURE;
    }
  }

  // Testing HT extraction one by one by selecting center of each HT, note we keep level == 9
  std::cout << "Testing HT extraction of each of the 50 HTs of the grid" << std::endl;
  vtkDataArray* htgXCoords = htg->GetXCoordinates();
  vtkDataArray* htgYCoords = htg->GetYCoordinates();
  vtkDataArray* htgZCoords = htg->GetZCoordinates();
  for (vtkIdType zHT = 0; zHT < htgZCoords->GetNumberOfValues() - 1; ++zHT)
  {
    for (vtkIdType yHT = 0; yHT < htgYCoords->GetNumberOfValues() - 1; ++yHT)
    {
      for (vtkIdType xHT = 0; xHT < htgXCoords->GetNumberOfValues() - 1; ++xHT)
      {
        const double coordCenter[3] = {
          (htgXCoords->GetTuple1(xHT) + htgXCoords->GetTuple1(xHT + 1)) / 2.0,
          (htgYCoords->GetTuple1(yHT) + htgYCoords->GetTuple1(yHT + 1)) / 2.0,
          (htgZCoords->GetTuple1(zHT) + htgZCoords->GetTuple1(zHT + 1)) / 2.0
        };
        const double coords[6] = { coordCenter[0], coordCenter[0], coordCenter[1], coordCenter[1],
          coordCenter[2], coordCenter[2] };
        reader->SetFileName(fname.c_str());
        reader->SetCoordinatesBoundingBox(
          coords[0], coords[1], coords[2], coords[3], coords[4], coords[5]);
        reader->Update();
        vtkHyperTreeGrid* htgRead = vtkHyperTreeGrid::SafeDownCast(reader->GetOutputDataObject(0));

        const size_t nbTrees = count_trees(htgRead);
        if (nbTrees - 1)
        {
          std::cout << "Got " << nbTrees << " instead of 1" << std::endl;
          return EXIT_FAILURE;
        }
      }
    }
  }
  // We select a bounding box larger than HT grid
  {
    std::array<double, 6> out_bbox = { htgXCoords->GetRange()[0] - 1e+8,
      htgXCoords->GetRange()[1] + 1e+8, htgYCoords->GetRange()[0] - 1e+8,
      htgYCoords->GetRange()[1] + 1e+8, htgZCoords->GetRange()[0] - 1e+8,
      htgZCoords->GetRange()[1] + 1e+8 };
    std::cout << "Selecting larger bounding box: ";
    for (double e : out_bbox)
    {
      std::cout << e << " ";
    }
    std::cout << std::endl;

    reader->SetFileName(fname.c_str());
    reader->SetCoordinatesBoundingBox(
      out_bbox[0], out_bbox[1], out_bbox[2], out_bbox[3], out_bbox[4], out_bbox[5]);
    reader->Update();
    vtkHyperTreeGrid* htgRead = vtkHyperTreeGrid::SafeDownCast(reader->GetOutputDataObject(0));
    const size_t nbTrees = count_trees(htgRead);
    const size_t maxTrees = htgRead->GetMaxNumberOfTrees();
    if (nbTrees - htgRead->GetMaxNumberOfTrees())
    {
      std::cout << "Got " << nbTrees << " instead of " << maxTrees << " trees" << std::endl;
      return EXIT_FAILURE;
    }
  }
  // We select 9 HTs at the center of the 5x5 HT grid
  std::cout << "Testing extraction of the 3x3x1 HTs at the center of the 5x5x2 HT grid"
            << std::endl;
  const double coordsTest[6] = { -6, 2, -6, 2, 0, 10 };
  reader->SetFileName(fname.c_str());
  reader->SetCoordinatesBoundingBox(
    coordsTest[0], coordsTest[1], coordsTest[2], coordsTest[3], coordsTest[4], coordsTest[5]);
  reader->Update();
  vtkHyperTreeGrid* htgRead = vtkHyperTreeGrid::SafeDownCast(reader->GetOutputDataObject(0));
  const size_t nbTrees = count_trees(htgRead);
  if (nbTrees - 9)
  {
    std::cout << "Got trees##" << nbTrees << " instead of 9" << std::endl;
    return EXIT_FAILURE;
  }
  if (htgRead->GetNumberOfCells() - 11425)
  {
    std::cout << "Got A cells##" << htgRead->GetNumberOfCells() << " instead of 11425" << std::endl;
    return EXIT_FAILURE;
  }

  //------------------------------------------------------------------------------------
  // Creation of a DepthLimiter filter without creation of a new HTG mesh
  vtkNew<vtkHyperTreeGridDepthLimiter> depthlimiter;
  depthlimiter->SetInputConnection(reader->GetOutputPort());
  depthlimiter->SetDepth(3);
  depthlimiter->SetJustCreateNewMask(true); // set member DepthLimiter on actual HTG mesh
  depthlimiter->Update();
  // Number of cells unchanged, the DepthLimiter is not yet taken into account
  if (depthlimiter->GetHyperTreeGridOutput()->GetNumberOfCells() - 11425)
  {
    std::cout << "Got B cells##" << depthlimiter->GetHyperTreeGridOutput()->GetNumberOfCells()
              << " instead of 11425" << std::endl;
    return EXIT_FAILURE;
  }

  // Write to file, this time the DepthLimiter will be taken into account
  writer->SetFileName(fname.c_str());
  writer->SetDataModeToAppended();
  writer->SetInputData(depthlimiter->GetHyperTreeGridOutput());
  writer->SetDataSetMajorVersion(2);
  writer->Write();

  // Read the written HTG and check the effective cell number reduction
  vtkNew<vtkXMLHyperTreeGridReader> reader2;
  reader2->SetFileName(fname.c_str());
  reader2->Update();
  htgRead = vtkHyperTreeGrid::SafeDownCast(reader2->GetOutputDataObject(0));
  // WARNING The correct behavior of the write filter is only available in
  //         data set major version 2.
  //         In data set major version 1, it is as if the application of the
  //         DepthLimiter filter, with set member DepthLimiter on actual HTG
  //         mesh, had no impact.
  if (htgRead->GetNumberOfCells() - 689)
  {
    std::cout << "Got C cells##" << htgRead->GetNumberOfCells() << " instead of 689" << std::endl;
    return EXIT_FAILURE;
  }

  //------------------------------------------------------------------------------------
  // Creation of a DepthLimiter filter with creation of a new HTG mesh
  depthlimiter->SetJustCreateNewMask(false); // create a new HTG on filter output
  depthlimiter->Update();
  // Number of cells already reduced in DepthLimiter output
  if (depthlimiter->GetHyperTreeGridOutput()->GetNumberOfCells() - 689)
  {
    std::cout << "Got D cells##" << depthlimiter->GetHyperTreeGridOutput()->GetNumberOfCells()
              << " instead of 689" << std::endl;
    return EXIT_FAILURE;
  }

  // Write to file, to check that written HTG is still reduced
  writer->Write();

  // Read the written HTG and check the cell number coherence
  reader2->Update();
  htgRead = vtkHyperTreeGrid::SafeDownCast(reader2->GetOutputDataObject(0));
  if (htgRead->GetNumberOfCells() - 689)
  {
    std::cout << "Got E cells##" << htgRead->GetNumberOfCells() << " instead of 689" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
