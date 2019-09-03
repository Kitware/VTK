#include "vtkActor.h"
#include "vtkCellData.h"
#include "vtkCompositePolyDataMapper2.h"
#include "vtkExpandMarkedElements.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSignedCharArray.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"

namespace
{

vtkSmartPointer<vtkDataSet> GetSphere(int part, int num_parts)
{
  vtkNew<vtkSphereSource> sphere;
  sphere->SetPhiResolution(6);
  sphere->SetPhiResolution(6);
  sphere->SetStartTheta(360.0 * part / num_parts);
  sphere->SetEndTheta(360.0 * (part + 1) / num_parts);
  sphere->Update();
  auto ds = sphere->GetOutput();

  vtkNew<vtkSignedCharArray> selectedCells;
  selectedCells->SetName("MarkedCells");
  selectedCells->SetNumberOfTuples(ds->GetNumberOfCells());
  selectedCells->FillComponent(0, 0);
  selectedCells->SetTypedComponent(20, 0, 1);
  ds->GetCellData()->AddArray(selectedCells);
  return ds;
}
}
int TestExpandMarkedElements(int argc, char* argv[])
{
  vtkNew<vtkMultiBlockDataSet> mb;
  for (int cc = 0; cc < 3; ++cc)
  {
    mb->SetBlock(cc, ::GetSphere(cc, 3));
  }

  vtkNew<vtkExpandMarkedElements> filter;
  filter->SetInputDataObject(mb);
  filter->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, "MarkedCells");

  vtkNew<vtkCompositePolyDataMapper2> mapper;
  mapper->SetInputConnection(filter->GetOutputPort());
  mapper->SetScalarModeToUseCellFieldData();
  mapper->SelectColorArray("MarkedCells");

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->EdgeVisibilityOn();

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);

  vtkNew<vtkRenderWindow> renWin;
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
  renWin->AddRenderer(renderer);

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
