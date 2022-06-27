/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastMapperShadows.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkCellArray.h>
#include <vtkColorTransferFunction.h>
#include <vtkFloatArray.h>
#include <vtkGPUVolumeRayCastMapper.h>
#include <vtkImageData.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkLight.h>
#include <vtkNew.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkSystemIncludes.h>
#include <vtkTestErrorObserver.h>
#include <vtkTestUtilities.h>
#include <vtkTesting.h>
#include <vtkUniformGrid.h>
#include <vtkVertex.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>

static inline void coordsToIdx(double coords[3], double spacing[3], double origin[3], int res[3])
{
  for (int s = 0; s < 3; s++)
  {
    res[s] = static_cast<int>(ceil((coords[s] - origin[s]) / spacing[s]));
    res[s] = std::max(res[s], 0);
  }
}

class ImageDataAABox
{
private:
  double CoordsMin[3] = { 0, 0, 0 };
  double CoordsMax[3] = { 0, 0, 0 };
  double BoxValue = 0.0;

public:
  ImageDataAABox() = default;
  ImageDataAABox(double min[3], double max[3], double value = 0.0)
    : BoxValue(value)
  {
    for (int i = 0; i < 3; i++)
    {
      CoordsMin[i] = min[i];
      CoordsMax[i] = max[i];
    }
  };
  ImageDataAABox(double min_x, double min_y, double min_z, double max_x, double max_y, double max_z,
    double value = 0.0)
  {
    CoordsMin[0] = min_x;
    CoordsMin[1] = min_y;
    CoordsMin[2] = min_z;
    CoordsMax[0] = max_x;
    CoordsMax[1] = max_y;
    CoordsMax[2] = max_z;
    BoxValue = value;
  }
  ~ImageDataAABox() = default;

  void SetValue(double value) { BoxValue = value; };

  void AddBoxToArray(vtkFloatArray* data, vtkImageData* grid)
  {
    double origin[3];
    double spacing[3];
    int dimension[3];
    // doesn't support non-null extents, sorry !
    grid->GetOrigin(origin);
    grid->GetSpacing(spacing);
    grid->GetDimensions(dimension);

    int idxMin[3];
    int idxMax[3];
    coordsToIdx(CoordsMin, spacing, origin, idxMin);
    coordsToIdx(CoordsMax, spacing, origin, idxMax);

    for (int s = 0; s < 3; s++)
    {
      if (idxMin[s] >= dimension[s])
      {
        idxMin[s] = dimension[s] - 1;
      }
      if (idxMax[s] >= dimension[s])
      {
        idxMax[s] = dimension[s] - 1;
      }
    }

    // now we fill the array
    for (int i = idxMin[0]; i < idxMax[0]; i++)
    {
      for (int j = idxMin[1]; j < idxMax[1]; j++)
      {
        for (int k = idxMin[2]; k < idxMax[2]; k++)
        {
          int idxArray = k * dimension[0] * dimension[1] + j * dimension[0] + i;
          float value = data->GetValue(idxArray);
          value = BoxValue;
          data->SetValue(idxArray, value);
        }
      }
    }
  };
};

typedef std::vector<ImageDataAABox> BoxList;

//-----------------------------
int TestGPURayCastMapperShadows(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  // grid is between 0 and 1 in world coords
  double origin[3] = { 0, 0, 0 };
  double spacing[3] = { 0.005, 0.005, 0.005 };
  int dimension[3] = { 200, 200, 200 };

  BoxList Boxes;
  // wall
  Boxes.push_back(ImageDataAABox(0.05, 0.05, 0.05, 0.1, 0.95, 0.95, 1.0));
  // box
  Boxes.push_back(ImageDataAABox(0.6, 0.35, 0.35, 0.9, 0.65, 0.65, 2.0));

  // Camera Parameters
  double camera_position[3] = { 1.85, -1.27, 0.97 };
  double camera_focal[3] = { 0.498, 0.498, 0.498 };
  double camera_up[3] = { 0.0, 0.0, 1.0 };
  double camera_parallel_scale = 1.2;

  //------------

  vtkNew<vtkFloatArray> dataArray;
  dataArray->SetNumberOfComponents(1);
  dataArray->SetNumberOfTuples(static_cast<vtkIdType>(dimension[0] * dimension[1] * dimension[2]));

  // init to zero
  memset(dataArray->GetVoidPointer(0), 0,
    static_cast<long>(dimension[0] * dimension[1] * dimension[2]) * sizeof(float));

  vtkNew<vtkUniformGrid> grid;
  grid->SetOrigin(origin);
  grid->SetSpacing(spacing);
  grid->SetDimensions(dimension);

  // populate the array
  for (auto& box : Boxes)
  {
    box.AddBoxToArray(dataArray, grid);
  }

  grid->GetPointData()->SetScalars(dataArray);

  // volume properties
  vtkNew<vtkVolumeProperty> volProp;
  volProp->SetDiffuse(1.0);
  volProp->SetSpecular(1.0);
  volProp->SetAmbient(1.0);
  volProp->SetSpecularPower(100.0);
  volProp->SetShade(1);
  volProp->SetInterpolationType(VTK_NEAREST_INTERPOLATION);

  volProp->SetTransferFunctionModeTo1D();

  vtkNew<vtkPiecewiseFunction> opacityTF;
  opacityTF->RemoveAllPoints();
  opacityTF->AddPoint(0.0, 0.0);
  opacityTF->AddPoint(2.0, 0.8);

  vtkNew<vtkColorTransferFunction> colorTF;
  colorTF->RemoveAllPoints();
  colorTF->AddRGBPoint(0.0, 1.0, 1.0, 1.0);
  colorTF->AddRGBPoint(1.0, 1.0, 1.0, 1.0);
  colorTF->AddRGBPoint(1.8, 1.0, 0.0, 0.0);
  colorTF->AddRGBPoint(2.0, 1.0, 0.0, 0.0);

  volProp->SetScalarOpacity(opacityTF);
  volProp->SetScalarOpacityUnitDistance(spacing[0] * 0.1);
  volProp->SetColor(colorTF);

  // Mapper
  vtkNew<vtkGPUVolumeRayCastMapper> volMapper;
  volMapper->SetUseJittering(true);
  volMapper->SetAutoAdjustSampleDistances(false);
  volMapper->SetSampleDistance(spacing[0] * 0.5);
  volMapper->SetInputData(grid);
  volMapper->SetBlendModeToComposite();
  volMapper->SetGlobalIlluminationReach(0.82);
  volMapper->SetVolumetricScatteringBlending(2.0);

  // Volume
  vtkNew<vtkVolume> vol;
  vol->SetMapper(volMapper);
  vol->SetProperty(volProp);

  // Renderer
  vtkNew<vtkRenderer> ren;
  ren->SetBackground(0.2, 0.2, 0.2);
  ren->SetTwoSidedLighting(false);
  ren->AddVolume(vol);
  // -> light
  ren->ClearLights();
  ren->RemoveAllLights();

  double lightPosition[3] = { 1.3, 0.5, 1.0 };
  double lightFocalPoint[3] = { 0.0, 0.5, 0.2 };
  vtkNew<vtkLight> light;
  light->SetLightTypeToSceneLight();
  light->SetPosition(lightPosition);
  light->SetPositional(true);
  light->SetAmbientColor(0.3, 0.2, 0.1);
  light->SetConeAngle(60);
  light->SetFocalPoint(lightFocalPoint);
  light->SetIntensity(1.0);
  ren->AddLight(light);

  // -> camera
  vtkCamera* cam = ren->GetActiveCamera();
  cam->SetPosition(camera_position);
  cam->SetFocalPoint(camera_focal);
  cam->SetViewUp(camera_up);
  cam->SetParallelScale(camera_parallel_scale);

  // Render window
  vtkNew<vtkRenderWindow> renwin;
  renwin->AddRenderer(ren);
  renwin->SetSize(600, 600);

  // Render interactor
  vtkNew<vtkRenderWindowInteractor> renint;
  renint->SetRenderWindow(renwin);
  vtkNew<vtkInteractorStyleTrackballCamera> renintstyle;
  renint->SetInteractorStyle(renintstyle);

  renwin->Render();
  int retVal = vtkTesting::Test(argc, argv, renwin, 90);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    renint->Start();
  }

  return !((retVal == vtkTesting::PASSED) || (retVal == vtkTesting::DO_INTERACTOR));
}
