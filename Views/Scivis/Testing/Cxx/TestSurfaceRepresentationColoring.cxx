// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Exercises how a surface representation reports and switches its coloring:
// going back to a solid color, and what blocks nobody has touched report.

#include "ScivisTestUtilities.h"
#include "vtkBlockProperties.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPolyData.h"
#include "vtkScivisView.h"
#include "vtkSphereSource.h"
#include "vtkSurfaceRepresentation.h"

#include <iostream>

namespace
{

// ColorBySolidColor completes the family: it stops coloring by an array without
// disturbing the color to go back to.
int TestColorBySolidColor()
{
  vtkNew<vtkSphereSource> sphere;
  vtkNew<vtkSurfaceRepresentation> surface;
  surface->SetInputConnection(sphere->GetOutputPort());

  surface->SetColor(0.2, 0.4, 0.6);
  surface->ColorByPointArray("Normals");
  CHECK(surface->GetScalarVisibility(), "coloring by an array did not take");
  CHECK(surface->GetColor()[1] == 0.4, "coloring by an array threw away the solid color");

  surface->ColorBySolidColor();
  CHECK(!surface->GetScalarVisibility(), "the array is still being drawn");
  CHECK(surface->GetColor()[1] == 0.4, "the color to go back to was not kept");

  return EXIT_SUCCESS;
}

// A block nobody has touched is drawn the way the representation is, and that is
// what the getters say.  The display attributes underneath would say black and
// zero, which reads as an invisible black block.
int TestUntouchedBlocksReportWhatIsDrawn()
{
  vtkNew<vtkPartitionedDataSet> partitions;
  for (int i = 0; i < 3; ++i)
  {
    vtkNew<vtkSphereSource> sphere;
    sphere->SetCenter(i * 3.0, 0.0, 0.0);
    sphere->Update();
    vtkNew<vtkPolyData> copy;
    copy->DeepCopy(sphere->GetOutput());
    partitions->SetPartition(i, copy);
  }
  vtkNew<vtkPartitionedDataSetCollection> collection;
  collection->SetPartitionedDataSet(0, partitions);

  vtkNew<vtkSurfaceRepresentation> surface;
  surface->SetInputDataObject(collection);
  surface->SetColor(0.2, 0.4, 0.6);
  surface->SetOpacity(0.75);

  vtkNew<vtkScivisView> view;
  view->AddRepresentation(surface);
  view->Render();

  vtkBlockProperties* blocks = surface->GetBlocks();

  double color[3];
  blocks->GetColor(2, color);
  CHECK(color[0] == 0.2 && color[1] == 0.4 && color[2] == 0.6,
    "an untouched block does not report the color it is drawn in");
  CHECK(
    blocks->GetOpacity(2) == 0.75, "an untouched block does not report the opacity it is drawn at");
  CHECK(blocks->GetVisibility(2), "an untouched block reports itself hidden");

  // Once a block is given its own, that is what comes back.
  blocks->SetColor(2, 1.0, 0.0, 0.0);
  blocks->SetOpacity(2, 0.25);
  blocks->GetColor(2, color);
  CHECK(color[0] == 1.0 && color[1] == 0.0, "a block's own color did not come back");
  CHECK(blocks->GetOpacity(2) == 0.25, "a block's own opacity did not come back");

  // Its neighbour is still drawn like the representation.
  blocks->GetColor(3, color);
  CHECK(color[0] == 0.2 && blocks->GetOpacity(3) == 0.75,
    "setting one block changed what another reports");

  // And changing the representation moves the untouched ones with it.
  surface->SetOpacity(0.5);
  CHECK(
    blocks->GetOpacity(3) == 0.5, "an untouched block did not follow the representation's opacity");
  CHECK(blocks->GetOpacity(2) == 0.25, "a block with its own opacity followed the representation");

  return EXIT_SUCCESS;
}

}

int TestSurfaceRepresentationColoring(int, char*[])
{
  if (TestColorBySolidColor() != EXIT_SUCCESS ||
    TestUntouchedBlocksReportWhatIsDrawn() != EXIT_SUCCESS)
  {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
