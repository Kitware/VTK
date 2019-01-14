#include "vtkSphereSource.h"
#include "vtkSmartPointer.h"
#include "vtkAppendPolyData.h"
#include "vtkCamera.h"
#include "vtkDepthSortPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRegressionTestImage.h"
#include "vtkCallbackCommand.h"
#include "vtkCellData.h"
#include "vtkColorTransferFunction.h"

int TestDepthSortPolyData(int argc, char *argv[])
{
  vtkRenderer *ren = vtkRenderer::New();
  ren->SetBackground(1.0, 1.0, 1.0);

  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->SetSize(400, 400);
  renWin->AddRenderer(ren);
  ren->Delete();

  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);
  renWin->Delete();

  // generate some geometry for each mode and dir
  int sortMode[] = {vtkDepthSortPolyData::VTK_SORT_FIRST_POINT,
    vtkDepthSortPolyData::VTK_SORT_BOUNDS_CENTER, vtkDepthSortPolyData::VTK_SORT_PARAMETRIC_CENTER};

  int sortDir[] = {vtkDepthSortPolyData::VTK_DIRECTION_BACK_TO_FRONT,
    vtkDepthSortPolyData::VTK_DIRECTION_FRONT_TO_BACK, vtkDepthSortPolyData::VTK_DIRECTION_SPECIFIED_VECTOR};

  vtkCamera *cam = vtkCamera::New();
  cam->SetPosition(1,2,0);
  cam->SetFocalPoint(1,1,0);

  for (size_t j = 0; j < sizeof(sortMode)/sizeof(int); ++j)
  {
    for (size_t i = 0; i < sizeof(sortDir)/sizeof(int); ++i)
    {
      vtkSphereSource *ss = vtkSphereSource::New();
      ss->SetThetaResolution(64);
      ss->SetPhiResolution(64);
      ss->SetRadius(0.25);
      ss->SetCenter(j, i, 0.0);
      ss->Update();

      vtkDepthSortPolyData *ds = vtkDepthSortPolyData::New();
      ds->SetDirection(sortDir[i]);
      ds->SetDepthSortMode(sortMode[j]);
      ds->SortScalarsOn();
      ds->SetInputConnection(ss->GetOutputPort(0));
      if (i == vtkDepthSortPolyData::VTK_DIRECTION_SPECIFIED_VECTOR)
      {
        ds->SetOrigin(0.0, 0.0, 0.0);
        ds->SetVector(0.5, 0.5, 0.125);
      }
      else
      {
        ds->SetCamera(cam);
      }
      ss->Delete();

      vtkPolyDataMapper *pdm = vtkPolyDataMapper::New();
      pdm->SetInputConnection(ds->GetOutputPort(0));
      ds->Delete();

      vtkIdType nc = ss->GetOutput()->GetNumberOfCells();
      vtkColorTransferFunction *lut = vtkColorTransferFunction::New();
      lut->SetColorSpaceToRGB();
      lut->AddRGBPoint(0.0, 0.0, 0.0, 1.0);
      lut->AddRGBPoint(nc, 1.0, 0.0, 0.0);
      lut->SetColorSpaceToDiverging();
      lut->Build();
      pdm->SetLookupTable(lut);
      pdm->SetScalarVisibility(1);
      pdm->SelectColorArray("sortedCellIds");
      pdm->SetUseLookupTableScalarRange(1);
      pdm->SetScalarMode(VTK_SCALAR_MODE_USE_CELL_FIELD_DATA);
      lut->Delete();

      vtkActor *act = vtkActor::New();
      act->SetMapper(pdm);
      pdm->Delete();

      ren->AddActor(act);
      act->Delete();
    }
  }

  cam->Delete();
  cam = ren->GetActiveCamera();
  cam->SetPosition(1,1,10);
  ren->ResetCamera();
  cam->Zoom(1.25);

  iren->Initialize();
  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  iren->Delete();
  return !retVal;
}
