#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkCompositePolyDataMapper.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkExodusIIReader.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"

int TestExodusTetra15(int argc, char* argv[])
{
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/tetra15.g");
  if (!fname)
  {
    cout << "Could not obtain filename for test data.\n";
    return 1;
  }

  vtkNew<vtkExodusIIReader> rdr;
  if (!rdr->CanReadFile(fname))
  {
    cout << "Cannot read \"" << fname << "\"\n";
    return 1;
  }
  rdr->SetFileName(fname);
  rdr->Update();
  vtkIndent ind;

  vtkNew<vtkDataSetSurfaceFilter> surface;
  vtkNew<vtkCompositePolyDataMapper> mapper;
  vtkNew<vtkActor> actor;
  vtkNew<vtkRenderWindow> renWin;
  vtkNew<vtkRenderer> ren;
  vtkNew<vtkRenderWindowInteractor> iren;

  surface->SetInputConnection(rdr->GetOutputPort());
  mapper->SetInputConnection(surface->GetOutputPort());
  actor->SetMapper(mapper);
  renWin->AddRenderer(ren);
  iren->SetRenderWindow(renWin);

  ren->AddActor(actor);
  ren->SetBackground(1, 1, 1);
  renWin->SetSize(300, 300);
  auto cam = ren->GetActiveCamera();
  cam->SetPosition(10., 10., 5.);
  cam->SetViewUp(0., 0.4, 1.);
  ren->ResetCamera();
  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);

  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  delete[] fname;
  return !retVal;
}
