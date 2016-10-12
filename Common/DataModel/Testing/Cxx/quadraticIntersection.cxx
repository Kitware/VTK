/*=========================================================================

  Program:   Visualization Toolkit
  Module:    quadraticIntersection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/
// .NAME
// .SECTION Description
// This program tests quadratic cell IntersectWithLine() methods.

#include <sstream>

#include "vtkDebugLeaks.h"

#include <vtkBiQuadraticQuad.h>
#include <vtkBiQuadraticQuadraticHexahedron.h>
#include <vtkBiQuadraticQuadraticWedge.h>
#include <vtkBiQuadraticTriangle.h>
#include <vtkCamera.h>
#include <vtkCubicLine.h>
#include <vtkIdList.h>
#include <vtkMath.h>
#include <vtkMinimalStandardRandomSequence.h>
#include <vtkPoints.h>
#include <vtkQuadraticEdge.h>
#include <vtkQuadraticTriangle.h>
#include <vtkQuadraticQuad.h>
#include <vtkQuadraticHexahedron.h>
#include <vtkQuadraticTetra.h>
#include <vtkQuadraticWedge.h>
#include <vtkQuadraticPyramid.h>
#include <vtkQuadraticLinearQuad.h>
#include <vtkQuadraticLinearWedge.h>
#include "vtkRegressionTestImage.h"
#include <vtkSmartPointer.h>
#include <vtkTriQuadraticHexahedron.h>

#include <vtkVersion.h>
#include <vtkSmartPointer.h>
#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>

void ViewportRange(int testNum,double* range)
{
  range[0] = 0.2*(testNum%5);
  range[1] = range[0] + 0.2;
  range[2] = (1./3.)*(testNum/5);
  range[3] = range[2] + (1./3.);
}

void RandomCircle(vtkMinimalStandardRandomSequence* sequence,
                  double radius,
                  double* offset,
                  double* value)
{
  double theta = 2.*vtkMath::Pi()*sequence->GetValue();
  sequence->Next();
  value[0] = radius*cos(theta) + offset[0];
  value[1] = radius*sin(theta) + offset[1];
}

void RandomSphere(vtkMinimalStandardRandomSequence* sequence,
                  double radius,
                  double* offset,
                  double* value)
{
  double theta = 2.*vtkMath::Pi()*sequence->GetValue();
  sequence->Next();
  double phi = vtkMath::Pi()*sequence->GetValue();
  sequence->Next();
  value[0] = radius*cos(theta)*sin(phi) + offset[0];
  value[1] = radius*sin(theta)*sin(phi) + offset[1];
  value[2] = radius*cos(phi) + offset[2];
}

void IntersectWithCell(unsigned nTest,
                       vtkMinimalStandardRandomSequence* sequence,
                       bool threeDimensional,
                       double radius,
                       double* offset,
                       vtkCell* cell,
                       vtkSmartPointer<vtkRenderWindow> renderWindow)
{
  double p[2][3];
  p[0][2] = p[1][2] = 0.;
  double tol = 1.e-7;
  double t;
  double intersect[3];
  double pcoords[3];
  int subId;

  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkCellArray> vertices = vtkSmartPointer<vtkCellArray>::New();

  for (unsigned i=0;i<nTest;i++)
  {
    if (threeDimensional)
    {
      RandomSphere(sequence,radius,offset,p[0]);
      RandomSphere(sequence,radius,offset,p[1]);
    }
    else
    {
      RandomCircle(sequence,radius,offset,p[0]);
      RandomCircle(sequence,radius,offset,p[1]);
    }

    if (cell->IntersectWithLine(p[0],p[1],tol,t,intersect,pcoords,subId))
    {
      vtkIdType pid = points->InsertNextPoint(intersect);
      vertices->InsertNextCell(1,&pid);
    }
  }

  vtkSmartPointer<vtkCamera> camera = vtkSmartPointer<vtkCamera>::New();
  camera->SetPosition(2, 2, 2);
  camera->SetFocalPoint(offset[0],offset[1],offset[2]);

  vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
  renderer->SetActiveCamera(camera);
  renderWindow->AddRenderer(renderer);
  double dim[4];
  static int testNum = 0;
  ViewportRange(testNum++,dim);
  renderer->SetViewport(dim[0],dim[2],dim[1],dim[3]);

  vtkSmartPointer<vtkPolyData> point = vtkSmartPointer<vtkPolyData>::New();

  point->SetPoints(points);
  point->SetVerts(vertices);

  vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputData(point);

  vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);
  renderer->AddActor(actor);
  renderer->ResetCamera();

  renderWindow->Render();
}

int TestIntersectWithLine(int argc, char* argv[])
{
  std::ostringstream strm;
  strm << "Test vtkCell::IntersectWithLine Start" << endl;

  vtkSmartPointer<vtkRenderWindow> renderWindow =
    vtkSmartPointer<vtkRenderWindow>::New();
  renderWindow->SetMultiSamples(0);
  renderWindow->SetSize(500,300);

  vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();

  renderWindowInteractor->SetRenderWindow(renderWindow);

  vtkMinimalStandardRandomSequence* sequence =
    vtkMinimalStandardRandomSequence::New();

  sequence->SetSeed(1);

  unsigned nTest = 1.e4;
  double radius = 1.5;
  double center[3] = {0.5,0.25,0.};

  // vtkQuadraticEdge
  vtkQuadraticEdge *edge = vtkQuadraticEdge::New();
  edge->GetPointIds()->SetId(0,0);
  edge->GetPointIds()->SetId(1,1);
  edge->GetPointIds()->SetId(2,2);

  edge->GetPoints()->SetPoint(0, 0, 0, 0);
  edge->GetPoints()->SetPoint(1, 1, 0, 0);
  edge->GetPoints()->SetPoint(2, 0.5, 0.25, 0);

  IntersectWithCell(nTest,sequence,false,radius,center,edge,renderWindow);

  edge->Delete();

  // vtkQuadraticTriangle
  vtkQuadraticTriangle *tri = vtkQuadraticTriangle::New();
  tri->GetPointIds()->SetId(0,0);
  tri->GetPointIds()->SetId(1,1);
  tri->GetPointIds()->SetId(2,2);
  tri->GetPointIds()->SetId(3,3);
  tri->GetPointIds()->SetId(4,4);
  tri->GetPointIds()->SetId(5,5);

  tri->GetPoints()->SetPoint(0, 0, 0, 0);
  tri->GetPoints()->SetPoint(1, 1, 0, 0);
  tri->GetPoints()->SetPoint(2, 0.5, 0.8, 0);
  tri->GetPoints()->SetPoint(3, 0.5, 0.0, 0);
  tri->GetPoints()->SetPoint(4, 0.75, 0.4, 0);
  tri->GetPoints()->SetPoint(5, 0.25, 0.4, 0);

  center[0] = 0.5;
  center[1] = 0.5;
  center[2] = 0.;
  // interestingly, triangles are invisible edge-on. Test in 3D
  IntersectWithCell(nTest,sequence,true,radius,center,tri,renderWindow);

  tri->Delete();

  // vtkQuadraticQuad
  vtkQuadraticQuad *quad = vtkQuadraticQuad::New();
  quad->GetPointIds()->SetId(0,0);
  quad->GetPointIds()->SetId(1,1);
  quad->GetPointIds()->SetId(2,2);
  quad->GetPointIds()->SetId(3,3);
  quad->GetPointIds()->SetId(4,4);
  quad->GetPointIds()->SetId(5,5);
  quad->GetPointIds()->SetId(6,6);
  quad->GetPointIds()->SetId(7,7);

  quad->GetPoints()->SetPoint(0, 0.0, 0.0, 0.0);
  quad->GetPoints()->SetPoint(1, 1.0, 0.0, 0.0);
  quad->GetPoints()->SetPoint(2, 1.0, 1.0, 0.0);
  quad->GetPoints()->SetPoint(3, 0.0, 1.0, 0.0);
  quad->GetPoints()->SetPoint(4, 0.5, 0.0, 0.0);
  quad->GetPoints()->SetPoint(5, 1.0, 0.5, 0.0);
  quad->GetPoints()->SetPoint(6, 0.5, 1.0, 0.0);
  quad->GetPoints()->SetPoint(7, 0.0, 0.5, 0.0);

  IntersectWithCell(nTest,sequence,true,radius,center,quad,renderWindow);

  quad->Delete();

  // vtkQuadraticTetra
  vtkQuadraticTetra *tetra = vtkQuadraticTetra::New();
  tetra->GetPointIds()->SetId(0,0);
  tetra->GetPointIds()->SetId(1,1);
  tetra->GetPointIds()->SetId(2,2);
  tetra->GetPointIds()->SetId(3,3);
  tetra->GetPointIds()->SetId(4,4);
  tetra->GetPointIds()->SetId(5,5);
  tetra->GetPointIds()->SetId(6,6);
  tetra->GetPointIds()->SetId(7,7);
  tetra->GetPointIds()->SetId(8,8);
  tetra->GetPointIds()->SetId(9,9);

  tetra->GetPoints()->SetPoint(0, 0.0, 0.0, 0.0);
  tetra->GetPoints()->SetPoint(1, 1.0, 0.0, 0.0);
  tetra->GetPoints()->SetPoint(2, 0.5, 0.8, 0.0);
  tetra->GetPoints()->SetPoint(3, 0.5, 0.4, 1.0);
  tetra->GetPoints()->SetPoint(4, 0.5, 0.0,  0.0);
  tetra->GetPoints()->SetPoint(5, 0.75, 0.4, 0.0);
  tetra->GetPoints()->SetPoint(6, 0.25, 0.4, 0.0);
  tetra->GetPoints()->SetPoint(7, 0.25, 0.2, 0.5);
  tetra->GetPoints()->SetPoint(8, 0.75, 0.2, 0.5);
  tetra->GetPoints()->SetPoint(9, 0.50, 0.6, 0.5);

  IntersectWithCell(nTest,sequence,true,radius,center,tetra,renderWindow);

  tetra->Delete();

  // vtkQuadraticHexahedron
  vtkQuadraticHexahedron *hex = vtkQuadraticHexahedron::New();
  hex->GetPointIds()->SetId(0,0);
  hex->GetPointIds()->SetId(1,1);
  hex->GetPointIds()->SetId(2,2);
  hex->GetPointIds()->SetId(3,3);
  hex->GetPointIds()->SetId(4,4);
  hex->GetPointIds()->SetId(5,5);
  hex->GetPointIds()->SetId(6,6);
  hex->GetPointIds()->SetId(7,7);
  hex->GetPointIds()->SetId(8,8);
  hex->GetPointIds()->SetId(9,9);
  hex->GetPointIds()->SetId(10,10);
  hex->GetPointIds()->SetId(11,11);
  hex->GetPointIds()->SetId(12,12);
  hex->GetPointIds()->SetId(13,13);
  hex->GetPointIds()->SetId(14,14);
  hex->GetPointIds()->SetId(15,15);
  hex->GetPointIds()->SetId(16,16);
  hex->GetPointIds()->SetId(17,17);
  hex->GetPointIds()->SetId(18,18);
  hex->GetPointIds()->SetId(19,19);

  hex->GetPoints()->SetPoint( 0, 0, 0, 0  );
  hex->GetPoints()->SetPoint( 1, 1, 0, 0  );
  hex->GetPoints()->SetPoint( 2, 1, 1, 0  );
  hex->GetPoints()->SetPoint( 3, 0, 1, 0  );
  hex->GetPoints()->SetPoint( 4, 0, 0, 1  );
  hex->GetPoints()->SetPoint( 5, 1, 0, 1  );
  hex->GetPoints()->SetPoint( 6, 1, 1, 1  );
  hex->GetPoints()->SetPoint( 7, 0, 1, 1  );
  hex->GetPoints()->SetPoint( 8, 0.5, 0, 0);
  hex->GetPoints()->SetPoint( 9, 1, 0.5, 0);
  hex->GetPoints()->SetPoint(10, 0.5, 1, 0);
  hex->GetPoints()->SetPoint(11, 0, 0.5, 0);
  hex->GetPoints()->SetPoint(12, 0.5, 0, 1);
  hex->GetPoints()->SetPoint(13, 1, 0.5, 1);
  hex->GetPoints()->SetPoint(14, 0.5, 1, 1);
  hex->GetPoints()->SetPoint(15, 0, 0.5, 1);
  hex->GetPoints()->SetPoint(16, 0, 0, 0.5);
  hex->GetPoints()->SetPoint(17, 1, 0, 0.5);
  hex->GetPoints()->SetPoint(18, 1, 1, 0.5);
  hex->GetPoints()->SetPoint(19, 0, 1, 0.5);

  IntersectWithCell(nTest,sequence,true,radius,center,hex,renderWindow);

  hex->Delete();

  // vtkQuadraticWedge
  vtkQuadraticWedge* wedge = vtkQuadraticWedge::New();
  double *pcoords = wedge->GetParametricCoords();
  for (int i = 0; i < wedge->GetNumberOfPoints(); ++i)
  {
    wedge->GetPointIds()->SetId(i, i);
    wedge->GetPoints()->SetPoint(i,
                                 *(pcoords + 3 * i),
                                 *(pcoords + 3 * i + 1),
                                 *(pcoords + 3 * i + 2));
  }

  IntersectWithCell(nTest,sequence,true,radius,center,wedge,renderWindow);

  wedge->Delete();

  // vtkQuadraticPyramid
  vtkQuadraticPyramid *pyra = vtkQuadraticPyramid::New();
  pyra->GetPointIds()->SetId(0,0);
  pyra->GetPointIds()->SetId(1,1);
  pyra->GetPointIds()->SetId(2,2);
  pyra->GetPointIds()->SetId(3,3);
  pyra->GetPointIds()->SetId(4,4);
  pyra->GetPointIds()->SetId(5,5);
  pyra->GetPointIds()->SetId(6,6);
  pyra->GetPointIds()->SetId(7,7);
  pyra->GetPointIds()->SetId(8,8);
  pyra->GetPointIds()->SetId(9,9);
  pyra->GetPointIds()->SetId(10,10);
  pyra->GetPointIds()->SetId(11,11);
  pyra->GetPointIds()->SetId(12,12);

  pyra->GetPoints()->SetPoint( 0, 0, 0, 0  );
  pyra->GetPoints()->SetPoint( 1, 1, 0, 0  );
  pyra->GetPoints()->SetPoint( 2, 1, 1, 0  );
  pyra->GetPoints()->SetPoint( 3, 0, 1, 0  );
  pyra->GetPoints()->SetPoint( 4, 0, 0, 1  );
  pyra->GetPoints()->SetPoint( 5, 0.5, 0, 0  );
  pyra->GetPoints()->SetPoint( 6, 1, 0.5, 0  );
  pyra->GetPoints()->SetPoint( 7, 0.5, 1, 0  );
  pyra->GetPoints()->SetPoint( 8, 0, 0.5, 0  );
  pyra->GetPoints()->SetPoint( 9, 0, 0, 0.5  );
  pyra->GetPoints()->SetPoint(10, 0.5, 0, 0.5  );
  pyra->GetPoints()->SetPoint(11, 0.5, 0.5, 0.5  );
  pyra->GetPoints()->SetPoint(12, 0, 0.5, 0.5  );

  IntersectWithCell(nTest,sequence,true,radius,center,pyra,renderWindow);

  pyra->Delete();

  // vtkQuadraticLinearQuad
  vtkQuadraticLinearQuad *quadlin = vtkQuadraticLinearQuad::New();
  double* paramcoor = quadlin->GetParametricCoords();
  int i;

  for(i = 0; i < quadlin->GetNumberOfPoints(); i++)
    quadlin->GetPointIds()->SetId(i,i);

  for(i = 0; i < quadlin->GetNumberOfPoints(); i++)
    quadlin->GetPoints()->SetPoint(i, paramcoor[i*3], paramcoor[i*3 + 1], paramcoor[i*3 + 2]);

  IntersectWithCell(nTest,sequence,true,radius,center,quadlin,renderWindow);

  quadlin->Delete();

  // vtkBiQuadraticQuad
  vtkBiQuadraticQuad *biquad = vtkBiQuadraticQuad::New();
  paramcoor = biquad->GetParametricCoords();

  for(i = 0; i < biquad->GetNumberOfPoints(); i++)
    biquad->GetPointIds()->SetId(i,i);

  for(i = 0; i < biquad->GetNumberOfPoints(); i++)
    biquad->GetPoints()->SetPoint(i, paramcoor[i*3], paramcoor[i*3 + 1], paramcoor[i*3 + 2]);

  IntersectWithCell(nTest,sequence,true,radius,center,biquad,renderWindow);

  biquad->Delete();

  // vtkQuadraticLinearWedge
  vtkQuadraticLinearWedge *wedgelin = vtkQuadraticLinearWedge::New();
  paramcoor = wedgelin->GetParametricCoords();

  for(i = 0; i < wedgelin->GetNumberOfPoints(); i++)
    wedgelin->GetPointIds()->SetId(i,i);

  for(i = 0; i < wedgelin->GetNumberOfPoints(); i++)
    wedgelin->GetPoints()->SetPoint(i, paramcoor[i*3], paramcoor[i*3 + 1], paramcoor[i*3 + 2]);

  IntersectWithCell(nTest,sequence,true,radius,center,wedgelin,renderWindow);

  wedgelin->Delete();

  // vtkBiQuadraticQuadraticWedge
  vtkBiQuadraticQuadraticWedge *biwedge = vtkBiQuadraticQuadraticWedge::New();
  paramcoor = biwedge->GetParametricCoords();

  for(i = 0; i < biwedge->GetNumberOfPoints(); i++)
    biwedge->GetPointIds()->SetId(i,i);

  for(i = 0; i < biwedge->GetNumberOfPoints(); i++)
    biwedge->GetPoints()->SetPoint(i, paramcoor[i*3], paramcoor[i*3 + 1], paramcoor[i*3 + 2]);

  IntersectWithCell(nTest,sequence,true,radius,center,biwedge,renderWindow);

  biwedge->Delete();

  // vtkBiQuadraticQuadraticHexahedron
  vtkBiQuadraticQuadraticHexahedron *bihex = vtkBiQuadraticQuadraticHexahedron::New();
  paramcoor = bihex->GetParametricCoords();

  for(i = 0; i < bihex->GetNumberOfPoints(); i++)
    bihex->GetPointIds()->SetId(i,i);

  for(i = 0; i < bihex->GetNumberOfPoints(); i++)
    bihex->GetPoints()->SetPoint(i, paramcoor[i*3], paramcoor[i*3 + 1], paramcoor[i*3 + 2]);

  IntersectWithCell(nTest,sequence,true,radius,center,bihex,renderWindow);

  bihex->Delete();

  // vtkTriQuadraticHexahedron
  vtkTriQuadraticHexahedron *trihex = vtkTriQuadraticHexahedron::New();
  paramcoor = trihex->GetParametricCoords();

  for(i = 0; i < trihex->GetNumberOfPoints(); i++)
    trihex->GetPointIds()->SetId(i,i);

  for(i = 0; i < trihex->GetNumberOfPoints(); i++)
    trihex->GetPoints()->SetPoint(i, paramcoor[i*3], paramcoor[i*3 + 1], paramcoor[i*3 + 2]);

  IntersectWithCell(nTest,sequence,true,radius,center,trihex,renderWindow);

  trihex->Delete();

  // vtkBiQuadraticTriangle
  vtkBiQuadraticTriangle *bitri = vtkBiQuadraticTriangle::New();
  bitri->GetPointIds()->SetId(0,0);
  bitri->GetPointIds()->SetId(1,1);
  bitri->GetPointIds()->SetId(2,2);
  bitri->GetPointIds()->SetId(3,3);
  bitri->GetPointIds()->SetId(4,4);
  bitri->GetPointIds()->SetId(5,5);

  bitri->GetPoints()->SetPoint(0, 0, 0, 0);
  bitri->GetPoints()->SetPoint(1, 1, 0, 0);
  bitri->GetPoints()->SetPoint(2, 0.5, 0.8, 0);
  bitri->GetPoints()->SetPoint(3, 0.5, 0.0, 0);
  bitri->GetPoints()->SetPoint(4, 0.75, 0.4, 0);
  bitri->GetPoints()->SetPoint(5, 0.25, 0.4, 0);
  bitri->GetPoints()->SetPoint(6, 0.45, 0.24, 0);

  IntersectWithCell(nTest,sequence,true,radius,center,bitri,renderWindow);

  bitri->Delete();

  // vtkCubicLine
  vtkCubicLine *culine = vtkCubicLine::New();
  culine->GetPointIds()->SetId(0,0);
  culine->GetPointIds()->SetId(1,1);
  culine->GetPointIds()->SetId(2,2);
  culine->GetPointIds()->SetId(3,3);

  culine->GetPoints()->SetPoint(0, 0, 0, 0);
  culine->GetPoints()->SetPoint(1, 1, 0, 0);
  culine->GetPoints()->SetPoint(2, (1.0/3.0), -0.1, 0);
  culine->GetPoints()->SetPoint(3, (1.0/3.0), 0.1, 0);

  IntersectWithCell(nTest,sequence,false,radius,center,culine,renderWindow);

  culine->Delete();

  strm << "Test vtkCell::IntersectWithLine End" << endl;

  sequence->Delete();

  renderWindowInteractor->Initialize();

  int retVal = vtkRegressionTestImage(renderWindow.GetPointer());
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
    retVal = vtkRegressionTester::PASSED;
  }
  return (retVal == vtkRegressionTester::PASSED) ? EXIT_SUCCESS : EXIT_FAILURE;
}

int quadraticIntersection(int argc, char* argv[])
{
  return TestIntersectWithLine(argc,argv);
}
