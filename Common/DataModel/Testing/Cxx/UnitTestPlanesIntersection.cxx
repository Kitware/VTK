/*=========================================================================

  Program:   Visualization Toolkit
  Module:    UnitTestPlanesIntersection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSmartPointer.h"
#include "vtkPlanesIntersection.h"

#include "vtkTetra.h"
#include "vtkPoints.h"
#include "vtkBoundingBox.h"
#include "vtkRegularPolygonSource.h"
#include "vtkDoubleArray.h"
#include "vtkMath.h"
#include "vtkTestErrorObserver.h"

#include <sstream>
static vtkSmartPointer<vtkTetra> MakeTetra();

#define CHECK_ERROR_MSG(msg, status)                   \
  { \
  std::string expectedMsg(msg); \
  if (!errorObserver->GetError()) \
  { \
    std::cout << "Failed to catch any error. Expected the error message to contain \"" << expectedMsg << std::endl; \
    status++; \
  } \
  else \
  { \
    std::string gotMsg(errorObserver->GetErrorMessage()); \
    if (gotMsg.find(expectedMsg) == std::string::npos) \
    { \
      std::cout << "Error message does not contain \"" << expectedMsg << "\" got \n\"" << gotMsg << std::endl; \
      status++; \
    } \
  } \
  } \
  errorObserver->Clear()

int UnitTestPlanesIntersection(int, char*[])
{
  int status = 0;
  {
  vtkSmartPointer<vtkPlanesIntersection> aPlanes =
    vtkSmartPointer<vtkPlanesIntersection>::New();
  std::cout << "  Testing Print of an PlanesIntersection...";
  std::ostringstream planesPrint;
  aPlanes->Print(planesPrint);
  std::cout << "PASSED" << std::endl;
  }

  {
  std::cout << "  Testing Convert3DCell...";
  vtkSmartPointer<vtkTetra> aTetra = MakeTetra();
  vtkPlanesIntersection *aPlanes =
    vtkPlanesIntersection::Convert3DCell(aTetra);
  if (aTetra->GetNumberOfFaces() != aPlanes->GetNumberOfPlanes())
  {
    status++;
    std::cout << "FAILED" << std::endl;
  }
  else
  {
    std::cout << "PASSED" << std::endl;
  }
  aPlanes->Delete();
  }

  {
  std::cout << "  Testing Region Vertices...";
  vtkSmartPointer<vtkTetra> aTetra = MakeTetra();
  vtkPlanesIntersection *aPlanes =
    vtkPlanesIntersection::Convert3DCell(aTetra);
  int numVertices = aPlanes->GetNumberOfRegionVertices();
  if (numVertices != 4)
  {
    std::cout << " GetNumberOfRegionVertices() got " << numVertices
              << " but expected 4 ";
    std::cout << "FAILED" << std::endl;
    ++status;
  }
  else
  {
    std::cout << "PASSED" << std::endl;
  }
  aPlanes->Delete();
  }

  {
  std::cout << "  Testing PolygonIntersectsBBox...";
  int status4 = 0;
  vtkBoundingBox bbox1(-10, 10, -10, 10, -10, 10);

  // create a polygon
  vtkSmartPointer<vtkRegularPolygonSource> polygon =
    vtkSmartPointer<vtkRegularPolygonSource>::New();
  polygon->SetNumberOfSides(15);
  double center[3] = {0.0, 0.0, 0.0};
  double radius = 10.0;
  polygon->SetCenter(center);
  polygon->SetRadius(radius);
  polygon->Update();
  double bounds[6];
  bbox1.GetBounds(bounds);

  int result = vtkPlanesIntersection::PolygonIntersectsBBox(
    bounds,
    polygon->GetOutput()->GetPoints());
  if (result == 0)
  {
    ++status4;
    std::cout << " PolygonIntersectsBBox() fails bbox contains ";
  }

  // bbox outside
  vtkBoundingBox bbox3(100, 200, 100, 200, 100, 200);
  bbox3.GetBounds(bounds);

  result = vtkPlanesIntersection::PolygonIntersectsBBox(
    bounds,
    polygon->GetOutput()->GetPoints());
  if (result != 0)
  {
    ++status4;
    std::cout << " PolygonIntersectsBBox() fils bbox outside ";
  }

  // bbox straddles
  vtkBoundingBox bbox2(0, 200, 0, 200, 0, 200);
  bbox2.GetBounds(bounds);

  result = vtkPlanesIntersection::PolygonIntersectsBBox(
    bounds,
    polygon->GetOutput()->GetPoints());
  if (result != 0)
  {
    ++status4;
    std::cout << " PolygonIntersectsBBox() fils bbox outside ";
  }

  if (status4)
  {
    std::cout << "FAILED" << std::endl;
  }
  else
  {
    std::cout << "PASSED" << std::endl;
  }
  }

  {
  std::cout << "  Testing IntersectsRegion...";
  int status2 = 0;
  vtkBoundingBox bbox(-10, 10, -10, 10, -10, 10);
  double xmin, xmax, ymin, ymax, zmin, zmax;
  bbox.GetMinPoint(xmin, ymin, zmin);
  bbox.GetMaxPoint(xmax, ymax, zmax);

  vtkSmartPointer<vtkPoints> points =
    vtkSmartPointer<vtkPoints>::New();

  points->InsertNextPoint(xmin, ymin, zmin);
  points->InsertNextPoint(xmax, ymin, zmin);
  points->InsertNextPoint(xmax, ymax, zmin);
  points->InsertNextPoint(xmin, ymax, zmin);

  points->InsertNextPoint(xmin, ymin, zmax);
  points->InsertNextPoint(xmax, ymin, zmax);
  points->InsertNextPoint(xmax, ymax, zmax);
  points->InsertNextPoint(xmin, ymax, zmax);

  vtkSmartPointer<vtkTetra> aTetra = MakeTetra();
  vtkPlanesIntersection *aPlanes =
    vtkPlanesIntersection::Convert3DCell(aTetra);
  std::ostringstream planesPrint;
  aPlanes->Print(planesPrint);

  if (aPlanes->IntersectsRegion(points) == 0)
  {
    ++status2;
  }
  points->SetPoint(0, -.01, -.01, -.01);
  points->SetPoint(1, .01, -.01, -.01);
  points->SetPoint(2, .01, .01, -.01);
  points->SetPoint(3, -.01, .01, -.01);

  points->SetPoint(4, -.01, -.01, .01);
  points->SetPoint(5, .01, -.01, .01);
  points->SetPoint(6, .01, .01, .01);
  points->SetPoint(7, -.01, .01, .01);
  points->Modified();
  // box is entirely inside
  if (aPlanes->IntersectsRegion(points) != 1)
  {
    ++status2;
  }

  points->SetPoint(0, 1000.0, 1000.0, 1000.0);
  points->SetPoint(1, 2000.0, 1000.0, 1000.0);
  points->SetPoint(2, 2000.0, 2000.0, 1000.0);
  points->SetPoint(3, 1000.0, 2000.0, 1000.0);

  points->SetPoint(4, 1000.0, 1000.0, 2000.0);
  points->SetPoint(5, 2000.0, 1000.0, 2000.0);
  points->SetPoint(6, 2000.0, 2000.0, 2000.0);
  points->SetPoint(7, 1000.0, 2000.0, 2000.0);
  points->Modified();

  // box is entirely outside
  if (aPlanes->IntersectsRegion(points) != 0)
  {
    std::cout << "Box entirely outside failed ";
    ++status2;
  }
  points->SetPoint(0, 0.0, 0.0, 0.0);
  points->SetPoint(1, 10.0, 0.0, 0.0);
  points->SetPoint(2, 10.0, 10.0, 0.0);
  points->SetPoint(3, 0.0, 10.0, 0.0);

  points->SetPoint(4, 0.0, 0.0, 10.0);
  points->SetPoint(5, 10.0, 0.0, 10.0);
  points->SetPoint(6, 10.0, 10.0, 10.0);
  points->SetPoint(7, 0.0, 10.0, 10.0);
  points->Modified();

  // box straddles region
  if (aPlanes->IntersectsRegion(points) != 1)
  {
    std::cout << "Box straddling region failed ";
    ++status2;
  }

  aPlanes->Delete();
  if (status2)
  {
    std::cout << "FAILED" << std::endl;
    ++status;
  }
  else
  {
    std::cout << "PASSED" << std::endl;
  }
  }

  {
  std::cout << "  Testing Set/GetRegionVertices...";
  int status3 = 0;

  vtkSmartPointer<vtkTetra> aTetra = MakeTetra();
  vtkPlanesIntersection *aPlanes =
    vtkPlanesIntersection::Convert3DCell(aTetra);
  int numberOfRegionVertices = aPlanes->GetNumRegionVertices();
  std::vector<double> regionVertices(numberOfRegionVertices * 3);

  int got = aPlanes->GetRegionVertices(
    &(*regionVertices.begin()),
    numberOfRegionVertices);
  if (got != numberOfRegionVertices)
  {
    ++status3;
    std::cout << " GetRegionVertices() got " << got
              << " but expected " << numberOfRegionVertices << " ";
  }
  aPlanes->SetRegionVertices(
    &(*regionVertices.begin()),
    numberOfRegionVertices);
  // Repeat to exercise Delete()
  aPlanes->SetRegionVertices(
    &(*regionVertices.begin()),
    numberOfRegionVertices);

 // Ask for fewer region vertices
  got = aPlanes->GetRegionVertices(
    &(*regionVertices.begin()),
    1);
  if (got != 1)
  {
    ++status3;
    std::cout << " GetRegionVertices() got " << got
              << " but expected 1 ";
  }

  vtkSmartPointer<vtkPlanesIntersection> regionPlane =
    vtkSmartPointer<vtkPlanesIntersection>::New();
  vtkSmartPointer<vtkPoints> rpoints =
    vtkSmartPointer<vtkPoints>::New();
  rpoints->InsertNextPoint  (-1.0, 0.0, 0.0);
  rpoints->InsertNextPoint  (1.0, 0.0, 0.0);
  rpoints->InsertNextPoint  (0.0, -1.0, 0.0);
  rpoints->InsertNextPoint  (0.0, 1.0, 0.0);
  rpoints->InsertNextPoint  (0.0, 0.0, -1.0);
  rpoints->InsertNextPoint  (0.0, 0.0, 1.0);
  regionPlane->SetRegionVertices(rpoints);

  // Repeat to test Delete()
  regionPlane->SetRegionVertices(rpoints);

  vtkSmartPointer<vtkTest::ErrorObserver>  errorObserver =
    vtkSmartPointer<vtkTest::ErrorObserver>::New();
  double v;
  vtkSmartPointer<vtkPlanesIntersection> empty =
    vtkSmartPointer<vtkPlanesIntersection>::New();
  empty->AddObserver(vtkCommand::ErrorEvent, errorObserver);
  empty->GetRegionVertices(&v, 0);
  CHECK_ERROR_MSG("invalid region", status3);

  if (status3)
  {
    ++status;
    std::cout << "FAILED" << std::endl;
  }
  else
  {
    std::cout << "PASSED" << std::endl;
  }
  aPlanes->Delete();
  }

  {
  std::cout << "  Testing SetRegionVertices...";
  int status5 = 0;
  vtkSmartPointer<vtkPlanesIntersection> aPlanes =
    vtkSmartPointer<vtkPlanesIntersection>::New();
  vtkSmartPointer<vtkPoints> points =
    vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkDoubleArray> normals =
    vtkSmartPointer<vtkDoubleArray>::New();
  normals->SetNumberOfComponents(3);

  points->InsertNextPoint  (-1.0, 0.0, 0.0);
  normals->InsertNextTuple3(-1.0, 0.0, 0.0);
  points->InsertNextPoint  (1.0, 0.0, 0.0);
  normals->InsertNextTuple3(1.0, 0.0, 0.0);
  points->InsertNextPoint  (0.0, -1.0, 0.0);
  normals->InsertNextTuple3(0.0, -1.0, 0.0);
  points->InsertNextPoint  (0.0, 1.0, 0.0);
  normals->InsertNextTuple3(0.0, 1.0, 0.0);
  points->InsertNextPoint  (0.0, 0.0, -1.0);
  normals->InsertNextTuple3(0.0, 0.0, -1.0);
  points->InsertNextPoint  (0.0, 0.0, 1.0);
  normals->InsertNextTuple3(0.0, 0.0, 1.0);
  aPlanes->SetPoints(points);
  aPlanes->SetNormals(normals);

  int numberOfRegionVertices = aPlanes->GetNumRegionVertices();
  if (numberOfRegionVertices != 8)
  {
    ++status5;
    std::cout << " GetNumRegionVertices() got " << numberOfRegionVertices
              << " but expected 8 ";
  }
  std::vector<double> regionVertices(numberOfRegionVertices * 3);

  aPlanes->GetRegionVertices(
    &(*regionVertices.begin()),
    numberOfRegionVertices);
  aPlanes->SetRegionVertices(
    &(*regionVertices.begin()),
    numberOfRegionVertices);

  if (status5)
  {
    ++status;
    std::cout << "FAILED" << std::endl;
  }
  else
  {
    std::cout << "PASSED" << std::endl;
  }
  }

  {
  std::cout << "  Testing IntersectsRegion Errors...";
  vtkSmartPointer<vtkTest::ErrorObserver>  errorObserver =
    vtkSmartPointer<vtkTest::ErrorObserver>::New();

  vtkBoundingBox bbox(-10, 10, -10, 10, -10, 10);
  double xmin, xmax, ymin, ymax, zmin, zmax;
  bbox.GetMinPoint(xmin, ymin, zmin);
  bbox.GetMaxPoint(xmax, ymax, zmax);

  vtkSmartPointer<vtkPoints> points =
    vtkSmartPointer<vtkPoints>::New();

  points->InsertNextPoint(xmin, ymin, zmin);
  points->InsertNextPoint(xmax, ymin, zmin);
  points->InsertNextPoint(xmax, ymax, zmin);
  points->InsertNextPoint(xmin, ymax, zmin);

  points->InsertNextPoint(xmin, ymin, zmax);
  points->InsertNextPoint(xmax, ymin, zmax);
  points->InsertNextPoint(xmax, ymax, zmax);
  points->InsertNextPoint(xmin, ymax, zmax);

  // empty planes
  vtkSmartPointer<vtkPlanesIntersection> empty =
    vtkSmartPointer<vtkPlanesIntersection>::New();
  empty->AddObserver(vtkCommand::ErrorEvent, errorObserver);

  int status1 = 0;
  if (empty->IntersectsRegion(points) != 0)
  {
    ++status1;
    std::cout << "FAILED" << std::endl;
  }
  else
  {
    CHECK_ERROR_MSG("invalid region - less than 4 planes", status1);
  }

  // Invalid Region
  vtkSmartPointer<vtkPlanesIntersection> invalidRegion =
    vtkSmartPointer<vtkPlanesIntersection>::New();
  invalidRegion->AddObserver(vtkCommand::ErrorEvent, errorObserver);

  vtkSmartPointer<vtkPoints> npoints =
    vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkDoubleArray> normals =
    vtkSmartPointer<vtkDoubleArray>::New();
  normals->SetNumberOfComponents(3);

  npoints->InsertNextPoint  (-1.0, 0.0, 0.0);
  normals->InsertNextTuple3(-1.0, 0.0, 0.0);
  npoints->InsertNextPoint  (-1.0, 0.0, 0.0);
  normals->InsertNextTuple3(-1.0, 0.0, 0.0);
  npoints->InsertNextPoint  (-1.0, 0.0, 0.0);
  normals->InsertNextTuple3(-1.0, 0.0, 0.0);
  npoints->InsertNextPoint  (-1.0, 0.0, 0.0);
  normals->InsertNextTuple3(-1.0, 0.0, 0.0);
  invalidRegion->SetPoints(npoints);
  invalidRegion->SetNormals(normals);

  if (invalidRegion->IntersectsRegion(points) != 0)
  {
    ++status1;
    std::cout << "FAILED" << std::endl;
  }
  else
  {
    CHECK_ERROR_MSG("Invalid region: zero-volume intersection", status1);
  }
  vtkSmartPointer<vtkPlanesIntersection> invalidBox =
    vtkSmartPointer<vtkPlanesIntersection>::New();
  invalidBox->AddObserver(vtkCommand::ErrorEvent, errorObserver);

  vtkSmartPointer<vtkPoints> points2 =
    vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkDoubleArray> normals2 =
    vtkSmartPointer<vtkDoubleArray>::New();
  normals2->SetNumberOfComponents(3);

  points2->InsertNextPoint  (-1.0, 0.0, 0.0);
  normals2->InsertNextTuple3(-1.0, 0.0, 0.0);
  points2->InsertNextPoint  (1.0, 0.0, 0.0);
  normals2->InsertNextTuple3(1.0, 0.0, 0.0);
  points2->InsertNextPoint  (0.0, -1.0, 0.0);
  normals2->InsertNextTuple3(0.0, -1.0, 0.0);
  points2->InsertNextPoint  (0.0, 1.0, 0.0);
  normals2->InsertNextTuple3(0.0, 1.0, 0.0);
  points2->InsertNextPoint  (0.0, 0.0, -1.0);
  normals2->InsertNextTuple3(0.0, 0.0, -1.0);
  points2->InsertNextPoint  (0.0, 0.0, 1.0);
  normals2->InsertNextTuple3(0.0, 0.0, 1.0);
  invalidBox->SetPoints(points2);
  invalidBox->SetNormals(normals2);

  vtkSmartPointer<vtkPoints> badBox =
    vtkSmartPointer<vtkPoints>::New();

  badBox->InsertNextPoint(xmin, ymin, zmin);
  badBox->InsertNextPoint(xmax, ymin, zmin);
  badBox->InsertNextPoint(xmax, ymax, zmin);
  badBox->InsertNextPoint(xmin, ymax, zmin);

  badBox->InsertNextPoint(xmin, ymin, zmax);
  badBox->InsertNextPoint(xmax, ymin, zmax);
  badBox->InsertNextPoint(xmax, ymax, zmax);

  if (invalidBox->IntersectsRegion(badBox) != 0)
  {
    ++status1;
    std::cout << "FAILED" << std::endl;
  }
  else
  {
    CHECK_ERROR_MSG("invalid box", status1);
  }

  if (status1)
  {
    ++status;
    std::cout << "FAILED" << std::endl;
  }
  else
  {
    std::cout << "PASSED" << std::endl;
  }
  }

  if (status)
  {
    return EXIT_FAILURE;
  }
  else
  {
    return EXIT_SUCCESS;
  }
}


vtkSmartPointer<vtkTetra> MakeTetra()
{
  vtkSmartPointer<vtkTetra> aTetra =
    vtkSmartPointer<vtkTetra>::New();
  aTetra->GetPointIds()->SetId(0,0);
  aTetra->GetPointIds()->SetId(1,1);
  aTetra->GetPointIds()->SetId(2,2);
  aTetra->GetPointIds()->SetId(3,3);
  aTetra->GetPoints()->SetPoint(0, -1.0, -1.0, -1.0);
  aTetra->GetPoints()->SetPoint(1,  1.0, -1.0, -1.0);
  aTetra->GetPoints()->SetPoint(2,  0.0,  1.0, -1.0);
  aTetra->GetPoints()->SetPoint(3,  0.5,  0.5,  1.0);
  return aTetra;
}
