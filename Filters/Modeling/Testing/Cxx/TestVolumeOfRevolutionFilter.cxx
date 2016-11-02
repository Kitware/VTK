/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestVolumeOfRevolutionFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <iostream>
#include <string>

#include <vtkActor.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkNew.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkSmartPointer.h>
#include <vtkTestUtilities.h>
#include <vtkVolumeOfRevolutionFilter.h>
#include <vtkProperty.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>

#include <vtkCellData.h>
#include <vtkVertex.h>
#include <vtkPointData.h>
#include <vtkPolyVertex.h>
#include <vtkLine.h>
#include <vtkPolyLine.h>
#include <vtkTriangle.h>
#include <vtkQuad.h>
#include <vtkPolygon.h>
#include <vtkTriangleStrip.h>

#include <vtkCellIterator.h>
#include <vtkUnstructuredGrid.h>

#include <vtkCharArray.h>
#include <vtkIntArray.h>
#include <vtkDataArray.h>
#include <vtkDoubleArray.h>
#include <vtkFloatArray.h>
#include <vtkLongArray.h>
#include <vtkLongLongArray.h>
#include <vtkSignedCharArray.h>
#include <vtkShortArray.h>
#include <vtkStringArray.h>
#include <vtkUnsignedIntArray.h>
#include <vtkUnsignedLongArray.h>
#include <vtkUnsignedLongLongArray.h>
#include <vtkUnsignedShortArray.h>

vtkSmartPointer<vtkPolyData> GeneratePolyData()
{
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();

  vtkSmartPointer<vtkVertex> vertex = vtkSmartPointer<vtkVertex>::New();
  vertex->GetPointIds()->SetId(0,points->InsertNextPoint(1.,1.,0.));

  vtkSmartPointer<vtkPolyVertex> polyVertex =
    vtkSmartPointer<vtkPolyVertex>::New();
  polyVertex->GetPointIds()->SetNumberOfIds(2);
  polyVertex->GetPointIds()->SetId(0,points->InsertNextPoint(.25,0.,0.));
  polyVertex->GetPointIds()->SetId(1,points->InsertNextPoint(0.,.35,0.));

  vtkSmartPointer<vtkCellArray> verts = vtkSmartPointer<vtkCellArray>::New();
  verts->InsertNextCell(vertex);
  verts->InsertNextCell(polyVertex);

  vtkSmartPointer<vtkLine> line = vtkSmartPointer<vtkLine>::New();
  line->GetPointIds()->SetId(0,points->InsertNextPoint(.75,0.,0.));
  line->GetPointIds()->SetId(1,points->InsertNextPoint(1.,0.,0.));

  vtkSmartPointer<vtkPolyLine> polyLine = vtkSmartPointer<vtkPolyLine>::New();
  polyLine->GetPointIds()->SetNumberOfIds(3);
  polyLine->GetPointIds()->SetId(0,points->InsertNextPoint(1.5,2.,0.));
  polyLine->GetPointIds()->SetId(1,points->InsertNextPoint(1.3,1.5,0.));
  polyLine->GetPointIds()->SetId(2,points->InsertNextPoint(1.75,2.,0.));

  vtkSmartPointer<vtkCellArray> lines = vtkSmartPointer<vtkCellArray>::New();
  lines->InsertNextCell(line);
  lines->InsertNextCell(polyLine);

  vtkSmartPointer<vtkTriangle> triangle = vtkSmartPointer<vtkTriangle>::New();
  triangle->GetPointIds()->SetId(0,points->InsertNextPoint(.5,-2.,0.));
  triangle->GetPointIds()->SetId(1,points->InsertNextPoint(1.5,-2.,0.));
  triangle->GetPointIds()->SetId(2,points->InsertNextPoint(1.5,-1.,0.));

  vtkSmartPointer<vtkQuad> quad = vtkSmartPointer<vtkQuad>::New();
  quad->GetPointIds()->SetId(0,points->InsertNextPoint(.5,-1.,0.));
  quad->GetPointIds()->SetId(1,points->InsertNextPoint(1.,-1.,0.));
  quad->GetPointIds()->SetId(2,points->InsertNextPoint(1.,.2,0.));
  quad->GetPointIds()->SetId(3,points->InsertNextPoint(.5,0.,0.));

  vtkSmartPointer<vtkPolygon> poly = vtkSmartPointer<vtkPolygon>::New();
  poly->GetPointIds()->SetNumberOfIds(5);
  poly->GetPointIds()->SetId(0,points->InsertNextPoint(2.,2.,0.));
  poly->GetPointIds()->SetId(1,points->InsertNextPoint(2.,3.,0.));
  poly->GetPointIds()->SetId(2,points->InsertNextPoint(3.,4.,0.));
  poly->GetPointIds()->SetId(3,points->InsertNextPoint(4.,6.,0.));
  poly->GetPointIds()->SetId(4,points->InsertNextPoint(6.,1.,0.));

  vtkSmartPointer<vtkCellArray> polys = vtkSmartPointer<vtkCellArray>::New();
  polys->InsertNextCell(triangle);
  polys->InsertNextCell(quad);
  polys->InsertNextCell(poly);

  vtkSmartPointer<vtkTriangleStrip> triangleStrip =
    vtkSmartPointer<vtkTriangleStrip>::New();
  triangleStrip->GetPointIds()->SetNumberOfIds(4);
  triangleStrip->GetPointIds()->SetId(0,points->InsertNextPoint(2.,0.,0.));
  triangleStrip->GetPointIds()->SetId(1,points->InsertNextPoint(2.,1.,0.));
  triangleStrip->GetPointIds()->SetId(2,points->InsertNextPoint(3.,0.,0.));
  triangleStrip->GetPointIds()->SetId(3,points->InsertNextPoint(3.5,1.,0.));

  vtkSmartPointer<vtkCellArray> strips = vtkSmartPointer<vtkCellArray>::New();
  strips->InsertNextCell(triangleStrip);

  vtkSmartPointer<vtkPolyData> pd = vtkSmartPointer<vtkPolyData>::New();
  pd->SetPoints(points);
  pd->SetVerts(verts);
  pd->SetLines(lines);
  pd->SetPolys(polys);
  pd->SetStrips(strips);

  vtkIdType nPoints = pd->GetNumberOfPoints();
  vtkIdType nCells = pd->GetNumberOfCells();

#define AddPointDataArray(dataType,vtkArrayType,nComponents,value)      \
  vtkSmartPointer<vtkArrayType> myp_##vtkArrayType =                    \
    vtkSmartPointer<vtkArrayType>::New();                               \
  std::string name_myp_##vtkArrayType = "p_";                           \
  name_myp_##vtkArrayType.append(#vtkArrayType);                        \
  myp_##vtkArrayType->SetName(name_myp_##vtkArrayType.c_str());         \
  myp_##vtkArrayType->SetNumberOfComponents(nComponents);               \
  myp_##vtkArrayType->SetNumberOfTuples(nPoints);                       \
  {                                                                     \
    dataType tuple[nComponents];                                        \
    for (vtkIdType j=0;j<nComponents;j++)                               \
    {                                                                   \
      tuple[j] = value;                                                 \
    }                                                                   \
    for (vtkIdType i=0;i<nPoints;i++)                                   \
    {                                                                   \
      for (vtkIdType j=0;j<nComponents;j++)                             \
      {                                                                 \
        tuple[j] += 1;                                                  \
      }                                                                 \
      myp_##vtkArrayType->SetTypedTuple(i,tuple);                       \
    }                                                                   \
  }                                                                     \
  pd->GetPointData()->AddArray(myp_##vtkArrayType)

#define AddCellDataArray(dataType,vtkArrayType,nComponents,value)       \
  vtkSmartPointer<vtkArrayType> myc_##vtkArrayType =                    \
    vtkSmartPointer<vtkArrayType>::New();                               \
  std::string name_myc_##vtkArrayType = "c_";                           \
  name_myc_##vtkArrayType.append(#vtkArrayType);                        \
  myc_##vtkArrayType->SetName(name_myc_##vtkArrayType.c_str());         \
  myc_##vtkArrayType->SetNumberOfComponents(nComponents);               \
  myc_##vtkArrayType->SetNumberOfTuples(nCells);                        \
  {                                                                     \
    dataType tuple[nComponents];                                        \
    for (vtkIdType j=0;j<nComponents;j++)                               \
    {                                                                   \
      tuple[j] = value;                                                 \
    }                                                                   \
    for (vtkIdType i=0;i<nCells;i++)                                    \
    {                                                                   \
      for (vtkIdType j=0;j<nComponents;j++)                             \
      {                                                                 \
        tuple[j] += 1;                                                  \
      }                                                                 \
      myc_##vtkArrayType->SetTypedTuple(i,tuple);                       \
    }                                                                   \
  }                                                                     \
  pd->GetCellData()->AddArray(myc_##vtkArrayType)

  AddPointDataArray(int,vtkIntArray,1,0);
  AddPointDataArray(long,vtkLongArray,1,0);
  AddPointDataArray(long long,vtkLongLongArray,1,0);
  AddPointDataArray(short,vtkShortArray,1,0);
  AddPointDataArray(unsigned int,vtkUnsignedIntArray,1,0);
  AddPointDataArray(unsigned long,vtkUnsignedLongArray,1,0);
  AddPointDataArray(unsigned long long,vtkUnsignedLongLongArray,1,0);
  AddPointDataArray(unsigned short,vtkUnsignedShortArray,1,0);
  AddPointDataArray(char,vtkCharArray,1,'0');
  AddPointDataArray(unsigned char,vtkUnsignedCharArray,1,'0');
  AddPointDataArray(signed char,vtkSignedCharArray,1,'0');
  AddPointDataArray(float,vtkFloatArray,1,0.0);
  AddPointDataArray(double,vtkDoubleArray,1,0.0);

  AddCellDataArray(int,vtkIntArray,1,0);
  AddCellDataArray(long,vtkLongArray,1,0);
  AddCellDataArray(long long,vtkLongLongArray,1,0);
  AddCellDataArray(short,vtkShortArray,1,0);
  AddCellDataArray(unsigned int,vtkUnsignedIntArray,1,0);
  AddCellDataArray(unsigned long,vtkUnsignedLongArray,1,0);
  AddCellDataArray(unsigned long long,vtkUnsignedLongLongArray,1,0);
  AddCellDataArray(unsigned short,vtkUnsignedShortArray,1,0);
  AddCellDataArray(char,vtkCharArray,1,'0');
  AddCellDataArray(unsigned char,vtkUnsignedCharArray,1,'0');
  AddCellDataArray(signed char,vtkSignedCharArray,1,'0');
  AddCellDataArray(float,vtkFloatArray,1,0.0);
  AddCellDataArray(double,vtkDoubleArray,1,0.0);

#if 0
  vtkSmartPointer<vtkStringArray> myc_vtkStringArray =
    vtkSmartPointer<vtkStringArray>::New();
  myc_vtkStringArray->SetName("string");
  myc_vtkStringArray->SetNumberOfComponents(1);
  myc_vtkStringArray->SetNumberOfTuples(nCells);
  {
    for (vtkIdType i=0;i<nCells;i++)
    {
      std::stringstream s; s << "test" << i;
      myc_vtkStringArray->SetValue(i,s.str().c_str());
    }
  }
  pd->GetCellData()->AddArray(myc_vtkStringArray);
#endif

  return pd;
}


//----------------------------------------------------------------------------
int TestVolumeOfRevolutionFilter( int argc, char * argv [] )
{
  vtkSmartPointer<vtkPolyData> pd = GeneratePolyData();

  double position[3] = {-1.,0.,0.};
  double direction[3] = {0.,1.,0.};

  vtkNew<vtkVolumeOfRevolutionFilter> revolve;
  revolve->SetSweepAngle(360.);
  revolve->SetAxisPosition(position);
  revolve->SetAxisDirection(direction);
  revolve->SetInputData(pd);
  revolve->Update();

  // test that the auxiliary arrays in vtkUnstructuredGrid output are correct
  {
    vtkUnstructuredGrid* ug = revolve->GetOutput();
    vtkCellIterator *it = ug->NewCellIterator();
    for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextCell())
    {
      vtkIdType npts, *ptIds;
      ug->GetCellPoints(it->GetCellId(), npts, ptIds);
      vtkIdType numberOfPoints = it->GetNumberOfPoints();
      if (npts != numberOfPoints)
      {
        return 1;
      }
      vtkIdList *pointIds = it->GetPointIds();
      for (vtkIdType i=0;i<numberOfPoints;i++)
      {
        if (ptIds[i] != pointIds->GetId(i))
        {
          return 1;
        }
      }
    }
    it->Delete();
  }

  vtkNew<vtkDataSetSurfaceFilter> surfaceFilter;
  surfaceFilter->SetInputConnection(revolve->GetOutputPort());

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(surfaceFilter->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper.GetPointer());
  actor->GetProperty()->SetColor(1.0, 0.0, 0.0);

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor.GetPointer());

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer.GetPointer());
  renderWindow->SetSize(300,300);

  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow.GetPointer());

  renderWindow->Render();

  int retVal = vtkRegressionTestImage(renderWindow.GetPointer());
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
  }

  return !retVal;
}
