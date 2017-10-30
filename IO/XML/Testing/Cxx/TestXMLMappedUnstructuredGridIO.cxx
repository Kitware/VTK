/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestXMLMappedUnstructuredGridIO

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkDataArray.h"
#include "vtkCell.h" // for cell types
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLUnstructuredGridReader.h"
#include "vtkXMLUnstructuredGridWriter.h"

#include "MappedGrid.h"

#include <string>
#include <fstream>
#include <algorithm>
#include <string>


using namespace std;

bool compareFiles(const string& p1, const string& p2) {
  ifstream f1(p1, ifstream::binary|ifstream::ate);
  ifstream f2(p2, ifstream::binary|ifstream::ate);

  if (f1.fail() || f2.fail()) {
    return false; //file problem
  }

  if (f1.tellg() != f2.tellg()) {
    return false; //size mismatch
  }

  //seek back to beginning and use equal to compare contents
  f1.seekg(0, ifstream::beg);
  f2.seekg(0, ifstream::beg);
  return equal(istreambuf_iterator<char>(f1.rdbuf()),
                    istreambuf_iterator<char>(),
                    istreambuf_iterator<char>(f2.rdbuf()));
}

int TestXMLMappedUnstructuredGridIO(int argc, char *argv[])
{
  vtkNew<vtkPoints> points;

  points->InsertNextPoint(0, 0, 0);
  points->InsertNextPoint(1, 0, 0);
  points->InsertNextPoint(1, 1, 0);
  points->InsertNextPoint(0, 1, 0);

  points->InsertNextPoint(0, 0, 1);
  points->InsertNextPoint(1, 0, 1);
  points->InsertNextPoint(1, 1, 1);
  points->InsertNextPoint(0, 1, 1);

  points->InsertNextPoint(.5, .5, 2);
  points->InsertNextPoint(.5, .5, -1);

  vtkNew<vtkUnstructuredGrid> ug;
  ug->SetPoints(points);


  ug->Allocate(3); // allocate for 3 cells

  vtkNew<vtkIdList> ids;

  // add a hexahedron of the first 8 points (i.e. a cube)
  ids->InsertNextId(0);
  ids->InsertNextId(1);
  ids->InsertNextId(2);
  ids->InsertNextId(3);
  ids->InsertNextId(4);
  ids->InsertNextId(5);
  ids->InsertNextId(6);
  ids->InsertNextId(7);
  ug->InsertNextCell(VTK_HEXAHEDRON, ids.GetPointer());
  ids->Reset();

  // add a polyhedron comprise of the top hexahedron face
  // and four triangles to the 9th point
  ids->InsertNextId(4);
  ids->InsertNextId(5);
  ids->InsertNextId(6);
  ids->InsertNextId(7);
  ids->InsertNextId(8);

  vtkNew<vtkIdList> faces;
  // top face of four points
  faces->InsertNextId(4);

  faces->InsertNextId(4);
  faces->InsertNextId(5);
  faces->InsertNextId(6);
  faces->InsertNextId(7);

  // four triangle side faces, each of three points
  faces->InsertNextId(3);
  faces->InsertNextId(4);
  faces->InsertNextId(5);
  faces->InsertNextId(8);

  faces->InsertNextId(3);
  faces->InsertNextId(5);
  faces->InsertNextId(6);
  faces->InsertNextId(8);

  faces->InsertNextId(3);
  faces->InsertNextId(6);
  faces->InsertNextId(7);
  faces->InsertNextId(8);

  faces->InsertNextId(3);
  faces->InsertNextId(7);
  faces->InsertNextId(4);
  faces->InsertNextId(8);

  // insert the polyhedron cell
  ug->InsertNextCell(VTK_POLYHEDRON, 5, ids.GetPointer()->GetPointer(0), 5, faces.GetPointer()->GetPointer(0));

  // put another pyramid on the bottom towards the 10th point
  faces->Reset();
  ids->Reset();

  // the list of points that the pyramid references
  ids->InsertNextId(0);
  ids->InsertNextId(1);
  ids->InsertNextId(2);
  ids->InsertNextId(3);
  ids->InsertNextId(9);

  // bottom face of four points
  faces->InsertNextId(4);

  faces->InsertNextId(0);
  faces->InsertNextId(1);
  faces->InsertNextId(2);
  faces->InsertNextId(3);

  // four side faces, each of three points
  faces->InsertNextId(3);
  faces->InsertNextId(0);
  faces->InsertNextId(1);
  faces->InsertNextId(9);

  faces->InsertNextId(3);
  faces->InsertNextId(1);
  faces->InsertNextId(2);
  faces->InsertNextId(9);

  faces->InsertNextId(3);
  faces->InsertNextId(2);
  faces->InsertNextId(3);
  faces->InsertNextId(9);

  faces->InsertNextId(3);
  faces->InsertNextId(3);
  faces->InsertNextId(0);
  faces->InsertNextId(9);

  // insert the cell. We now have two pyramids with a cube in between
  ug->InsertNextCell(VTK_POLYHEDRON, 5, ids.GetPointer()->GetPointer(0), 5, faces.GetPointer()->GetPointer(0));

  // for testing, we write in appended, ascii and binary mode and request that
  // the files are ** binary ** equal.
  //
  // first, find a file we can write to
  char* fname1 = vtkTestUtilities::ExpandDataFileName(argc, argv, "test_ug_input.vtu");
  char* fname2 = vtkTestUtilities::ExpandDataFileName(argc, argv, "test_mapped_input.vtu");

  vtkNew<vtkXMLUnstructuredGridWriter> w;
  w->SetInputData(ug);
  w->SetFileName(fname1);

  w->Update();
  if (points->GetData()->GetInformation()->Has( vtkDataArray::L2_NORM_RANGE() ) )
  {
    // for the normal unstructured grid the L2_NORM_RANGE is added. This
    // makes file comparison impossible. therefore, after the first Update()
    // remove the L2_NORM_RANGE information key and write the file again.
    points->GetData()->GetInformation()->Remove( vtkDataArray::L2_NORM_RANGE() );
  }
  w->Update();

  // create a mapped grid which basically takes the original grid
  // and uses it to map to.
  vtkNew<MappedGrid> mg;
  mg->GetImplementation()->Initialize(ug);

  vtkNew<vtkXMLUnstructuredGridWriter> w2;
  w2->SetInputData(mg);
  w2->SetFileName(fname2);
  w2->Update();

  string f1(fname1);
  string f2(fname2);

  // compare the files in appended, then ascii, then binary mode.
  bool same = compareFiles(f1, f2);
  if (!same)
  {
    return 1;
  }
  w->SetDataModeToAscii();
  w2->SetDataModeToAscii();
  w->Update();
  w2->Update();

  same = compareFiles(f1, f2);
  if (!same)
  {
    return 1;
  }
  w->SetDataModeToBinary();
  w2->SetDataModeToBinary();
  w->Update();
  w2->Update();

  same = compareFiles(f1, f2);
  if (!same)
  {
    return 1;
  }

  // clean up after ourselves: remove written files and free file names
  remove(fname1);
  remove(fname2);

  delete[] fname1;
  delete[] fname2;


  return 0;
}

// this is needed because the supporting MappedGrid.cxx file was added to the
// CMakeLists.txt file. If you know of a cleaner way to do this, please update this test.
int MappedGridImpl(int argc, char* argv[])
{
  return 0;
}
