#include "vtkVRMLExporter.h"
#include "vtkPLOT3DReader.h"
#include "vtkContourFilter.h"
#include "vtkPolyDataNormals.h"
#include "vtkStructuredGridOutlineFilter.h"
#include "vtkPolyDataMapper.h"

int main ()
{
  char arg1[80];
  float isoval;
  char isoType[20];
  char *env;
  
  // first get the form data
  env = getenv("CONTENT_LENGTH");
  if (!env) return -1;
  int inputLength = atoi(env);
  // a quick sanity check on the input
  if ((inputLength > 40)||(inputLength < 17)) return -1;
  cin >> arg1;
  //  printf("Content-type: text/plain\n\n");
  
  if (strncmp(arg1,"isoval=",7) == 0)
    {
    isoval = atof(arg1 + 7);
    strcpy(isoType,arg1 + 11);
    //    printf("Status: 200 %s %f %s\n",arg1,isoval,isoType);
    //    return 0;
    }
  else
    {
    isoval = atof(arg1 + inputLength - 4);
    strncpy(isoType,arg1 + 4,inputLength - 16);
    isoType[inputLength - 16] = '\0';
    //    printf("Status: 200 %i %s %f %s\n",inputLength,arg1,isoval,isoType);
    //    return 0;
    }
  

  vtkRenderer *ren1 = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->AddRenderer(ren1);
  
  // read data
  vtkPLOT3DReader *reader = vtkPLOT3DReader::New();
  reader->SetXYZFileName("/home/martink2/vtkdata/combxyz.bin");
  reader->SetQFileName("/home/martink2/vtkdata/combq.bin");
  reader->SetFileFormat(VTK_WHOLE_SINGLE_GRID_NO_IBLANKING);
  reader->SetScalarFunctionNumber(100);
  reader->SetVectorFunctionNumber(202);
  reader->Update(); //force a read to occur
  
  vtkContourFilter *iso = vtkContourFilter::New();
  iso->SetInput( reader->GetOutput() );
  iso->SetValue(0, isoval );

  vtkPolyDataNormals *normals = vtkPolyDataNormals::New();
  normals->SetInput( iso->GetOutput() );
  normals->SetFeatureAngle( 45 );
  normals->FlipNormalsOn();

  vtkPolyDataMapper *isoMapper = vtkPolyDataMapper::New();
  isoMapper->SetInput(normals->GetOutput());
  isoMapper->ScalarVisibilityOff();

  vtkActor *isoActor = vtkActor::New();
  isoActor->GetProperty()->SetColor(0.5,0.5,1.0);
  isoActor->SetMapper( isoMapper );

  // create outline
  vtkStructuredGridOutlineFilter *outlineF = 
    vtkStructuredGridOutlineFilter::New();
  outlineF->SetInput(reader->GetOutput());
  vtkPolyDataMapper *outlineMapper = vtkPolyDataMapper::New();
  outlineMapper->SetInput(outlineF->GetOutput());
  vtkActor *outline = vtkActor::New();
  outline->SetMapper(outlineMapper);
  outline->GetProperty()->SetAmbient(1);
  outline->GetProperty()->SetDiffuse(0);
  outline->GetProperty()->SetColor(0.5,1,0.5);
  
  // should we do the iso-surface
  if (strcmp(isoType,"Off")) 
    {
    ren1->AddActor( isoActor );
    }
  if (strcmp(isoType,"Transparent") == 0)
    {
    isoActor->GetProperty()->SetOpacity( 0.5 );
    }
  
  ren1->AddActor( outline );

  // Send out vrml header stuff
  fprintf(stdout,"Content-type: x-world/x-vrml\n");
  fprintf(stdout,"Pragma: no-cache\n\n");
  
  // write out VRRML 2.0 file
  vtkVRMLExporter *writer = vtkVRMLExporter::New();
  writer->SetInput( renWin );
  writer->SetFilePointer( stdout );
  writer->Write();

  return 0;
}

