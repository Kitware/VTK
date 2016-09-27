#include "SegyReader.h"
using namespace std;

#include <vtkSmartPointer.h>
#include <vtkStructuredPointsReader.h>
#include <vtkGPUVolumeRayCastMapper.h>
#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkVolumeProperty.h>
#include <vtkAxesActor.h>
#include <vtkImageShiftScale.h>
#include <vtkVolumeRayCastCompositeFunction.h>
#include <vtkVolumeRayCastMapper.h>
#include <vtkCubeAxesActor.h>
#include <vtkOrientationMarkerWidget.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyData.h>

#include <vtkPlane.h>
#include <vtkCellArray.h>
#include <vtkAxisActor.h>
#include <vtkProperty.h>
#include <vtkAxesActor.h>
#include <vtkCamera.h>
#include <vtkImageMapToColors.h>
#include <vtkWindowLevelLookupTable.h>
#include "vtkSeismicSliceWidget.h"

#include "vtkSeismicSliceCallback.h"

#include <vtkCallbackCommand.h>
#include <vtkLegendScaleActor.h>
#include <vtkColorLegend.h>

#include <vtkContext2D.h>
#include <vtkLegendBoxActor.h>

#include <vtkScalarBarActor.h>

#include <vtkPlaneWidget.h>
#include <vtkPolyDataMapper.h>

#include <vtkDataSetMapper.h>

#include <vtkGlyph3D.h>

#include <vtkSphereSource.h>

#include <vtkCubeSource.h>

#include "RdvReader.h"

#include "vtkSegy2DReader.h"
#include "vtkSegy3DReader.h"

#include <vtkPolyDataAlgorithm.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>

void expandBounds(double* bounds)
{
    double xRange = bounds[1] - bounds[0];
    double yRange = bounds[3] - bounds[2];
    double zRange = bounds[5] - bounds[4];

    double fraction = 0.05;

    bounds[0] -= xRange * fraction;
    bounds[1] += xRange * fraction;
    bounds[2] -= yRange * fraction;
    bounds[3] += yRange * fraction;
    bounds[4] -= zRange * fraction;
    bounds[5] += zRange * fraction;
}

void render(vtkImageData* id)
{
    vtkSmartPointer<vtkPiecewiseFunction> opacityTransferFunction = vtkPiecewiseFunction::New();
    opacityTransferFunction->AddPoint(0,1.0);
    opacityTransferFunction->AddPoint(50, 0.1);
    opacityTransferFunction->AddPoint(220,0.0);

    vtkSmartPointer<vtkColorTransferFunction> colorTransferFunction= vtkColorTransferFunction::New();
    colorTransferFunction->AddRGBPoint(0.0, 0.0, 0.0, 1.0);
    colorTransferFunction->AddRGBPoint(64.0, 1.0, 0.0, 0.0);
    colorTransferFunction->AddRGBPoint(128.0, 0.0, 0.0, 1.0);
    colorTransferFunction->AddRGBPoint(192.0, 0.0, 1.0, 0.0);
    colorTransferFunction->AddRGBPoint(255.0, 0.0, 0.2, 0.0);

    vtkSmartPointer<vtkVolumeProperty> volumeProperty = vtkVolumeProperty::New();
    volumeProperty->SetColor(colorTransferFunction);
    //volumeProperty->SetScalarOpacity(opacityTransferFunction);
    volumeProperty->SetInterpolationTypeToLinear();
    volumeProperty->SetDiffuse(0.4);
    volumeProperty->SetAmbient(0.6);
    volumeProperty->SetSpecular(0.2);
    volumeProperty->SetSpecularPower(70.0);


    vtkSmartPointer<vtkGPUVolumeRayCastMapper> volumeMapper = vtkGPUVolumeRayCastMapper::New();
    volumeMapper->SetInputData(id);

    vtkVolume *volume=vtkVolume::New();
    volume->SetMapper(volumeMapper);
    volume->SetProperty(volumeProperty);


    vtkRenderer* renderer =vtkRenderer::New();

    vtkRenderWindow* renwin=vtkRenderWindow::New();
    renwin->AddRenderer(renderer);

    vtkSmartPointer<vtkRenderWindowInteractor> interactor = vtkRenderWindowInteractor::New();
    interactor->SetRenderWindow(renwin);


    // Cube Axes
    vtkSmartPointer<vtkCubeAxesActor> cubeAxesActor = vtkCubeAxesActor::New();
    cubeAxesActor->SetXTitle("X /m");
    cubeAxesActor->SetYTitle("Y /m");
    cubeAxesActor->SetZTitle("Z /m");
    cubeAxesActor->SetCamera(renderer->GetActiveCamera());
    double* expandedBounds = id->GetBounds();
    expandBounds(expandedBounds);
    cubeAxesActor->SetBounds(expandedBounds);
    renderer->AddActor(cubeAxesActor);

    vtkCamera* camera = renderer->GetActiveCamera();
    camera->SetFocalPoint( volume->GetCenter() );
    camera->SetPosition(0, 0, -50);

    renderer->AddVolume(volume);
    renderer->SetBackground(0.0, 0.0, 0.0);



    vtkSmartPointer<vtkSeismicSliceWidget> sliceWidgetX = vtkSeismicSliceWidget::New();
    sliceWidgetX->SetInteractor(renwin->GetInteractor());
    sliceWidgetX->SetPlaceFactor(1.25);
    sliceWidgetX->SetInputData(id);
    sliceWidgetX->PlaceWidget();
    sliceWidgetX->SetPlaneOrientationToXAxes();
    sliceWidgetX->On();

    vtkSmartPointer<vtkSeismicSliceWidget> sliceWidgetY = vtkSeismicSliceWidget::New();
    sliceWidgetY->SetInteractor(renwin->GetInteractor());
    sliceWidgetY->SetPlaceFactor(1.25);
    sliceWidgetY->SetInputData(id);
    sliceWidgetY->PlaceWidget();
    sliceWidgetY->SetPlaneOrientationToYAxes();
    sliceWidgetY->On();



    vtkSeismicSliceCallback* callback = vtkSeismicSliceCallback::New();
    sliceWidgetX->AddObserver(vtkCommand::InteractionEvent, callback);
    sliceWidgetY->AddObserver(vtkCommand::InteractionEvent, callback);


    // Legend
    vtkSmartPointer<vtkScalarBarActor> scalarBar = vtkScalarBarActor::New();
    scalarBar->SetLookupTable(colorTransferFunction);
    renderer->AddActor(scalarBar);

    volumeMapper->AddClippingPlane(sliceWidgetX->GetPlane());
    volumeMapper->AddClippingPlane(sliceWidgetY->GetPlane());

    renwin->Render();

    interactor->Start();
}

void demo2D()
{
    vtkSmartPointer<vtkColorTransferFunction> colorTransferFunction= vtkColorTransferFunction::New();
    colorTransferFunction->AddRGBPoint(0.0, 0.0, 0.0, 1.0);
    colorTransferFunction->AddRGBPoint(64.0, 1.0, 0.0, 0.0);
    colorTransferFunction->AddRGBPoint(128.0, 0.0, 0.0, 1.0);
    colorTransferFunction->AddRGBPoint(192.0, 0.0, 1.0, 0.0);
    colorTransferFunction->AddRGBPoint(255.0, 0.0, 0.2, 0.0);

    vtkSmartPointer<vtkRenderer> renderer =
            vtkSmartPointer<vtkRenderer>::New();
    vtkSmartPointer<vtkRenderWindow> renderWindow =
            vtkSmartPointer<vtkRenderWindow>::New();
    renderWindow->AddRenderer(renderer);
    vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor =
            vtkSmartPointer<vtkRenderWindowInteractor>::New();
    renderWindowInteractor->SetRenderWindow(renderWindow);

    vector<string> files;
    files.push_back("data/lineA.sgy");
    files.push_back("data/lineB.sgy");
    files.push_back("data/lineC.sgy");
    files.push_back("data/lineD.sgy");
    files.push_back("data/lineE.sgy");

    auto file = files[0];
    //for(auto file : files)
    {
        vtkSmartPointer<vtkSegy2DReader> reader = vtkSmartPointer<vtkSegy2DReader>::New();
        reader->SetFileName(file.c_str());
        reader->Update();

        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();

        vtkSmartPointer<vtkPolyDataMapper> mapper =
                vtkSmartPointer<vtkPolyDataMapper>::New();


        mapper->SetInputConnection(reader->GetOutputPort());
        mapper->ScalarVisibilityOn();
        mapper->SetLookupTable(colorTransferFunction);
        actor->SetMapper(mapper);
        //actor->SetTexture(texture);

        renderer->AddActor(actor);
    }


    renderWindow->Render();
    renderWindowInteractor->Start();
}

void demo3D()
{
    vtkSmartPointer<vtkSegy3DReader> reader = vtkSmartPointer<vtkSegy3DReader>::New();
    reader->SetFileName("data/waha8.sgy");
    vtkSmartPointer<vtkImageData> imageData = reader->GetImage(0);
    render(imageData);
}

void demoRDV()
{

    vtkSmartPointer<vtkColorTransferFunction> colorTransferFunction= vtkColorTransferFunction::New();
    colorTransferFunction->AddRGBPoint(300.0, 0.0, 0.0, 1.0);
    colorTransferFunction->AddRGBPoint(600.0, 1.0, 0.0, 0.0);
    colorTransferFunction->AddRGBPoint(900.0, 0.0, 0.0, 1.0);
    colorTransferFunction->AddRGBPoint(1200.0, 0.0, 1.0, 0.0);
    colorTransferFunction->AddRGBPoint(1500.0, 0.0, 0.2, 0.0);


    RdvReader reader;

    auto polyData = vtkPolyData::New();
    reader.Read("data/Events.rdv", polyData);

    vtkSmartPointer<vtkGlyph3D> glyph3D =
            vtkSmartPointer<vtkGlyph3D>::New();
    glyph3D->SetInputData(polyData);

    vtkSmartPointer<vtkSphereSource> cellSource =
            vtkSmartPointer<vtkSphereSource>::New();

    cellSource->SetRadius(0.01);



    glyph3D->SetSourceConnection(cellSource->GetOutputPort());
    glyph3D->Update();

    auto mapper = vtkPolyDataMapper::New();
    mapper->SetInputData(glyph3D->GetOutput());
    mapper->SetLookupTable(colorTransferFunction);

    auto actor = vtkActor::New();
    actor->SetMapper(mapper);

    auto renderer = vtkRenderer::New();
    renderer->AddActor(actor);

    auto window = vtkRenderWindow::New();
    window->AddRenderer(renderer);

    auto interactor = vtkRenderWindowInteractor::New();
    interactor->SetRenderWindow(window);

    vtkSmartPointer<vtkCubeAxesActor> cubeAxesActor = vtkCubeAxesActor::New();
    cubeAxesActor->SetXTitle("X /m");
    cubeAxesActor->SetYTitle("Y /m");
    cubeAxesActor->SetZTitle("Z /m");
    cubeAxesActor->SetCamera(renderer->GetActiveCamera());
    double* expandedBounds = polyData->GetBounds();
    expandBounds(expandedBounds);
    cubeAxesActor->SetBounds(expandedBounds);
    renderer->AddActor(cubeAxesActor);

    vtkCamera* camera = renderer->GetActiveCamera();
    camera->SetFocalPoint( polyData->GetCenter() );
    camera->SetPosition(0, 0, -50);

    vtkSmartPointer<vtkScalarBarActor> scalarBar = vtkScalarBarActor::New();
    scalarBar->SetLookupTable(colorTransferFunction);
    renderer->AddActor(scalarBar);

    window->Render();
    interactor->Start();
}

int main(int argc, char** argv)
{
    if(argc > 1 && strcmp(argv[1], "2") == 0)
    {
        demo2D();
    }
    else if(argc > 1 && strcmp(argv[1], "3") == 0)
    {
        demo3D();
    }
    else
    {
        demoRDV();
    }
    return 0;
}
