#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkConeSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkWindowToImageFilter.h"
#include "vtkPNMReader.h"
#include "vtkImageDifference.h"

int main( int vtkNotUsed(argc), char *vtkNotUsed(argv[]) )
{
  // create a rendering window and renderer
  vtkRenderer *ren = vtkRenderer::New();
  vtkRenderWindow *renWindow = vtkRenderWindow::New();
    renWindow->AddRenderer(ren);
  renWindow->SetSize( 300, 300 );

  // create an actor and give it cone geometry
  vtkConeSource *cone = vtkConeSource::New();
    cone->SetResolution(8);
  vtkPolyDataMapper *coneMapper = vtkPolyDataMapper::New();
    coneMapper->SetInput(cone->GetOutput());
  vtkActor *coneActor = vtkActor::New();
    coneActor->SetMapper(coneMapper);

  // assign our actor to the renderer
  ren->AddActor(coneActor);

  // draw the resulting scene
  renWindow->Render();
  renWindow->Render();

  vtkWindowToImageFilter *w2if = vtkWindowToImageFilter::New();
    w2if->SetInput(renWindow);

  vtkImageDifference *imgDiff = vtkImageDifference::New();
    
  vtkPNMReader *rtpnm = vtkPNMReader::New();
    rtpnm->SetFileName("valid/Cone.cxx.ppm");

  imgDiff->SetInput(w2if->GetOutput());
  imgDiff->SetImage(rtpnm->GetOutput());
  imgDiff->Update();

  if (imgDiff->GetThresholdedError() <= 10) 
    {
    cerr << "C++ smoke test passed." << endl;
    } 
    else 
    {
    cerr << "C++ smoke test failed." << endl;
    }	
}
