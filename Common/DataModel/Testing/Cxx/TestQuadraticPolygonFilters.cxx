/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestQuadraticPolygonFilters.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkCellPicker.h"
#include "vtkClipDataSet.h"
#include "vtkContourFilter.h"
#include "vtkDataSetMapper.h"
#include "vtkDoubleArray.h"
#include "vtkGeometryFilter.h"
#include "vtkIdTypeArray.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkOutlineFilter.h"
#include "vtkPlane.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataNormals.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTransform.h"
#include "vtkUnstructuredGrid.h"

int TestPicker(vtkRenderWindow *renWin, vtkRenderer *renderer);

vtkIdType GetCellIdFromPickerPosition(vtkRenderer *ren, int x, int y);

int TestQuadraticPolygonFilters(int argc, char* argv[])
{
  // create the object
  int npts = 12;

  vtkIdType* connectivityQuadPoly1 = new vtkIdType[npts / 2];
  vtkIdType* connectivityQuadPoly2 = new vtkIdType[npts / 2];
  vtkIdType* connectivityQuads = new vtkIdType[npts];

  vtkNew<vtkPoints> points;
  points->SetNumberOfPoints(npts);

  double ray = 1.0;
  double thetaStep = 4.0 * vtkMath::Pi() / npts;
  double theta;
  for (int i = 0; i < npts/2; i++)
  {
    if (i < npts / 4)
    {
      theta = thetaStep * i * 2;
    }
    else
    {
      theta = thetaStep * (i-npts/4) * 2 + thetaStep;
    }

    double x = ray * cos(theta);
    double y = ray * sin(theta);
    points->SetPoint(i, x, y, 0.0);
    points->SetPoint(npts / 2 + i, x, y, 1.0);

    connectivityQuadPoly1[i] = i;
    connectivityQuadPoly2[i] = npts / 2 + i;
    if (i < npts / 4)
    {
      connectivityQuads[4*i+0] = i;
      connectivityQuads[4*i+1] = (i + 1) % (npts / 4);
      connectivityQuads[4*i+2] = ((i + 1) % (npts / 4)) + npts / 2;
      connectivityQuads[4*i+3] = i + npts / 2;
    }
  }

  vtkNew<vtkUnstructuredGrid> ugrid;
  ugrid->SetPoints(points.GetPointer());
  ugrid->InsertNextCell(VTK_QUADRATIC_POLYGON, npts/2, connectivityQuadPoly1);
  ugrid->InsertNextCell(VTK_QUADRATIC_POLYGON, npts/2, connectivityQuadPoly2);
  for (int i = 0; i < npts/4; i++)
  {
    ugrid->InsertNextCell(VTK_QUAD,4,connectivityQuads + i * 4);
  }

  delete[] connectivityQuadPoly1;
  delete[] connectivityQuadPoly2;
  delete[] connectivityQuads;

  // to get the cell id with the picker
  vtkNew<vtkIdTypeArray> id;
  id->SetName("CellID");
  id->SetNumberOfComponents(1);
  id->SetNumberOfTuples(ugrid->GetNumberOfCells());
  for (int i = 0; i < ugrid->GetNumberOfCells(); i++)
  {
    id->SetValue(i, i);
  }
  ugrid->GetCellData()->AddArray(id.GetPointer());

  // Setup the scalars
  vtkNew<vtkDoubleArray> scalars;
  scalars->SetNumberOfComponents(1);
  scalars->SetNumberOfTuples(ugrid->GetNumberOfPoints());
  scalars->SetName("Scalars");
  scalars->SetValue(0, 1);
  scalars->SetValue(1, 2);
  scalars->SetValue(2, 2);
  scalars->SetValue(3, 1);
  scalars->SetValue(4, 2);
  scalars->SetValue(5, 1);
  scalars->SetValue(6, 1);
  scalars->SetValue(7, 2);
  scalars->SetValue(8, 2);
  scalars->SetValue(9, 1);
  scalars->SetValue(10, 2);
  scalars->SetValue(11, 1);
  ugrid->GetPointData()->SetScalars(scalars.GetPointer());

  // clip filter
  //vtkNew<vtkPlane> plane;
  //plane->SetOrigin(0, 0, 0);
  //plane->SetNormal(1, 0, 0);
  vtkNew<vtkClipDataSet> clip;
  //clip->SetClipFunction(plane);
  //clip->GenerateClipScalarsOn();
  clip->SetValue(1.5);
  clip->SetInputData(ugrid.GetPointer());
  clip->Update();
  vtkNew<vtkDataSetMapper> clipMapper;
  clipMapper->SetInputConnection( clip->GetOutputPort());
  clipMapper->SetScalarRange(1.0, 2.0);
  clipMapper->InterpolateScalarsBeforeMappingOn();
  vtkNew<vtkActor> clipActor;
  clipActor->SetPosition(0.0, 2.0, 0.0);
  clipActor->SetMapper(clipMapper.GetPointer());

  // contour filter
  vtkNew<vtkContourFilter> contourFilter;
  contourFilter->SetInputData(ugrid.GetPointer());
  contourFilter->SetValue(0,1.5);
  contourFilter->Update();
  vtkNew<vtkPolyDataNormals> contourNormals;
  contourNormals->SetInputConnection(contourFilter->GetOutputPort());
  vtkNew<vtkPolyDataMapper> contourMapper;
  contourMapper->SetInputConnection(contourNormals->GetOutputPort());
  contourMapper->ScalarVisibilityOff();
  vtkNew<vtkActor> contourActor;
  contourActor->SetMapper(contourMapper.GetPointer());
  contourActor->GetProperty()->SetColor(0,0,0);
  contourActor->SetPosition(0.0,0.01,0.01);

  // outline filter
  vtkNew<vtkOutlineFilter> outlineFilter;
  outlineFilter->SetInputData(ugrid.GetPointer());
  vtkNew<vtkPolyDataMapper> outlineMapper;
  outlineMapper->SetInputConnection(outlineFilter->GetOutputPort());
  vtkNew<vtkActor> outlineActor;
  outlineActor->SetMapper(outlineMapper.GetPointer());
  outlineActor->GetProperty()->SetColor(0,0,0);
  outlineActor->SetPosition(0.0,0.01,0.01);

  // geometry filter
  vtkNew<vtkGeometryFilter> geometryFilter;
  geometryFilter->SetInputData(ugrid.GetPointer());
  geometryFilter->Update();
  vtkNew<vtkPolyDataMapper> geometryMapper;
  geometryMapper->SetInputConnection(geometryFilter->GetOutputPort());
  geometryMapper->SetScalarRange(1.0, 2.0);
  geometryMapper->InterpolateScalarsBeforeMappingOn();
  vtkNew<vtkActor> geometryActor;
  geometryActor->SetMapper(geometryMapper.GetPointer());

  // drawing
  vtkNew<vtkRenderer> ren;
  ren->SetBackground(1, 1, 1);
  ren->AddActor(geometryActor.GetPointer());
  ren->AddActor(outlineActor.GetPointer());
  ren->AddActor(clipActor.GetPointer());
  ren->AddActor(contourActor.GetPointer());
  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(ren.GetPointer());
  renWin->SetSize(600, 600);
  renWin->SetMultiSamples(0);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());
  renWin->Render();

  // tests
  if (TestPicker(renWin.GetPointer(), ren.GetPointer()) == EXIT_FAILURE)
  {
    return EXIT_FAILURE;
  }

  int retVal = vtkRegressionTestImage(renWin.GetPointer());
  if ( retVal == vtkRegressionTester::DO_INTERACTOR )
  {
    iren->Start();
    retVal = vtkRegressionTester::PASSED;
  }

  return (retVal == vtkRegressionTester::PASSED) ? EXIT_SUCCESS : EXIT_FAILURE;
}

int TestPicker(vtkRenderWindow *renWin, vtkRenderer *renderer)
{
  // Sets the camera
  double cPos[3] = { 5.65647, 0.857996, 6.71491 };
  double cUp[3] = { 0.0212226, 0.999769, 0.00352794 };
  vtkCamera *camera = renderer->GetActiveCamera();
  camera->SetPosition(cPos);
  camera->SetViewUp(cUp);
  renderer->ResetCameraClippingRange();
  renWin->Render();
  renWin->Render();
  renWin->Render();

  // Sets the reference values
  int nbTests = 17;
  int values[] = { 218, 244, 1,  290, 244, 1,
                   201, 168, 1,  319, 166, 1,
                   223, 63,  1,  303, 46,  1,
                   330, 238, 2,  420, 173, 2,
                   376, 165, 2,  372, 128, 4,
                   411, 149, 4,  348, 266, 0,
                   416, 203, 0,  391, 269, 0,
                   412, 119, 0,  391, 61,  0,
                   340, 72,  0 };

  for (int i = 0; i < nbTests * 3; i += 3)
  {
    if ( GetCellIdFromPickerPosition(renderer, values[i], values[i+1]) !=  values[i+2] )
    {
      cerr << "ERROR:  selected cell type is "
           << GetCellIdFromPickerPosition(renderer, values[i], values[i+1])
           << ", should be " << values[i+2] << endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}

vtkIdType GetCellIdFromPickerPosition(vtkRenderer *ren, int x, int y)
{
  vtkNew<vtkCellPicker> picker;
  picker->SetTolerance(0.0005);

  // Pick from this location.
  picker->Pick(x, y, 0, ren);

  vtkIdType cellId = -1;
  if (picker->GetDataSet())
  {
      vtkIdTypeArray * ids = vtkArrayDownCast<vtkIdTypeArray>(
        picker->GetDataSet()->GetCellData()->GetArray("CellID"));
      cellId = ids->GetValue(picker->GetCellId());
  }

  return cellId;
}
