#include "vtkVRMLExporter.h"
#include "vtkPLOT3DReader.h"
#include "vtkContourFilter.h"
#include "vtkPolyDataNormals.h"
#include "vtkStructuredGridOutlineFilter.h"
#include "vtkStructuredGridGeometryFilter.h"
#include "vtkPolyDataMapper.h"

#include "SaveImage.h"

char *getCGIValue(char *key, char *input)
{
  int idx, found, end;
  int len = strlen(input);
  int klen = strlen(key);
  char *result;

  // first find the key
  found = -1;
  for (idx = 0; idx < len; idx++)
    {
    if (strncmp(key,input+idx,klen) == 0)
      {
      found = idx;
      break;
      }
    }
  
  if (found == -1) return NULL;

  // find end of return value
  end = -1;
  found = found + strlen(key);
  for (idx = found; idx < len; idx++)
    {
    if (input[idx] == '&')
      {
      end = idx;
      break;
      }
    }
  
  if (end == -1) end = len;
  
  result = new char [end - found + 1];
  for (idx = found; idx < end; idx++)
    {
    result[idx - found] = input[idx];
    }
  result[end-found] = '\0';
  
  return result;
}

int main( int argc, char *argv[] )
{
  char arg1[1024];
  float isoval;
  char *isoType;
  char *env;
  char *probeOn;
  char *probeCont;
  float probeLoc;
  
  // first get the form data
  env = getenv("CONTENT_LENGTH");
  if (!env) return -1;
  int inputLength = atoi(env);
  cin >> arg1;
  
  isoval = atof(getCGIValue((char *) "isoval=",arg1));
  isoType = getCGIValue((char *) "iso=",arg1);
  probeLoc = atof(getCGIValue((char *) "probeloc=",arg1));
  probeCont = getCGIValue((char *) "probecont=",arg1);
  probeOn = getCGIValue((char *) "probe=",arg1);
  
  vtkRenderer *ren1 = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->AddRenderer(ren1);
  
  // read data
  vtkPLOT3DReader *reader = vtkPLOT3DReader::New();
  reader->SetXYZFileName("/home/martink/vtkdata/combxyz.bin");
  reader->SetQFileName("/home/martink/vtkdata/combq.bin");
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
  outline->GetProperty()->SetDiffuse(1);
  outline->GetProperty()->SetColor(0.5,1,0.5);
  
  vtkStructuredGridGeometryFilter *plane = 
    vtkStructuredGridGeometryFilter::New();
  plane->SetInput(reader->GetOutput());
  plane->SetExtent(1,100,1,100,probeLoc,probeLoc);

  vtkPolyDataMapper *contourMapper = vtkPolyDataMapper::New();
  contourMapper->SetScalarRange(reader->GetOutput()->GetScalarRange());
  if (!strcmp(probeCont,"On"))
    {
    vtkContourFilter *contour = vtkContourFilter::New();
    contour->SetInput(plane->GetOutput());
    contour->GenerateValues(50,reader->GetOutput()->GetScalarRange());
    contourMapper->SetInput(contour->GetOutput());
    }
  else
    {
    contourMapper->SetInput(plane->GetOutput());
    }
  
  vtkActor *planeActor = vtkActor::New();
  planeActor->SetMapper(contourMapper);

  // should we do the iso-surface
  if (strcmp(probeOn,"Off")) 
    {
    ren1->AddActor( planeActor );
    }
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

  renWin->SetSize( 300, 300 );
  renWin->Render();

  SAVEIMAGE( renWin );

  return 0;
}

