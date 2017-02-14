/*=========================================================================

  Program:   Visualization Toolkit
  Module:    UnitTestDataSetSurfaceFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSmartPointer.h"

#include "vtkDataSetSurfaceFilter.h"

#include "vtkAppendFilter.h"
#include "vtkPlaneSource.h"
#include "vtkRegularPolygonSource.h"
#include "vtkTriangleFilter.h"
#include "vtkStripper.h"

#include "vtkUnstructuredGrid.h"
#include "vtkRectilinearGrid.h"
#include "vtkUniformGrid.h"
#include "vtkStructuredGrid.h"

#include "vtkPoints.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkPointLocator.h"
#include "vtkDoubleArray.h"

#include "vtkTetra.h"
#include "vtkHexahedron.h"
#include "vtkPolyData.h"
#include "vtkPolyLine.h"
#include "vtkQuadraticWedge.h"
#include "vtkGenericCell.h"

#include "vtkCommand.h"
#include "vtkTestErrorObserver.h"

#include <sstream>
#include <map>

static vtkSmartPointer<vtkDataSet> CreatePolyData(const int xres, const int yres);
static vtkSmartPointer<vtkDataSet> CreateTriangleStripData(const int xres, const int yres);
static vtkSmartPointer<vtkDataSet> CreateTetraData();
static vtkSmartPointer<vtkDataSet> CreatePolygonData(int sides = 6);
static vtkSmartPointer<vtkDataSet> CreateQuadraticWedgeData();
static vtkSmartPointer<vtkDataSet> CreateUniformGrid(unsigned int, unsigned int, unsigned int);
static vtkSmartPointer<vtkDataSet> CreateRectilinearGrid();
static vtkSmartPointer<vtkDataSet> CreateStructuredGrid(bool blank = false);
static vtkSmartPointer<vtkDataSet> CreateBadAttributes();
static vtkSmartPointer<vtkDataSet> CreateGenericCellData(int cellType);

namespace test
{
// What to expect for a cell
class CellDescription
{
  public:
  CellDescription (int cellType, int numCells)
  {
    this->Type = cellType;
    this->Cells = numCells;
  }
  CellDescription() : Type(0), Cells(0)
    {}
  int Type;
  int Cells;
};
}
int UnitTestDataSetSurfaceFilter(int, char*[])
{
  int status = EXIT_SUCCESS;
  {
  std::cout << "Testing empty print...";
  vtkSmartPointer<vtkDataSetSurfaceFilter> filter =
    vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
  std::ostringstream emptyPrint;
  filter->Print(emptyPrint);
  std::cout << "PASSED." << std::endl;
  }
  {
  std::map<std::string,test::CellDescription> typesToProcess;
  typesToProcess["Vertex"] = test::CellDescription(VTK_VERTEX, 1);
  typesToProcess["Line"] = test::CellDescription(VTK_LINE, 1);
  typesToProcess["Triangle"] = test::CellDescription(VTK_TRIANGLE, 1);
  typesToProcess["Pixel"] = test::CellDescription(VTK_PIXEL, 1);
  typesToProcess["Quad"] = test::CellDescription(VTK_QUAD, 1);
  typesToProcess["Tetra"] = test::CellDescription(VTK_TETRA, 4);
  typesToProcess["Voxel"] = test::CellDescription(VTK_VOXEL, 6);
  typesToProcess["Hexahedron"] = test::CellDescription(VTK_HEXAHEDRON, 6);
  typesToProcess["Wedge"] = test::CellDescription(VTK_WEDGE, 5);
  typesToProcess["Pyramid"] = test::CellDescription(VTK_PYRAMID, 5);
  typesToProcess["PentagonalPrism"] = test::CellDescription(VTK_PENTAGONAL_PRISM, 7);
  typesToProcess["HexagonalPrism"] = test::CellDescription(VTK_HEXAGONAL_PRISM, 8);
  typesToProcess["QuadraticEdge"] = test::CellDescription(VTK_QUADRATIC_EDGE, 2);
  typesToProcess["QuadraticTriangle"] = test::CellDescription(VTK_QUADRATIC_TRIANGLE, 1);
  typesToProcess["QuadraticQuad"] = test::CellDescription(VTK_QUADRATIC_QUAD, 1);
  typesToProcess["QuadraticTetra"] = test::CellDescription(VTK_QUADRATIC_TETRA, 16);
  typesToProcess["QuadraticHexahedron"] = test::CellDescription(VTK_QUADRATIC_HEXAHEDRON, 36);
  typesToProcess["QuadraticWedge"] = test::CellDescription(VTK_QUADRATIC_WEDGE, 26);
  typesToProcess["QuadraticPyramid"] = test::CellDescription(VTK_QUADRATIC_PYRAMID, 22);
  typesToProcess["BiQuadraticQuad"] = test::CellDescription(VTK_BIQUADRATIC_QUAD, 8);
  typesToProcess["TriQuadraticHexahedron"] = test::CellDescription(VTK_TRIQUADRATIC_HEXAHEDRON, 768);
  typesToProcess["QuadraticLinearQuad"] = test::CellDescription(VTK_QUADRATIC_LINEAR_QUAD, 4);
  typesToProcess["QuadraticLinearWedge"] = test::CellDescription(VTK_QUADRATIC_LINEAR_WEDGE, 20);
  typesToProcess["BiQuadraticQuadraticWedge"] = test::CellDescription(VTK_BIQUADRATIC_QUADRATIC_WEDGE, 32);

  std::map <std::string, test::CellDescription>::iterator it;
  for (it = typesToProcess.begin(); it != typesToProcess.end(); ++it)
  {
    std::cout << "Testing (" << it->first << ")...";
    vtkSmartPointer<vtkDataSetSurfaceFilter> filter =
      vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
    filter->SetInputData (CreateGenericCellData(it->second.Type));
    filter->PassThroughCellIdsOn();
    filter->PassThroughPointIdsOn();
    if (it->first == "QuadraticTriangle" || it->first == "QuadraticQuad")
    {
      filter->SetNonlinearSubdivisionLevel(0);
    }
    if (it->first == "TriQuadraticHexahedron")
    {
      filter->SetNonlinearSubdivisionLevel(3);
    }
    filter->Update();
    int got = filter->GetOutput()->GetNumberOfCells();
    int expected = it->second.Cells;
    if (got != expected)
    {
      std::cout << " got " << got << " cells but expected " << expected;
      std::cout << " FAILED." << std::endl;
      status++;
    }
    else
    {
      std::cout << " # of cells: " << got;
      std::cout << " PASSED." << std::endl;
    }
    std::cout.flush();
  }
  }
  {
  std::cout << "Testing default settings (PolyData)...";
  vtkSmartPointer<vtkDataSetSurfaceFilter> filter =
    vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
  filter->SetInputData (CreatePolyData(10, 20));
  filter->Update();
  int got = filter->GetOutput()->GetNumberOfCells();
  std::cout << " # of cells: " << got;
  std::cout << " PASSED." << std::endl;
  }
  {
  std::cout << "Testing (TriangleStrips)...";
  vtkSmartPointer<vtkDataSetSurfaceFilter> filter =
    vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
  filter->SetInputData (CreateTriangleStripData(10, 20));
  filter->PassThroughCellIdsOff();
  filter->PassThroughPointIdsOff();
  filter->Update();
  int got = filter->GetOutput()->GetNumberOfCells();
  std::cout << " # of cells: " << got;
  std::cout << " PASSED." << std::endl;
  }
  {
  std::cout << "Testing (PolyData Polygons)...";
  vtkSmartPointer<vtkDataSetSurfaceFilter> filter =
    vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
  filter->SetInputData (CreatePolygonData(9));
  filter->PassThroughCellIdsOn();
  filter->PassThroughPointIdsOn();
  filter->Update();
  int got = filter->GetOutput()->GetNumberOfCells();
  std::cout << " # of cells: " << got;
  std::cout << " PASSED." << std::endl;
  }
  {
  std::cout << "Testing (UnstructuredGrid, QuadraticWedge, Tetra, PassThroughCellIds, PassThroughPointIds)...";
  vtkSmartPointer<vtkAppendFilter> append =
    vtkSmartPointer<vtkAppendFilter>::New();
  append->AddInputData(CreateTetraData());
  append->AddInputData(CreateQuadraticWedgeData());

  vtkSmartPointer<vtkDataSetSurfaceFilter> filter =
    vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
  filter->SetInputConnection(append->GetOutputPort());
  filter->PassThroughCellIdsOn();
  filter->PassThroughPointIdsOn();
  filter->Update();
  int got = filter->GetOutput()->GetNumberOfCells();
  std::cout << " # of cells: " << got;
  std::cout << " PASSED." << std::endl;
  }
  {
  std::cout << "Testing (UniformGrid(5,10,1), UseStripsOn, PassThroughCellIds, PassThroughPointIds)...";
  vtkSmartPointer<vtkDataSetSurfaceFilter> filter =
    vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
  filter->SetInputData (CreateUniformGrid(5, 10, 1));
  filter->PassThroughCellIdsOn();
  filter->PassThroughPointIdsOn();
  filter->UseStripsOn();
  filter->Update();
  int got = filter->GetOutput()->GetNumberOfCells();
  std::cout << " # of cells: " << got;
  std::ostringstream fullPrint;
  filter->Print(fullPrint);

  std::cout << " PASSED." << std::endl;
  }
  {
  std::cout << "Testing (UniformGrid(1,5,10), UseStripsOn, PassThroughCellIds, PassThroughPointIds)...";
  vtkSmartPointer<vtkDataSetSurfaceFilter> filter =
    vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
  filter->SetInputData (CreateUniformGrid(1, 5, 10));
  filter->PassThroughCellIdsOn();
  filter->PassThroughPointIdsOn();
  filter->UseStripsOn();
  filter->Update();
  int got = filter->GetOutput()->GetNumberOfCells();
  std::cout << " # of cells: " << got;
  std::ostringstream fullPrint;
  filter->Print(fullPrint);

  std::cout << " PASSED." << std::endl;
  }
  {
  std::cout << "Testing (UniformGrid(5,1,10), UseStripsOn, PassThroughCellIds, PassThroughPointIds)...";
  vtkSmartPointer<vtkDataSetSurfaceFilter> filter =
    vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
  filter->SetInputData (CreateUniformGrid(5, 1, 10));
  filter->PassThroughCellIdsOn();
  filter->PassThroughPointIdsOn();
  filter->UseStripsOn();
  filter->Update();
  int got = filter->GetOutput()->GetNumberOfCells();
  std::cout << " # of cells: " << got;
  std::ostringstream fullPrint;
  filter->Print(fullPrint);

  std::cout << " PASSED." << std::endl;
  }
  {
  std::cout << "Testing (UniformGrid, UseStripsOff, PassThroughCellIds, PassThroughPointIds)...";
  vtkSmartPointer<vtkDataSetSurfaceFilter> filter =
    vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
  filter->SetInputData (CreateUniformGrid(10, 5, 1));
  filter->PassThroughCellIdsOn();
  filter->PassThroughPointIdsOn();
  filter->UseStripsOff();
  filter->Update();
  int got = filter->GetOutput()->GetNumberOfCells();
  std::cout << " # of cells: " << got;
  std::cout << " PASSED." << std::endl;
  }
  {
  std::cout << "Testing DataSetExecute...";
  vtkSmartPointer<vtkDataSetSurfaceFilter> filter =
    vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
  filter->PassThroughCellIdsOn();
  filter->PassThroughPointIdsOn();

  vtkSmartPointer<vtkDataSet> ugrid = CreateUniformGrid(10, 5, 1);

  vtkSmartPointer<vtkPolyData> polyData =
    vtkSmartPointer<vtkPolyData>::New();
  filter->DataSetExecute(ugrid, polyData);

  int got = polyData->GetNumberOfCells();
  std::cout << " # of cells: " << got;

  std::cout << " PASSED." << std::endl;
  }
  {
  std::cout << "Testing UniformGridExecute all faces...";
  vtkSmartPointer<vtkDataSetSurfaceFilter> filter =
    vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
  filter->PassThroughCellIdsOn();
  filter->PassThroughPointIdsOn();

  vtkSmartPointer<vtkDataSet> ugrid = CreateUniformGrid(10, 5, 1);

  vtkSmartPointer<vtkPolyData> polyData =
    vtkSmartPointer<vtkPolyData>::New();
  vtkUniformGrid *grid = vtkUniformGrid::SafeDownCast(ugrid);
  int* tmpext = grid->GetExtent();
  vtkIdType ext[6];
  ext[0] = tmpext[0]; ext[1] = tmpext[1];
  ext[2] = tmpext[2]; ext[3] = tmpext[3];
  ext[4] = tmpext[4]; ext[5] = tmpext[5];
  bool faces[6] = {true, true, true, true, true, true};
  filter->UniformGridExecute(ugrid, polyData, ext, ext, faces);

  int got = polyData->GetNumberOfCells();
  std::cout << " # of cells: " << got;

  std::cout << " PASSED." << std::endl;
  }
  {
  std::cout << "Testing UniformGridExecute three faces...";
  vtkSmartPointer<vtkDataSetSurfaceFilter> filter =
    vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
  filter->PassThroughCellIdsOn();
  filter->PassThroughPointIdsOn();

  vtkSmartPointer<vtkDataSet> ugrid = CreateUniformGrid(10, 5, 2);

  vtkSmartPointer<vtkPolyData> polyData =
    vtkSmartPointer<vtkPolyData>::New();
  vtkUniformGrid *grid = vtkUniformGrid::SafeDownCast(ugrid);
  int* tmpext = grid->GetExtent();
  vtkIdType ext[6];
  ext[0] = tmpext[0]; ext[1] = tmpext[1];
  ext[2] = tmpext[2]; ext[3] = tmpext[3];
  ext[4] = tmpext[4]; ext[5] = tmpext[5];
  bool faces[6] = {true, false, true, false, true, false};
  filter->UniformGridExecute(ugrid, polyData, ext, ext, faces);

  int got = polyData->GetNumberOfCells();
  std::cout << " # of cells: " << got;

  std::cout << " PASSED." << std::endl;
  }
  {
  std::cout << "Testing (RectilinearGrid, PassThroughCellIds, PassThroughPointIds)...";
  vtkSmartPointer<vtkDataSetSurfaceFilter> filter =
    vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
  filter->SetInputData (CreateRectilinearGrid());
  filter->PassThroughCellIdsOn();
  filter->PassThroughPointIdsOn();
  filter->Update();

  int got = filter->GetOutput()->GetNumberOfCells();
  std::cout << " # of cells: " << got;
  std::cout << " PASSED." << std::endl;
  }
  {
  std::cout << "Testing (StructuredGrid, PassThroughCellIds, PassThroughPointIds)...";
  vtkSmartPointer<vtkDataSetSurfaceFilter> filter =
    vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
  filter->SetInputData (CreateStructuredGrid());
  filter->PassThroughCellIdsOn();
  filter->PassThroughPointIdsOn();
  filter->Update();
  vtkPolyData *output = filter->GetOutput();
  if (output->GetNumberOfCells() != 10)
  {
    std::cerr << "Incorrect number of cells generated by vtkDataSetSurfaceFilter!\n"
              << "Expected: 10, Found: " << output->GetNumberOfCells();
    return 1;
  }
  else if(output->GetNumberOfPoints() != 32)
  {
    std::cerr << "Incorrect number of points generated by vtkDataSetSurfaceFilter\n"
              << "Expected 32, Found : " << output->GetNumberOfPoints();
    return 1;
  }
  std::cout << " PASSED." << std::endl;
  }
  {
  std::cout << "Testing (StructuredGrid, Blanking, PassThroughCellIds, PassThroughPointIds)...";
  vtkSmartPointer<vtkDataSetSurfaceFilter> filter =
    vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
  vtkSmartPointer<vtkDataSet> input = CreateStructuredGrid(true);
  filter->SetInputData(input.GetPointer());
  filter->PassThroughCellIdsOn();
  filter->PassThroughPointIdsOn();
  filter->Update();
  vtkPolyData *output = filter->GetOutput();
  if (output->GetNumberOfCells() != 6)
  {
    std::cerr << "Incorrect number of cells generated by vtkDataSetSurfaceFilter!\n"
              << "Expected: 6, Found: " << output->GetNumberOfCells();
    return 1;
  }
  else if(output->GetNumberOfPoints() != 24)
  {
    std::cerr << "Incorrect number of points generated by vtkDataSetSurfaceFilter\n"
              << "Expected 24, Found : " << output->GetNumberOfPoints();
    return 1;
  }
  //verify that the blanked point is not present in output.
  double blankPt[3];
  input->GetPoint(6,blankPt);
  for(vtkIdType ptId = 0; ptId < output->GetNumberOfPoints(); ++ptId)
  {
    double x[3];
    output->GetPoint(ptId,x);
    if(vtkMath::Distance2BetweenPoints(blankPt, x) < 1.0e-5)
    {
      std::cerr << "Blanked point included in vtkDataSetSurfaceFilter output!\n"
                << "ptId: " << ptId << '\n';
      return 1;
    }
  }
  std::cout << " PASSED." << std::endl;
  }
  // Error and warnings
  {
  std::cout << "Testing UniformGridExecute strips not supported error...";
  vtkSmartPointer<vtkTest::ErrorObserver>  errorObserver =
    vtkSmartPointer<vtkTest::ErrorObserver>::New();
  vtkSmartPointer<vtkDataSetSurfaceFilter> filter =
    vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
  filter->UseStripsOn();
  filter->AddObserver(vtkCommand::ErrorEvent, errorObserver);
  vtkSmartPointer<vtkDataSet> ugrid = CreateUniformGrid(10, 5, 1);

  vtkSmartPointer<vtkPolyData> polyData =
    vtkSmartPointer<vtkPolyData>::New();
  vtkUniformGrid *grid = vtkUniformGrid::SafeDownCast(ugrid);
  int* tmpext = grid->GetExtent();
  vtkIdType ext[6];
  ext[0] = tmpext[0]; ext[1] = tmpext[1];
  ext[2] = tmpext[2]; ext[3] = tmpext[3];
  ext[4] = tmpext[4]; ext[5] = tmpext[5];
  bool faces[6] = {true, true, true, true, true, true};
  filter->UniformGridExecute(ugrid, polyData, ext, ext, faces);
  int status1 = errorObserver->CheckErrorMessage("Strips are not supported for uniform grid!");
  if (status1)
  {
    std::cout << " FAILED." << std::endl;
    status++;
  }
  else
  {
    std::cout << " PASSED." << std::endl;
  }
  }
  {
  std::cout << "Testing cells == 0 warning...";
  vtkSmartPointer<vtkTest::ErrorObserver>  warningObserver =
    vtkSmartPointer<vtkTest::ErrorObserver>::New();

  vtkSmartPointer<vtkDataSetSurfaceFilter> filter =
    vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
  filter->SetInputData (vtkSmartPointer<vtkPolyData>::New());
  filter->AddObserver(vtkCommand::WarningEvent, warningObserver);
  filter->Update();

  int status1 = warningObserver->CheckWarningMessage("Number of cells is zero, no data to process.");
  if (status1)
  {
    std::cout << " FAILED." << std::endl;
    status++;
  }
  else
  {
    std::cout << " PASSED." << std::endl;
  }
  }
  {
  std::cout << "Testing DataSetExecute cells == 0 warning...";
  vtkSmartPointer<vtkTest::ErrorObserver>  warningObserver =
    vtkSmartPointer<vtkTest::ErrorObserver>::New();

  vtkSmartPointer<vtkDataSetSurfaceFilter> filter =
    vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
  filter->AddObserver(vtkCommand::WarningEvent, warningObserver);

  vtkSmartPointer<vtkUnstructuredGrid> ugrid =
    vtkSmartPointer<vtkUnstructuredGrid>::New();

  vtkSmartPointer<vtkPolyData> polyData =
    vtkSmartPointer<vtkPolyData>::New();
  filter->DataSetExecute(ugrid, polyData);

  int status1 = warningObserver->CheckWarningMessage("Number of cells is zero, no data to process.");
  if (status1)
  {
    std::cout << " FAILED." << std::endl;
    status++;
  }
  else
  {
    std::cout << " PASSED." << std::endl;
  }
  }
  {
  std::cout << "Testing StructuredExecute invalid dataset error...";
  vtkSmartPointer<vtkTest::ErrorObserver>  errorObserver =
    vtkSmartPointer<vtkTest::ErrorObserver>::New();

  vtkSmartPointer<vtkDataSetSurfaceFilter> filter =
    vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
  filter->AddObserver(vtkCommand::ErrorEvent, errorObserver);

  vtkSmartPointer<vtkUnstructuredGrid> ugrid =
    vtkSmartPointer<vtkUnstructuredGrid>::New();

  vtkSmartPointer<vtkPolyData> polyData =
    vtkSmartPointer<vtkPolyData>::New();
  vtkIdType ext[6];
  ext[0] = 0; ext[1] = 1;
  ext[2] = 0; ext[3] = 1;
  ext[4] = 0; ext[5] = 1;

  filter->StructuredExecute(ugrid, polyData, ext, ext);

  int status1 = errorObserver->CheckErrorMessage("Invalid data set type: 4");
  if (status1)
  {
    std::cout << " FAILED." << std::endl;
    status++;
  }
  else
  {
    std::cout << " PASSED." << std::endl;
  }
  }
  {
  std::cout << "Testing BadAttributes error...";
  vtkSmartPointer<vtkTest::ErrorObserver>  errorObserver =
    vtkSmartPointer<vtkTest::ErrorObserver>::New();

  vtkSmartPointer<vtkDataSetSurfaceFilter> filter =
    vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
  filter->SetInputData (CreateBadAttributes());
  filter->PassThroughCellIdsOn();
  filter->PassThroughPointIdsOn();
  filter->GetInput()->AddObserver(vtkCommand::ErrorEvent, errorObserver);
  filter->Update();

  int status1 = errorObserver->CheckErrorMessage("Point array PointDataTestArray with 1 components, only has 2 tuples but there are 3 points");
  if (status1)
  {
    std::cout << " FAILED." << std::endl;
    status++;
  }
  else
  {
    std::cout << " PASSED." << std::endl;
  }
  }
  return status;
}

vtkSmartPointer<vtkDataSet> CreateTriangleStripData(const int xres, const int yres)
{
  vtkSmartPointer<vtkPlaneSource> plane =
    vtkSmartPointer<vtkPlaneSource>::New();
  plane->SetXResolution(xres);
  plane->SetYResolution(yres);
  plane->Update();

  vtkSmartPointer<vtkTriangleFilter> tris =
    vtkSmartPointer<vtkTriangleFilter>::New();
  tris->SetInputConnection(plane->GetOutputPort());
  vtkSmartPointer<vtkStripper> stripper =
    vtkSmartPointer<vtkStripper>::New();
  stripper->SetInputConnection(tris->GetOutputPort());
  stripper->Update();

  vtkSmartPointer<vtkUnstructuredGrid> unstructuredGrid =
    vtkSmartPointer<vtkUnstructuredGrid>::New();
  unstructuredGrid->SetPoints(stripper->GetOutput()->GetPoints());
  unstructuredGrid->SetCells(VTK_TRIANGLE_STRIP, stripper->GetOutput()->GetStrips());
  return unstructuredGrid;
}

vtkSmartPointer<vtkDataSet> CreatePolyData(const int xres, const int yres)
{
  vtkSmartPointer<vtkPlaneSource> plane =
    vtkSmartPointer<vtkPlaneSource>::New();
  plane->SetXResolution(xres);
  plane->SetYResolution(yres);
  plane->Update();

  vtkSmartPointer<vtkTriangleFilter> tris =
    vtkSmartPointer<vtkTriangleFilter>::New();
  tris->SetInputConnection(plane->GetOutputPort());
  vtkSmartPointer<vtkStripper> stripper =
    vtkSmartPointer<vtkStripper>::New();
  stripper->SetInputConnection(tris->GetOutputPort());
  stripper->Update();

  vtkSmartPointer<vtkPolyData> pd =
    vtkSmartPointer<vtkPolyData>::New();
  pd = plane->GetOutput();
  return pd;
}

vtkSmartPointer<vtkDataSet> CreatePolygonData(int sides)
{
  vtkSmartPointer<vtkRegularPolygonSource> polygon =
    vtkSmartPointer<vtkRegularPolygonSource>::New();
  polygon->SetNumberOfSides(sides);
  polygon->Update();
  vtkSmartPointer<vtkIntArray> cellData =
    vtkSmartPointer<vtkIntArray>::New();
  cellData->SetNumberOfTuples(polygon->GetOutput()->GetNumberOfCells());
  cellData->SetName("CellDataTestArray");
  vtkIdType c = 0;
  for (vtkIdType i = 0; i < polygon->GetOutput()->GetNumberOfCells(); ++i)
  {
      cellData->SetTuple1(c++, i);
  }
  vtkSmartPointer<vtkIntArray> pointData =
    vtkSmartPointer<vtkIntArray>::New();
  pointData->SetNumberOfTuples(polygon->GetOutput()->GetNumberOfPoints());
  pointData->SetName("PointDataTestArray");
  c = 0;
  for (int i = 0; i < polygon->GetOutput()->GetNumberOfPoints(); ++i)
  {
      pointData->SetTuple1(c++, i);
  }

  vtkSmartPointer<vtkPolyData> pd =
    vtkSmartPointer<vtkPolyData>::New();
  pd = polygon->GetOutput();
  pd->GetPointData()->SetScalars(pointData);
  pd->GetCellData()->SetScalars(cellData);

  return pd;
}

vtkSmartPointer<vtkDataSet> CreateTetraData()
{
  vtkSmartPointer<vtkPoints> points =
    vtkSmartPointer<vtkPoints>:: New();
  points->InsertNextPoint(0, 0, 0);
  points->InsertNextPoint(1, 0, 0);
  points->InsertNextPoint(1, 1, 0);
  points->InsertNextPoint(0, 1, 1);
  points->InsertNextPoint(5, 5, 5);
  points->InsertNextPoint(6, 5, 5);
  points->InsertNextPoint(6, 6, 5);
  points->InsertNextPoint(5, 6, 6);

  vtkSmartPointer<vtkUnstructuredGrid> unstructuredGrid =
    vtkSmartPointer<vtkUnstructuredGrid>::New();
  unstructuredGrid->SetPoints(points);

  vtkSmartPointer<vtkTetra> tetra =
    vtkSmartPointer<vtkTetra>::New();
  tetra->GetPointIds()->SetId(0, 4);
  tetra->GetPointIds()->SetId(1, 5);
  tetra->GetPointIds()->SetId(2, 6);
  tetra->GetPointIds()->SetId(3, 7);

  vtkSmartPointer<vtkCellArray> cellArray =
    vtkSmartPointer<vtkCellArray>::New();
  cellArray->InsertNextCell(tetra);
  unstructuredGrid->SetCells(VTK_TETRA, cellArray);

  vtkSmartPointer<vtkIntArray> pointData =
    vtkSmartPointer<vtkIntArray>::New();
  pointData->SetNumberOfTuples(unstructuredGrid->GetNumberOfPoints());
  pointData->SetName("PointDataTestArray");
  int c = 0;
  for (vtkIdType id = 0; id < tetra->GetNumberOfPoints(); ++id)
  {
    pointData->SetTuple1(c++, id);
  }
  unstructuredGrid->GetPointData()->SetScalars(pointData);

  return unstructuredGrid;
}
vtkSmartPointer<vtkDataSet> CreateQuadraticWedgeData()
{
  vtkSmartPointer<vtkQuadraticWedge> aWedge =
    vtkSmartPointer<vtkQuadraticWedge>::New();
  double *pcoords = aWedge->GetParametricCoords();
  for (int i = 0; i < aWedge->GetNumberOfPoints(); ++i)
  {
    aWedge->GetPointIds()->SetId(i, i);
    aWedge->GetPoints()->SetPoint(i,
                                  *(pcoords + 3 * i),
                                  *(pcoords + 3 * i + 1),
                                  *(pcoords + 3 * i + 2));
  }

  vtkSmartPointer<vtkUnstructuredGrid> unstructuredGrid =
    vtkSmartPointer<vtkUnstructuredGrid>::New();
  unstructuredGrid->SetPoints(aWedge->GetPoints());

  vtkSmartPointer<vtkCellArray> cellArray =
    vtkSmartPointer<vtkCellArray>::New();
  cellArray->InsertNextCell(aWedge);
  unstructuredGrid->SetCells(VTK_QUADRATIC_WEDGE, cellArray);
  return unstructuredGrid;
}
vtkSmartPointer<vtkDataSet> CreateUniformGrid(
  unsigned int dimx, unsigned int dimy, unsigned int dimz)
{
  vtkSmartPointer<vtkUniformGrid> image =
    vtkSmartPointer<vtkUniformGrid>::New();

  image->SetDimensions(dimx, dimy, dimz);

  image->AllocateScalars(VTK_UNSIGNED_CHAR,1);

  for(unsigned int x = 0; x < dimx; x++)
  {
    for(unsigned int y = 0; y < dimy; y++)
    {
      for(unsigned int z = 0; z < dimz; z++)
      {
        unsigned char* pixel = static_cast<unsigned char*>(image->GetScalarPointer(x,y,0));
        if(x < dimx/2)
        {
          pixel[0] = 50;
        }
        else
        {
          pixel[0] = 150;
        }
      }
    }
  }
  return image;
}

vtkSmartPointer<vtkDataSet> CreateGenericCellData(int cellType)
{
  vtkSmartPointer<vtkGenericCell> aCell =
    vtkSmartPointer<vtkGenericCell>::New();
  aCell->SetCellType(cellType);
  if (aCell->RequiresInitialization())
  {
    aCell->Initialize();
  }

  int numPts   = aCell->GetNumberOfPoints();
  double *pcoords = aCell->GetParametricCoords();
  for(int j=0; j<numPts; ++j)
  {
    aCell->GetPointIds()->SetId(j,j);
    aCell->GetPoints()->SetPoint(j, pcoords + 3*j);
  }

  vtkSmartPointer<vtkIntArray> pointData =
    vtkSmartPointer<vtkIntArray>::New();
  pointData->SetNumberOfTuples(numPts);
  pointData->SetName("PointDataTestArray");
  for(int j=0; j<numPts; ++j)
  {
    pointData->SetTuple1(j, j);
  }

  vtkSmartPointer<vtkUnstructuredGrid> unstructuredGrid =
    vtkSmartPointer<vtkUnstructuredGrid>::New();
  unstructuredGrid->SetPoints(aCell->GetPoints());
  unstructuredGrid->GetPointData()->SetScalars(pointData);

  vtkSmartPointer<vtkCellArray> cellArray =
    vtkSmartPointer<vtkCellArray>::New();
  cellArray->InsertNextCell(aCell);
  unstructuredGrid->SetCells(cellType, cellArray);
  return unstructuredGrid;
}

vtkSmartPointer<vtkDataSet> CreateRectilinearGrid()
{
  vtkSmartPointer<vtkRectilinearGrid> grid =
    vtkSmartPointer<vtkRectilinearGrid>::New();
  grid->SetDimensions(2,3,1);

  vtkSmartPointer<vtkDoubleArray> xArray =
    vtkSmartPointer<vtkDoubleArray>::New();
  xArray->InsertNextValue(0.0);
  xArray->InsertNextValue(2.0);

  vtkSmartPointer<vtkDoubleArray> yArray =
    vtkSmartPointer<vtkDoubleArray>::New();
  yArray->InsertNextValue(0.0);
  yArray->InsertNextValue(1.0);
  yArray->InsertNextValue(2.0);

  vtkSmartPointer<vtkDoubleArray> zArray =
    vtkSmartPointer<vtkDoubleArray>::New();
  zArray->InsertNextValue(0.0);

  grid->SetXCoordinates(xArray);
  grid->SetYCoordinates(yArray);
  grid->SetZCoordinates(zArray);

  return grid;
}

// Generate a 1x2x1 vtkStructuredGrid with 12 points.
vtkSmartPointer<vtkDataSet> CreateStructuredGrid(bool blank)
{
  vtkSmartPointer<vtkStructuredGrid> grid =
    vtkSmartPointer<vtkStructuredGrid>::New();

  vtkSmartPointer<vtkPoints> points =
    vtkSmartPointer<vtkPoints>::New();
  double x, y, z;

  x = 0.0;
  y = 0.0;
  z = 0.0;

  for(unsigned int k = 0; k < 2; k++)
  {
    z += 2.0;
    for(unsigned int j = 0; j < 3; j++)
    {
      y += 1.0;
      for(unsigned int i = 0; i < 2; i++)
      {
        x += .5;
        points->InsertNextPoint(x, y, z);
      }
    }
  }

  // Specify the dimensions of the grid
  grid->SetDimensions(2,3,2);
  grid->SetPoints(points);

  // When blanking==true, the 6th point (0th cell) is blanked.
  if (blank)
  {
    grid->BlankPoint(points->GetNumberOfPoints() / 2);
  }
  return grid;
}

vtkSmartPointer<vtkDataSet> CreateBadAttributes()
{
  vtkSmartPointer<vtkPolyLine> aPolyLine =
    vtkSmartPointer<vtkPolyLine>::New();
  aPolyLine->GetPointIds()->SetNumberOfIds(3);
  aPolyLine->GetPointIds()->SetId(0,0);
  aPolyLine->GetPointIds()->SetId(1,1);
  aPolyLine->GetPointIds()->SetId(2,2);

  aPolyLine->GetPoints()->SetNumberOfPoints(3);
  aPolyLine->GetPoints()->SetPoint (0, 10.0, 20.0, 30.0);
  aPolyLine->GetPoints()->SetPoint (1, 10.0, 30.0, 30.0);
  aPolyLine->GetPoints()->SetPoint (2, 10.0, 30.0, 40.0);

  vtkSmartPointer<vtkUnstructuredGrid> unstructuredGrid =
    vtkSmartPointer<vtkUnstructuredGrid>::New();
  unstructuredGrid->SetPoints(aPolyLine->GetPoints());

  vtkSmartPointer<vtkIntArray> pointData =
    vtkSmartPointer<vtkIntArray>::New();
  pointData->SetNumberOfTuples(2);
  pointData->SetName("PointDataTestArray");
  for(int j=0; j<2; ++j)
  {
    pointData->SetTuple1(j, j);
  }

  vtkSmartPointer<vtkCellArray> cellArray =
    vtkSmartPointer<vtkCellArray>::New();
  cellArray->InsertNextCell(aPolyLine);
  unstructuredGrid->SetCells(VTK_POLY_LINE, cellArray);
  unstructuredGrid->GetPointData()->SetScalars(pointData);

  return unstructuredGrid;
}
