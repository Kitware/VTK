// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Description
// Tests QQuickVTKItem

#include "QQuickVTKItem.h"
#include "TestQQuickCommon.h"
#include "vtkActor.h"
#include "vtkColorTransferFunction.h"
#include "vtkConeSource.h"
#include "vtkGenericOpenGLRenderWindow.h"
#include "vtkGlyph3DMapper.h"
#include "vtkNew.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkSmartVolumeMapper.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"
#include "vtkWindowToImageFilter.h"
#include "vtkXMLImageDataReader.h"

#include <QApplication>

namespace
{
int Argc = 0;
char** Argv = nullptr;

/*=========================================================================*/

struct MyGeomItem : QQuickVTKItem
{
  struct Data : vtkObject
  {
    static Data* New();
    vtkTypeMacro(Data, vtkObject);
  };

  vtkUserData initializeVTK(vtkRenderWindow* renderWindow) override
  {
    auto vtk = vtkNew<Data>();

    // Create a cone pipeline and add it to the view
    vtkNew<vtkRenderer> renderer;
    vtkNew<vtkActor> actor;
    vtkNew<vtkPolyDataMapper> mapper;
    vtkNew<vtkConeSource> cone;
    renderWindow->AddRenderer(renderer);
    mapper->SetInputConnection(cone->GetOutputPort());
    actor->SetMapper(mapper);
    renderer->AddActor(actor);
    renderer->ResetCamera();

    return vtk;
  }
};
vtkStandardNewMacro(MyGeomItem::Data);

/*=========================================================================*/

struct MyVolumeItem : QQuickVTKItem
{
  struct Data : vtkObject
  {
    static Data* New();
    vtkTypeMacro(Data, vtkObject);
  };

  vtkUserData initializeVTK(vtkRenderWindow* renderWindow) override
  {
    auto vtk = vtkNew<Data>();

    vtkNew<vtkRenderer> renderer;
    renderWindow->AddRenderer(renderer);

    // Create a volume pipeline and add it to the view
    vtkNew<vtkSmartVolumeMapper> volumeMapper;
    vtkNew<vtkXMLImageDataReader> reader;
    const char* volumeFile =
      vtkTestUtilities::ExpandDataFileName(Argc, Argv, "Data/vase_1comp.vti");
    reader->SetFileName(volumeFile);
    volumeMapper->SetInputConnection(reader->GetOutputPort());
    delete[] volumeFile;
    double scalarRange[2];
    volumeMapper->GetInput()->GetScalarRange(scalarRange);
    volumeMapper->SetAutoAdjustSampleDistances(1);
    volumeMapper->SetBlendModeToComposite();
    vtkNew<vtkPiecewiseFunction> scalarOpacity;
    scalarOpacity->AddPoint(scalarRange[0], 0.0);
    scalarOpacity->AddPoint(scalarRange[1], 0.09);
    vtkNew<vtkVolumeProperty> volumeProperty;
    volumeProperty->ShadeOff();
    volumeProperty->SetInterpolationType(VTK_LINEAR_INTERPOLATION);
    volumeProperty->SetScalarOpacity(scalarOpacity);
    vtkSmartPointer<vtkColorTransferFunction> colorTransferFunction =
      volumeProperty->GetRGBTransferFunction(0);
    colorTransferFunction->RemoveAllPoints();
    colorTransferFunction->AddRGBPoint(scalarRange[0], 0.6, 0.4, 0.1);
    // colorTransferFunction->AddRGBPoint(scalarRange[1], 0.2, 0.1, 0.3);
    vtkSmartPointer<vtkVolume> volume = vtkSmartPointer<vtkVolume>::New();
    volume->SetMapper(volumeMapper);
    volume->SetProperty(volumeProperty);
    renderer->AddVolume(volume);
    renderer->ResetCamera();

    return vtk;
  }
};
vtkStandardNewMacro(MyVolumeItem::Data);

/*=========================================================================*/

struct MyGlyphItem : QQuickVTKItem
{
  struct Data : vtkObject
  {
    static Data* New();
    vtkTypeMacro(Data, vtkObject);
  };

  vtkUserData initializeVTK(vtkRenderWindow* renderWindow) override
  {
    auto vtk = vtkNew<Data>();

    vtkNew<vtkRenderer> renderer;
    renderWindow->AddRenderer(renderer);

    // Create the glyph pipeline
    vtkNew<vtkSphereSource> sphere;
    vtkNew<vtkGlyph3DMapper> glyphMapper;
    vtkNew<vtkConeSource> squad;
    glyphMapper->SetInputConnection(sphere->GetOutputPort());
    glyphMapper->SetSourceConnection(squad->GetOutputPort());
    glyphMapper->SetOrientationArray("Normals");
    vtkNew<vtkActor> glyphActor;
    glyphActor->SetMapper(glyphMapper);
    glyphActor->GetProperty()->SetDiffuseColor(0.5, 1.0, 0.8);
    renderer->AddActor(glyphActor);
    renderer->ResetCamera();

    return vtk;
  }
};
vtkStandardNewMacro(MyGlyphItem::Data);
}

/*=========================================================================*/

int TestQQuickVTKItem_3(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  QQuickVTKItem::setGraphicsApi();
  QApplication app(argc, argv);
  Argc = argc;
  Argv = argv;

  qmlRegisterType<MyGeomItem>("Vtk", 1, 0, "MyGeomItem");
  qmlRegisterType<MyGlyphItem>("Vtk", 1, 0, "MyGlyphItem");
  qmlRegisterType<MyVolumeItem>("Vtk", 1, 0, "MyVolumeItem");

  return detail::performTest(argc, argv, "qrc:///TestQQuickVTKItem_3.qml");
}
