// This program test the ports by setting up a simple pipeline.

#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkDataSetReader.h"
#include "vtkFieldDataToAttributeDataFilter.h"
#include "vtkGeometryFilter.h"
#include "vtkCutMaterial.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkActor2D.h"
#include "vtkTextMapper.h"
#include "vtkScalarBarActor.h"


int main( int argc, char *argv[] )
{
  float *center;
  float *normal;
  vtkDataSet *data;
  vtkDataArray *materialArray;
  int numMaterials;
  int materialIdx;
  int numArrays;
  int arrayIdx;
  char *arrayName;
  vtkDataArray *array;
  int num, tmp;
  int idx;
  char str[1024];
  float range[2];
  
  if( (argc < 2))
    {
    cerr << "Missing file name. Usage:\n";
    cerr << "FayShow dataFileName\n";
    cerr << " dataFileName should be the path of a VTK data file.\n";
    exit(0);
    }  

  vtkDataSetReader *reader = vtkDataSetReader::New();
  reader->SetFileName(argv[1]);
  reader->Update();
  data = reader->GetOutput();
  if (! data || data->GetNumberOfCells() == 0)
    {
    cerr << "No data." << endl;
    exit(0);
    }
  
  // Get the material array.
  materialArray = data->GetCellData()->GetFieldData()->GetArray("material");
  if (materialArray == NULL)
    {
    cerr << "Expecting a cell data array named \"material\"" << endl;
    exit(0);
    }
  // Find out how many materials we should have.
  // We could package the array into a scalars, but 
  // I would rather eventually put a method into vtkDataArray.
  numMaterials = 0;
  num = materialArray->GetNumberOfTuples();
  for (idx = 0; idx < num; ++idx)
    {
    tmp = (int)(materialArray->GetComponent(idx, 0));
    if (tmp > numMaterials)
      {
      numMaterials = tmp;
      }
    }
  numArrays = data->GetCellData()->GetFieldData()->GetNumberOfArrays();
  
  
  // Create the pipeline
  // Create the render and render window.
  vtkRenderer *ren1 = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
    renWin->AddRenderer(ren1);
    renWin->SetSize(450, 450);
    ren1->SetBackground(0.0, 0.0, 0.0);

  vtkCutMaterial *cut = vtkCutMaterial::New();
    cut->SetInput(data);  
  vtkPolyDataMapper *mapper1 = vtkPolyDataMapper::New();
    mapper1->SetInput(cut->GetOutput());
    mapper1->ScalarVisibilityOn();
    mapper1->SetScalarModeToUseCellFieldData();
  vtkActor *actor1 = vtkActor::New();
    actor1->SetMapper(mapper1);
  ren1->AddActor(actor1);
  
  vtkTextMapper *textMapper = vtkTextMapper::New();
    textMapper->SetJustificationToLeft();
  vtkActor2D *textActor = vtkActor2D::New();
    textActor->SetMapper(textMapper);
    textActor->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
    textActor->GetPositionCoordinate()->SetValue(0.1, 0.9);
  ren1->AddActor(textActor);

  vtkScalarBarActor *scalarBar = vtkScalarBarActor::New();
    scalarBar->SetLookupTable(mapper1->GetLookupTable());
    scalarBar->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
    scalarBar->GetPositionCoordinate()->SetValue(0.1, 0.01);
    scalarBar->SetOrientationToHorizontal();
    scalarBar->SetWidth(0.8);
    scalarBar->SetHeight(0.17);
  ren1->AddActor(scalarBar);    
      
  // Loop through all materials and fields.
  for (materialIdx = 1; materialIdx <= numMaterials; ++materialIdx)
    {
    for (arrayIdx = 0; arrayIdx < numArrays; ++arrayIdx)
      {
      array = data->GetCellData()->GetFieldData()->GetArray(arrayIdx);
      arrayName = data->GetCellData()->GetFieldData()->GetArrayName(arrayIdx);
      cerr << materialIdx << " " << arrayName << endl;
      if (arrayName && strlen(arrayName) && strcmp(arrayName, "material") != 0)
	{
	cut->SetMaterial(materialIdx);
	cut->SetArrayName(arrayName);
	cut->Update();
	if (cut->GetOutput()->GetNumberOfCells() != 0)
	  {
	  center = cut->GetCenterPoint();
	  normal = cut->GetNormal();
	  sprintf(str, "material: %d,  View Plane Normal = %f, %f, %f", materialIdx, normal[0], normal[1], normal[2]);
	  textMapper->SetInput(str);

	  scalarBar->SetTitle(arrayName);
	  
	  mapper1->ColorByArrayComponent(arrayName, 0);
	  // There has to be a more direct way to reset the color range.
	  mapper1->GetColors()->GetRange(range);
	  mapper1->SetScalarRange(range[0], range[1]+0.0001);
	  
	  ren1->GetActiveCamera()->SetViewUp(cut->GetUpVector());
	  ren1->GetActiveCamera()->SetFocalPoint(center);
	  ren1->GetActiveCamera()->SetPosition(center[0]+normal[0], center[1]+normal[1], center[2]+normal[2]);
	  ren1->ResetCamera();
	  
	  renWin->Render();

	  sprintf(str, "FayShow_%d_%s.ppm", materialIdx, arrayName);
	  renWin->SetFileName(str);
	  renWin->SaveImageAsPPM();
	  }
	}
      }
    }  
}









