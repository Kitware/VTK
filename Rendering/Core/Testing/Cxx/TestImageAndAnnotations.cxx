#include <vtkCornerAnnotation.h>
#include <vtkImageData.h>
#include <vtkImageMapper.h>
#include <vtkInteractorStyleImage.h>
#include <vtkProperty2D.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkTextProperty.h>

#include <vtkTestUtilities.h>
#include <vtkRegressionTestImage.h>

namespace
{

vtkSmartPointer<vtkImageData> CreateColorImage(unsigned int dim, bool transparent)
{
  vtkSmartPointer<vtkImageData> image = vtkSmartPointer<vtkImageData>::New();
  image->SetDimensions(dim, dim, 1);
  image->AllocateScalars(VTK_UNSIGNED_CHAR, 4);

  for (unsigned int x = 0; x < dim; x++)
  {
    for (unsigned int y = 0; y < dim; y++)
    {
      unsigned char* pixel = static_cast<unsigned char*>(image->GetScalarPointer(x, y, 0));
      pixel[0] = 255;
      pixel[1] = 0;
      pixel[2] = 255;
      pixel[3] = transparent ? 127 : 255;
    }
  }

  return image;
}

vtkSmartPointer<vtkActor2D> CreateImageActor(int dim, int displayLocation, bool transparent)
{
  vtkSmartPointer<vtkImageData> colorImage = CreateColorImage(dim, transparent);

  vtkSmartPointer<vtkImageMapper> imageMapper = vtkSmartPointer<vtkImageMapper>::New();
  imageMapper->SetInputData(colorImage);
  imageMapper->SetColorWindow(255);
  imageMapper->SetColorLevel(127.5);

  vtkSmartPointer<vtkActor2D> imageActor = vtkSmartPointer<vtkActor2D>::New();
  imageActor->SetMapper(imageMapper);
  imageActor->SetPosition(dim, 0);
  imageActor->GetProperty()->SetDisplayLocation(displayLocation);

  return imageActor;
}

} // namespace

int TestImageAndAnnotations(int argc, char *argv[])
{
  // Setup renderer
  vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();

  // Setup render window
  vtkSmartPointer<vtkRenderWindow> renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
  renderWindow->SetSize(600, 600);
  renderWindow->AddRenderer(renderer);

  // Setup render window interactor
  vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  vtkSmartPointer<vtkInteractorStyleImage> style =
    vtkSmartPointer<vtkInteractorStyleImage>::New();
  renderWindowInteractor->SetInteractorStyle(style);

  // Setup corner annotation
  vtkSmartPointer<vtkCornerAnnotation> cornerAnnotation =
    vtkSmartPointer<vtkCornerAnnotation>::New();
  cornerAnnotation->SetLinearFontScaleFactor(2);
  cornerAnnotation->SetNonlinearFontScaleFactor(1);
  cornerAnnotation->SetMaximumFontSize(20);
  cornerAnnotation->SetText(0, "background/opaque"); // lower left
  cornerAnnotation->SetText(1, "foreground/opaque"); // lower right
  cornerAnnotation->SetText(2, "background/transparent"); // upper left
  cornerAnnotation->SetText(3, "foreground/transparent"); // upper right
  cornerAnnotation->GetTextProperty()->SetColor(1, 1, 1);

  renderer->AddViewProp(cornerAnnotation);

  // Setup images
  const unsigned int Dim = 300;
  {
    // lower left: background/opaque
    bool transparent = false;
    vtkSmartPointer<vtkActor2D> imageActor = CreateImageActor(Dim, VTK_BACKGROUND_LOCATION, transparent);
    imageActor->SetPosition(0, 0);
    renderer->AddActor(imageActor);
  }
  {
    // lower right: foreground/opaque
    bool transparent = false;
    vtkSmartPointer<vtkActor2D> imageActor = CreateImageActor(Dim, VTK_FOREGROUND_LOCATION, transparent);
    imageActor->SetPosition(Dim, 0);
    renderer->AddActor(imageActor);
  }
  {
    // upper left: background/transparent
    bool transparent = true;
    vtkSmartPointer<vtkActor2D> imageActor = CreateImageActor(Dim, VTK_BACKGROUND_LOCATION, transparent);
    imageActor->SetPosition(0, Dim);
    renderer->AddActor(imageActor);
  }
  {
    // upper right: foreground/transparent
    bool transparent = true;
    vtkSmartPointer<vtkActor2D> imageActor = CreateImageActor(Dim, VTK_FOREGROUND_LOCATION, transparent);
    imageActor->SetPosition(Dim, Dim);
    renderer->AddActor(imageActor);
  }

  renderer->ResetCamera();

  // Render and start interaction
  renderWindowInteractor->SetRenderWindow(renderWindow);
  renderWindowInteractor->Initialize();

  int retVal = vtkRegressionTestImage( renderWindow );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
  }

  return !retVal;
}
