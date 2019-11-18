/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOSPRayTetrahedraMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Description
// This test verifies that we can use ospray to volume render a
// vtk unstructured grid that contains tets, hexes, and wedges.

#include "vtkAppendFilter.h"
#include "vtkCellType.h"
#include "vtkCellTypeSource.h"
#include "vtkColorTransferFunction.h"
#include "vtkDataArray.h"
#include "vtkDataSetTriangleFilter.h"
#include "vtkImageCast.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkOSPRayPass.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPointData.h"
#include "vtkRandomAttributeGenerator.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkStructuredPointsReader.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"
#include "vtkThreshold.h"
#include "vtkTransform.h"
#include "vtkTransformFilter.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridVolumeRayCastMapper.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"

namespace
{

static const char* TestOSPRayTetrahedraMapperLog = "# StreamVersion 1\n"
                                                   "EnterEvent 299 0 0 0 0 0 0\n"
                                                   "MouseMoveEvent 299 0 0 0 0 0 0\n"
                                                   "MouseMoveEvent 298 2 0 0 0 0 0\n"
                                                   "MouseMoveEvent 297 4 0 0 0 0 0\n"
                                                   "MouseMoveEvent 297 6 0 0 0 0 0\n"
                                                   "MouseMoveEvent 296 8 0 0 0 0 0\n"
                                                   "LeaveEvent 399 -8 0 0 0 0 0\n";

} // end anon namespace

int TestOSPRayTetrahedraMapperMixedCells(int argc, char* argv[])
{
  bool useOSP = true;
  for (int i = 0; i < argc; i++)
  {
    if (!strcmp(argv[i], "-GL"))
    {
      cerr << "GL" << endl;
      useOSP = false;
    }
  }

  int blockDims[3] = { 24, 24, 24 };

  // Seed this for the random attribute generators:
  vtkMath::RandomSeed(0);

  vtkNew<vtkAppendFilter> datasetBuilder;
  datasetBuilder->SetOutputPointsPrecision(VTK_TYPE_FLOAT32);

  // Create an interesting tetrahedral dataset:
  {
    // Create the reader for the data
    // This is the data the will be volume rendered
    vtkNew<vtkStructuredPointsReader> reader;
    const char* file1 = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/ironProt.vtk");
    reader->SetFileName(file1);
    reader->Update();

    // currently ospray only supports float, remove when that
    // changes in ospray version that ParaView packages.
    vtkNew<vtkImageCast> toFloat;
    toFloat->SetInputConnection(reader->GetOutputPort());
    toFloat->SetOutputScalarTypeToFloat();

    // convert from vtkImageData to vtkUnstructuredGrid, remove
    // any cells where all values are below 80
    vtkNew<vtkThreshold> thresh;
    thresh->ThresholdByUpper(80);
    thresh->AllScalarsOff();
    thresh->SetInputConnection(toFloat->GetOutputPort());

    // make sure we have only tetrahedra
    vtkNew<vtkDataSetTriangleFilter> trifilter;
    trifilter->SetInputConnection(thresh->GetOutputPort());

    datasetBuilder->AddInputConnection(trifilter->GetOutputPort());
  }

  // Now generate some wedges:
  {
    vtkNew<vtkCellTypeSource> wedgeSource;
    wedgeSource->SetOutputPrecision(VTK_TYPE_FLOAT32);
    wedgeSource->SetCellType(VTK_WEDGE);
    wedgeSource->SetBlocksDimensions(blockDims);

    double scalarRange[2];
    double bounds[6];
    datasetBuilder->Update();
    datasetBuilder->GetOutput()->GetBounds(bounds);
    datasetBuilder->GetOutput()->GetPointData()->GetScalars()->GetRange(scalarRange);

    vtkNew<vtkTransform> transform;
    transform->Identity();
    transform->Translate(bounds[0], 0, bounds[5]);

    vtkNew<vtkTransformFilter> wedgeTransformer;
    wedgeTransformer->SetTransform(transform);
    wedgeTransformer->SetInputConnection(wedgeSource->GetOutputPort());

    vtkNew<vtkRandomAttributeGenerator> scalarGen;
    scalarGen->SetDataType(VTK_TYPE_FLOAT32);
    scalarGen->GeneratePointScalarsOn();
    scalarGen->SetComponentRange(scalarRange[0], scalarRange[1]);
    scalarGen->SetInputConnection(wedgeTransformer->GetOutputPort());

    // Grab the output so we can rename the scalar array to match the tets:
    scalarGen->Update();
    auto ds = vtkUnstructuredGrid::SafeDownCast(scalarGen->GetOutput());
    ds->GetPointData()->GetScalars()->SetName("scalars");

    datasetBuilder->AddInputData(ds);
  }

  // Now add some hexahedra:
  {
    vtkNew<vtkCellTypeSource> hexSource;
    hexSource->SetOutputPrecision(VTK_TYPE_FLOAT32);
    hexSource->SetCellType(VTK_HEXAHEDRON);
    hexSource->SetBlocksDimensions(blockDims);

    double scalarRange[2];
    double bounds[6];
    datasetBuilder->Update();
    datasetBuilder->GetOutput()->GetBounds(bounds);
    datasetBuilder->GetOutput()->GetPointData()->GetScalars()->GetRange(scalarRange);

    vtkNew<vtkTransform> transform;
    transform->Identity();
    transform->Translate(bounds[1], 0, bounds[5]);

    vtkNew<vtkTransformFilter> hexTransformer;
    hexTransformer->SetTransform(transform);
    hexTransformer->SetInputConnection(hexSource->GetOutputPort());

    vtkNew<vtkRandomAttributeGenerator> scalarGen;
    scalarGen->SetDataType(VTK_TYPE_FLOAT32);
    scalarGen->GeneratePointScalarsOn();
    scalarGen->SetComponentRange(scalarRange[0], scalarRange[1]);
    scalarGen->SetInputConnection(hexTransformer->GetOutputPort());

    // Grab the output so we can rename the scalar array to match the tets:
    scalarGen->Update();
    auto ds = vtkUnstructuredGrid::SafeDownCast(scalarGen->GetOutput());
    ds->GetPointData()->GetScalars()->SetName("scalars");

    datasetBuilder->AddInputData(ds);
  }

  // Create transfer mapping scalar value to opacity
  vtkNew<vtkPiecewiseFunction> opacityTransferFunction;
  opacityTransferFunction->AddPoint(80, 0.0);
  opacityTransferFunction->AddPoint(120, 0.2);
  opacityTransferFunction->AddPoint(255, 0.2);

  // Create transfer mapping scalar value to color
  vtkNew<vtkColorTransferFunction> colorTransferFunction;
  colorTransferFunction->AddRGBPoint(80.0, 0.0, 0.0, 0.0);
  colorTransferFunction->AddRGBPoint(120.0, 0.0, 0.0, 1.0);
  colorTransferFunction->AddRGBPoint(160.0, 1.0, 0.0, 0.0);
  colorTransferFunction->AddRGBPoint(200.0, 0.0, 1.0, 0.0);
  colorTransferFunction->AddRGBPoint(255.0, 0.0, 1.0, 1.0);

  // The property describes how the data will look
  vtkNew<vtkVolumeProperty> volumeProperty;
  volumeProperty->SetColor(colorTransferFunction.GetPointer());
  volumeProperty->SetScalarOpacity(opacityTransferFunction.GetPointer());
  volumeProperty->ShadeOff();
  volumeProperty->SetInterpolationTypeToLinear();

  // The mapper / ray cast function know how to render the data
  vtkNew<vtkUnstructuredGridVolumeRayCastMapper> volumeMapper;
  volumeMapper->SetInputConnection(datasetBuilder->GetOutputPort());

  // The volume holds the mapper and the property and
  // can be used to position/orient the volume
  vtkNew<vtkVolume> volume;
  volume->SetMapper(volumeMapper.GetPointer());
  volume->SetProperty(volumeProperty.GetPointer());

  vtkNew<vtkRenderer> ren1;
  ren1->AddVolume(volume.GetPointer());

  // Create the renderwindow, interactor and renderer
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetMultiSamples(0);
  renderWindow->SetSize(401, 399); // NPOT size
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow.GetPointer());
  vtkNew<vtkInteractorStyleTrackballCamera> style;
  iren->SetInteractorStyle(style.GetPointer());
  ren1->SetBackground(0.3, 0.3, 0.4);
  renderWindow->AddRenderer(ren1.GetPointer());

  ren1->ResetCamera();
  renderWindow->Render();

  // Attach OSPRay render pass
  vtkNew<vtkOSPRayPass> osprayPass;
  if (useOSP)
  {
    ren1->SetPass(osprayPass.GetPointer());
  }

  renderWindow->Render();
  int retVal;
  retVal = !(
    vtkTesting::InteractorEventLoop(argc, argv, iren.GetPointer(), TestOSPRayTetrahedraMapperLog));
  return !retVal;
}
