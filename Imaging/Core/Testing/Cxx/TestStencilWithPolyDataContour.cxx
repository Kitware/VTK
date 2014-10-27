/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestStencilWithPolyDataContour.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkImageData.h"
#include "vtkPoints.h"
#include "vtkCellArray.h"
#include "vtkPolyData.h"
#include "vtkPolyDataToImageStencil.h"
#include "vtkTransform.h"
#include "vtkImageStencil.h"
#include "vtkCamera.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkInteractorStyleImage.h"
#include "vtkImageSlice.h"
#include "vtkImageSliceMapper.h"
#include "vtkImageProperty.h"
#include "vtkSmartPointer.h"

#include <string.h>
#include <math.h>

// this contour used to cause trouble
static double contour[262][2] = {
  { -58.105499, 199.574005 },
  { -58.105499, 186.878998 },
  { -57.617199, 186.878998 },
  { -57.617199, 185.414001 },
  { -57.128899, 185.414001 },
  { -57.128899, 184.438004 },
  { -56.640598, 184.438004 },
  { -56.640598, 183.460999 },
  { -56.152302, 183.460999 },
  { -56.152302, 182.483994 },
  { -55.664101, 182.483994 },
  { -55.664101, 181.996002 },
  { -55.175800, 181.996002 },
  { -55.175800, 181.507996 },
  { -54.687500, 181.507996 },
  { -54.687500, 181.020004 },
  { -54.199200, 181.020004 },
  { -54.199200, 180.531006 },
  { -53.710899, 180.531006 },
  { -53.710899, 180.042999 },
  { -53.222698, 180.042999 },
  { -53.222698, 179.554993 },
  { -52.246101, 179.554993 },
  { -52.246101, 179.065994 },
  { -51.269501, 179.065994 },
  { -51.269501, 178.578003 },
  { -50.292999, 178.578003 },
  { -50.292999, 177.602005 },
  { -49.804699, 177.602005 },
  { -49.804699, 176.625000 },
  { -49.316399, 176.625000 },
  { -49.316399, 176.136993 },
  { -48.828098, 176.136993 },
  { -48.828098, 175.647995 },
  { -48.339802, 175.647995 },
  { -48.339802, 175.160004 },
  { -47.851601, 175.160004 },
  { -47.851601, 174.671997 },
  { -47.363300, 174.671997 },
  { -47.363300, 174.184006 },
  { -46.875000, 174.184006 },
  { -46.875000, 173.695007 },
  { -46.386700, 173.695007 },
  { -46.386700, 173.207001 },
  { -45.898399, 173.207001 },
  { -45.898399, 172.718994 },
  { -44.921902, 172.718994 },
  { -44.921902, 172.229996 },
  { -43.457001, 172.229996 },
  { -43.457001, 171.742004 },
  { -36.132801, 171.742004 },
  { -36.132801, 172.229996 },
  { -35.156200, 172.229996 },
  { -35.156200, 172.718994 },
  { -34.667999, 172.718994 },
  { -34.667999, 172.229996 },
  { -24.902300, 172.229996 },
  { -24.902300, 172.718994 },
  { -23.437500, 172.718994 },
  { -23.437500, 173.207001 },
  { -22.949200, 173.207001 },
  { -22.949200, 172.718994 },
  { -15.136700, 172.718994 },
  { -15.136700, 173.207001 },
  { -14.648400, 173.207001 },
  { -14.648400, 172.718994 },
  { -7.812500, 172.718994 },
  { -7.812500, 173.207001 },
  { -6.347660, 173.207001 },
  { -6.347660, 173.695007 },
  { -5.371090, 173.695007 },
  { -5.371090, 174.184006 },
  { -4.882810, 174.184006 },
  { -4.882810, 174.671997 },
  { -3.906250, 174.671997 },
  { -3.906250, 175.160004 },
  { -3.417970, 175.160004 },
  { -3.417970, 175.647995 },
  { -2.929690, 175.647995 },
  { -2.929690, 176.136993 },
  { -2.441410, 176.136993 },
  { -2.441410, 177.113007 },
  { -1.953120, 177.113007 },
  { -1.953120, 177.602005 },
  { -1.464840, 177.602005 },
  { -1.464840, 178.578003 },
  { -0.976563, 178.578003 },
  { -0.976563, 180.042999 },
  { -0.488281, 180.042999 },
  { -0.488281, 180.531006 },
  { -0.000000, 180.531006 },
  { -0.000000, 181.020004 },
  { 0.488281, 181.020004 },
  { 0.488281, 181.507996 },
  { 0.976563, 181.507996 },
  { 0.976563, 181.996002 },
  { 1.464840, 181.996002 },
  { 1.464840, 182.483994 },
  { 1.953120, 182.483994 },
  { 1.953120, 182.973007 },
  { 2.441410, 182.973007 },
  { 2.441410, 183.949005 },
  { 2.929690, 183.949005 },
  { 2.929690, 184.925995 },
  { 3.417970, 184.925995 },
  { 3.417970, 185.901993 },
  { 3.906250, 185.901993 },
  { 3.906250, 188.832001 },
  { 4.394530, 188.832001 },
  { 4.394530, 202.992004 },
  { 3.906250, 202.992004 },
  { 3.906250, 205.434006 },
  { 3.417970, 205.434006 },
  { 3.417970, 206.897995 },
  { 2.929690, 206.897995 },
  { 2.929690, 207.875000 },
  { 2.441410, 207.875000 },
  { 2.441410, 208.852005 },
  { 1.953120, 208.852005 },
  { 1.953120, 209.339996 },
  { 1.464840, 209.339996 },
  { 1.464840, 210.315994 },
  { 0.976563, 210.315994 },
  { 0.976563, 211.292999 },
  { 0.488281, 211.292999 },
  { 0.488281, 218.128998 },
  { -0.000000, 218.128998 },
  { -0.000000, 220.082001 },
  { -0.488281, 220.082001 },
  { -0.488281, 221.059006 },
  { -0.976563, 221.059006 },
  { -0.976563, 222.035004 },
  { -1.464840, 222.035004 },
  { -1.464840, 223.011993 },
  { -1.953120, 223.011993 },
  { -1.953120, 223.988007 },
  { -2.441410, 223.988007 },
  { -2.441410, 224.964996 },
  { -2.929690, 224.964996 },
  { -2.929690, 225.940994 },
  { -3.417970, 225.940994 },
  { -3.417970, 226.429993 },
  { -3.906250, 226.429993 },
  { -3.906250, 226.917999 },
  { -4.394530, 226.917999 },
  { -4.394530, 227.406006 },
  { -4.882810, 227.406006 },
  { -4.882810, 227.895004 },
  { -5.371090, 227.895004 },
  { -5.371090, 228.382996 },
  { -5.859380, 228.382996 },
  { -5.859380, 228.871002 },
  { -6.347660, 228.871002 },
  { -6.347660, 229.358994 },
  { -7.324220, 229.358994 },
  { -7.324220, 229.848007 },
  { -8.300780, 229.848007 },
  { -8.300780, 230.335999 },
  { -9.277340, 230.335999 },
  { -9.277340, 230.824005 },
  { -10.253900, 230.824005 },
  { -10.253900, 231.311996 },
  { -11.718800, 231.311996 },
  { -11.718800, 231.800995 },
  { -12.695300, 231.800995 },
  { -12.695300, 232.289001 },
  { -13.671900, 232.289001 },
  { -13.671900, 232.776993 },
  { -14.160200, 232.776993 },
  { -14.160200, 233.266006 },
  { -15.136700, 233.266006 },
  { -15.136700, 233.753998 },
  { -16.113300, 233.753998 },
  { -16.113300, 234.242004 },
  { -17.578100, 234.242004 },
  { -17.578100, 234.729996 },
  { -18.554701, 234.729996 },
  { -18.554701, 235.218994 },
  { -20.019501, 235.218994 },
  { -20.019501, 235.707001 },
  { -21.484400, 235.707001 },
  { -21.484400, 236.195007 },
  { -27.832001, 236.195007 },
  { -27.832001, 235.707001 },
  { -29.296900, 235.707001 },
  { -29.296900, 235.218994 },
  { -31.250000, 235.218994 },
  { -31.250000, 234.729996 },
  { -33.203098, 234.729996 },
  { -33.203098, 234.242004 },
  { -34.667999, 234.242004 },
  { -34.667999, 233.753998 },
  { -37.597698, 233.753998 },
  { -37.597698, 233.266006 },
  { -38.574200, 233.266006 },
  { -38.574200, 232.776993 },
  { -39.550800, 232.776993 },
  { -39.550800, 232.289001 },
  { -40.527302, 232.289001 },
  { -40.527302, 231.800995 },
  { -41.503899, 231.800995 },
  { -41.503899, 231.311996 },
  { -41.992199, 231.311996 },
  { -41.992199, 230.824005 },
  { -42.968800, 230.824005 },
  { -42.968800, 230.335999 },
  { -43.945301, 230.335999 },
  { -43.945301, 229.848007 },
  { -44.433601, 229.848007 },
  { -44.433601, 229.358994 },
  { -45.410198, 229.358994 },
  { -45.410198, 228.871002 },
  { -45.898399, 228.871002 },
  { -45.898399, 228.382996 },
  { -46.386700, 228.382996 },
  { -46.386700, 227.895004 },
  { -46.875000, 227.895004 },
  { -46.875000, 227.406006 },
  { -47.363300, 227.406006 },
  { -47.363300, 226.917999 },
  { -47.851601, 226.917999 },
  { -47.851601, 226.429993 },
  { -48.339802, 226.429993 },
  { -48.339802, 225.453003 },
  { -48.828098, 225.453003 },
  { -48.828098, 224.477005 },
  { -49.316399, 224.477005 },
  { -49.316399, 223.988007 },
  { -49.804699, 223.988007 },
  { -49.804699, 223.500000 },
  { -50.292999, 223.500000 },
  { -50.292999, 222.522995 },
  { -50.781200, 222.522995 },
  { -50.781200, 222.035004 },
  { -51.269501, 222.035004 },
  { -51.269501, 221.546997 },
  { -51.757801, 221.546997 },
  { -51.757801, 221.059006 },
  { -52.246101, 221.059006 },
  { -52.246101, 220.082001 },
  { -52.734402, 220.082001 },
  { -52.734402, 219.104996 },
  { -53.222698, 219.104996 },
  { -53.222698, 218.128998 },
  { -53.710899, 218.128998 },
  { -53.710899, 217.151993 },
  { -54.199200, 217.151993 },
  { -54.199200, 216.175995 },
  { -54.687500, 216.175995 },
  { -54.687500, 214.710999 },
  { -55.175800, 214.710999 },
  { -55.175800, 212.270004 },
  { -55.664101, 212.270004 },
  { -55.664101, 206.897995 },
  { -56.152302, 206.897995 },
  { -56.152302, 205.921997 },
  { -56.640598, 205.921997 },
  { -56.640598, 203.968994 },
  { -57.128899, 203.968994 },
  { -57.128899, 201.039001 },
  { -57.617199, 201.039001 },
  { -57.617199, 199.574005 },
};

int TestStencilWithPolyDataContour(int, char *[])
{
  vtkSmartPointer<vtkImageData> image =
    vtkSmartPointer<vtkImageData>::New();
  double spacing[3] = { 0.9765625, 0.9765625, 1.0 };
  double origin[3] = { -61.035206, 163.441589, 0.0 };
  int extent[6] = { 0, 65, 0, 71, 0, 0 };
  double center[3] = {
    origin[0] + 0.5*spacing[0]*extent[1],
    origin[1] + 0.5*spacing[1]*extent[3],
    0.0 };
  image->SetSpacing(spacing);
  image->SetOrigin(origin);
  image->SetExtent(extent);
  image->AllocateScalars(VTK_UNSIGNED_CHAR, 1);
  unsigned char *vptr =
    static_cast<unsigned char *>(image->GetScalarPointer());
  memset(vptr, 255, image->GetNumberOfPoints());

  vtkSmartPointer<vtkCellArray> lines =
    vtkSmartPointer<vtkCellArray>::New();
  vtkSmartPointer<vtkPoints> points =
    vtkSmartPointer<vtkPoints>::New();
  points->SetNumberOfPoints(262);
  lines->InsertNextCell(262);
  for (vtkIdType i = 0; i < 262; i++)
    {
    points->SetPoint(i, contour[i][0], contour[i][1], 0.0);
    lines->InsertCellPoint(i);
    }

  // add a couple spurs to make sure PolyDataToImageStencil
  // can deal with them
  vtkIdType ptId0 = 50;
  vtkIdType ptId1 = 0;
  double point[3];
  points->GetPoint(ptId0, point);
  point[0] += 2;
  point[1] -= 1;
  ptId1 = points->InsertNextPoint(point);
  lines->InsertNextCell(2);
  lines->InsertCellPoint(ptId0);
  lines->InsertCellPoint(ptId1);

  ptId0 = 200;
  points->GetPoint(ptId0, point);
  point[0] += 1.234;
  point[1] += 0.0;
  ptId1 = points->InsertNextPoint(point);
  lines->InsertNextCell(2);
  lines->InsertCellPoint(ptId0);
  lines->InsertCellPoint(ptId1);

  vtkSmartPointer<vtkPolyData> polyData =
    vtkSmartPointer<vtkPolyData>::New();
  polyData->SetPoints(points);
  polyData->SetLines(lines);

  vtkSmartPointer<vtkPolyDataToImageStencil> stencilSource =
    vtkSmartPointer<vtkPolyDataToImageStencil>::New();
  stencilSource->SetOutputOrigin(origin);
  stencilSource->SetOutputSpacing(spacing);
  stencilSource->SetOutputWholeExtent(extent);
  stencilSource->SetInputData(polyData);

  vtkSmartPointer<vtkImageStencil> stencil =
    vtkSmartPointer<vtkImageStencil>::New();
  stencil->SetInputData(image);
  stencil->SetStencilConnection(stencilSource->GetOutputPort());
  stencil->Update();

  vtkSmartPointer<vtkImageSliceMapper> mapper =
    vtkSmartPointer<vtkImageSliceMapper>::New();
  mapper->BorderOn();
  mapper->SetInputConnection(stencil->GetOutputPort());

  vtkSmartPointer<vtkImageSlice> actor =
    vtkSmartPointer<vtkImageSlice>::New();
  actor->GetProperty()->SetColorWindow(255.0);
  actor->GetProperty()->SetColorLevel(127.5);
  actor->GetProperty()->SetInterpolationTypeToNearest();
  actor->SetMapper(mapper);

  vtkSmartPointer<vtkRenderer> renderer =
    vtkSmartPointer<vtkRenderer>::New();
  renderer->AddViewProp(actor);

  vtkCamera *camera = renderer->GetActiveCamera();
  camera->ParallelProjectionOn();
  camera->SetParallelScale(40.0*spacing[1]);
  camera->SetFocalPoint(center[0], center[1], center[2]);
  camera->SetPosition(center[0], center[1], center[2] + 10.0);
  camera->SetViewUp(0.0, 1.0, 0.0);
  camera->SetClippingRange(5.0, 15.0);

  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::New();
  renWin->AddRenderer(renderer);
  renWin->SetSize(200, 200);

  vtkSmartPointer<vtkInteractorStyleImage> style =
    vtkSmartPointer<vtkInteractorStyleImage>::New();

  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);
  iren->SetInteractorStyle(style);
  iren->Initialize();

  renWin->Render();
  iren->Start();

  return EXIT_SUCCESS;
}
