#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkStructuredPoints.h"
#include "vtkScalars.h"
#include "vtkContourFilter.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"

#include "SaveImage.h"

int main( int argc, char *argv[] )
{
  int i, j, k, kOffset, jOffset, offset;
  float x, y, z, s, sp;
  
  vtkRenderer *renderer = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
    renWin->AddRenderer(renderer);
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow(renWin);

  vtkStructuredPoints *vol = vtkStructuredPoints::New();
      vol->SetDimensions(26,26,26);
      vol->SetOrigin(-0.5,-0.5,-0.5);
      sp = 1.0/25.0;
      vol->SetSpacing(sp, sp, sp);

  vtkScalars *scalars = vtkScalars::New();
  for (k=0; k<26; k++)
    {
    z = -0.5 + k*sp;
    kOffset = k * 26 * 26;
    for (j=0; j<26; j++) 
      {
      y = -0.5 + j*sp;
      jOffset = j * 26;
      for (i=0; i<26; i++) 
        {
        x = -0.5 + i*sp;
        s = x*x + y*y + z*z - (0.4*0.4);
        offset = i + jOffset + kOffset;
        scalars->InsertScalar(offset,s);
        }
      }
    }
  vol->GetPointData()->SetScalars(scalars);
  scalars->Delete();

  vtkContourFilter *contour = vtkContourFilter::New();
      contour->SetInput(vol);
      contour->SetValue(0,0.0);

  vtkPolyDataMapper *volMapper = vtkPolyDataMapper::New();
      volMapper->SetInput(contour->GetOutput());
      volMapper->ScalarVisibilityOff();
  vtkActor *volActor = vtkActor::New();
      volActor->SetMapper(volMapper);

  renderer->AddActor(volActor);
      renderer->SetBackground(1,1,1);
  renWin->SetSize(300,300);

  // interact with data
  renWin->Render();

  SAVEIMAGE( renWin );

  iren->Start();

  // Clean up
  renderer->Delete();
  renWin->Delete();
  iren->Delete();
  vol->Delete();
  scalars->Delete();
  contour->Delete();
  volMapper->Delete();
  volActor->Delete();
}
