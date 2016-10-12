#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkMultiBlockDataGroupFilter.h"
#include "vtkCompositePolyDataMapper2.h"
#include "vtkCompositeDataDisplayAttributes.h"
#include "vtkRegressionTestImage.h"
#include "vtkLookupTable.h"
#include "vtkActor.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkDataArray.h"
#include "vtkColorTransferFunction.h"
#include "vtkArrayCalculator.h"

int TestBlockOpacity(int argc, char* argv[])
{
  // Standard rendering classes
  vtkSmartPointer< vtkRenderer > renderer = vtkSmartPointer< vtkRenderer >::New();
  vtkSmartPointer< vtkRenderWindow > renWin = vtkSmartPointer< vtkRenderWindow >::New();
  renWin->SetMultiSamples(0);
  renWin->SetAlphaBitPlanes(1);
  renWin->AddRenderer(renderer);
  vtkSmartPointer< vtkRenderWindowInteractor > iren =
      vtkSmartPointer< vtkRenderWindowInteractor >::New();
  iren->SetRenderWindow(renWin);

  // We create a multiblock dataset with one block (a sphere) and set the
  // block opacity to .75

  vtkSmartPointer< vtkSphereSource > sphere = vtkSmartPointer< vtkSphereSource >::New();
  sphere->SetRadius(0.5);
  sphere->SetCenter(0.0,0.0,0.0);
  sphere->SetThetaResolution(8);
  sphere->SetPhiResolution(8);
  sphere->Update();
//  sphere->GetOutput()->GetPointData()->SetActiveScalars(name);

  vtkSmartPointer< vtkArrayCalculator > calc = vtkSmartPointer< vtkArrayCalculator >::New();
  calc->SetInputConnection(sphere->GetOutputPort());
  calc->AddCoordinateScalarVariable("x",0);
  calc->AddCoordinateScalarVariable("y",1);
  calc->AddCoordinateScalarVariable("z",2);
  calc->SetFunction("(x-y)*z");
  calc->SetResultArrayName("result");
  calc->Update();

  double range[2];

  calc->GetOutput()->GetPointData()->GetScalars()->GetRange(range);

  vtkSmartPointer< vtkMultiBlockDataGroupFilter > groupDatasets =
      vtkSmartPointer< vtkMultiBlockDataGroupFilter >::New();
  groupDatasets->SetInputConnection(calc->GetOutputPort());
  groupDatasets->Update();

  vtkSmartPointer< vtkCompositePolyDataMapper2 > mapper =
      vtkSmartPointer< vtkCompositePolyDataMapper2 >::New();
  mapper->SetInputConnection(groupDatasets->GetOutputPort(0));
  mapper->SetColorModeToMapScalars();
  mapper->SetScalarModeToUsePointData();
  mapper->ScalarVisibilityOn();

  vtkSmartPointer< vtkCompositeDataDisplayAttributes > attrs =
      vtkSmartPointer< vtkCompositeDataDisplayAttributes >::New();
  mapper->SetCompositeDataDisplayAttributes(attrs);
  mapper->SetBlockOpacity(1,0.5);

  vtkSmartPointer< vtkColorTransferFunction > lut =
      vtkSmartPointer< vtkColorTransferFunction >::New();
  // This creates a blue to red lut.
  lut->AddHSVPoint(range[0],0.667,1,1);
  lut->AddHSVPoint(range[1],0,1,1);
  lut->SetColorSpaceToDiverging();
  lut->SetVectorModeToMagnitude();
  mapper->SetLookupTable(lut);
  mapper->SetInterpolateScalarsBeforeMapping(1);

  vtkSmartPointer< vtkActor > actor = vtkSmartPointer< vtkActor >::New();
  actor->SetMapper(mapper);
  renderer->AddActor(actor);

  renderer->SetUseDepthPeeling(1);
  // reasonable depth peeling settings
  // no more than 50 layers of transluceny
  renderer->SetMaximumNumberOfPeels(50);
  // stop when less than 2 in 1000 pixels changes
  renderer->SetOcclusionRatio(0.002);

  // Standard testing code.
  renderer->SetBackground(0.5,0.5,0.5);
  renWin->SetSize(300,300);
  renWin->Render();

  if(renderer->GetLastRenderingUsedDepthPeeling())
  {
    cout<<"depth peeling was used"<<endl;
  }
  else
  {
    cout<<"depth peeling was not used (alpha blending instead)"<<endl;
  }
  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
