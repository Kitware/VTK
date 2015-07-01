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

void render(vtkImageData* id)
{
    vtkPiecewiseFunction *opacityTransferFunction = vtkPiecewiseFunction::New();
    opacityTransferFunction->AddPoint(0,0.0);
    opacityTransferFunction->AddPoint(220,1.0);

    vtkColorTransferFunction *colorTransferFunction= vtkColorTransferFunction::New();
    colorTransferFunction->AddRGBPoint(0.000,  1.00, 1.00, 1.00);
    colorTransferFunction->AddRGBPoint(64.00,  1.00, 0.0, 0.0);
    colorTransferFunction->AddRGBPoint(180.0,  0.00, 1.00, 0.00);
    colorTransferFunction->AddRGBPoint(240.0,  0.00, 0.00, 1.00);

    vtkVolumeProperty *volumeProperty = vtkVolumeProperty::New();
    volumeProperty->SetColor(colorTransferFunction);
    volumeProperty->SetScalarOpacity(opacityTransferFunction);
    volumeProperty->SetInterpolationTypeToLinear();
    volumeProperty->SetDiffuse(0.4);
    volumeProperty->SetAmbient(0.6);
    volumeProperty->SetSpecular(0.2);
    volumeProperty->SetSpecularPower(70.0);


    vtkVolumeRayCastCompositeFunction *compositeFunction=vtkVolumeRayCastCompositeFunction::New();
    vtkVolumeRayCastMapper *volumeMapper=vtkVolumeRayCastMapper::New();
    volumeMapper->SetInputData(id);
    volumeMapper->SetVolumeRayCastFunction(compositeFunction);

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
    //SegyReader reader("/Users/jiachen/Desktop/SEGYReader/yr99jd85-0616z.segy.mp3");
    //SegyReader reader("/Users/jiachen/Desktop/SEGYReader/namorado_EDT_gocad.SGY");
    //SegyReader reader("/Users/jiachen/Desktop/SegyVisualizer/Data/ar55.9934.mgl0812.axis1.migration.segy");
    //SegyReader reader("/Users/jiachen/Desktop/SegyVisualizer/Data/ar55.9935.mgl0812.axis1.migration.segy");
    //SegyReader reader("/Users/jiachen/Desktop/SegyVisualizer/Data/ar55.9936.mgl0812.axis2.migration.segy");
    //SegyReader reader("/Users/jiachen/Desktop/SegyVisualizer/Data/ar55.9937.mgl0812.axis3.stack.segy");
    SegyReader reader;
    reader.LoadFromFile("/Users/jiachen/Desktop/SegyVisualizer/Data/waha8.sgy");

    vtkImageData* id = vtkImageData::New();
    reader.ExportData(id);

    render(id);
}