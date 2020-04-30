#include <vtkActor.h>
#include <vtkImageActor.h>
#include <vtkImageData.h>
#include <vtkImageMapper3D.h>
#include <vtkNew.h>
#include <vtkOpenGLRenderWindow.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSphereSource.h>
#include <vtkTestUtilities.h>
#include <vtkUnsignedCharArray.h>

int TestRenderToImage(int argc, char* argv[])
{
  vtkNew<vtkSphereSource> sphereSource;
  sphereSource->SetCenter(0.0, 0.0, 0.0);
  sphereSource->SetRadius(5.0);
  sphereSource->Update();

  // Visualize
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(sphereSource->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer);
  renderWindow->SetMultiSamples(0);

  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);
  renderWindow->Render();

  // Render to the image
  vtkOpenGLRenderWindow* glRenderWindow = vtkOpenGLRenderWindow::SafeDownCast(renderWindow);

  glRenderWindow->SetShowWindow(false);
  glRenderWindow->SetUseOffScreenBuffers(true);
  renderWindow->Render();
  // Create an (empty) image at the window size
  const int* size = renderWindow->GetSize();
  vtkNew<vtkImageData> image;
  image->SetDimensions(size[0], size[1], 1);
  image->AllocateScalars(VTK_UNSIGNED_CHAR, 3);
  renderWindow->GetPixelData(0, 0, size[0] - 1, size[1] - 1, 0,
    vtkArrayDownCast<vtkUnsignedCharArray>(image->GetPointData()->GetScalars()));
  glRenderWindow->SetShowWindow(true);
  glRenderWindow->SetUseOffScreenBuffers(false);

  // Now add the actor
  renderer->AddActor(actor);
  renderer->ResetCamera();
  renderWindow->Render();

  glRenderWindow->SetShowWindow(false);
  glRenderWindow->SetUseOffScreenBuffers(true);
  renderWindow->Render();
  // Capture the framebuffer to the image, again
  renderWindow->GetPixelData(0, 0, size[0] - 1, size[1] - 1, 0,
    vtkArrayDownCast<vtkUnsignedCharArray>(image->GetPointData()->GetScalars()));
  glRenderWindow->SetShowWindow(true);
  glRenderWindow->SetUseOffScreenBuffers(false);

  // Create a new image actor and remove the geometry one
  vtkNew<vtkImageActor> imageActor;
  imageActor->GetMapper()->SetInputData(image);
  renderer->RemoveActor(actor);
  renderer->AddActor(imageActor);

  // Background color white to distinguish image boundary
  renderer->SetBackground(1, 1, 1);

  renderWindow->Render();
  renderer->ResetCamera();
  renderWindow->Render();
  int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
  }

  return !retVal;
}
