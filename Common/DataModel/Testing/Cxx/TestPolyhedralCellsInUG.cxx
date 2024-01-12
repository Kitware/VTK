/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPolyhedron.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkActor.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCubeSource.h"
#include "vtkDataArray.h"
#include "vtkDataSetMapper.h"
#include "vtkElevationFilter.h"
#include "vtkExtractEdges.h"
#include "vtkGenericCell.h"
#include "vtkIdList.h"
#include "vtkPointData.h"
#include "vtkPointLocator.h"
#include "vtkPoints.h"
#include "vtkPolyhedron.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkShrinkFilter.h"
#include "vtkSmartPointer.h"
#include "vtkStructuredGridReader.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLUnstructuredGridReader.h"
#include "vtkXMLUnstructuredGridWriter.h"

#include "vtkRegressionTestImage.h"
#include "vtkTestUtilities.h"
#include <vtksys/SystemTools.hxx>

#if !defined(TESTING_DEPRECATED_SUPRESS_SUPPORTED)
#if defined(__GNUC__)
#define TESTING_DEPRECATED_SUPRESS_SUPPORTED
#define TESTING_DEPRECATED_SUPRESS_BEGIN                                                           \
  _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wdeprecated-declarations\"")
#define TESTING_DEPRECATED_SUPRESS_END _Pragma("GCC diagnostic pop")
#elif defined(__clang__) && !defined(__INTEL_COMPILER)
#define TESTING_DEPRECATED_SUPRESS_SUPPORTED
#define TESTING_DEPRECATED_SUPRESS_BEGIN                                                           \
  _Pragma("clang diagnostic push") _Pragma("clang diagnostic ignored "                             \
                                           "\"-Wdeprecated-declarations\"")
#define TESTING_DEPRECATED_SUPRESS_END _Pragma("clang diagnostic pop")
#elif defined(_MSC_VER)
#define TESTING_DEPRECATED_SUPRESS_SUPPORTED
#define TESTING_DEPRECATED_SUPRESS_BEGIN __pragma(warning(push)) __pragma(warning(disable : 4996))
#define TESTING_DEPRECATED_SUPRESS_END __pragma(warning(pop))
#elif defined(__INTEL_COMPILER) && !defined(__ICL)
#define TESTING_DEPRECATED_SUPRESS_SUPPORTED
#define TESTING_DEPRECATED_SUPRESS_BEGIN                                                           \
  _Pragma("warning(push)") _Pragma("warning(disable:1478 1786)")
#define TESTING_DEPRECATED_SUPRESS_END _Pragma("warning(pop)")
#else
#define TESTING_DEPRECATED_SUPRESS_SUPPORTED
#define TESTING_DEPRECATED_SUPRESS_BEGIN
#define TESTING_DEPRECATED_SUPRESS_END
#endif
#endif

namespace
{
bool compare_doublevec(const double x[3], const double y[3], const double e)
{
  return (((x[0] - y[0]) < (e)) && ((x[0] - y[0]) > -(e)) && ((x[1] - y[1]) < (e)) &&
    ((x[1] - y[1]) > -(e)) && ((x[2] - y[2]) < (e)) && ((x[2] - y[2]) > -(e)));
}

bool compare_double(const double x, const double y, const double e)
{
  return ((x - y) < (e) && (x - y) > -(e));
}
}

// Test of vtkUnstructuredGrid support for Polyhedral Cells.
// A structured grid is converted to a polyhedral mesh.
int TestPolyhedralCellsInUG(int argc, char* argv[])
{
  const double tol = 0.001;
  double p1[3] = { -100, 0, 0 };
  double p2[3] = { 100, 0, 0 };
  double t, x[3], pc[3];
  int subId = 0;

  // create the a cube
  vtkNew<vtkCubeSource> cube;
  cube->SetXLength(10);
  cube->SetYLength(10);
  cube->SetZLength(20);
  cube->SetCenter(0, 0, 0);
  cube->Update();

  // add scaler
  vtkNew<vtkElevationFilter> ele;
  ele->SetInputConnection(cube->GetOutputPort());
  ele->SetLowPoint(0, 0, -10);
  ele->SetHighPoint(0, 0, 10);
  ele->Update();
  vtkPolyData* poly = vtkPolyData::SafeDownCast(ele->GetOutput());

  // create a test polyhedron
  vtkIdType pointIds[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };

  vtkNew<vtkCellArray> faces;
  vtkIdType face0[4] = { 0, 2, 6, 4 };
  vtkIdType face1[4] = { 1, 3, 7, 5 };
  vtkIdType face2[4] = { 0, 1, 3, 2 };
  vtkIdType face3[4] = { 4, 5, 7, 6 };
  vtkIdType face4[4] = { 0, 1, 5, 4 };
  vtkIdType face5[4] = { 2, 3, 7, 6 };
  faces->InsertNextCell(4, face0);
  faces->InsertNextCell(4, face1);
  faces->InsertNextCell(4, face2);
  faces->InsertNextCell(4, face3);
  faces->InsertNextCell(4, face4);
  faces->InsertNextCell(4, face5);

  vtkNew<vtkCellArray> faceLocations;
  vtkIdType faceIds[6] = { 0, 1, 2, 3, 4, 5 };
  faceLocations->InsertNextCell(6, faceIds);

  vtkNew<vtkCellArray> cells;
  cells->InsertNextCell(8, pointIds);

  vtkNew<vtkUnsignedCharArray> cellTypes;
  cellTypes->InsertNextValue(VTK_POLYHEDRON);

  vtkNew<vtkUnstructuredGrid> ugrid0;
  ugrid0->SetPoints(poly->GetPoints());
  ugrid0->GetPointData()->ShallowCopy(poly->GetPointData());

  ugrid0->SetPolyhedralCells(cellTypes, cells, faceLocations, faces);

  vtkCellArray* facesHandle = ugrid0->GetPolyhedronFaces();
  vtkCellArray* faceLocationsHandle = ugrid0->GetPolyhedronFaceLocations();

  vtkIdTypeArray* faceStream = nullptr;
  vtkIdTypeArray* faceStreamLocations = nullptr;
  // disable warning while testing backward compatibility layer
  TESTING_DEPRECATED_SUPRESS_BEGIN
  faceStreamLocations = ugrid0->GetFaceLocations();
  faceStream = ugrid0->GetFaces();
  TESTING_DEPRECATED_SUPRESS_END

  // check Legacy cache is correct
  if (faceStreamLocations->GetNumberOfTuples() != faceLocationsHandle->GetNumberOfCells())
  {
    std::cout << "Error Legacy backward compatibility layer is not coherent for faceLocations."
              << std::endl;
    return EXIT_FAILURE;
  }
  if (faceStream->GetValue(1) != facesHandle->GetCellSize(0))
  {
    std::cout << "Error Legacy backward compatibility layer is not coherent for faces."
              << std::endl;
    return EXIT_FAILURE;
  }

  vtkNew<vtkUnstructuredGrid> ugrid1;
  ugrid1->SetPoints(poly->GetPoints());
  ugrid1->GetPointData()->DeepCopy(poly->GetPointData());
  // disable warning because we are testing backward compatibility layer
  TESTING_DEPRECATED_SUPRESS_BEGIN
  ugrid1->SetCells(cellTypes, cells, faceStreamLocations, faceStream);
  TESTING_DEPRECATED_SUPRESS_END

  vtkPolyhedron* polyhedron = vtkPolyhedron::SafeDownCast(ugrid0->GetCell(0));
  if (!polyhedron)
  {
    std::cerr << "SafeDownCast to vtkPolyhedron failed" << std::endl;
    return EXIT_FAILURE;
  }

  vtkCellArray* cell = ugrid0->GetCells();
  vtkNew<vtkIdTypeArray> pids;
  cell->ExportLegacyFormat(pids);

  std::cout << "num of cells: " << cell->GetNumberOfCells() << std::endl;
  std::cout << "num of tuples: " << pids->GetNumberOfTuples() << std::endl;
  for (int i = 0; i < pids->GetNumberOfTuples(); i++)
  {
    std::cout << pids->GetValue(i) << " ";
  }
  std::cout << std::endl;
  cell->Print(std::cout);

  vtkPolyhedron* polyhedron_copy = vtkPolyhedron::SafeDownCast(ugrid1->GetCell(0));
  if (!polyhedron_copy)
  {
    std::cerr << "SafeDownCast to vtkPolyhedron failed" << std::endl;
    return EXIT_FAILURE;
  }

  vtkCellArray* cell_copy = ugrid1->GetCells();
  vtkNew<vtkIdTypeArray> pids_copy;
  cell_copy->ExportLegacyFormat(pids_copy);

  std::cout << "Deepcopy num of cells: " << cell_copy->GetNumberOfCells() << std::endl;
  std::cout << "Deepcopy num of tuples: " << pids_copy->GetNumberOfTuples() << std::endl;
  for (int i = 0; i < pids_copy->GetNumberOfTuples(); i++)
  {
    std::cout << pids_copy->GetValue(i) << " ";
  }
  std::cout << std::endl;
  cell_copy->Print(std::cout);

  // Print out basic information
  std::cout << "Testing polyhedron is a cube of with bounds "
            << "[-5, 5, -5, 5, -10, 10]. It has " << polyhedron->GetNumberOfEdges() << " edges and "
            << polyhedron->GetNumberOfFaces() << " faces." << std::endl;

  std::cout << "Testing polyhedron deepcopy is a cube of with bounds "
            << "[-5, 5, -5, 5, -10, 10]. It has " << polyhedron_copy->GetNumberOfEdges()
            << " edges and " << polyhedron_copy->GetNumberOfFaces() << " faces." << std::endl;

  //
  // test writer
  //
  // Delete any existing files to prevent false failures
  if (vtksys::SystemTools::FileExists("test.vtu"))
  {
    vtksys::SystemTools::RemoveFile("test.vtu");
  }
  vtkNew<vtkXMLUnstructuredGridWriter> writer;
  writer->SetInputData(ugrid0);
  writer->SetFileName("test.vtu");
  writer->SetDataModeToAscii();
  writer->Update();
  std::cout << "finished writing the polyhedron mesh to test.vtu " << std::endl;

  //
  // test reader
  vtkNew<vtkXMLUnstructuredGridReader> reader;
  reader->SetFileName("test.vtu");
  reader->Update();
  std::cout << "finished reading the polyhedron mesh from test.vtu " << std::endl;

  vtkUnstructuredGrid* ugrid = reader->GetOutput();
  polyhedron = vtkPolyhedron::SafeDownCast(ugrid->GetCell(0));
  if (!polyhedron)
  {
    std::cerr << "SafeDownCast to vtkPolyhedron failed" << std::endl;
    return EXIT_FAILURE;
  }

  // Delete any existing files to prevent false failures
  if (vtksys::SystemTools::FileExists("test1.vtu"))
  {
    vtksys::SystemTools::RemoveFile("test1.vtu");
  }
  // write again to help compare
  writer->SetInputData(ugrid);
  writer->SetFileName("test1.vtu");
  writer->SetDataModeToAscii();
  writer->Update();

  // test the polyhedron functions
  // test intersection
  int hit = polyhedron->IntersectWithLine(p1, p2, tol, t, x, pc, subId); // should hit
  if (!hit)
  {
    cerr << "Expected  intersection, but missed." << std::endl;
    return EXIT_FAILURE;
  }

  // test inside
  int inside = polyhedron->IsInside(p1, tol); // should be out
  if (inside)
  {
    cerr << "Expect point [" << p1[0] << ", " << p1[1] << ", " << p1[2]
         << "] to be outside the polyhedral, but it's inside." << std::endl;
    return EXIT_FAILURE;
  }

  p2[0] = 0.0;
  p2[1] = 0.0;
  p2[2] = 0.0;
  inside = polyhedron->IsInside(p2, tol); // should be in
  if (!inside)
  {
    cerr << "Expect point [" << p2[0] << ", " << p2[1] << ", " << p2[2]
         << "] to be inside the polyhedral, but it's outside." << std::endl;
    return EXIT_FAILURE;
  }

  // test EvaluatePosition and interpolation function
  double weights[8], closestPoint[3], dist2;

  for (int i = 0; i < 8; i++)
  {
    double v;
    poly->GetPointData()->GetScalars()->GetTuple(i, &v);
    std::cout << v << " ";
  }
  std::cout << std::endl;

  // case 0: point on the polyhedron
  x[0] = 5.0;
  x[1] = 0.0;
  x[2] = 0.0;
  polyhedron->EvaluatePosition(x, closestPoint, subId, pc, dist2, weights);

  std::cout << "weights for point [" << x[0] << ", " << x[1] << ", " << x[2] << "]:" << std::endl;
  for (int i = 0; i < 8; i++)
  {
    std::cout << weights[i] << " ";
  }
  std::cout << std::endl;

  double refWeights[8] = { 0.0, 0.0, 0.0, 0.0, 0.25, 0.25, 0.25, 0.25 };
  for (int i = 0; i < 8; i++)
  {
    if (!::compare_double(refWeights[i], weights[i], tol * 0.01))
    {
      std::cout << "Error computing the weights for a point on the polyhedron." << std::endl;
      return EXIT_FAILURE;
    }
  }

  double refClosestPoint[3] = { 5.0, 0.0, 0.0 };
  if (!::compare_doublevec(closestPoint, refClosestPoint, tol * 0.01))
  {
    std::cout << "Error finding the closet point of a point on the polyhedron." << std::endl;
    return EXIT_FAILURE;
  }

  double refDist2 = 0.0;
  if (!::compare_double(dist2, refDist2, tol * 0.001))
  {
    std::cout << "Error computing the distance for a point on the polyhedron." << std::endl;
    return EXIT_FAILURE;
  }

  // case 1: point inside the polyhedron
  x[0] = 0.0;
  x[1] = 0.0;
  x[2] = 0.0;
  polyhedron->EvaluatePosition(x, closestPoint, subId, pc, dist2, weights);

  std::cout << "weights for point [" << x[0] << ", " << x[1] << ", " << x[2] << "]:" << std::endl;
  for (int i = 0; i < 8; i++)
  {
    std::cout << weights[i] << " ";
  }
  std::cout << std::endl;

  double refWeights1[8] = { 0.125, 0.125, 0.125, 0.125, 0.125, 0.125, 0.125, 0.125 };
  for (int i = 0; i < 8; i++)
  {
    if (!::compare_double(refWeights1[i], weights[i], tol * 0.01))
    {
      std::cout << "Error computing the weights for a point inside the polyhedron." << std::endl;
      return EXIT_FAILURE;
    }
  }

  if (!::compare_double(dist2, refDist2, tol * 0.001))
  {
    std::cout << "Error computing the distance for a point inside the polyhedron." << std::endl;
    return EXIT_FAILURE;
  }

  // case 2: point outside the polyhedron
  x[0] = 8.0;
  x[1] = 0.0;
  x[2] = 0.0;
  polyhedron->EvaluatePosition(x, closestPoint, subId, pc, dist2, weights);

  std::cout << "weights for point [" << x[0] << ", " << x[1] << ", " << x[2] << "]:" << std::endl;
  for (int i = 0; i < 8; i++)
  {
    std::cout << weights[i] << " ";
  }
  std::cout << std::endl;

  double refWeights2[8] = { 0.0307, 0.0307, 0.0307, 0.0307, 0.2193, 0.2193, 0.2193, 0.2193 };
  for (int i = 0; i < 8; i++)
  {
    if (!::compare_double(refWeights2[i], weights[i], tol * 0.1))
    {
      std::cout << "Error computing the weights for a point outside the polyhedron." << std::endl;
      return EXIT_FAILURE;
    }
  }

  if (!::compare_doublevec(closestPoint, refClosestPoint, tol * 0.01))
  {
    std::cout << "Error finding the closet point of a point outside the polyhedron." << std::endl;
    return EXIT_FAILURE;
  }

  refDist2 = 9.0;
  if (!::compare_double(dist2, refDist2, tol * 0.001))
  {
    std::cout << "Error computing the distance for a point outside the polyhedron." << std::endl;
    return EXIT_FAILURE;
  }

  // test evaluation location
  double weights1[8] = { 0.0 };
  polyhedron->EvaluateLocation(subId, pc, x, weights1);

  double refPoint[3] = { 8.0, 0.0, 0.0 };
  if (!::compare_doublevec(refPoint, x, tol * 0.01))
  {
    std::cout << "Error evaluate the point location for its parameter coordinate." << std::endl;
    return EXIT_FAILURE;
  }

  for (int i = 0; i < 8; i++)
  {
    if (!::compare_double(refWeights2[i], weights1[i], tol * 0.1))
    {
      std::cout << "Error computing the weights based on parameter coordinates." << std::endl;
      return EXIT_FAILURE;
    }
  }

  // test derivative
  pc[0] = 0;
  pc[1] = 0.5;
  pc[2] = 0.5;
  polyhedron->EvaluateLocation(subId, pc, x, weights1);

  double deriv[3] = { 0.0 };
  double values[8] = { 0.0 };
  vtkDataArray* dataArray = poly->GetPointData()->GetScalars();
  for (int i = 0; i < 8; i++)
  {
    dataArray->GetTuple(i, values + i);
  }
  polyhedron->Derivatives(subId, pc, values, 1, deriv);

  std::cout << "derivative for point [" << x[0] << ", " << x[1] << ", " << x[2]
            << "]:" << std::endl;
  for (int i = 0; i < 3; i++)
  {
    std::cout << deriv[i] << " ";
  }
  std::cout << std::endl;

  double refDeriv[3] = { 0.0, 0.0, 0.05 };
  if (!::compare_doublevec(refDeriv, deriv, tol * 0.01))
  {
    std::cout << "Error computing derivative for a point inside the polyhedron." << std::endl;
    return EXIT_FAILURE;
  }

  // test triangulation
  vtkNew<vtkPoints> tetraPoints;
  vtkNew<vtkIdList> tetraIdList;
  polyhedron->Triangulate(0, tetraIdList, tetraPoints);

  std::cout << std::endl << "Triangulation result:" << std::endl;

  for (int i = 0; i < tetraPoints->GetNumberOfPoints(); i++)
  {
    double* pt = tetraPoints->GetPoint(i);
    std::cout << "point #" << i << ": [" << pt[0] << ", " << pt[1] << ", " << pt[2] << "]"
              << std::endl;
  }

  vtkIdType* ids = tetraIdList->GetPointer(0);
  for (int i = 0; i < tetraIdList->GetNumberOfIds(); i += 4)
  {
    std::cout << "tetra #" << i / 4 << ":" << ids[i] << " " << ids[i + 1] << " " << ids[i + 2]
              << " " << ids[i + 3] << std::endl;
  }

  vtkNew<vtkUnstructuredGrid> tetraGrid;
  for (int i = 0; i < tetraIdList->GetNumberOfIds(); i += 4)
  {
    tetraGrid->InsertNextCell(VTK_TETRA, 4, ids + i);
  }
  tetraGrid->SetPoints(poly->GetPoints());
  tetraGrid->GetPointData()->DeepCopy(poly->GetPointData());

  // test contour
  vtkNew<vtkPointLocator> locator;
  vtkNew<vtkCellArray> resultPolys;
  vtkNew<vtkPointData> resultPd;
  vtkNew<vtkCellData> resultCd;
  vtkNew<vtkPoints> resultPoints;
  resultPoints->ShallowCopy(ugrid0->GetPoints());
  locator->InitPointInsertion(resultPoints, ugrid0->GetBounds());

  polyhedron->Contour(0.5, tetraGrid->GetPointData()->GetScalars(), locator, nullptr, nullptr,
    resultPolys, tetraGrid->GetPointData(), resultPd, tetraGrid->GetCellData(), 0, resultCd);

  // output the contour
  vtkNew<vtkUnstructuredGrid> contourResult;
  contourResult->SetPoints(locator->GetPoints());
  contourResult->SetCells(VTK_POLYGON, resultPolys);
  contourResult->GetPointData()->DeepCopy(resultPd);

  // test clip
  vtkNew<vtkPointLocator> locator1;
  vtkNew<vtkCellArray> resultPolys1;
  vtkNew<vtkPointData> resultPd1;
  vtkNew<vtkCellData> resultCd1;
  vtkNew<vtkPoints> resultPoints1;
  resultPoints1->DeepCopy(ugrid0->GetPoints());
  locator1->InitPointInsertion(resultPoints1, ugrid0->GetBounds());

  polyhedron->Clip(0.5, tetraGrid->GetPointData()->GetScalars(), locator1, resultPolys1,
    tetraGrid->GetPointData(), resultPd1, tetraGrid->GetCellData(), 0, resultCd1, 0);

  // output the clipped polyhedron
  vtkNew<vtkUnstructuredGrid> clipResult;
  clipResult->SetPoints(locator1->GetPoints());
  clipResult->SetCells(VTK_POLYHEDRON, resultPolys1);
  clipResult->GetPointData()->DeepCopy(resultPd1);

  // shrink to show the gaps between tetrahedrons.
  vtkNew<vtkShrinkFilter> shrink;
  shrink->SetInputData(tetraGrid);
  shrink->SetShrinkFactor(0.7);

  // create actors
  vtkNew<vtkDataSetMapper> mapper;
  mapper->SetInputData(poly);

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  vtkNew<vtkDataSetMapper> contourMapper;
  contourMapper->SetInputData(contourResult);

  vtkNew<vtkActor> contourActor;
  contourActor->SetMapper(contourMapper);

  vtkNew<vtkDataSetMapper> clipPolyhedronMapper;
  clipPolyhedronMapper->SetInputData(clipResult);

  vtkNew<vtkActor> clipPolyhedronActor;
  clipPolyhedronActor->SetMapper(clipPolyhedronMapper);

  // Create rendering infrastructure
  vtkNew<vtkProperty> prop;
  prop->LightingOff();
  prop->SetRepresentationToSurface();
  prop->EdgeVisibilityOn();
  prop->SetLineWidth(3.0);
  prop->SetOpacity(0.8);

  // set property
  actor->SetProperty(prop);
  contourActor->SetProperty(prop);
  clipPolyhedronActor->SetProperty(prop);

  vtkNew<vtkRenderer> ren;
  ren->AddActor(actor);
  ren->AddActor(contourActor);
  ren->AddActor(clipPolyhedronActor);
  ren->SetBackground(.5, .5, .5);

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(ren);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  iren->Initialize();

  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  // Clean the test produced files if all went well
  if (vtksys::SystemTools::FileExists("test1.vtu") && retVal != EXIT_FAILURE)
  {
    vtksys::SystemTools::RemoveFile("test1.vtu");
  }
  if (vtksys::SystemTools::FileExists("test1.vtu") && retVal != EXIT_FAILURE)
  {
    vtksys::SystemTools::RemoveFile("test1.vtu");
  }
  return !retVal;
}
