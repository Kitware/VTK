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
// float array ("elevationVector") using the loaded polygonal data.  Polygons
// are rendered with the ValuePass to its internal floating point frame-buffer.
// The rendered float image is then queried from the vtkValuePass and used to
// generate a color image using vtkLookupTable, the color image is rendered with
// an image actor on-screen. This is repeated for each component.

#include "vtkRegressionTestImage.h"
#include "vtkTestUtilities.h"
#include "vtkActor.h"
#include "vtkCameraPass.h"
#include "vtkCellArray.h"
#include "vtkElevationFilter.h"
#include "vtkInformation.h"
#include "vtkOpenGLRenderer.h"
#include "vtkPLYReader.h"
#include "vtkPointData.h"
#include "vtkPointDataToCellData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkRenderPassCollection.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSequencePass.h"
#include "vtkSmartPointer.h"
#include "vtkValuePass.h"

#include "vtkCamera.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkAbstractMapper.h"
#include "vtkXMLPolyDataReader.h"
#include "vtkFloatArray.h"
#include "vtkLookupTable.h"
#include "vtkColorSeries.h"
#include "vtkImageData.h"
#include "vtkImageActor.h"
#include "vtkImageMapper3D.h"
#include "vtkArrayCalculator.h"


void GenerateElevationArray(vtkSmartPointer<vtkPLYReader> reader)
{
  vtkPolyData* data = reader->GetOutput();
  double* bounds = data->GetBounds();

  vtkSmartPointer<vtkElevationFilter> elevation =
    vtkSmartPointer<vtkElevationFilter>::New();
  elevation->SetInputConnection(reader->GetOutputPort());

  /// Use vtkElevation to generate an array per component. vtkElevation generates
  /// a projected distance from each point in the dataset to the line, with respect to
  /// the LowPoint ([0, 1] in this case. This is different from having the actual
  /// coordinates of a given point.
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
    /// Enums defined in vtkAbstractMapper
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

  /// Generate a 3-component vector array using the single components
  /// form elevation.
  vtkSmartPointer<vtkArrayCalculator> calc = vtkSmartPointer<vtkArrayCalculator>::New();
  calc->SetInputConnection(reader->GetOutputPort());
  calc->SetAttributeModeToUsePointData();
  calc->AddScalarArrayName("delta_x");
  calc->AddScalarArrayName("delta_y");
  calc->AddScalarArrayName("delta_z");
  calc->SetFunction("delta_x * iHat + delta_y * jHat + delta_z * kHat");
  calc->SetResultArrayName("elevationVector");
  calc->Update();

  vtkPolyData* result = vtkPolyData::SafeDownCast(calc->GetOutput());
  int outCellFlag;
  vtkDataArray* coordArray = vtkAbstractMapper::GetScalars(result,
    VTK_SCALAR_MODE_USE_POINT_FIELD_DATA, VTK_GET_ARRAY_BY_NAME/*acc mode*/,
    0/*arr id*/, "elevationVector", outCellFlag);
  if (!coordArray)
    {
    std::cout << "->> Error: could not find array!" << std::endl;
    return;
    }

  /// Include the elevation vector in the original data
  data->GetPointData()->AddArray(coordArray);
}


///////////////////////////////////////////////////////////////////////////////
int TestValuePassFloatingPoint(int argc, char *argv[])
{
  // Load data
  const char *fileName =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dragon.ply");
  vtkSmartPointer<vtkPLYReader> reader =
    vtkSmartPointer<vtkPLYReader>::New();
  reader->SetFileName(fileName);
  reader->Update();

  // Prepare a 3-component array (data will be appended to reader's output)
  GenerateElevationArray(reader);

  vtkSmartPointer<vtkPolyDataMapper> mapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputConnection(reader->GetOutputPort());
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
  window->SetSize(320, 320);

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
  //valuePass->SetScalarRange(bounds[0], bounds[1]); /*use the full range*/
  valuePass->SetInputArrayToProcess(VTK_SCALAR_MODE_USE_POINT_FIELD_DATA,
    "elevationVector");

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

  if (RenderingMode == vtkValuePass::FLOATING_POINT)
    {
    /// Prepare a lut to map the floating point values
    vtkSmartPointer<vtkLookupTable> lut = vtkSmartPointer<vtkLookupTable>::New();
    lut->SetAlpha(1.0);
    //lut->SetRange(elevRange[0], elevRange[1]); /*use the full range*/
    vtkSmartPointer<vtkColorSeries> series = vtkSmartPointer<vtkColorSeries>::New();
    series->SetColorScheme(vtkColorSeries::WARM);
    //series->SetColorScheme(vtkColorSeries::BREWER_DIVERGING_SPECTRAL_11);
    series->BuildLookupTable(lut, vtkColorSeries::ORDINAL);

    /// Render each component in a separate image
    std::vector<vtkSmartPointer<vtkImageData> > colorImages;
    for(int c = 0; c < 3; c++)
      {
      valuePass->SetInputComponentToProcess(c);
      window->Render();

      /// Get the result.
      vtkFloatArray* result = valuePass->GetFloatImageData(renderer);
      std::vector<int> ext = valuePass->GetFloatImageExtents(renderer);

      /// Map the resulting float image to a color table
      vtkUnsignedCharArray* colored = lut->MapScalars(result, VTK_COLOR_MODE_DEFAULT,
        0/* single comp*/);

      /// Create an image dataset to render in a quad.
      vtkSmartPointer<vtkImageData> colorIm = vtkSmartPointer<vtkImageData>::New();
      colorIm->SetExtent(&(ext.front()));
      colorIm->GetPointData()->SetScalars(colored);
      colorImages.push_back(colorIm);
      }

    // Render the image on-screen
    renderer->RemoveActor(actor);

    vtkSmartPointer<vtkImageActor> ia_x = vtkSmartPointer<vtkImageActor>::New();
    ia_x->GetMapper()->SetInputData(colorImages.at(0));
    renderer->AddActor(ia_x);

    vtkSmartPointer<vtkImageActor> ia_y = vtkSmartPointer<vtkImageActor>::New();
    ia_y->RotateX(90);
    ia_y->GetMapper()->SetInputData(colorImages.at(1));
    renderer->AddActor(ia_y);

    vtkSmartPointer<vtkImageActor> ia_z = vtkSmartPointer<vtkImageActor>::New();
    ia_z->RotateY(-90);
    ia_z->GetMapper()->SetInputData(colorImages.at(2));
    renderer->AddActor(ia_z);

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

  return !retVal;
}
