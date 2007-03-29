/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestExtraction.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This tests vtkSelection and vtkExtractSelection.
// 
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit
// -W        => write data files at each step for closer inspection
// -S        => draw sample data set in wireframe with each result

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

#include <vtkImageData.h>
#include <vtkUnstructuredGrid.h>
#include <vtkUnstructuredGridWriter.h>
#include <vtkXMLDataSetWriter.h>
#include <vtkPolyDataWriter.h>
#include <vtkPoints.h>
#include <vtkPointData.h>
#include <vtkCellData.h>
#include <vtkIdTypeArray.h>
#include <vtkDoubleArray.h>
#include <vtkCell.h>
#include <vtkDataSetMapper.h>
#include <vtkActor.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkCamera.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkInteractorStyleRubberBandPick.h>
#include <vtkIdList.h>
#include <vtkThreshold.h>

#include <vtkInformation.h>
#include <vtkSelection.h>
#include <vtkExtractSelection.h>

#define XCELLS 3
#define YCELLS 3
#define ZCELLS 3

vtkRenderer *renderer = NULL;
vtkImageData *sampleData = NULL;
int DrawSampleData = 0;

enum {COLORBYCELL, COLORBYPOINT};

void showMe(vtkDataSet *result, int X, int Y, int CellOrPoint, vtkDataArray *array)
{
  vtkDataSet *copy = result->NewInstance();
  copy->DeepCopy(result);
  vtkDataSetMapper *mapper = vtkDataSetMapper::New();
  mapper->SetInput(copy);
  double *range = array->GetRange();
  if (CellOrPoint == COLORBYCELL)
    {
    copy->GetCellData()->SetActiveScalars(array->GetName());
    mapper->SetScalarModeToUseCellData();    
    }
  else
    {
    copy->GetPointData()->SetActiveScalars(array->GetName());
    mapper->SetScalarModeToUsePointData();
    }  
  mapper->SetScalarRange(range[0], range[1]);
  vtkActor *actor = vtkActor::New();
  actor->SetPosition(X*4,Y*4, 0);
  actor->SetMapper(mapper);
  actor->GetProperty()->SetPointSize(6.0);
  mapper->Delete();
  renderer->AddActor(actor);
  actor->Delete();
  copy->Delete();

  if (DrawSampleData)
    {
    vtkDataSetMapper *mapper2 = vtkDataSetMapper::New();
    mapper2->SetScalarModeToUseCellFieldData();    
    if (CellOrPoint == COLORBYCELL)
      {
      mapper2->SetScalarModeToUseCellFieldData();    
      mapper2->SelectColorArray("Forward Cell Ids");
      mapper2->SetScalarRange(10, 36);
    }
    else
      {
      mapper2->SetScalarModeToUsePointFieldData();
      mapper2->SelectColorArray("Forward Point Ids");
      mapper2->SetScalarRange(10, 73);
      }  
    mapper2->SetInput(sampleData);
    vtkActor *actor2 = vtkActor::New();
    actor2->GetProperty()->SetRepresentationToWireframe();
    actor2->SetMapper(mapper2);
    actor2->SetPosition(X*4,Y*4, 0);
    renderer->AddActor(actor2);
    mapper2->Delete();
    actor2->Delete();
    }
}

int TestExtraction(int argc, char *argv[])
{
  int DoWrite = 0;
  for (int i = 0; i < argc; i++)
    {
    if (!strcmp(argv[i], "-W"))
      {
      DoWrite = 1;
      }
    if (!strcmp(argv[i], "-S"))
      {
      DrawSampleData = 1;
      }
    }

  //--------------------------------------------------------------------------
  //create a visualization pipeline to see the results
  renderer = vtkRenderer::New();  
  vtkRenderWindow *renwin = vtkRenderWindow::New();
  renwin->SetSize(600,600);
  renwin->AddRenderer(renderer);
  
  vtkRenderWindowInteractor *rwi = vtkRenderWindowInteractor::New();
  rwi->SetRenderWindow(renwin);  

  //--------------------------------------------------------------------------
  //create a test data set with known structure and data values
  //the structure will look like a Rubix' cube
  //the values will be:
  //three double arrays containing X,Y,and Z coordinates for
  //each point and cell, where the cell coordinates are the center of the cell
  //two id type arrays containing Id's or labels that range from 10 to 
  //numpts/cells+10, with one array being the reverse of the other
  //the scalars datasetattibute will be the X array
  //the globalids datasetattribute will be the forward running id array

  sampleData = vtkImageData::New();
  sampleData->Initialize();
  sampleData->SetSpacing(1.0,1.0,1.0);
  sampleData->SetOrigin(0.0,0.0,0.0);
  sampleData->SetDimensions(XCELLS+1,YCELLS+1,ZCELLS+1);
  sampleData->SetWholeExtent(0,XCELLS+1,0,YCELLS+1,0,ZCELLS+1);
  sampleData->AllocateScalars();

  vtkIdTypeArray *pia = vtkIdTypeArray::New();
  pia->SetNumberOfComponents(1);
  pia->SetName("Point Counter");
  sampleData->GetPointData()->AddArray(pia);
  
  vtkIdTypeArray *piaF = vtkIdTypeArray::New();
  piaF->SetNumberOfComponents(1);
  piaF->SetName("Forward Point Ids");
  sampleData->GetPointData()->AddArray(piaF);

  vtkIdTypeArray *piaR = vtkIdTypeArray::New();
  piaR->SetNumberOfComponents(1);
  piaR->SetName("Reverse Point Ids");
  sampleData->GetPointData()->AddArray(piaR);

  vtkDoubleArray *pxa = vtkDoubleArray::New();
  pxa->SetNumberOfComponents(1);
  pxa->SetName("Point X");
  sampleData->GetPointData()->AddArray(pxa);
  
  vtkDoubleArray *pya = vtkDoubleArray::New();
  pya->SetNumberOfComponents(1);
  pya->SetName("Point Y");
  sampleData->GetPointData()->AddArray(pya);
  
  vtkDoubleArray *pza = vtkDoubleArray::New();
  pza->SetNumberOfComponents(1);
  pza->SetName("Point Z");
  sampleData->GetPointData()->AddArray(pza);

  //vtkPoints *points = vtkPoints::New();
  vtkIdType pcnt = 0;
  for (int i = 0; i < ZCELLS+1; i++)
    {
    for (int j = 0; j < YCELLS+1; j++)
      {
      for (int k = 0; k < XCELLS+1; k++)
        {
        //points->InsertNextPoint(k,j,i);

        pia->InsertNextValue(pcnt);
        int idF = pcnt + 10;
        int idR = (XCELLS+1)*(YCELLS+1)*(ZCELLS+1)-1 - pcnt + 10;
        piaF->InsertNextValue(idF);
        piaR->InsertNextValue(idR);        
        pcnt++;

        pxa->InsertNextValue((double)k);
        pya->InsertNextValue((double)j);
        pza->InsertNextValue((double)i);
        }
      }
    }
  
  //sampleData->SetPoints(points);
  //points->Delete();
  
  //vtkIdList *ids = vtkIdList::New();
  
  vtkIdTypeArray *cia = vtkIdTypeArray::New();
  cia->SetNumberOfComponents(1);
  cia->SetName("Cell Count");
  sampleData->GetCellData()->AddArray(cia);

  vtkIdTypeArray *ciaF = vtkIdTypeArray::New();
  ciaF->SetNumberOfComponents(1);
  ciaF->SetName("Forward Cell Ids");
  sampleData->GetCellData()->AddArray(ciaF);

  vtkIdTypeArray *ciaR = vtkIdTypeArray::New();
  ciaR->SetNumberOfComponents(1);
  ciaR->SetName("Reverse Cell Ids");
  sampleData->GetCellData()->AddArray(ciaR);

  vtkDoubleArray *cxa = vtkDoubleArray::New();
  cxa->SetNumberOfComponents(1);
  cxa->SetName("Cell X");
  sampleData->GetCellData()->AddArray(cxa);

  vtkDoubleArray *cya = vtkDoubleArray::New();
  cya->SetNumberOfComponents(1);
  cya->SetName("Cell Y");
  sampleData->GetCellData()->AddArray(cya);
  
  vtkDoubleArray *cza = vtkDoubleArray::New();
  cza->SetNumberOfComponents(1);
  cza->SetName("Cell Z");
  sampleData->GetCellData()->AddArray(cza);
  
  vtkIdType ccnt = 0;
  for (int i = 0; i < ZCELLS; i++)
    {
    for (int j = 0; j < YCELLS; j++)
      {
      for (int k = 0; k < XCELLS; k++)
        {
        /*
        ids->Reset();
        if (ZCELLS > 1)
          {
          ids->InsertId(0, (i)*(YCELLS+1)*(XCELLS+1) + (j)*(XCELLS+1) + (k));
          ids->InsertId(1, (i)*(YCELLS+1)*(XCELLS+1) + (j)*(XCELLS+1) + (k+1));
          ids->InsertId(2, (i)*(YCELLS+1)*(XCELLS+1) + (j+1)*(XCELLS+1) + (k));
          ids->InsertId(3, (i)*(YCELLS+1)*(XCELLS+1) + (j+1)*(XCELLS+1) + (k+1));
          ids->InsertId(4, (i+1)*(YCELLS+1)*(XCELLS+1) + (j)*(XCELLS+1) + (k));
          ids->InsertId(5, (i+1)*(YCELLS+1)*(XCELLS+1) + (j)*(XCELLS+1) + (k+1));
          ids->InsertId(6, (i+1)*(YCELLS+1)*(XCELLS+1) + (j+1)*(XCELLS+1) + (k));
          ids->InsertId(7, (i+1)*(YCELLS+1)*(XCELLS+1) + (j+1)*(XCELLS+1) + (k+1));          
          sampleData->InsertNextCell(VTK_VOXEL, ids);
          }
        else
          {
          ids->InsertId(0, (i)*(YCELLS+1)*(XCELLS+1) + (j)*(XCELLS+1) + (k));
          ids->InsertId(1, (i)*(YCELLS+1)*(XCELLS+1) + (j)*(XCELLS+1) + (k+1));
          ids->InsertId(2, (i)*(YCELLS+1)*(XCELLS+1) + (j+1)*(XCELLS+1) + (k));
          ids->InsertId(3, (i)*(YCELLS+1)*(XCELLS+1) + (j+1)*(XCELLS+1) + (k+1));
          sampleData->InsertNextCell(VTK_PIXEL, ids);
          }
        */
        cia->InsertNextValue(ccnt);

        int idF = ccnt + 10;
        int idR = (XCELLS)*(YCELLS)*(ZCELLS)-1 - ccnt + 10;
        ciaF->InsertNextValue(idF);
        ciaR->InsertNextValue(idR);
        ccnt++;

        cxa->InsertNextValue(((double)k+0.5));
        cya->InsertNextValue(((double)j+0.5));
        cza->InsertNextValue(((double)i+0.5));
        
        }
      }
    }
  //ids->Delete();  
  
  sampleData->GetPointData()->SetGlobalIds(piaF);
  sampleData->GetPointData()->SetScalars(pxa);
  
  sampleData->GetCellData()->SetGlobalIds(ciaF);
  sampleData->GetCellData()->SetScalars(cxa);
  
  //save the test data set
  vtkXMLDataSetWriter *xwriter = vtkXMLDataSetWriter::New(); 
  xwriter->SetInput(sampleData);
  xwriter->SetFileName("sampleData.vti");
  if (DoWrite)
    {
    xwriter->Write();
    }
  xwriter->Delete();


  //-------------------------------------------------------------------------
  //Setup the components of the pipeline
  vtkSelection *sel = vtkSelection::New();
  vtkExtractSelection *ext = vtkExtractSelection::New();
  ext->SetInput(0, sampleData);
  ext->SetInput(1, sel);
  vtkUnstructuredGridWriter *writer = vtkUnstructuredGridWriter::New();

  vtkUnstructuredGrid *extGrid;

  //-------------------------------------------------------------------------
  //Test extract GLOBALIDS filter on cells
  vtkIdTypeArray *cellIds = NULL;

  sel->Clear();
  sel->GetProperties()->Set(
    vtkSelection::CONTENT_TYPE(), vtkSelection::GLOBALIDS);
  cellIds = vtkIdTypeArray::New();
  cellIds->SetNumberOfComponents(1);
  cellIds->SetNumberOfTuples(5);
  cellIds->SetTuple1(0,  9); //just before first cell -miss
  cellIds->SetTuple1(1,  10); //first cell
  cellIds->SetTuple1(2,  11); //second cells (distinguishes from reverse ids)
  cellIds->SetTuple1(3,  36); //last cell
  cellIds->SetTuple1(4,  37); //just beyond last cell -miss
  sel->SetSelectionList(cellIds);
  cellIds->Delete();

  ext->Update();
  extGrid = vtkUnstructuredGrid::SafeDownCast(ext->GetOutput());
  writer->SetInput(extGrid);
  writer->SetFileName("extCellGIds.vtk");
  if (DoWrite)
    {
    writer->Write();
    }
  showMe(extGrid, 0, 0, COLORBYCELL, ciaF);

  sel->GetProperties()->Set(vtkSelection::INVERSE(), 1);
  ext->Update();
  extGrid = vtkUnstructuredGrid::SafeDownCast(ext->GetOutput());
  writer->SetInput(extGrid);
  writer->SetFileName("extCellGIdsNOT.vtk");
  if (DoWrite)
    {
    writer->Write();
    }
  showMe(extGrid, 1, 0, COLORBYCELL, ciaF);

  //Test extract VALUES filter on cells
  sel->Clear();
  sel->GetProperties()->Set(
    vtkSelection::CONTENT_TYPE(), vtkSelection::VALUES);
  sel->GetProperties()->Set(
    vtkSelection::ARRAY_NAME(), "Reverse Cell Ids");
  cellIds = vtkIdTypeArray::New();
  cellIds->SetNumberOfComponents(1);
  cellIds->SetNumberOfTuples(5);
  cellIds->SetTuple1(0,  9); //just passed last -miss
  cellIds->SetTuple1(1,  10); //last
  cellIds->SetTuple1(2,  11); //next to last (distinguishes from forward ids)
  cellIds->SetTuple1(3,  36); //first
  cellIds->SetTuple1(4,  37); //just before first -miss
  sel->SetSelectionList(cellIds);
  cellIds->Delete();

  ext->Update();
  extGrid = vtkUnstructuredGrid::SafeDownCast(ext->GetOutput());
  writer->SetInput(extGrid);
  writer->SetFileName("extCellValues.vtk");
  if (DoWrite)
    {
    writer->Write();
    }
  showMe(extGrid, 0, 1, COLORBYCELL, ciaR);
  
  sel->GetProperties()->Set(vtkSelection::INVERSE(), 1);
  ext->Update();
  extGrid = vtkUnstructuredGrid::SafeDownCast(ext->GetOutput());
  writer->SetInput(extGrid);
  writer->SetFileName("extCellValuesNOT.vtk");
  if (DoWrite)
    {
    writer->Write();
    }
  showMe(extGrid, 1, 1, COLORBYCELL, ciaR);

  //Test extract INDICES filter on cells
  sel->Clear();
  sel->GetProperties()->Set(
    vtkSelection::CONTENT_TYPE(), vtkSelection::INDICES);
  cellIds = vtkIdTypeArray::New();
  cellIds->SetNumberOfComponents(1);
  cellIds->SetNumberOfTuples(5);
  cellIds->SetTuple1(0,  0); 
  cellIds->SetTuple1(1,  1);
  cellIds->SetTuple1(2,  2);
  cellIds->SetTuple1(3,  26); //last
  cellIds->SetTuple1(4,  27); //just outside -miss
  sel->SetSelectionList(cellIds);
  cellIds->Delete();

  ext->Update();
  extGrid = vtkUnstructuredGrid::SafeDownCast(ext->GetOutput());
  writer->SetInput(extGrid);
  writer->SetFileName("extCellIndices.vtk");
  if (DoWrite)
    {
    writer->Write();
    }
  showMe(extGrid, 0, 2, COLORBYCELL, cia);
  
  sel->GetProperties()->Set(vtkSelection::INVERSE(), 1);
  ext->Update();
  extGrid = vtkUnstructuredGrid::SafeDownCast(ext->GetOutput());
  writer->SetInput(extGrid);
  writer->SetFileName("extCellIndicesNOT.vtk");
  if (DoWrite)
    {
    writer->Write();
    }
  showMe(extGrid, 1, 2, COLORBYCELL, cia);

  //-------------------------------------------------------------------------
  //Test extract GLOBALIDS filter on points
  vtkIdTypeArray *pointIds = NULL;

  sel->Clear();
  sel->GetProperties()->Set(
    vtkSelection::CONTENT_TYPE(), vtkSelection::GLOBALIDS);
  sel->GetProperties()->Set(
    vtkSelection::FIELD_TYPE(), vtkSelection::POINT);  
  pointIds = vtkIdTypeArray::New();
  pointIds->SetNumberOfComponents(1);
  pointIds->SetNumberOfTuples(5);
  pointIds->SetTuple1(0,  9);  //jsut before first -miss
  pointIds->SetTuple1(1,  10); //first
  pointIds->SetTuple1(2,  11); //second
  pointIds->SetTuple1(3,  73); //last
  pointIds->SetTuple1(4,  74); //just passed last -miss
  sel->SetSelectionList(pointIds);
  pointIds->Delete();

  ext->Update();
  extGrid = vtkUnstructuredGrid::SafeDownCast(ext->GetOutput());
  writer->SetInput(extGrid);
  writer->SetFileName("extPointGIds.vtk");
  if (DoWrite)
    {
    writer->Write();
    }
  showMe(extGrid, 3, 0, COLORBYPOINT, piaF);
  
  sel->GetProperties()->Set(vtkSelection::INVERSE(), 1);
  ext->Update();
  extGrid = vtkUnstructuredGrid::SafeDownCast(ext->GetOutput());
  writer->SetInput(extGrid);
  writer->SetFileName("extPointGIdsNOT.vtk");
  if (DoWrite)
    {
    writer->Write();
    }
  showMe(extGrid, 4, 0, COLORBYPOINT, piaF);

  //Test extract VALUES filter on points
  sel->Clear();
  sel->GetProperties()->Set(
    vtkSelection::CONTENT_TYPE(), vtkSelection::VALUES);
  sel->GetProperties()->Set(
    vtkSelection::FIELD_TYPE(), vtkSelection::POINT);  
  sel->GetProperties()->Set(
    vtkSelection::ARRAY_NAME(), "Reverse Point Ids");
  pointIds = vtkIdTypeArray::New();
  pointIds->SetNumberOfComponents(1);
  pointIds->SetNumberOfTuples(5);
  pointIds->SetTuple1(0,  9);  //just beyond last -miss
  pointIds->SetTuple1(1,  10); //last
  pointIds->SetTuple1(2,  11); //next to last 
  pointIds->SetTuple1(3,  73); //first
  pointIds->SetTuple1(4,  74); //just before first -miss
  sel->SetSelectionList(pointIds);
  pointIds->Delete();

  ext->Update();
  extGrid = vtkUnstructuredGrid::SafeDownCast(ext->GetOutput());
  writer->SetInput(extGrid);
  writer->SetFileName("extPointValues.vtk");
  if (DoWrite)
    {
    writer->Write();
    }
  showMe(extGrid, 3, 1, COLORBYPOINT, piaR);
  
  sel->GetProperties()->Set(vtkSelection::INVERSE(), 1);
  ext->Update();
  extGrid = vtkUnstructuredGrid::SafeDownCast(ext->GetOutput());
  writer->SetInput(extGrid);
  writer->SetFileName("extPointValuesNOT.vtk");
  if (DoWrite)
    {
    writer->Write();
    }
  showMe(extGrid, 4, 1, COLORBYPOINT, piaR);

  //Test extract INDICES filter on points
  sel->Clear();
  sel->GetProperties()->Set(
    vtkSelection::CONTENT_TYPE(), vtkSelection::INDICES);
  sel->GetProperties()->Set(
    vtkSelection::FIELD_TYPE(), vtkSelection::POINT);  
  pointIds = vtkIdTypeArray::New();
  pointIds->SetNumberOfComponents(1);
  pointIds->SetNumberOfTuples(5);
  pointIds->SetTuple1(0,  0); //first
  pointIds->SetTuple1(1,  1); //second
  pointIds->SetTuple1(2,  2); //third
  pointIds->SetTuple1(3,  63);//last
  pointIds->SetTuple1(4,  64); //just beyond last -miss
  sel->SetSelectionList(pointIds);
  pointIds->Delete();

  ext->Update();
  extGrid = vtkUnstructuredGrid::SafeDownCast(ext->GetOutput());
  writer->SetInput(extGrid);
  writer->SetFileName("extPointIndices.vtk");
  if (DoWrite)
    {
    writer->Write();
    }
  showMe(extGrid, 3, 2, COLORBYPOINT, pia);
  
  sel->GetProperties()->Set(vtkSelection::INVERSE(), 1);
  ext->Update();
  extGrid = vtkUnstructuredGrid::SafeDownCast(ext->GetOutput());
  writer->SetInput(extGrid);
  writer->SetFileName("extPointIndicesNOT.vtk");
  if (DoWrite)
    {
    writer->Write();
    }
  showMe(extGrid, 4, 2, COLORBYPOINT, pia);

  //-------------------------------------------------------------------------
  //test the extract LOCATIONS filter on cells
  sel->Clear();
  sel->GetProperties()->Set(
    vtkSelection::CONTENT_TYPE(), vtkSelection::LOCATIONS);
  vtkDoubleArray *cellLocs = vtkDoubleArray::New();
  cellLocs->SetNumberOfComponents(3);
  cellLocs->SetNumberOfTuples(4);
  cellLocs->SetTuple3(0, 0.0, 0.99, 0.5); //on the edge of two cells, pick one
  //grr different data set types cell locators return different cells so I could not use 1.0 and had to make it 0.99 to make it consistent
  cellLocs->SetTuple3(1, 2.5, 1.5, 0.5); //inside a cell
  cellLocs->SetTuple3(2, 2.5, 2.1, 2.9); //inside a cell
  cellLocs->SetTuple3(3, 5.0, 5.0, 5.0); //outside of all cells
  sel->SetSelectionList(cellLocs);
  cellLocs->Delete();

  ext->Update();
  extGrid = vtkUnstructuredGrid::SafeDownCast(ext->GetOutput());
  writer->SetInput(extGrid);
  writer->SetFileName("extCellLocations.vtk");
  if (DoWrite)
    {
    writer->Write();
    }
  showMe(extGrid, 0, 3, COLORBYCELL, cia);

  sel->GetProperties()->Set(vtkSelection::INVERSE(), 1);
  ext->Update();
  extGrid = vtkUnstructuredGrid::SafeDownCast(ext->GetOutput());
  writer->SetInput(extGrid);
  writer->SetFileName("extCellLocationsNOT.vtk");
  if (DoWrite)
    {
    writer->Write();
    }
  showMe(extGrid, 1, 3, COLORBYCELL, cia);

  //-------------------------------------------------------------------------
  //test the extract LOCATIONS filter on points

  sel->Clear();
  sel->GetProperties()->Set(
    vtkSelection::CONTENT_TYPE(), vtkSelection::LOCATIONS);
  sel->GetProperties()->Set(vtkSelection::FIELD_TYPE(), vtkSelection::POINT);
  sel->GetProperties()->Set(vtkSelection::EPSILON(), 0.3);
  vtkDoubleArray *pointLocs = vtkDoubleArray::New();
  pointLocs->SetNumberOfComponents(3);
  pointLocs->SetNumberOfTuples(3);
  pointLocs->SetTuple3(0, 0.0, 0.0, 0.29); //just close enough to the first point
  pointLocs->SetTuple3(1, 1.0, 0.0, 0.31); //just a bit outside
  pointLocs->SetTuple3(2, 1.0, 1.0, 3.1); //outside the dataset, but close enough
  sel->SetSelectionList(pointLocs);
  pointLocs->Delete();

  ext->Update();
  extGrid = vtkUnstructuredGrid::SafeDownCast(ext->GetOutput());
  writer->SetInput(extGrid);
  writer->SetFileName("extPointLocations.vtk");
  if (DoWrite)
    {
    writer->Write();
    }
  showMe(extGrid, 3, 3, COLORBYPOINT, pia);

  sel->GetProperties()->Set(vtkSelection::INVERSE(), 1);
  ext->Update();
  extGrid = vtkUnstructuredGrid::SafeDownCast(ext->GetOutput());
  writer->SetInput(extGrid);
  writer->SetFileName("extPointLocationsNOT.vtk");
  if (DoWrite)
    {
    writer->Write();
    }
  showMe(extGrid, 4, 3, COLORBYPOINT, pia);

  sel->GetProperties()->Set(vtkSelection::INVERSE(), 0);
  sel->GetProperties()->Set(vtkSelection::CONTAINING_CELLS(), 1);
  ext->Update();
  extGrid = vtkUnstructuredGrid::SafeDownCast(ext->GetOutput());
  writer->SetInput(extGrid);
  writer->SetFileName("extPointLocationsWithCells.vtk");
  if (DoWrite)
    {
    writer->Write();
    }
  showMe(extGrid, 5, 3, COLORBYPOINT, pia);

  //-------------------------------------------------------------------------
  //test the extract THRESHOLD filter on cell data
  sel->Clear();
  sel->GetProperties()->Set(
    vtkSelection::CONTENT_TYPE(), vtkSelection::THRESHOLDS);
  vtkDoubleArray *cellThresh = vtkDoubleArray::New();
  cellThresh->SetNumberOfComponents(1);
  cellThresh->SetNumberOfTuples(2);
  cellThresh->SetTuple1(0, 1.9); //the nine rightmost(+X) cells are in here
  cellThresh->SetTuple1(1, 3.1);
  sel->SetSelectionList(cellThresh);
  cellThresh->Delete();

  ext->Update();
  extGrid = vtkUnstructuredGrid::SafeDownCast(ext->GetOutput());
  writer->SetInput(extGrid);
  writer->SetFileName("extCellThresholds.vtk");
  if (DoWrite)
    {
    writer->Write();
    }
  showMe(extGrid, 0, 4, COLORBYCELL, cxa);
  
  sel->GetProperties()->Set(vtkSelection::INVERSE(), 1);
  ext->Update();
  extGrid = vtkUnstructuredGrid::SafeDownCast(ext->GetOutput());
  writer->SetInput(extGrid);
  writer->SetFileName("extCellThresholdsNOT.vtk");
  if (DoWrite)
    {
    writer->Write();
    }
  showMe(extGrid, 1, 4, COLORBYCELL, cxa);

  //-------------------------------------------------------------------------
  //test the extract THRESHOLD filter on point data
  sel->Clear();
  sel->GetProperties()->Set(
    vtkSelection::CONTENT_TYPE(), vtkSelection::THRESHOLDS);
  sel->GetProperties()->Set(vtkSelection::FIELD_TYPE(), vtkSelection::POINT);
  vtkDoubleArray *pointThresh = vtkDoubleArray::New();
  pointThresh->SetNumberOfComponents(1);
  pointThresh->SetNumberOfTuples(2);
  pointThresh->SetTuple1(0, 0.9);  //the 18 leftmost cells have points in here
  pointThresh->SetTuple1(1, 1.1);
  sel->SetSelectionList(pointThresh);
  pointThresh->Delete();

  ext->Update();
  extGrid = vtkUnstructuredGrid::SafeDownCast(ext->GetOutput());
  writer->SetInput(extGrid);
  writer->SetFileName("extPointThresholds.vtk");
  if (DoWrite)
    {
    writer->Write();
    }
  showMe(extGrid, 3, 4, COLORBYPOINT, pxa);

  sel->GetProperties()->Set(vtkSelection::INVERSE(), 1);
  ext->Update();
  extGrid = vtkUnstructuredGrid::SafeDownCast(ext->GetOutput());
  writer->SetInput(extGrid);
  writer->SetFileName("extPointThresholdsNOT.vtk");
  if (DoWrite)
    {
    writer->Write();
    }
  showMe(extGrid, 4, 4, COLORBYPOINT, pxa);

  //-------------------------------------------------------------------------
  //test the extract FRUSTUM filter
  sel->Clear();
  sel->GetProperties()->Set(
    vtkSelection::CONTENT_TYPE(), vtkSelection::FRUSTUM);
  vtkDoubleArray *frustcorners = vtkDoubleArray::New();
  frustcorners->SetNumberOfComponents(4);
  frustcorners->SetNumberOfTuples(8);
  //a small frustum within the 3 lower left (-X,-Y) cells
  frustcorners->SetTuple4(0,  0.1, 0.1,  3.1, 0.0);
  frustcorners->SetTuple4(1,  0.1, 0.1, 0.1, 0.0);
  frustcorners->SetTuple4(2,  0.1,  0.9,  3.1, 0.0);
  frustcorners->SetTuple4(3,  0.1,  0.9, 0.1, 0.0);
  frustcorners->SetTuple4(4,  0.9, 0.1,  3.1, 0.0);
  frustcorners->SetTuple4(5,  0.9, 0.1, 0.1, 0.0);
  frustcorners->SetTuple4(6,  0.9,  0.9,  3.1, 0.0);
  frustcorners->SetTuple4(7,  0.9,  0.9, 0.1, 0.0);
  sel->SetSelectionList(frustcorners);
  frustcorners->Delete();

  ext->Update();
  extGrid = vtkUnstructuredGrid::SafeDownCast(ext->GetOutput());
  writer->SetInput(extGrid);
  writer->SetFileName("extFrustum.vtk");
  if (DoWrite)
    {
    writer->Write();
    }
  showMe(extGrid, 0, 5, COLORBYCELL, cia);
  
  sel->GetProperties()->Set(vtkSelection::INVERSE(), 1);
  ext->Update();
  extGrid = vtkUnstructuredGrid::SafeDownCast(ext->GetOutput());
  writer->SetInput(extGrid);
  writer->SetFileName("extFrustumNOT.vtk");
  if (DoWrite)
    {
    writer->Write();
    }
  showMe(extGrid, 1, 5, COLORBYCELL, cia);

  sel->GetProperties()->Set(vtkSelection::INVERSE(), 0);
  sel->GetProperties()->Set(vtkSelection::PRESERVE_TOPOLOGY(), 1);
  ext->Update();
  vtkImageData *extIData = vtkImageData::SafeDownCast(ext->GetOutput());
  xwriter = vtkXMLDataSetWriter::New(); 
  xwriter->SetInput(extIData);
  xwriter->SetFileName("extFrustumPT.vti");
  if (DoWrite)
    {
    xwriter->Write();
    }
  xwriter->Delete();
  vtkDataArray *da = extIData->GetCellData()->GetArray("vtkInsidedness");
  showMe(extIData, 2, 5, COLORBYCELL, da);

  //-------------------------------------------------------------------------
  vtkCamera *cam = renderer->GetActiveCamera();
  cam->SetPosition(-6, -2, 45);
  cam->SetFocalPoint(10, 11, 2);
  cam->SetViewUp(0,1,0);
  renderer->SetActiveCamera(cam);
  
  int retVal = vtkRegressionTestImage( renwin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    rwi->Start();
    }

  //cleanup
  sel->Delete();
  ext->Delete();

  pia->Delete();
  piaF->Delete();
  piaR->Delete();
  pxa->Delete();
  pya->Delete();
  pza->Delete();
  cia->Delete();
  ciaF->Delete();
  ciaR->Delete();
  cxa->Delete();
  cya->Delete();
  cza->Delete();

  writer->Delete();
  renderer->Delete();
  sampleData->Delete();
  renwin->Delete();
  rwi->Delete();

  return !retVal;
}
