
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

#include "vtksys/SystemTools.hxx"

#include <sstream>
#include "vtkMapper.h"

#include "vtkXMLPolyDataWriter.h"
#include "vtkTriangleFilter.h"

int main( int argc, char * argv [] )
{
  // note that the executable name is stripped out already
  // so argc argv will not have it

  // Files for testing demonstrate updated functionality for OBJ import:
  //       polydata + textures + actor properties all get loaded.
  if(argc < 3)
  {
    std::cerr<<"expected vtkimportobj OutputDirectory File1.obj [File2.obj.mtl] [texture1]"<<std::endl;
    return -1;
  }

  std::string filenameOBJ(argv[2]);

  std::string filenameMTL,texfile1;

  if(argc >= 4)
  {
    filenameMTL = argv[3];
  }

  if(argc >= 5)
  {
    texfile1 = argv[4];
  }
  std::string texture_path1 = vtksys::SystemTools::GetFilenamePath(texfile1);

  vtkNew<vtkOBJImporter> importer;

  if(argc > 5)
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

  // save out data for actors
  for (int i = 0; i < ren->GetActors()->GetNumberOfItems(); i++)
  {
    std::ostringstream os;
    os << argv[1] << "/Model" << i << ".vtp";
    vtkActor *act = (vtkActor *)ren->GetActors()->GetItemAsObject(i);
    vtkNew<vtkTriangleFilter> trif;
    trif->SetInputData((vtkDataObject *)act->GetMapper()->GetInput());
    vtkNew<vtkXMLPolyDataWriter> writer;
    writer->SetFileName(os.str().c_str());
    writer->SetInputConnection(trif->GetOutputPort());
    writer->Write();
    std::string desc = importer->GetOutputDescription(i);
    cerr << "Wrote " << os.str() << " " << desc << "\n";
  }

  if(ren->GetActors()->GetNumberOfItems() == 0)
  {
    std::cerr << "failed to get an actor created?!" << std::endl;
    return -1;
  }

  ren->SetBackground(0.4,0.5,0.6);
  ren->ResetCamera();

  renWin->SetSize(800,600);
  //iren->Start();

  return 0;
}
