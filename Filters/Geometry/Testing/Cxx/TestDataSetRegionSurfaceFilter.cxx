/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDataSetRegionSurfaceFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkCellArray.h>
#include <vtkCellData.h>
// #include <vtkCellIterator.h>
#include <vtkDataSetRegionSurfaceFilter.h>
// #include <vtkDoubleArray.h>
// #include <vtkGenericCell.h>
#include <vtkIntArray.h>
#include <vtkPointData.h>
#include <vtkPointLocator.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSmartPointer.h>
#include <vtkTestUtilities.h>
#include <vtkTetra.h>
#include <vtkTriangle.h>
#include <vtkUnstructuredGrid.h>
#include <vtkVersion.h>

namespace
{
void AddTetra(const double* p0,
              const double* p1,
              const double* p2,
              const double* p3,
              vtkPointLocator* pointLocator,
              vtkCellArray* cells)
{
  vtkSmartPointer<vtkTetra> t =
    vtkSmartPointer<vtkTetra>::New();
  static vtkIdType bIndices[4][4] = {{0,0,0,1},
                                     {1,0,0,0},
                                     {0,1,0,0},
                                     {0,0,1,0}};
  vtkIdType order = 1;
  vtkIdType nPoints = 4;
  t->GetPointIds()->SetNumberOfIds(nPoints);
  t->GetPoints()->SetNumberOfPoints(nPoints);
  t->Initialize();
  double p[3];
  vtkIdType pId;
  vtkIdType* bIndex;
  for (vtkIdType i = 0; i < nPoints; i++)
  {
    bIndex = bIndices[i];
    for (vtkIdType j = 0; j < 3; j++)
    {
      p[j] = (p0[j]*bIndex[3])/order + (p1[j]*bIndex[0])/order +
        (p2[j]*bIndex[1])/order + (p3[j]*bIndex[2])/order;
    }
    pointLocator->InsertUniquePoint(p, pId);
    t->GetPointIds()->SetId(i,pId);
  }
  cells->InsertNextCell(t);
}
}

int TestDataSetRegionSurfaceFilter(int argc, char* argv[])
{
  // This test constructs a meshed cube comprised of linear tetrahedra and
  // assigns a material ID to each cell according to the octant in which it
  // lies. It then applies the vtkDataSetRegionSurfaceFilter and visualizes the
  // results.

  vtkIdType nX = 2;
  vtkIdType nY = 2;
  vtkIdType nZ = 2;

  vtkSmartPointer<vtkUnstructuredGrid> unstructuredGrid =
    vtkSmartPointer<vtkUnstructuredGrid>::New();

  vtkSmartPointer<vtkPoints> pointArray =
    vtkSmartPointer<vtkPoints>::New();

  vtkSmartPointer<vtkPointLocator> pointLocator =
    vtkSmartPointer<vtkPointLocator>::New();
  double bounds[6] = {-1.,1.,-1.,1.,-1.,1.};
  pointLocator->InitPointInsertion(pointArray,bounds);

  vtkSmartPointer<vtkCellArray> cellArray =
    vtkSmartPointer<vtkCellArray>::New();

  double p[8][3];
  double dx = (bounds[1] - bounds[0])/nX;
  double dy = (bounds[3] - bounds[2])/nY;
  double dz = (bounds[5] - bounds[4])/nZ;
  for (vtkIdType i = 0; i < 8; i++)
  {
    for (vtkIdType j = 0; j < 3; j++)
    {
      p[i][j] = bounds[2*j];
    }
  }
  p[1][0] += dx;
  p[2][0] += dx;
  p[2][1] += dy;
  p[3][1] += dy;
  p[5][0] += dx;
  p[5][2] += dz;
  p[6][0] += dx;
  p[6][1] += dy;
  p[6][2] += dz;
  p[7][1] += dy;
  p[7][2] += dz;

  vtkSmartPointer<vtkIntArray> region = vtkSmartPointer<vtkIntArray>::New();
  region->SetName("Regions");
  region->SetNumberOfTuples(5*nX*nY*nZ);
  int counter = 0;

  for (vtkIdType xInc = 0; xInc < nX; xInc++)
  {
    p[0][1] = p[1][1] = p[4][1] = p[5][1] = bounds[2];
    p[2][1] = p[3][1] = p[6][1] = p[7][1] = bounds[2] + dy;

    for (vtkIdType yInc = 0; yInc < nY; yInc++)
    {
      p[0][2] = p[1][2] = p[2][2] = p[3][2] = bounds[4];
      p[4][2] = p[5][2] = p[6][2] = p[7][2] = bounds[4] + dz;

      for (vtkIdType zInc = 0; zInc < nZ; zInc++)
      {
        AddTetra(p[0],p[1],p[2],p[5], pointLocator, cellArray);
        AddTetra(p[0],p[2],p[3],p[7], pointLocator, cellArray);
        AddTetra(p[0],p[5],p[7],p[4], pointLocator, cellArray);
        AddTetra(p[2],p[5],p[6],p[7], pointLocator, cellArray);
        AddTetra(p[0],p[2],p[5],p[7], pointLocator, cellArray);

        int r = 4*(2*xInc/nX) + 2*(2*yInc/nY) + (2*zInc/nZ);
        for (vtkIdType i = 0; i < 5; i++)
        {
          region->SetTypedTuple(counter++, &r);
        }

        for (vtkIdType i = 0; i < 8; i++)
        {
          p[i][2] += dz;
        }
      }

      for (vtkIdType i = 0; i < 8; i++)
      {
        p[i][1] += dy;
      }
    }

    for (vtkIdType i = 0; i < 8; i++)
    {
      p[i][0] += dx;
    }
  }

  unstructuredGrid->SetPoints(pointArray);
  unstructuredGrid->SetCells(VTK_TETRA, cellArray);

  vtkIdType nPoints = unstructuredGrid->GetPoints()->GetNumberOfPoints();

  double maxDist = 0;
  for (vtkIdType i = 0; i < nPoints; i++)
  {
    double xyz[3];
    unstructuredGrid->GetPoints()->GetPoint(i,xyz);
    double dist = sqrt(xyz[0]*xyz[0] + xyz[1]*xyz[1] + xyz[2]*xyz[2]);
    maxDist = (dist > maxDist ? dist : maxDist);
  }

  unstructuredGrid->GetCellData()->AddArray(region);
  unstructuredGrid->GetCellData()->SetScalars(region);

  // Visualize
  vtkSmartPointer<vtkDataSetRegionSurfaceFilter> surfaceFilter =
    vtkSmartPointer<vtkDataSetRegionSurfaceFilter>::New();
  surfaceFilter->SetRegionArrayName("Regions");
  surfaceFilter->SetInputData(unstructuredGrid);

  vtkSmartPointer<vtkPolyDataMapper> mapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputConnection(surfaceFilter->GetOutputPort());
  mapper->SetScalarRange(0,7);

  vtkSmartPointer<vtkActor> actor =
    vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);

  vtkSmartPointer<vtkCamera> camera =
    vtkSmartPointer<vtkCamera>::New();
  camera->SetPosition(3.*maxDist, 3.*maxDist, -3.*maxDist);
  camera->SetFocalPoint(.0, .0, 0.);

  vtkSmartPointer<vtkRenderer> renderer =
    vtkSmartPointer<vtkRenderer>::New();
  renderer->SetActiveCamera(camera);

   vtkSmartPointer<vtkRenderWindow> renderWindow =
    vtkSmartPointer<vtkRenderWindow>::New();
  renderWindow->AddRenderer(renderer);
  vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  renderWindowInteractor->SetRenderWindow(renderWindow);

  renderer->AddActor(actor);

  renderWindow->Render();

  int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
  }

 return !retVal;
}
