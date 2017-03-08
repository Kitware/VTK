
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

#include <sstream>
#include "vtkMapper.h"

int TestOBJImporter( int argc, char * argv [] )
{
  // note that the executable name is stripped out already
  // so argc argv will not have it

  // Files for testing demonstrate updated functionality for OBJ import:
  //       polydata + textures + actor properties all get loaded.
  if(argc < 2)
  {
    std::cerr<<"expected TestName File1.obj [File2.obj.mtl]  [texture1]  ... "<<std::endl;
    return -1;
  }

  std::string filenameOBJ(argv[1]);

  std::string filenameMTL,texfile1;

  if(argc >= 3)
  {
    filenameMTL = argv[2];
  }

  if(argc >= 4)
  {
    texfile1 = argv[3];
  }
  std::string texture_path1 = vtksys::SystemTools::GetFilenamePath(texfile1);

  vtkNew<vtkOBJImporter> importer;

  if(argc > 4)
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
