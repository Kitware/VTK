#include <vtkActor.h>
#include <vtkActor2D.h>
#include <vtkCoordinate.h>
#include <vtkLineSource.h>
#include <vtkLookupTable.h>
#include <vtkNew.h>
#include <vtkPlaneSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyDataMapper2D.h>
#include <vtkProperty.h>
#include <vtkProperty2D.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>

#include <vtkRegressionTestImage.h>
#include <vtkTestUtilities.h>

int TestActor2D(int argc, char* argv[])
{
  vtkNew<vtkLookupTable> lut;
  lut->SetNumberOfTableValues(6);
  lut->SetTableRange(0.0, 1.0);

  vtkNew<vtkPlaneSource> planeSource1;
  planeSource1->SetOrigin(0.0, 0.0, 0.0);
  planeSource1->SetPoint1(0.5, 0.0, 0.0);
  planeSource1->SetPoint2(0.0, 0.5, 0.0);

  vtkNew<vtkPolyDataMapper> mapper1;
  mapper1->SetInputConnection(planeSource1->GetOutputPort());
  mapper1->ScalarVisibilityOn();
  mapper1->SetLookupTable(lut);
  mapper1->UseLookupTableScalarRangeOn();
  mapper1->SetScalarModeToUsePointFieldData();
  mapper1->ColorByArrayComponent("TextureCoordinates", 0);
  mapper1->InterpolateScalarsBeforeMappingOn();

  vtkNew<vtkActor> actor1;
  actor1->SetMapper(mapper1);
  actor1->GetProperty()->SetColor(1.0, 0.0, 0.0);

  vtkNew<vtkPlaneSource> planeSource2;
  planeSource2->SetOrigin(-0.5, 0.0, 0.0);
  planeSource2->SetPoint1(0.0, 0.0, 0.0);
  planeSource2->SetPoint2(-0.5, 0.5, 0.0);

  vtkNew<vtkCoordinate> pCoord;
  pCoord->SetCoordinateSystemToWorld();

  vtkNew<vtkCoordinate> coord;
  coord->SetCoordinateSystemToNormalizedViewport();
  coord->SetReferenceCoordinate(pCoord);

  vtkNew<vtkPolyDataMapper2D> mapper2;
  mapper2->SetInputConnection(planeSource2->GetOutputPort());
  mapper2->SetLookupTable(lut);
  mapper2->ScalarVisibilityOff();
  mapper2->SetTransformCoordinate(coord);

  vtkNew<vtkActor2D> actor2;
  actor2->SetMapper(mapper2);
  actor2->GetProperty()->SetColor(1.0, 1.0, 0.0);

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor1);
  renderer->AddActor(actor2);

  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
  renWin->Render();
  renderer->ResetCamera();
  renderer->SetBackground(1.0, 0.0, 0.0);
  renWin->SetSize(300, 300);

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
