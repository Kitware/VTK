#include "vtkActor.h"
#include "vtkCompositeDataDisplayAttributes.h"
#include "vtkCompositePolyDataMapper2.h"
#include "vtkCubeSource.h"
#include "vtkDataArray.h"
#include "vtkLookupTable.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"

#include <set>

static vtkSmartPointer<vtkMultiBlockDataSet> vtkCreateData()
{
  auto data = vtkSmartPointer<vtkMultiBlockDataSet>::New();
  data->SetNumberOfBlocks(3 * 3 * 2);
  int blk = 0;
  for (int i = 0; i < 3; ++i)
  {
    for (int j = 0; j < 3; ++j)
    {
      vtkNew<vtkSphereSource> ssrc;
      ssrc->SetRadius(0.4);
      ssrc->SetCenter(i, j, 0.0);
      ssrc->Update();

      vtkNew<vtkCubeSource> csrc;
      csrc->SetBounds(i - 0.4, i + 0.4, j - 0.4, j + 0.4, 0.6, 1.4);
      csrc->Update();

      vtkNew<vtkPolyData> sphere;
      vtkNew<vtkPolyData> cube;

      sphere->DeepCopy(ssrc->GetOutputDataObject(0));
      cube->DeepCopy(csrc->GetOutputDataObject(0));
      data->SetBlock(blk++, sphere);
      data->SetBlock(blk++, cube);
    }
  }
  return data;
}

int TestBlockVisibility(int argc, char* argv[])
{
  // Standard rendering classes
  vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renWin = vtkSmartPointer<vtkRenderWindow>::New();
  renWin->SetMultiSamples(0);
  renWin->SetAlphaBitPlanes(1);
  renWin->AddRenderer(renderer);
  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  // We create a multiblock dataset with 18 blocks (spheres & cubes) and set the
  // block visibility to a pattern.

  auto mbds = vtkCreateData();

  vtkSmartPointer<vtkCompositePolyDataMapper2> mapper =
    vtkSmartPointer<vtkCompositePolyDataMapper2>::New();
  mapper->SetInputDataObject(mbds);
  // mapper->SetColorModeToMapScalars();
  // mapper->SetScalarModeToUsePointData();
  // mapper->ScalarVisibilityOn();
  mapper->ScalarVisibilityOff();

  vtkSmartPointer<vtkCompositeDataDisplayAttributes> attrs =
    vtkSmartPointer<vtkCompositeDataDisplayAttributes>::New();
  mapper->SetCompositeDataDisplayAttributes(attrs);

  const int visblocks[] = { 0, 3, 4, 7, 8, 11, 13, 14, 17 };
  std::set<int> vis(visblocks, visblocks + sizeof(visblocks) / sizeof(visblocks[0]));
  for (int i = 0; i < static_cast<int>(mbds->GetNumberOfBlocks()); ++i)
  {
    vtkDataObject* blk = mbds->GetBlock(i);
    attrs->SetBlockVisibility(blk, vis.find(i) != vis.end() ? 1 : 0);
  }

  int numVisited = 0;
  int numVisible = 0;
  attrs->VisitVisibilities([&numVisited, &numVisible](vtkDataObject*, bool visible) {
    if (visible)
    {
      ++numVisible;
    }
    ++numVisited;
    return false; // do not terminate loop early.
  });

  if (numVisited != static_cast<int>(mbds->GetNumberOfBlocks()))
  {
    vtkGenericWarningMacro("ERROR: Visited " << numVisited << " blocks instead of expected "
                                             << mbds->GetNumberOfBlocks());
  }

  if (numVisible != static_cast<int>(vis.size()))
  {
    vtkGenericWarningMacro(
      "ERROR: " << numVisible << " visible blocks instead of expected " << vis.size());
  }

  vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);
  renderer->AddActor(actor);

  // Standard testing code.
  renderer->SetBackground(0.5, 0.5, 0.5);
  renWin->SetSize(300, 300);
  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
