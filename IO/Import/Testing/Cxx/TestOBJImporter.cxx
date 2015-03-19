
#include "vtkVRMLImporter.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkOBJImporter.h"
#include "vtkTestUtilities.h"
#include "vtkNew.h"
#include "vtkJPEGWriter.h"
#include "vtkPNGWriter.h"
#include "vtkImageCanvasSource2D.h"
#include "vtkImageCast.h"

std::string  createTestJPG()
{
    std::string outputFilename = "flare.jpg";
    int extent[6] = {0, 99, 0, 99, 0, 0};
    vtkSmartPointer<vtkImageCanvasSource2D> imageSource = vtkSmartPointer<vtkImageCanvasSource2D>::New();
    imageSource->SetExtent(extent);
    imageSource->SetScalarTypeToUnsignedChar();
    imageSource->SetNumberOfScalarComponents(3);
    imageSource->SetDrawColor(127, 45, 255);
    imageSource->FillBox(0, 99, 0, 99);
    imageSource->SetDrawColor(255,100,200);
    imageSource->FillBox(40, 70, 20, 50);
    imageSource->Update();

    vtkSmartPointer<vtkImageCast> castFilter = vtkSmartPointer<vtkImageCast>::New();
    castFilter->SetOutputScalarTypeToUnsignedChar ();
    castFilter->SetInputConnection(imageSource->GetOutputPort());
    castFilter->Update();

    vtkSmartPointer<vtkJPEGWriter> writer = vtkSmartPointer<vtkJPEGWriter>::New();
    writer->SetFileName(outputFilename.c_str());
    writer->SetInputConnection(castFilter->GetOutputPort());
    writer->Write();
    return outputFilename;
}

std::string  createTestPNG()
{
    std::string outputFilename = "map1024.png";
    int extent[6] = {0, 99, 0, 99, 0, 0};
    vtkSmartPointer<vtkImageCanvasSource2D> imageSource = vtkSmartPointer<vtkImageCanvasSource2D>::New();
    imageSource->SetExtent(extent);
    imageSource->SetScalarTypeToUnsignedChar();
    imageSource->SetNumberOfScalarComponents(3);
    imageSource->SetDrawColor(240, 200, 220);
    imageSource->FillBox(0, 99, 0, 99);
    imageSource->SetDrawColor(60,60,60);
    imageSource->FillBox(40, 70, 20, 50);

    imageSource->SetDrawColor(6,6,6);
    imageSource->FillBox(20, 24, 20, 40);

    imageSource->Update();

    vtkSmartPointer<vtkImageCast> castFilter = vtkSmartPointer<vtkImageCast>::New();
    castFilter->SetOutputScalarTypeToUnsignedChar ();
    castFilter->SetInputConnection(imageSource->GetOutputPort());
    castFilter->Update();

    vtkSmartPointer<vtkPNGWriter> writer = vtkSmartPointer<vtkPNGWriter>::New();
    writer->SetFileName(outputFilename.c_str());
    writer->SetInputConnection(castFilter->GetOutputPort());
    writer->Write();
    return outputFilename;
}

namespace
{
int s_interactive = 0;

bool bInteractive()
{
    return (s_interactive>0);
}

}

int TestOBJImporter( int argc, char * argv [] )
{
    // Files for testing demonstrate updated functionality for OBJ import:
    //       polydata + textures + actor properties all get loaded.

    std::string filenameOBJ;
    std::string filenameMTL;
    vtkNew<vtkOBJImporter> importer;
    if(argc==0)
    {
        filenameOBJ = "MiG-35.obj";
        filenameMTL = "MiG-35.obj.mtl";
    }
    else
    {
        if( argc < 3 )
        {
            std::cerr << "invalid args; expected: "<< argv[0]
                      << " File.obj "<<" File.obj.mtl " << std::endl;
            return 1;
        }
        else
        {
            // Ok, set the filenames from which to load geometry + material info.
            filenameOBJ = std::string(argv[1]);
            filenameMTL = std::string(argv[2]);
            if(argc > 3)
                s_interactive = 1;
        }
    }
    std::string jpgfile = createTestJPG();
    std::string pngfile = createTestPNG();
    importer->SetFileName(filenameOBJ.data());
    importer->SetFileNameMTL(filenameMTL.data());

    vtkNew<vtkRenderer> ren;
    vtkNew<vtkRenderWindow> renWin;
    vtkNew<vtkRenderWindowInteractor> iren;

    renWin->AddRenderer(ren.Get());
    iren->SetRenderWindow(renWin.Get());
    importer->SetRenderWindow(renWin.Get());
    importer->Update();

    if( bInteractive() )
    {
        renWin->SetSize(800,600);
        renWin->SetAAFrames(3);
        iren->Start();
    }
    return 0;
}
