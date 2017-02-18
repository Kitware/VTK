/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestValuePassFloatingPoint.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Description:
// Tests vtkValuePass in FLOATING_POINT mode. The test generates a 3-component
// float array ("elevationVector") using the loaded polygonal data (points and cells).
// Polygons are rendered with the ValuePass to its internal floating point frame-buffer.
// The rendered float image is then queried from the vtkValuePass and used to
// generate a color image using vtkLookupTable, the color image is rendered with
// an image actor on-screen. This is repeated for each component.

#include "vtkActor.h"
#include "vtkArrayCalculator.h"
#include "vtkCamera.h"
#include "vtkCameraPass.h"
#include "vtkCellData.h"
#include "vtkElevationFilter.h"
#include "vtkFloatArray.h"
#include "vtkImageActor.h"
#include "vtkImageData.h"
#include "vtkImageMapper3D.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkLookupTable.h"
#include "vtkOpenGLRenderer.h"
#include "vtkPointData.h"
#include "vtkPointDataToCellData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderPassCollection.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSequencePass.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"
#include "vtkValuePass.h"


void GenerateElevationArray(vtkSmartPointer<vtkPolyDataAlgorithm> source)
{
  vtkPolyData* data = source->GetOutput();
  const double* bounds = data->GetBounds();

  vtkSmartPointer<vtkElevationFilter> elevation =
    vtkSmartPointer<vtkElevationFilter>::New();
  elevation->SetInputConnection(source->GetOutputPort());

  // Use vtkElevation to generate an array per component. vtkElevation generates
  // a projected distance from each point in the dataset to the line, with respect to
  // the LowPoint ([0, 1] in this case. This is different from having the actual
  // coordinates of a given point.
  for (int c = 0; c < 3; c++)
  {
    std::string name;
    switch (c)
    {
      case 0:
        name = "delta_x";
        elevation->SetLowPoint(bounds[0], 0.0, 0.0);
        elevation->SetHighPoint(bounds[1], 0.0, 0.0);
        break;
      case 1:
        name = "delta_y";
        elevation->SetLowPoint(0.0, bounds[2], 0.0);
        elevation->SetHighPoint(0.0, bounds[3], 0.0);
        break;
      case 2:
        name = "delta_z";
        elevation->SetLowPoint(0.0, 0.0, bounds[4]);
        elevation->SetHighPoint(0.0, 0.0, bounds[5]);
        break;
    }
    elevation->Update();

    vtkPolyData* result = vtkPolyData::SafeDownCast(elevation->GetOutput());
    int outCellFlag;
    // Enums defined in vtkAbstractMapper
    vtkDataArray* elevArray = vtkAbstractMapper::GetScalars(result,
      VTK_SCALAR_MODE_USE_POINT_FIELD_DATA, VTK_GET_ARRAY_BY_NAME/*acc mode*/,
      0/*arr id*/, "Elevation"/*arr name*/, outCellFlag);
    if (!elevArray)
    {
      std::cout << "->> Error: could not find array!" << std::endl;
      return;
    }

    elevArray->SetName(name.c_str());
    data->GetPointData()->AddArray(elevArray);
  }

  // Generate a 3-component vector array using the single components
  // form elevation

  // Point data
  vtkSmartPointer<vtkArrayCalculator> calc =
    vtkSmartPointer<vtkArrayCalculator>::New();
  calc->SetInputConnection(source->GetOutputPort());
  calc->SetAttributeModeToUsePointData();
  calc->AddScalarArrayName("delta_x");
  calc->AddScalarArrayName("delta_y");
  calc->AddScalarArrayName("delta_z");
  calc->SetFunction("delta_x * iHat + delta_y * jHat + delta_z * kHat");
  calc->SetResultArrayName("elevationVector");
  calc->Update();

  // Cell data
  vtkSmartPointer<vtkPointDataToCellData> p2c =
    vtkSmartPointer<vtkPointDataToCellData>::New();
  p2c->SetInputConnection(calc->GetOutputPort());
  p2c->PassPointDataOn();
  p2c->Update();

  /// Include the elevation vector (point and cell data) in the original data
  vtkPolyData* outputP2c = vtkPolyData::SafeDownCast(p2c->GetOutput());
  data->GetPointData()->AddArray(calc->GetOutput()->GetPointData()->GetArray(
    "elevationVector"));
  data->GetCellData()->AddArray(outputP2c->GetCellData()->GetArray("elevationVector"));
};

//------------------------------------------------------------------------------
void RenderComponentImages(std::vector<vtkSmartPointer<vtkImageData> >& colorImOut,
  vtkRenderWindow* window, vtkRenderer* renderer,
  vtkValuePass* valuePass, int dataMode, char const* name)
{
  valuePass->SetInputArrayToProcess(dataMode, name);

  // Prepare a lut to map the floating point values
  vtkSmartPointer<vtkLookupTable> lut = vtkSmartPointer<vtkLookupTable>::New();
  lut->SetAlpha(1.0);
  lut->Build();

  // Render each component in a separate image
  for(int c = 0; c < 3; c++)
  {
    valuePass->SetInputComponentToProcess(c);
    window->Render();

    /// Get the resulting values
    vtkFloatArray* result = valuePass->GetFloatImageDataArray(renderer);
    int* ext = valuePass->GetFloatImageExtents();

    // Map the resulting float image to a color table
    vtkUnsignedCharArray* colored = lut->MapScalars(result, VTK_COLOR_MODE_DEFAULT,
      0/* single comp*/);

    // Create an image dataset to render in a quad.
    vtkSmartPointer<vtkImageData> colorIm = vtkSmartPointer<vtkImageData>::New();
    colorIm->SetExtent(ext);
    colorIm->GetPointData()->SetScalars(colored);
    colorImOut.push_back(colorIm);
    colored->Delete();
  }
};

///////////////////////////////////////////////////////////////////////////////
int TestValuePassFloatingPoint(int argc, char *argv[])
{
  // Load data
  vtkSmartPointer<vtkSphereSource> sphere = vtkSmartPointer<vtkSphereSource>::New();
  sphere->SetThetaResolution(8.0);
  sphere->SetPhiResolution(8.0);
  sphere->Update();

  // Prepare a 3-component array (data will be appended to reader's output)
  GenerateElevationArray(sphere);
  vtkSmartPointer<vtkPolyDataMapper> mapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputData(sphere->GetOutput());
  mapper->ScalarVisibilityOn();

  vtkSmartPointer<vtkActor> actor =
    vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);

  // Setup rendering and interaction
  vtkSmartPointer<vtkRenderWindowInteractor> interactor =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();

  vtkSmartPointer<vtkInteractorStyleTrackballCamera> style =
    vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
  interactor->SetInteractorStyle(style);

  vtkSmartPointer<vtkRenderWindow> window =
    vtkSmartPointer<vtkRenderWindow>::New();
  window->SetMultiSamples(0);
  window->SetSize(640, 640);

  vtkSmartPointer<vtkRenderer> renderer =
    vtkSmartPointer<vtkRenderer>::New();

  window->AddRenderer(renderer);
  interactor->SetRenderWindow(window);

  renderer->AddActor(actor);
  renderer->SetBackground(0.2, 0.2, 0.5);

  // Setup the value pass
  //int const RenderingMode = vtkValuePass::INVERTIBLE_LUT;
  int const RenderingMode = vtkValuePass::FLOATING_POINT;
  int const comp = 0;

  vtkSmartPointer<vtkValuePass> valuePass =
    vtkSmartPointer<vtkValuePass>::New();
  valuePass->SetRenderingMode(RenderingMode);
  valuePass->SetInputComponentToProcess(comp);
  // Initial data mode
  valuePass->SetInputArrayToProcess(VTK_SCALAR_MODE_USE_POINT_FIELD_DATA,
    "elevationVector");
  //valuePass->SetInputArrayToProcess(VTK_SCALAR_MODE_USE_CELL_FIELD_DATA,
  //  "elevationVector");

  // 3. Add it to a sequence of passes
  vtkSmartPointer<vtkRenderPassCollection> passes =
    vtkSmartPointer<vtkRenderPassCollection>::New();
  passes->AddItem(valuePass);

  vtkSmartPointer<vtkSequencePass> sequence =
    vtkSmartPointer<vtkSequencePass>::New();
  sequence->SetPasses(passes);

  vtkSmartPointer<vtkCameraPass> cameraPass =
    vtkSmartPointer<vtkCameraPass>::New();
  cameraPass->SetDelegatePass(sequence);

  vtkOpenGLRenderer *glRenderer =
    vtkOpenGLRenderer::SafeDownCast(renderer.GetPointer());

  // Render the value pass
  glRenderer->SetPass(cameraPass);
  window->Render();

  // Check whether the RenderingMode change (this could happen due to a lack of
  // extension/context support
  if (valuePass->GetRenderingMode() == vtkValuePass::FLOATING_POINT)
  {
    // Render point data images
    std::vector<vtkSmartPointer<vtkImageData> > colorImagesPoint;
    RenderComponentImages(colorImagesPoint, window, renderer, valuePass,
      VTK_SCALAR_MODE_USE_POINT_FIELD_DATA, "elevationVector");

    // Render cell data images
   std::vector<vtkSmartPointer<vtkImageData> > colorImagesCell;
    RenderComponentImages(colorImagesCell, window, renderer, valuePass,
      VTK_SCALAR_MODE_USE_CELL_FIELD_DATA, "elevationVector");

    ////// Render results on-screen
    renderer->RemoveActor(actor);

    // Add image actors to display the point dataArray's componets
    vtkSmartPointer<vtkImageActor> ia_x = vtkSmartPointer<vtkImageActor>::New();
    ia_x->GetMapper()->SetInputData(colorImagesPoint.at(0));
    renderer->AddActor(ia_x);

    vtkSmartPointer<vtkImageActor> ia_y = vtkSmartPointer<vtkImageActor>::New();
    ia_y->RotateX(90);
    ia_y->GetMapper()->SetInputData(colorImagesPoint.at(1));
    renderer->AddActor(ia_y);

    vtkSmartPointer<vtkImageActor> ia_z = vtkSmartPointer<vtkImageActor>::New();
    ia_z->RotateY(-90);
    ia_z->GetMapper()->SetInputData(colorImagesPoint.at(2));
    renderer->AddActor(ia_z);

    // Add image actors to display cell dataArray's components
    vtkSmartPointer<vtkImageActor> iacell_x = vtkSmartPointer<vtkImageActor>::New();
    iacell_x->SetPosition(-500, 600, 600);
    iacell_x->GetMapper()->SetInputData(colorImagesCell.at(0));
    renderer->AddActor(iacell_x);

    vtkSmartPointer<vtkImageActor> iacell_y = vtkSmartPointer<vtkImageActor>::New();
    iacell_y->RotateX(90);
    iacell_y->SetPosition(-500, 600, 600);
    iacell_y->GetMapper()->SetInputData(colorImagesCell.at(1));
    renderer->AddActor(iacell_y);

    vtkSmartPointer<vtkImageActor> iacell_z = vtkSmartPointer<vtkImageActor>::New();
    iacell_z->RotateY(-90);
    iacell_z->SetPosition(-500, 600, 600);
    iacell_z->GetMapper()->SetInputData(colorImagesCell.at(2));
    renderer->AddActor(iacell_z);

    // Adjust viewpoint
    vtkCamera* cam = renderer->GetActiveCamera();
    cam->SetPosition(2, 2, 2);
    cam->SetFocalPoint(0, 0, 1);
    renderer->ResetCamera();

    // Use the default pass to render the colored image.
    glRenderer->SetPass(NULL);
    window->Render();
  }

  // initialize render loop
  int retVal = vtkRegressionTestImage(window.GetPointer());
  if( retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    interactor->Start();
  }

  valuePass->ReleaseGraphicsResources(window);
  return !retVal;
}
