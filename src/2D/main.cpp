#include "SegyReader.h"
using namespace std;

#include <vtkSmartPointer.h>
#include <vtkStructuredPointsReader.h>
#include <vtkVolumeTextureMapper3D.h>
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
#include <vtkMatrix4x4.h>

#include <vtkQuad.h>
#include <vtkCellArray.h>

void render(vtkImageData* id)
{
    vtkPiecewiseFunction *opacityTransferFunction = vtkPiecewiseFunction::New();
    opacityTransferFunction->AddPoint(0,1.0);
    opacityTransferFunction->AddPoint(220,1.0);

    vtkColorTransferFunction *colorTransferFunction= vtkColorTransferFunction::New();
    colorTransferFunction->AddRGBPoint(0.000,  1.00, 0.00, 0.00);
    //colorTransferFunction->AddRGBPoint(60.000,  1.00, 1.00, 1.00);
    //colorTransferFunction->AddRGBPoint(128.00,  1.00, 1.0, 1.0);
    //makecolorTransferFunction->AddRGBPoint(200.000,  1.00, 1.00, 1.00);
    colorTransferFunction->AddRGBPoint(255.0,  0.00, 0.00, 1.00);

    vtkVolumeProperty *volumeProperty = vtkVolumeProperty::New();
    volumeProperty->SetColor(colorTransferFunction);
    volumeProperty->SetScalarOpacity(opacityTransferFunction);
    volumeProperty->SetInterpolationTypeToNearest();
    volumeProperty->SetDiffuse(0.4);
    volumeProperty->SetAmbient(0.6);
    volumeProperty->SetSpecular(0.2);
    volumeProperty->SetSpecularPower(70.0);


    vtkVolumeTextureMapper3D *volumeMapper = vtkVolumeTextureMapper3D::New();
    volumeMapper->SetInputData(id);

    vtkVolume *volume=vtkVolume::New();
    volume->SetMapper(volumeMapper);
    volume->SetProperty(volumeProperty);

    vtkRenderer* ren=vtkRenderer::New();
    ren->AddVolume(volume);
    ren->SetBackground(1.0, 1.0, 1.0);

    vtkRenderWindow* renwin=vtkRenderWindow::New();
    renwin->AddRenderer(ren);

    vtkSmartPointer<vtkRenderWindowInteractor> iren = vtkSmartPointer<vtkRenderWindowInteractor>::New();
    iren->SetRenderWindow(renwin);

    renwin->Render();
    iren->Start();
}

int main(int argc, char** argv) {
    SegyReader reader;
    reader.LoadFromFile("/Users/jiachen/Desktop/SegyVisualizer2D/Data/lineA.sgy");
    vtkSmartPointer<vtkActor> actor1 = vtkSmartPointer<vtkActor>::New();
    reader.render2d(actor1);

    SegyReader reader2;
    reader2.LoadFromFile("/Users/jiachen/Desktop/SegyVisualizer2D/Data/lineB.sgy");
    vtkSmartPointer<vtkActor> actor2 = vtkSmartPointer<vtkActor>::New();
    reader2.render2d(actor2);

    SegyReader reader3;
    reader3.LoadFromFile("/Users/jiachen/Desktop/SegyVisualizer2D/Data/lineC.sgy");
    vtkSmartPointer<vtkActor> actor3 = vtkSmartPointer<vtkActor>::New();
    reader3.render2d(actor3);

    SegyReader reader4;
    reader4.LoadFromFile("/Users/jiachen/Desktop/SegyVisualizer2D/Data/lineD.sgy");
    vtkSmartPointer<vtkActor> actor4 = vtkSmartPointer<vtkActor>::New();
    reader4.render2d(actor4);

    SegyReader reader5;
    reader5.LoadFromFile("/Users/jiachen/Desktop/SegyVisualizer2D/Data/lineE.sgy");
    vtkSmartPointer<vtkActor> actor5 = vtkSmartPointer<vtkActor>::New();
    reader5.render2d(actor5);




    vtkSmartPointer<vtkRenderer> renderer =
            vtkSmartPointer<vtkRenderer>::New();
    vtkSmartPointer<vtkRenderWindow> renderWindow =
            vtkSmartPointer<vtkRenderWindow>::New();
    renderWindow->AddRenderer(renderer);
    vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor =
            vtkSmartPointer<vtkRenderWindowInteractor>::New();
    renderWindowInteractor->SetRenderWindow(renderWindow);
    renderer->AddActor(actor1);
    renderer->AddActor(actor2);
    renderer->AddActor(actor3);
    renderer->AddActor(actor4);
    renderer->AddActor(actor5);
    renderWindow->Render();
    renderWindowInteractor->Start();
}