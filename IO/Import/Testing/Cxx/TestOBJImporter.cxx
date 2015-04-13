
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

#include "vtkTestUtilities.h"

std::string  createTestJPG(const std::string& path)
{
    std::string outputFilename = path + "/flare.jpg";
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

std::string  createTestPNG(const std::string& path)
{
    std::string outputFilename = path + "/map1024.png";
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

    if(argc < 6)
    {
      std::cerr<<"expected TestName -D  File1.obj -D File2.obj.mtl"<<std::endl;
      return -1;
    }

    std::string filenameOBJ(argv[2]);
    std::string filenameMTL(argv[3]);

    std::string tmppath(argv[5]);
    vtkNew<vtkOBJImporter> importer;

    if(argc > 6)
      s_interactive = 1;

    std::string jpgfile = createTestJPG(tmppath);
    std::string pngfile = createTestPNG(tmppath);
    std::cout<<"created texture images: "<<jpgfile<<", "<<pngfile<<std::endl;

    importer->SetFileName(filenameOBJ.data());
    importer->SetFileNameMTL(filenameMTL.data());
    importer->SetTexturePath(tmppath.data());

    vtkNew<vtkRenderer> ren;
    vtkNew<vtkRenderWindow> renWin;
    vtkNew<vtkRenderWindowInteractor> iren;

    renWin->AddRenderer(ren.Get());
    iren->SetRenderWindow(renWin.Get());
    importer->SetRenderWindow(renWin.Get());
    importer->Update();

    ren->ResetCamera();

    if( 1 > ren->GetActors()->GetNumberOfItems() )
    {
      std::cerr << "failed to get an actor created?!" << std::endl;
      return -1;
    }
    if( bInteractive() )
    {
        renWin->SetSize(800,600);
        renWin->SetAAFrames(3);
        iren->Start();
    }
    return 0;
}
