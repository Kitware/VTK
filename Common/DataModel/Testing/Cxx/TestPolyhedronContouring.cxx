/*=========================================================================

Program:   Visualization Toolkit
Module:    TestPolyhedron6.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkClipDataSet.h"
#include "vtkContourFilter.h"
#include "vtkDoubleArray.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLUnstructuredGridReader.h"

using namespace std;

int TestPolyhedronContouring(int argc, char* argv[])
{
  vtkObject::GlobalWarningDisplayOff();
  vtkNew<vtkXMLUnstructuredGridReader> r;
  vtkNew<vtkContourFilter> cf;

  if (argc < 3)
  {
    cout << "Not enough arguments. Passing test nonetheless.";
    return EXIT_SUCCESS;
  }

  {
    char* fname = argv[1];
    r->SetFileName(fname);
    r->Update();

    vtkUnstructuredGrid* grid = r->GetOutput();
    cf->SetInputArrayToProcess(
      0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "AirVolumeFraction");
    cf->SetInputData(grid);
    cf->SetValue(0, 0.5);
    cf->Update();

    vtkPolyData* polys = cf->GetOutput();
    if (polys->GetNumberOfCells() != 2)
    {
      cerr << "Number of polys not 2 (as expected), but " << polys->GetNumberOfCells() << endl;
      return EXIT_FAILURE;
    }

    vtkNew<vtkClipDataSet> cd;
    cd->SetInputData(grid);
    cd->SetInputArrayToProcess(
      0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "AirVolumeFraction");
    cd->SetValue(0.5);
    cd->SetInsideOut(0);
    cd->Update();

    vtkUnstructuredGrid* clip = cd->GetOutput();
    if (clip->GetNumberOfCells() != 2)
    {
      cerr << "Number of 'less' clipped cells not 2 (as expected), but " << clip->GetNumberOfCells()
           << endl;
      return EXIT_FAILURE;
    }

    vtkCell* clipCell0 = clip->GetCell(0);
    int nFaces0 = clipCell0->GetNumberOfFaces();
    if (nFaces0 != 4 && nFaces0 != 6)
    {
      cerr << "Expected one clipped cell with 4 and one with 10 faces, got " << nFaces0 << " faces."
           << endl;
      return EXIT_FAILURE;
    }

    vtkCell* clipCell1 = clip->GetCell(1);
    int nFaces1 = clipCell1->GetNumberOfFaces();
    if (nFaces1 != 4 && nFaces1 != 6)
    {
      cerr << "Expected one clipped cell with 4 and one with 10 faces, got " << nFaces1 << " faces."
           << endl;
      return EXIT_FAILURE;
    }

    cd->SetInsideOut(1);
    cd->Update();

    clip = cd->GetOutput();
    if (clip->GetNumberOfCells() != 1)
    {
      cerr << "Number of 'greater' clipped cells not 1 (as expected), but "
           << clip->GetNumberOfCells() << endl;
      return EXIT_FAILURE;
    }

    vtkCell* clipCell = clip->GetCell(0);
    if (clipCell->GetNumberOfFaces() != 10)
    {
      cerr << "Expected one clipped cell with 10 faces, got " << clipCell->GetNumberOfFaces()
           << "faces." << endl;
      return EXIT_FAILURE;
    }
  }

  // yet another problematic case, which gives an incorrect non-watertight warning in the old
  // contouring code
  {
    vtkNew<vtkPoints> pts;
    pts->InsertNextPoint(1, 0, 0);
    pts->InsertNextPoint(.5, 0, 0);
    pts->InsertNextPoint(0, 0, 0);

    pts->InsertNextPoint(1, 1, 1);
    pts->InsertNextPoint(1, 1, .5);
    pts->InsertNextPoint(1, 1, 0);

    pts->InsertNextPoint(1, 0, 1);
    pts->InsertNextPoint(1, .5, 0);
    pts->InsertNextPoint(0, 1, 1);

    pts->InsertNextPoint(.5, 1, 0);
    pts->InsertNextPoint(0, 1, 0);
    pts->InsertNextPoint(0, 0, 1);

    pts->InsertNextPoint(.5, .5, 0);
    pts->InsertNextPoint(0, .5, 0);

    vtkNew<vtkUnstructuredGrid> p;
    p->SetPoints(pts);
    p->Allocate(1);

    vtkIdType faceStream[] = { 6, 8, 3, 4, 5, 9, 10, 4, 8, 3, 6, 11, 6, 3, 6, 0, 7, 5, 4, 4, 9, 5,
      7, 12, 4, 10, 9, 12, 13, 4, 13, 12, 1, 2, 4, 12, 7, 0, 1, 5, 8, 11, 2, 13, 10, 5, 11, 6, 0, 1,
      2 };

    p->InsertNextCell(VTK_POLYHEDRON, 9, faceStream);

    double values[] = { 0.48828, 0.920027, 0.959499, 0.51357, 0.497449, 0.523359, 0.470217,
      0.498483, 0.956751, 0.928612, 0.971497, 0.942868, 0.93052, 0.961309 };

    vtkNew<vtkDoubleArray> arr;
    arr->SetArray(values, 14, 1);
    arr->SetName("AirVolumeFraction");
    p->GetPointData()->AddArray(arr);

    cf->SetInputArrayToProcess(
      0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "AirVolumeFraction");
    cf->SetInputData(p);
    cf->SetValue(0, 0.5);
    cf->Update();

    vtkPolyData* result = cf->GetOutput();
    if (result->GetNumberOfCells() != 1)
    {
      cerr << "Expected 1 contour polyhedron, got " << result->GetNumberOfCells() << endl;
      return EXIT_FAILURE;
    }

    vtkCell* contour = result->GetCell(0);
    if (contour->GetNumberOfPoints() != 7)
    {
      cerr << "Expected 7 contour points, got " << contour->GetNumberOfPoints() << endl;
      return EXIT_FAILURE;
    }

    r->SetFileName(argv[2]);
    r->Update();

    vtkUnstructuredGrid* cell_12851 = r->GetOutput();
    cf->SetInputData(cell_12851);
    cf->SetInputArrayToProcess(
      0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "AirVolumeFraction");
    cf->SetValue(0, 0.5);
    cf->Update();

    vtkPolyData* cell_12851_contour = cf->GetOutput();

    if (cell_12851_contour->GetNumberOfCells() != 1)
    {
      cerr << "cell_12851: Expected 1 contour polyhedron, got "
           << cell_12851_contour->GetNumberOfCells() << endl;
      return EXIT_FAILURE;
    }

    contour = cell_12851_contour->GetCell(0);
    if (contour->GetNumberOfPoints() != 3)
    {
      cerr << "cell_12851: Expected 3 contour points, got " << contour->GetNumberOfPoints() << endl;
      return EXIT_FAILURE;
    }
  }

  // Another problematic case. this one gave "problem in face navigation"
  // in an earlier approach to polyhedron face triangulation.  That problem
  // is now solved in vtkPolyhedron::TriangulatePolyhedralFaces
  {
    vtkNew<vtkPoints> pnts;
    pnts->InsertNextPoint(0.440016, 0.189264, 0.181594);
    pnts->InsertNextPoint(0.440537, 0.188737, 0.180708);
    pnts->InsertNextPoint(0.439976, 0.18893, 0.180698);
    pnts->InsertNextPoint(0.440257, 0.188834, 0.180703);
    pnts->InsertNextPoint(0.440597, 0.18926, 0.181462);
    pnts->InsertNextPoint(0.439896, 0.189791, 0.180785);
    pnts->InsertNextPoint(0.439833, 0.189866, 0.18164);
    pnts->InsertNextPoint(0.440492, 0.189543, 0.180782);
    pnts->InsertNextPoint(0.440567, 0.188999, 0.181085);
    pnts->InsertNextPoint(0.440306, 0.189262, 0.181528);
    pnts->InsertNextPoint(0.440499, 0.189503, 0.181569);
    pnts->InsertNextPoint(0.440166, 0.189685, 0.181605);

    vtkNew<vtkUnstructuredGrid> ba;
    ba->SetPoints(pnts);

    double values[] = { 0.544052, 0.479528, 0.485401, 0.491219, 0.522598, 0.460551, 0.508554,
      0.454234, 0.517886, 0.528239, 0.494647, 0.499257 };

    vtkNew<vtkDoubleArray> data;
    data->SetArray(values, 12, 1);
    data->SetName("AirVolumeFraction");

    ba->GetPointData()->AddArray(data);

    vtkIdType faceStream[] = { 5, 4, 8, 1, 3, 9, 4, 9, 3, 2, 0, 5, 4, 8, 1, 7, 10, 5, 1, 7, 5, 2, 3,

      4, 0, 2, 5, 6, 4, 9, 0, 6, 11, 4, 4, 9, 11, 10, 5, 10, 7, 5, 6, 11 };

    ba->InsertNextCell(VTK_POLYHEDRON, 8, faceStream);

    cf->SetInputArrayToProcess(
      0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "AirVolumeFraction");
    cf->SetInputData(ba);
    cf->SetValue(0, 0.5);
    cf->Update();

    vtkPolyData* result = cf->GetOutput();
    if (!result || result->GetNumberOfCells() != 1)
    {
      cerr << "Contouring failed for polyhedron cell" << endl;
      return EXIT_FAILURE;
    }
    vtkCell* contourCell = result->GetCell(0);

    if (contourCell->GetNumberOfPoints() != 7)
    {
      cerr << "Expected contour with 7 points, got " << contourCell->GetNumberOfPoints() << endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
