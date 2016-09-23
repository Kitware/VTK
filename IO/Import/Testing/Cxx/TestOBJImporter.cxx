
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
#include "vtkCamera.h"

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"
#include "vtksys/SystemTools.hxx"

int TestOBJImporter( int argc, char * argv [] )
{
    // Files for testing demonstrate updated functionality for OBJ import:
    //       polydata + textures + actor properties all get loaded.

    if(argc < (5))
    {
      std::cerr<<"expected TestName -D  File1.obj [File2.obj.mtl]  [texture1]  [texture2]  ... "<<std::endl;
      return -1;
    }

    std::string filenameOBJ(argv[2]);

    std::string filenameMTL,texfile1,texfile2;

    if(argc >= 6)
    {
      filenameMTL = argv[3];
    }

    if(argc >= 7)
    {
      texfile1 = argv[4];
    }

    if(argc >= 8)
    {
      texfile2 = argv[5];
    }

    std::string texture_path1 = vtksys::SystemTools::GetFilenamePath(texfile1);
    std::string texture_path2 = vtksys::SystemTools::GetFilenamePath(texfile2);
    if( 0 != texture_path1.compare(texture_path2) )
    {
      std::cerr<<" texture files should be in the same path: "<<texture_path1<<std::endl;
      return -2;
    }

    int lastArg = (argc <= 8) ? (argc-1) : 7;
    std::string tmppath(argv[lastArg]);
    vtkNew<vtkOBJImporter> importer;


    if(argc > 8)
    {
      importer->DebugOn();
    }

    importer->SetFileName(filenameOBJ.data());
    importer->SetFileNameMTL(filenameMTL.data());
    importer->SetTexturePath(texture_path1.data());

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
    ren->GetActiveCamera()->SetPosition(10,10,-10);
    ren->ResetCamera();
    int retVal = vtkRegressionTestImage(renWin.GetPointer());
    if( retVal == vtkRegressionTester::DO_INTERACTOR )
    {
      renWin->SetSize(800,600);
      renWin->SetAAFrames(3);
      iren->Start();
    }

    // Some tests do not produce images... allow them to pass.
    // But if we had an image, it must be within the threshold:
    return (
      retVal == vtkRegressionTester::PASSED ||
      retVal == vtkRegressionTester::NOT_RUN) ?
      0 : 1;
}
