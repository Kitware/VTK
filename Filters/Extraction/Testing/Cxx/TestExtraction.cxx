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
#include <vtkSelectionNode.h>
#include <vtkExtractSelection.h>

#define XCELLS 3
#define YCELLS 3
#define ZCELLS 3

static vtkRenderer *renderer = NULL;
static vtkImageData *sampleData = NULL;
static int DrawSampleData = 0;

enum {COLORBYCELL, COLORBYPOINT};

void showMe(vtkDataSet *result, int X, int Y, int CellOrPoint, vtkDataArray *array)
{
  vtkDataSet *copy = result->NewInstance();
  copy->DeepCopy(result);
  vtkDataSetMapper *mapper = vtkDataSetMapper::New();
  mapper->SetInputData(copy);
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
    mapper2->SetInputData(sampleData);
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
  renwin->SetMultiSamples(0);
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
  sampleData->AllocateScalars(VTK_DOUBLE, 1);

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
  xwriter->SetInputData(sampleData);
  xwriter->SetFileName("sampleData.vti");
  if (DoWrite)
  {
    xwriter->Write();
  }

  //-------------------------------------------------------------------------
  //Setup the components of the pipeline
  vtkSelection *selection = vtkSelection::New();
  vtkSelectionNode *sel = vtkSelectionNode::New();
  selection->AddNode(sel);
  vtkExtractSelection *ext = vtkExtractSelection::New();
  ext->SetInputData(0, sampleData);
  ext->SetInputData(1, selection);
  ext->PreserveTopologyOff();
  vtkUnstructuredGridWriter *writer = vtkUnstructuredGridWriter::New();

  vtkUnstructuredGrid *extGrid;
  vtkImageData *extIData;
  vtkDataArray *insideArray;

  //-------------------------------------------------------------------------
  //Test extract GLOBALIDS filter on cells
  vtkIdTypeArray *cellIds = NULL;

  sel->Initialize();
  sel->GetProperties()->Set(
    vtkSelectionNode::CONTENT_TYPE(), vtkSelectionNode::GLOBALIDS);
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
  if (DoWrite)
  {
    writer->SetInputConnection(ext->GetOutputPort());
    writer->SetFileName("ext_C_GID.vtk");
    writer->Write();
  }
  showMe(extGrid, 0, 0, COLORBYCELL, ciaF);

  sel->GetProperties()->Set(vtkSelectionNode::INVERSE(), 1);
  ext->Update();
  extGrid = vtkUnstructuredGrid::SafeDownCast(ext->GetOutput());
  if (DoWrite)
  {
    writer->SetInputConnection(ext->GetOutputPort());
    writer->SetFileName("ext_C_GID_I.vtk");
    writer->Write();
  }
  showMe(extGrid, 1, 0, COLORBYCELL, ciaF);

  sel->GetProperties()->Set(vtkSelectionNode::INVERSE(), 0);
  ext->PreserveTopologyOn();
  ext->Update();
  extIData = vtkImageData::SafeDownCast(ext->GetOutput());
  if (DoWrite)
  {
    xwriter->SetInputConnection(ext->GetOutputPort());
    xwriter->SetFileName("ext_C_GID_PT.vti");
    xwriter->Write();
  }
  insideArray = extIData->GetCellData()->GetArray("vtkInsidedness");
  showMe(extIData, 2, 0, COLORBYCELL, insideArray);

  //-------------------------------------------------------------------------
  //Test extract GLOBALIDS filter on points
  vtkIdTypeArray *pointIds = NULL;

  sel->Initialize();
  ext->PreserveTopologyOff();
  sel->GetProperties()->Set(
    vtkSelectionNode::CONTENT_TYPE(), vtkSelectionNode::GLOBALIDS);
  sel->GetProperties()->Set(
    vtkSelectionNode::FIELD_TYPE(), vtkSelectionNode::POINT);
  pointIds = vtkIdTypeArray::New();
  pointIds->SetNumberOfComponents(1);
  pointIds->SetNumberOfTuples(5);
  pointIds->SetTuple1(0,  9);  //just before first -miss
  pointIds->SetTuple1(1,  10); //first
  pointIds->SetTuple1(2,  11); //second
  pointIds->SetTuple1(3,  73); //last
  pointIds->SetTuple1(4,  74); //just passed last -miss
  sel->SetSelectionList(pointIds);
  pointIds->Delete();

  ext->Update();
  extGrid = vtkUnstructuredGrid::SafeDownCast(ext->GetOutput());
  if (DoWrite)
  {
    writer->SetInputConnection(ext->GetOutputPort());
    writer->SetFileName("ext_P_GID.vtk");
    writer->Write();
  }
  showMe(extGrid, 3, 0, COLORBYPOINT, piaF);

  sel->GetProperties()->Set(vtkSelectionNode::INVERSE(), 1);
  ext->Update();
  extGrid = vtkUnstructuredGrid::SafeDownCast(ext->GetOutput());
  if (DoWrite)
  {
    writer->SetInputConnection(ext->GetOutputPort());
    writer->SetFileName("ext_P_GID_I.vtk");
    writer->Write();
  }
  showMe(extGrid, 4, 0, COLORBYPOINT, piaF);

  sel->GetProperties()->Set(vtkSelectionNode::INVERSE(), 0);
  sel->GetProperties()->Set(vtkSelectionNode::CONTAINING_CELLS(), 1);
  ext->Update();
  extGrid = vtkUnstructuredGrid::SafeDownCast(ext->GetOutput());
  if (DoWrite)
  {
    writer->SetInputConnection(ext->GetOutputPort());
    writer->SetFileName("ext_P_GID_WC.vtk");
    writer->Write();
  }
  showMe(extGrid, 5, 0, COLORBYPOINT, piaF);

  sel->GetProperties()->Set(vtkSelectionNode::CONTAINING_CELLS(), 0);
  ext->PreserveTopologyOn();
  ext->Update();
  extIData = vtkImageData::SafeDownCast(ext->GetOutput());
  if (DoWrite)
  {
    xwriter->SetInputConnection(ext->GetOutputPort());
    xwriter->SetFileName("ext_P_GID_PT.vti");
    xwriter->Write();
  }
  insideArray = extIData->GetCellData()->GetArray("vtkInsidedness");
  if (insideArray)
  {
    cerr << "ERROR: Extract point global id without containing cells made cell inside array." << endl;
  }
  insideArray = extIData->GetPointData()->GetArray("vtkInsidedness");
  showMe(extIData, 6, 0, COLORBYPOINT, insideArray);

  sel->GetProperties()->Set(vtkSelectionNode::CONTAINING_CELLS(), 1);
  ext->PreserveTopologyOn();
  ext->Update();
  extIData = vtkImageData::SafeDownCast(ext->GetOutput());
  if (DoWrite)
  {
    xwriter->SetInputConnection(ext->GetOutputPort());
    xwriter->SetFileName("ext_P_GID_WC_PT.vtk");
    xwriter->Write();
  }
  insideArray = extIData->GetCellData()->GetArray("vtkInsidedness");
  showMe(extIData, 7, 0, COLORBYCELL, insideArray);

  //--------------------------------------------------------------------------
  //Test extract INDICES filter on cells
  sel->Initialize();
  ext->PreserveTopologyOff();
  sel->GetProperties()->Set(
    vtkSelectionNode::CONTENT_TYPE(), vtkSelectionNode::INDICES);
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
  if (DoWrite)
  {
    writer->SetInputConnection(ext->GetOutputPort());
    writer->SetFileName("ext_C_Ind.vtk");
    writer->Write();
  }
  showMe(extGrid, 0, 1, COLORBYCELL, cia);

  sel->GetProperties()->Set(vtkSelectionNode::INVERSE(), 1);
  ext->Update();
  extGrid = vtkUnstructuredGrid::SafeDownCast(ext->GetOutput());
  if (DoWrite)
  {
    writer->SetInputConnection(ext->GetOutputPort());
    writer->SetFileName("ext_C_Ind_I.vtk");
    writer->Write();
  }
  showMe(extGrid, 1, 1, COLORBYCELL, cia);

  sel->GetProperties()->Set(vtkSelectionNode::INVERSE(), 0);
  ext->PreserveTopologyOn();
  ext->Update();
  extIData = vtkImageData::SafeDownCast(ext->GetOutput());
  if (DoWrite)
  {
    xwriter->SetInputConnection(ext->GetOutputPort());
    xwriter->SetFileName("ext_C_Ind_PT.vti");
    xwriter->Write();
  }
  insideArray = extIData->GetCellData()->GetArray("vtkInsidedness");
  showMe(extIData, 2, 1, COLORBYCELL, insideArray);

  //--------------------------------------------------------------------------
  //Test extract INDICES filter on points
  sel->Initialize();
  ext->PreserveTopologyOff();
  sel->GetProperties()->Set(
    vtkSelectionNode::CONTENT_TYPE(), vtkSelectionNode::INDICES);
  sel->GetProperties()->Set(
    vtkSelectionNode::FIELD_TYPE(), vtkSelectionNode::POINT);
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
  if (DoWrite)
  {
    writer->SetInputConnection(ext->GetOutputPort());
    writer->SetFileName("ext_P_Ind_I.vtk");
    writer->Write();
  }
  showMe(extGrid, 3, 1, COLORBYPOINT, pia);

  sel->GetProperties()->Set(vtkSelectionNode::INVERSE(), 1);
  ext->Update();
  extGrid = vtkUnstructuredGrid::SafeDownCast(ext->GetOutput());
  if (DoWrite)
  {
    writer->SetInputConnection(ext->GetOutputPort());
    writer->SetFileName("ext_P_Ind_I.vtk");
    writer->Write();
  }
  showMe(extGrid, 4, 1, COLORBYPOINT, pia);

  sel->GetProperties()->Set(vtkSelectionNode::INVERSE(), 0);
  sel->GetProperties()->Set(vtkSelectionNode::CONTAINING_CELLS(), 1);
  ext->Update();
  extGrid = vtkUnstructuredGrid::SafeDownCast(ext->GetOutput());
  if (DoWrite)
  {
    writer->SetInputConnection(ext->GetOutputPort());
    writer->SetFileName("ext_P_Ind_WC.vtk");
    writer->Write();
  }
  showMe(extGrid, 5, 1, COLORBYPOINT, pia);

  sel->GetProperties()->Set(vtkSelectionNode::INVERSE(), 0);
  sel->GetProperties()->Set(vtkSelectionNode::CONTAINING_CELLS(), 0);
  ext->PreserveTopologyOn();
  ext->Update();
  extIData = vtkImageData::SafeDownCast(ext->GetOutput());
  if (DoWrite)
  {
    xwriter->SetInputConnection(ext->GetOutputPort());
    xwriter->SetFileName("ext_P_Ind_PT.vti");
    xwriter->Write();
  }
  insideArray = extIData->GetCellData()->GetArray("vtkInsidedness");
  if (insideArray)
  {
    cerr << "ERROR: Extract point indices without containing cells made cell inside array." << endl;
  }
  insideArray = extIData->GetPointData()->GetArray("vtkInsidedness");
  showMe(extIData, 6, 1, COLORBYPOINT, insideArray);

  sel->GetProperties()->Set(vtkSelectionNode::CONTAINING_CELLS(), 1);
  ext->Update();
  extIData = vtkImageData::SafeDownCast(ext->GetOutput());
  if (DoWrite)
  {
    xwriter->SetInputConnection(ext->GetOutputPort());
    xwriter->SetFileName("ext_P_Ind_PT_WC.vti");
    xwriter->Write();
  }
  insideArray = extIData->GetCellData()->GetArray("vtkInsidedness");
  showMe(extIData, 7, 1, COLORBYCELL, insideArray);

  //--------------------------------------------------------------------------
  //Test extract VALUES filter on cells
  sel->Initialize();
  ext->PreserveTopologyOff();
  sel->GetProperties()->Set(
    vtkSelectionNode::CONTENT_TYPE(), vtkSelectionNode::VALUES);
  cellIds = vtkIdTypeArray::New();
  cellIds->SetName("Reverse Cell Ids");
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
  if (DoWrite)
  {
    writer->SetInputConnection(ext->GetOutputPort());
    writer->SetFileName("ext_C_Val.vtk");
    writer->Write();
  }
  showMe(extGrid, 0, 2, COLORBYCELL, ciaR);

  sel->GetProperties()->Set(vtkSelectionNode::INVERSE(), 1);
  ext->Update();
  extGrid = vtkUnstructuredGrid::SafeDownCast(ext->GetOutput());
  if (DoWrite)
  {
    writer->SetInputConnection(ext->GetOutputPort());
    writer->SetFileName("ext_C_Val_I.vtk");
    writer->Write();
  }
  showMe(extGrid, 1, 2, COLORBYCELL, ciaR);

  sel->GetProperties()->Set(vtkSelectionNode::INVERSE(), 0);
  ext->PreserveTopologyOn();
  ext->Update();
  extIData = vtkImageData::SafeDownCast(ext->GetOutput());
  if (DoWrite)
  {
    xwriter->SetInputConnection(ext->GetOutputPort());
    xwriter->SetFileName("ext_C_Val_PT.vti");
    xwriter->Write();
  }
  insideArray = extIData->GetCellData()->GetArray("vtkInsidedness");
  showMe(extIData, 2, 2, COLORBYCELL, insideArray);

  //--------------------------------------------------------------------------
  //Test extract VALUES filter on points
  sel->Initialize();
  ext->PreserveTopologyOff();
  sel->GetProperties()->Set(
    vtkSelectionNode::CONTENT_TYPE(), vtkSelectionNode::VALUES);
  sel->GetProperties()->Set(
    vtkSelectionNode::FIELD_TYPE(), vtkSelectionNode::POINT);
  pointIds = vtkIdTypeArray::New();
  pointIds->SetName("Reverse Point Ids");
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
  if (DoWrite)
  {
    writer->SetInputConnection(ext->GetOutputPort());
    writer->SetFileName("ext_P_Val.vtk");
    writer->Write();
  }
  showMe(extGrid, 3, 2, COLORBYPOINT, piaR);

  sel->GetProperties()->Set(vtkSelectionNode::INVERSE(), 1);
  ext->Update();
  extGrid = vtkUnstructuredGrid::SafeDownCast(ext->GetOutput());
  if (DoWrite)
  {
    writer->SetInputConnection(ext->GetOutputPort());
    writer->SetFileName("ext_P_Val_I.vtk");
    writer->Write();
  }
  showMe(extGrid, 4, 2, COLORBYPOINT, piaR);

  sel->GetProperties()->Set(vtkSelectionNode::INVERSE(), 0);
  sel->GetProperties()->Set(vtkSelectionNode::CONTAINING_CELLS(), 1);
  ext->Update();
  extGrid = vtkUnstructuredGrid::SafeDownCast(ext->GetOutput());
  if (DoWrite)
  {
    writer->SetInputConnection(ext->GetOutputPort());
    writer->SetFileName("ext_P_Val_WC.vtk");
    writer->Write();
  }
  showMe(extGrid, 5, 2, COLORBYPOINT, piaR);

  sel->GetProperties()->Set(vtkSelectionNode::INVERSE(), 0);
  sel->GetProperties()->Set(vtkSelectionNode::CONTAINING_CELLS(), 0);
  ext->PreserveTopologyOn();
  ext->Update();
  extIData = vtkImageData::SafeDownCast(ext->GetOutput());
  if (DoWrite)
  {
    xwriter->SetInputConnection(ext->GetOutputPort());
    xwriter->SetFileName("ext_P_Val_PT.vtk");
    xwriter->Write();
  }
  insideArray = extIData->GetCellData()->GetArray("vtkInsidedness");
  if (insideArray)
  {
    cerr << "ERROR: Extract point values without containing cells made cell inside array." << endl;
  }
  insideArray = extIData->GetPointData()->GetArray("vtkInsidedness");
  showMe(extIData, 6, 2, COLORBYPOINT, insideArray);

  sel->GetProperties()->Set(vtkSelectionNode::CONTAINING_CELLS(), 1);
  ext->Update();
  extIData = vtkImageData::SafeDownCast(ext->GetOutput());
  if (DoWrite)
  {
    xwriter->SetInputConnection(ext->GetOutputPort());
    xwriter->SetFileName("ext_P_Val_PT_WC.vtk");
    xwriter->Write();
  }
  insideArray = extIData->GetCellData()->GetArray("vtkInsidedness");
  showMe(extIData, 7, 2, COLORBYCELL, insideArray);

  //-------------------------------------------------------------------------
  //test the extract THRESHOLD filter on cell data
  sel->Initialize();
  ext->PreserveTopologyOff();
  sel->GetProperties()->Set(
    vtkSelectionNode::CONTENT_TYPE(), vtkSelectionNode::THRESHOLDS);
  vtkDoubleArray *cellThresh = vtkDoubleArray::New();
  cellThresh->SetNumberOfComponents(1);
  cellThresh->SetNumberOfTuples(2);
  cellThresh->SetTuple1(0, 1.9); //the nine rightmost(+X) cells are in here
  cellThresh->SetTuple1(1, 3.1);
  sel->SetSelectionList(cellThresh);
  cellThresh->Delete();

  ext->Update();
  extGrid = vtkUnstructuredGrid::SafeDownCast(ext->GetOutput());
  if (DoWrite)
  {
    writer->SetInputConnection(ext->GetOutputPort());
    writer->SetFileName("ext_C_Thr.vtk");
    writer->Write();
  }
  showMe(extGrid, 0, 3, COLORBYCELL, cxa);

  sel->GetProperties()->Set(vtkSelectionNode::INVERSE(), 1);
  ext->Update();
  extGrid = vtkUnstructuredGrid::SafeDownCast(ext->GetOutput());
  if (DoWrite)
  {
    writer->SetInputConnection(ext->GetOutputPort());
    writer->SetFileName("ext_C_Thr_I.vtk");
    writer->Write();
  }
  showMe(extGrid, 1, 3, COLORBYCELL, cxa);

  sel->GetProperties()->Set(vtkSelectionNode::INVERSE(), 0);
  ext->PreserveTopologyOn();
  ext->Update();
  extIData = vtkImageData::SafeDownCast(ext->GetOutput());
  if (DoWrite)
  {
    xwriter->SetInputConnection(ext->GetOutputPort());
    xwriter->SetFileName("ext_C_Thr_PT.vtk");
    xwriter->Write();
  }
  insideArray = extIData->GetCellData()->GetArray("vtkInsidedness");
  showMe(extIData, 2, 3, COLORBYCELL, insideArray);

  //-------------------------------------------------------------------------
  //test the extract THRESHOLD filter on point data
  sel->Initialize();
  ext->PreserveTopologyOff();
  sel->GetProperties()->Set(
    vtkSelectionNode::CONTENT_TYPE(), vtkSelectionNode::THRESHOLDS);
  sel->GetProperties()->Set(vtkSelectionNode::FIELD_TYPE(), vtkSelectionNode::POINT);
  vtkDoubleArray *pointThresh = vtkDoubleArray::New();
  pointThresh->SetNumberOfComponents(1);
  pointThresh->SetNumberOfTuples(2);
  pointThresh->SetTuple1(0, 0.9);  //the 18 leftmost cells have points in here
  pointThresh->SetTuple1(1, 1.1);
  sel->SetSelectionList(pointThresh);
  pointThresh->Delete();

  ext->Update();
  extGrid = vtkUnstructuredGrid::SafeDownCast(ext->GetOutput());
  if (DoWrite)
  {
    writer->SetInputConnection(ext->GetOutputPort());
    writer->SetFileName("ext_P_Thr.vtk");
    writer->Write();
  }
  showMe(extGrid, 3, 3, COLORBYPOINT, pxa);

  sel->GetProperties()->Set(vtkSelectionNode::INVERSE(), 1);
  ext->Update();
  extGrid = vtkUnstructuredGrid::SafeDownCast(ext->GetOutput());
  if (DoWrite)
  {
    writer->SetInputConnection(ext->GetOutputPort());
    writer->SetFileName("ext_P_Thr_I.vtk");
    writer->Write();
  }
  showMe(extGrid, 4, 3, COLORBYPOINT, pxa);

  sel->GetProperties()->Set(vtkSelectionNode::INVERSE(), 0);
  sel->GetProperties()->Set(vtkSelectionNode::CONTAINING_CELLS(), 1);
  ext->Update();
  extGrid = vtkUnstructuredGrid::SafeDownCast(ext->GetOutput());
  if (DoWrite)
  {
    writer->SetInputConnection(ext->GetOutputPort());
    writer->SetFileName("ext_P_Thr_WC.vtk");
    writer->Write();
  }
  showMe(extGrid, 5, 3, COLORBYPOINT, pxa);

  sel->GetProperties()->Set(vtkSelectionNode::CONTAINING_CELLS(), 0);
  ext->PreserveTopologyOn();
  ext->Update();
  extIData = vtkImageData::SafeDownCast(ext->GetOutput());
  if (DoWrite)
  {
    xwriter->SetInputConnection(ext->GetOutputPort());
    xwriter->SetFileName("ext_P_Thr_PT.vtk");
    xwriter->Write();
  }
  insideArray = extIData->GetCellData()->GetArray("vtkInsidedness");
  if (insideArray)
  {
    cerr << "ERROR: Extract point thresh without containing cells made cell inside array." << endl;
  }
  insideArray = extIData->GetPointData()->GetArray("vtkInsidedness");
  showMe(extIData, 6, 3, COLORBYPOINT, insideArray);

  sel->GetProperties()->Set(vtkSelectionNode::CONTAINING_CELLS(), 1);
  ext->Update();
  extIData = vtkImageData::SafeDownCast(ext->GetOutput());
  if (DoWrite)
  {
    xwriter->SetInputConnection(ext->GetOutputPort());
    xwriter->SetFileName("ext_P_Thr_PT_WC.vtk");
    xwriter->Write();
  }
  insideArray = extIData->GetCellData()->GetArray("vtkInsidedness");
  showMe(extIData, 7, 3, COLORBYCELL, insideArray);

  //-------------------------------------------------------------------------
  //test the extract LOCATIONS filter on cells
  sel->Initialize();
  ext->PreserveTopologyOff();
  sel->GetProperties()->Set(
    vtkSelectionNode::CONTENT_TYPE(), vtkSelectionNode::LOCATIONS);
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
  if (DoWrite)
  {
    writer->SetInputConnection(ext->GetOutputPort());
    writer->SetFileName("ext_C_Loc.vtk");
    writer->Write();
  }
  showMe(extGrid, 0, 4, COLORBYCELL, cia);

  sel->GetProperties()->Set(vtkSelectionNode::INVERSE(), 1);
  ext->Update();
  extGrid = vtkUnstructuredGrid::SafeDownCast(ext->GetOutput());
  if (DoWrite)
  {
    writer->SetInputConnection(ext->GetOutputPort());
    writer->SetFileName("ext_C_Loc_I.vtk");
    writer->Write();
  }
  showMe(extGrid, 1, 4, COLORBYCELL, cia);

  sel->GetProperties()->Set(vtkSelectionNode::INVERSE(), 0);
  ext->PreserveTopologyOn();
  ext->Update();
  extIData = vtkImageData::SafeDownCast(ext->GetOutput());
  if (DoWrite)
  {
    xwriter->SetInputConnection(ext->GetOutputPort());
    xwriter->SetFileName("ext_C_Loc_PT.vti");
    xwriter->Write();
  }
  insideArray = extIData->GetCellData()->GetArray("vtkInsidedness");
  showMe(extIData, 2, 4, COLORBYCELL, insideArray);

  //-------------------------------------------------------------------------
  //test the extract LOCATIONS filter on points

  sel->Initialize();
  ext->PreserveTopologyOff();
  sel->GetProperties()->Set(
    vtkSelectionNode::CONTENT_TYPE(), vtkSelectionNode::LOCATIONS);
  sel->GetProperties()->Set(vtkSelectionNode::FIELD_TYPE(), vtkSelectionNode::POINT);
  sel->GetProperties()->Set(vtkSelectionNode::EPSILON(), 0.3);
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
  if (DoWrite)
  {
    writer->SetInputConnection(ext->GetOutputPort());
    writer->SetFileName("ext_P_Loc.vtk");
    writer->Write();
  }
  showMe(extGrid, 3, 4, COLORBYPOINT, pia);

  sel->GetProperties()->Set(vtkSelectionNode::INVERSE(), 1);
  ext->Update();
  extGrid = vtkUnstructuredGrid::SafeDownCast(ext->GetOutput());
  if (DoWrite)
  {
    writer->SetInputConnection(ext->GetOutputPort());
    writer->SetFileName("ext_P_Loc_I.vtk");
    writer->Write();
  }
  showMe(extGrid, 4, 4, COLORBYPOINT, pia);

  sel->GetProperties()->Set(vtkSelectionNode::INVERSE(), 0);
  sel->GetProperties()->Set(vtkSelectionNode::CONTAINING_CELLS(), 1);
  ext->Update();
  extGrid = vtkUnstructuredGrid::SafeDownCast(ext->GetOutput());
  if (DoWrite)
  {
    writer->SetInputConnection(ext->GetOutputPort());
    writer->SetFileName("ext_P_Loc_WC.vtk");
    writer->Write();
  }
  showMe(extGrid, 5, 4, COLORBYPOINT, pia);

  sel->GetProperties()->Set(vtkSelectionNode::INVERSE(), 0);
  sel->GetProperties()->Set(vtkSelectionNode::CONTAINING_CELLS(), 0);
  ext->PreserveTopologyOn();
  ext->Update();
  extIData = vtkImageData::SafeDownCast(ext->GetOutput());
  if (DoWrite)
  {
    xwriter->SetInputConnection(ext->GetOutputPort());
    xwriter->SetFileName("ext_P_Loc_PT.vti");
    xwriter->Write();
  }
  insideArray = extIData->GetPointData()->GetArray("vtkInsidedness");
  showMe(extIData, 6, 4, COLORBYPOINT, insideArray);

  sel->GetProperties()->Set(vtkSelectionNode::CONTAINING_CELLS(), 1);
  ext->Update();
  extIData = vtkImageData::SafeDownCast(ext->GetOutput());
  if (DoWrite)
  {
    xwriter->SetInputConnection(ext->GetOutputPort());
    xwriter->SetFileName("ext_P_Loc_PT_WC.vti");
    xwriter->Write();
  }
  insideArray = extIData->GetCellData()->GetArray("vtkInsidedness");
  showMe(extIData, 7, 4, COLORBYCELL, insideArray);

  //-------------------------------------------------------------------------
  //test the extract FRUSTUM filter
  sel->Initialize();
  ext->PreserveTopologyOff();
  sel->GetProperties()->Set(
    vtkSelectionNode::CONTENT_TYPE(), vtkSelectionNode::FRUSTUM);
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
  if (DoWrite)
  {
    writer->SetInputConnection(ext->GetOutputPort());
    writer->SetFileName("ext_Fru.vtk");
    writer->Write();
  }
  showMe(extGrid, 0, 5, COLORBYCELL, cia);

  sel->GetProperties()->Set(vtkSelectionNode::INVERSE(), 1);
  ext->Update();
  extGrid = vtkUnstructuredGrid::SafeDownCast(ext->GetOutput());
  if (DoWrite)
  {
    writer->SetInputConnection(ext->GetOutputPort());
    writer->SetFileName("ext_Fru_I.vtk");
    writer->Write();
  }
  showMe(extGrid, 1, 5, COLORBYCELL, cia);

  sel->GetProperties()->Set(vtkSelectionNode::INVERSE(), 0);
  ext->PreserveTopologyOn();
  ext->Update();
  extIData = vtkImageData::SafeDownCast(ext->GetOutput());
  if (DoWrite)
  {
    xwriter->SetInputConnection(ext->GetOutputPort());
    xwriter->SetFileName("ext_Fru_PT.vti");
    xwriter->Write();
  }
  insideArray = extIData->GetCellData()->GetArray("vtkInsidedness");
  showMe(extIData, 2, 5, COLORBYCELL, insideArray);

  //-------------------------------------------------------------------------
  //test the extract FRUSTUM filter on points

  sel->Initialize();
  ext->PreserveTopologyOff();
  sel->GetProperties()->Set(
    vtkSelectionNode::CONTENT_TYPE(), vtkSelectionNode::FRUSTUM);
  sel->GetProperties()->Set(vtkSelectionNode::FIELD_TYPE(), vtkSelectionNode::POINT);
  frustcorners = vtkDoubleArray::New();
  frustcorners->SetNumberOfComponents(4);
  frustcorners->SetNumberOfTuples(8);
  //a frustum containing the 4 lower left (-X,-Y) vertices
  frustcorners->SetTuple4(0,  -0.1, -0.1,  3.1, 0.0);
  frustcorners->SetTuple4(1,  -0.1, -0.1, -0.1, 0.0);
  frustcorners->SetTuple4(2,  -0.1,  0.1,  3.1, 0.0);
  frustcorners->SetTuple4(3,  -0.1,  0.1, -0.1, 0.0);
  frustcorners->SetTuple4(4,  0.1, -0.1,  3.1, 0.0);
  frustcorners->SetTuple4(5,  0.1, -0.1, -0.1, 0.0);
  frustcorners->SetTuple4(6,  0.1,  0.1,  3.1, 0.0);
  frustcorners->SetTuple4(7,  0.1,  0.1, -0.1, 0.0);
  sel->SetSelectionList(frustcorners);
  frustcorners->Delete();

  ext->Update();
  extGrid = vtkUnstructuredGrid::SafeDownCast(ext->GetOutput());
  if (DoWrite)
  {
    writer->SetInputConnection(ext->GetOutputPort());
    writer->SetFileName("ext_P_Fru.vtk");
    writer->Write();
  }
  showMe(extGrid, 3, 5, COLORBYPOINT, pia);

  sel->GetProperties()->Set(vtkSelectionNode::INVERSE(), 1);
  ext->Update();
  extGrid = vtkUnstructuredGrid::SafeDownCast(ext->GetOutput());
  if (DoWrite)
  {
    writer->SetInputConnection(ext->GetOutputPort());
    writer->SetFileName("ext_P_Fru_I.vtk");
    writer->Write();
  }
  showMe(extGrid, 4, 5, COLORBYPOINT, pia);

  sel->GetProperties()->Set(vtkSelectionNode::INVERSE(), 0);
  sel->GetProperties()->Set(vtkSelectionNode::CONTAINING_CELLS(), 1);
  ext->Update();
  extIData = vtkImageData::SafeDownCast(ext->GetOutput());
  if (DoWrite)
  {
    xwriter->SetInputConnection(ext->GetOutputPort());
    xwriter->SetFileName("ext_P_Fru_WC.vti");
    xwriter->Write();
  }
  showMe(extGrid, 5, 5, COLORBYPOINT, pia);

  sel->GetProperties()->Set(vtkSelectionNode::INVERSE(), 0);
  sel->GetProperties()->Set(vtkSelectionNode::CONTAINING_CELLS(), 0);
  ext->PreserveTopologyOn();
  ext->Update();
  extIData = vtkImageData::SafeDownCast(ext->GetOutput());
  if (DoWrite)
  {
    xwriter->SetInputConnection(ext->GetOutputPort());
    xwriter->SetFileName("ext_P_Fru_PT.vti");
    xwriter->Write();
  }
  insideArray = extIData->GetPointData()->GetArray("vtkInsidedness");
  showMe(extIData, 6, 5, COLORBYPOINT, insideArray);

  sel->GetProperties()->Set(vtkSelectionNode::CONTAINING_CELLS(), 1);
  ext->Update();
  extIData = vtkImageData::SafeDownCast(ext->GetOutput());
  if (DoWrite)
  {
    xwriter->SetInputConnection(ext->GetOutputPort());
    xwriter->SetFileName("ext_P_Fru_PT_WC.vti");
    xwriter->Write();
  }
  insideArray = extIData->GetCellData()->GetArray("vtkInsidedness");
  showMe(extIData, 7, 5, COLORBYCELL, insideArray);

  //-------------------------------------------------------------------------
  /*
  vtkCamera *cam = renderer->GetActiveCamera();
  cam->SetPosition(-6, -2, 45);
  cam->SetFocalPoint(10, 11, 2);
  cam->SetViewUp(0,1,0);
  renderer->SetActiveCamera(cam);
  */
  int retVal = vtkRegressionTestImageThreshold( renwin, 85 );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    rwi->Start();
  }

  //cleanup
  selection->Delete();
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

  xwriter->Delete();
  writer->Delete();
  renderer->Delete();
  sampleData->Delete();
  renwin->Delete();
  rwi->Delete();

  return !retVal;
}
