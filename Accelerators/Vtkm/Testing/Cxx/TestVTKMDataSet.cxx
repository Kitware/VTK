#include "vtkmDataSet.h"

#include "vtkCell.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkFloatArray.h"
#include "vtkGenericCell.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkImageData.h"
#include "vtkImageDataToPointSet.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkStructuredGrid.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkmDataArray.h"

#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>

#include <array>
#include <random>

namespace
{

//-----------------------------------------------------------------------------
class TestError
{
public:
  TestError(const std::string& message, int line)
    : Message(message)
    , Line(line)
  {
  }

  void PrintMessage(std::ostream& out) const
  {
    out << "Error at line " << this->Line << ": " << this->Message << "\n";
  }

private:
  std::string Message;
  int Line;
};

#define RAISE_TEST_ERROR(msg) throw TestError((msg), __LINE__)

#define TEST_VERIFY(cond, msg)                                                                     \
  if (!(cond))                                                                                     \
  RAISE_TEST_ERROR((msg))

inline bool IsEqualFloat(double a, double b, double e = 1e-6f)
{
  return (std::abs(a - b) <= e);
}

//-----------------------------------------------------------------------------
inline void TestEqualCells(vtkCell* c1, vtkCell* c2)
{
  TEST_VERIFY(c1->GetCellType() == c2->GetCellType(), "Cell types don't match");
  TEST_VERIFY(c1->GetNumberOfPoints() == c2->GetNumberOfPoints(), "Cell sizes don't match");
  for (vtkIdType i = 0; i < c1->GetNumberOfPoints(); ++i)
  {
    TEST_VERIFY(c1->GetPointId(i) == c2->GetPointId(i), "Cell point-ids don't match");
  }
}

inline void TestEqualVtkArrays(vtkAbstractArray* a1, vtkAbstractArray* a2)
{
  TEST_VERIFY(std::string(a1->GetName()) == std::string(a2->GetName()), "Array names don't match");
  TEST_VERIFY(a1->GetDataType() == a2->GetDataType(), "Array data-types don't match");
  TEST_VERIFY(
    a1->GetNumberOfTuples() == a2->GetNumberOfTuples(), "Array number of tuples don't match");
  TEST_VERIFY(a1->GetNumberOfComponents() == a2->GetNumberOfComponents(),
    "Array number of components don't match");

  auto da1 = vtkDataArray::SafeDownCast(a1);
  auto da2 = vtkDataArray::SafeDownCast(a2);
  double range1[2], range2[2];
  int numComps = da1->GetNumberOfComponents();
  for (int i = 0; i < numComps; ++i)
  {
    da1->GetRange(range1, i);
    da2->GetRange(range2, i);
    TEST_VERIFY(IsEqualFloat(range1[0], range2[0]) && IsEqualFloat(range1[1], range2[1]),
      "Array ranges don't match");
  }
}

void TestDataSets(vtkDataSet* dsVtk, vtkDataSet* dsVtkm)
{
  TEST_VERIFY(
    dsVtk->GetNumberOfPoints() == dsVtkm->GetNumberOfPoints(), "Number of points don't match");
  TEST_VERIFY(
    dsVtk->GetNumberOfCells() == dsVtkm->GetNumberOfCells(), "Number of cells don't match");
  double bounds1[6], bounds2[6];
  dsVtk->GetBounds(bounds1);
  dsVtkm->GetBounds(bounds2);
  TEST_VERIFY(IsEqualFloat(bounds1[0], bounds2[0]) && IsEqualFloat(bounds1[1], bounds2[1]) &&
      IsEqualFloat(bounds1[2], bounds2[2]) && IsEqualFloat(bounds1[3], bounds2[3]) &&
      IsEqualFloat(bounds1[4], bounds2[4]) && IsEqualFloat(bounds1[5], bounds2[5]),
    "Bounds don't match");

  vtkIdType numPoints = dsVtk->GetNumberOfPoints();
  for (vtkIdType i = 0; i < numPoints; ++i)
  {
    double x1[3], x2[3];
    dsVtk->GetPoint(i, x1);
    dsVtkm->GetPoint(i, x2);
    TEST_VERIFY(
      IsEqualFloat(x1[0], x2[0]) && IsEqualFloat(x1[1], x2[1]) && IsEqualFloat(x1[2], x2[2]),
      "'GetPoint` results don't match");

    vtkNew<vtkIdList> cellIds1, cellIds2;
    dsVtk->GetPointCells(i, cellIds1);
    dsVtkm->GetPointCells(i, cellIds2);
    cellIds1->Sort();
    cellIds2->Sort();
    TEST_VERIFY(cellIds1->GetNumberOfIds() == cellIds2->GetNumberOfIds(),
      "`GetPointCells` results don't match");
    for (vtkIdType j = 0; j < cellIds1->GetNumberOfIds(); ++j)
    {
      TEST_VERIFY(cellIds1->GetId(j) == cellIds2->GetId(j), "`GetPointCells` results don't match");
    }
  }

  vtkIdType numCells = dsVtk->GetNumberOfCells();
  for (vtkIdType i = 0; i < numCells; ++i)
  {
    TestEqualCells(dsVtk->GetCell(i), dsVtkm->GetCell(i));

    vtkNew<vtkGenericCell> gc1, gc2;
    dsVtk->GetCell(i, gc1);
    dsVtkm->GetCell(i, gc2);
    TestEqualCells(gc1, gc2);

    double bds1[6], bds2[6];
    dsVtk->GetCellBounds(i, bds1);
    dsVtkm->GetCellBounds(i, bds2);
    TEST_VERIFY(IsEqualFloat(bds1[0], bds2[0]) && IsEqualFloat(bds1[1], bds2[1]) &&
        IsEqualFloat(bds1[2], bds2[2]) && IsEqualFloat(bds1[3], bds2[3]) &&
        IsEqualFloat(bds1[4], bds2[4]) && IsEqualFloat(bds1[5], bds2[5]),
      "Cell bounds don't match");

    TEST_VERIFY(dsVtk->GetCellType(i) == dsVtkm->GetCellType(i), "Cell types don't match");

    vtkNew<vtkIdList> ptIds1, ptIds2;
    dsVtk->GetCellPoints(i, ptIds1);
    dsVtkm->GetCellPoints(i, ptIds2);
    for (vtkIdType j = 0; j < ptIds1->GetNumberOfIds(); ++j)
    {
      TEST_VERIFY(ptIds1->GetId(j) == ptIds2->GetId(j), "`GetCellPoints` results don't match");
    }
  }

  std::default_random_engine engine;
  std::uniform_real_distribution<double> d1(bounds1[0], bounds1[1]), d2(bounds1[2], bounds1[3]),
    d3(bounds1[4], bounds1[5]);
  static constexpr int numSamples = 100;
  for (int i = 0; i < numSamples; ++i)
  {
    double x[3] = { d1(engine), d2(engine), d3(engine) };

    auto pid1 = dsVtk->FindPoint(x);
    auto pid2 = dsVtkm->FindPoint(x);
    if (pid1 != pid2)
    {
      TEST_VERIFY(pid1 != -1 && pid2 != -1, "`FindPoint` results don't match");
      double x1[3], x2[3];
      dsVtk->GetPoint(pid1, x1);
      dsVtkm->GetPoint(pid1, x2);
      TEST_VERIFY(IsEqualFloat(
                    vtkMath::Distance2BetweenPoints(x, x1), vtkMath::Distance2BetweenPoints(x, x2)),
        "`FindPoint` results don't match");
    }

    int subId = 0;
    double pcoords1[3], pcoords2[3];
    double weights1[8], weights2[8];
    auto cid1 = dsVtk->FindCell(x, nullptr, -1, 1e-6, subId, pcoords1, weights1);
    auto cid2 = dsVtkm->FindCell(x, nullptr, -1, 1e-6, subId, pcoords2, weights2);

    // vtkDataSet and vtkmDataSet may find different cells if the point is too close to
    // the boundary of those cells
    if (cid1 != cid2)
    {
      if (cid2 >= 0)
      {
        // check if the point is inside or close to the vtkmDataSet found cell
        vtkCell* cell = dsVtk->GetCell(cid2);
        double dist2 = 0, pcoords[3], weights[8];
        if (cell->EvaluatePosition(x, nullptr, subId, pcoords, dist2, weights) == 0) // outside?
        {
          TEST_VERIFY(IsEqualFloat(cell->GetParametricDistance(pcoords), 0.0, 1e-3),
            "`FindCell` incorrect result by vtkmDataSet");
        }
      }
    }
    else if (cid1 == -1)
    {
      continue;
    }
    else
    {
      TEST_VERIFY(IsEqualFloat(pcoords1[0], pcoords2[0]) &&
          IsEqualFloat(pcoords1[1], pcoords2[1]) && IsEqualFloat(pcoords1[2], pcoords2[2]),
        "`FindCell` pcoords don't match");
      int count = dsVtk->GetCell(cid1)->GetNumberOfPoints();
      for (int j = 0; j < count; ++j)
      {
        TEST_VERIFY(IsEqualFloat(weights1[j], weights2[j]), "`FindCell` weights don't match");
      }
    }
  }

  // test fields
  int numPointFields = dsVtk->GetPointData()->GetNumberOfArrays();
  TEST_VERIFY(numPointFields == dsVtkm->GetPointData()->GetNumberOfArrays(),
    "Number of point-fields don't match");
  for (int i = 0; i < numPointFields; ++i)
  {
    TestEqualVtkArrays(dsVtk->GetPointData()->GetArray(i), dsVtkm->GetPointData()->GetArray(i));
  }

  int numCellFields = dsVtk->GetCellData()->GetNumberOfArrays();
  TEST_VERIFY(numCellFields == dsVtkm->GetCellData()->GetNumberOfArrays(),
    "Number of cell-fields don't match");
  for (int i = 0; i < numCellFields; ++i)
  {
    TestEqualVtkArrays(dsVtk->GetCellData()->GetArray(i), dsVtkm->GetCellData()->GetArray(i));
  }
}

//-----------------------------------------------------------------------------
inline void CoordsCopy(const vtkm::cont::CoordinateSystem& coords, vtkPoints* points)
{
  auto ptsPortal = coords.GetData().GetPortalConstControl();
  auto numPoints = coords.GetNumberOfPoints();

  points->SetDataTypeToFloat();
  points->SetNumberOfPoints(numPoints);
  auto ptsArray = vtkFloatArray::SafeDownCast(points->GetData());
  for (vtkIdType i = 0; i < numPoints; ++i)
  {
    auto pt = ptsPortal.Get(i);
    float tuple[3] = { pt[0], pt[1], pt[2] };
    ptsArray->SetTypedTuple(i, tuple);
  }
}

inline void FieldCopy(
  const vtkm::cont::ArrayHandle<float>& src, const char* name, vtkFloatArray* dst)
{
  auto portal = src.GetPortalConstControl();
  vtkm::Id length = portal.GetNumberOfValues();

  dst->SetName(name);
  dst->SetNumberOfComponents(1);
  dst->SetNumberOfTuples(length);
  for (vtkm::Id i = 0; i < length; ++i)
  {
    dst->SetValue(i, portal.Get(i));
  }
}

//-----------------------------------------------------------------------------
vtkm::cont::testing::MakeTestDataSet Maker;

void TestUniformDataSet()
{
  auto dataset = Maker.Make3DUniformDataSet0();
  auto coords =
    dataset.GetCoordinateSystem().GetData().Cast<vtkm::cont::ArrayHandleUniformPointCoordinates>();
  auto portal = coords.GetPortalConstControl();
  auto dims = portal.GetDimensions();
  auto origin = portal.GetOrigin();
  auto spacing = portal.GetSpacing();

  vtkNew<vtkFloatArray> pointField, cellField;
  FieldCopy(dataset.GetField("pointvar").GetData().Cast<vtkm::cont::ArrayHandle<float> >(),
    "pointvar", pointField);
  FieldCopy(dataset.GetField("cellvar").GetData().Cast<vtkm::cont::ArrayHandle<float> >(),
    "cellvar", cellField);

  vtkNew<vtkImageData> imageData;
  imageData->SetDimensions(dims[0], dims[1], dims[2]);
  imageData->SetOrigin(origin[0], origin[1], origin[2]);
  imageData->SetSpacing(spacing[0], spacing[1], spacing[2]);
  imageData->GetPointData()->AddArray(pointField);
  imageData->GetCellData()->AddArray(cellField);

  vtkNew<vtkImageDataToPointSet> voxToHex;
  voxToHex->SetInputData(imageData);
  voxToHex->Update();

  auto dsVtk = voxToHex->GetOutput();

  vtkNew<vtkmDataSet> dsVtkm;
  dsVtkm->SetVtkmDataSet(dataset);

  TestDataSets(dsVtk, dsVtkm);
}

void TestCurvilinearDataSet()
{
  auto dataset = Maker.Make3DRegularDataSet0();
  auto dims = dataset.GetCellSet().Cast<vtkm::cont::CellSetStructured<3> >().GetPointDimensions();

  vtkNew<vtkPoints> points;
  CoordsCopy(dataset.GetCoordinateSystem(), points);

  vtkNew<vtkFloatArray> pointField, cellField;
  FieldCopy(dataset.GetField("pointvar").GetData().Cast<vtkm::cont::ArrayHandle<float> >(),
    "pointvar", pointField);
  FieldCopy(dataset.GetField("cellvar").GetData().Cast<vtkm::cont::ArrayHandle<float> >(),
    "cellvar", cellField);

  vtkNew<vtkStructuredGrid> dsVtk;
  dsVtk->SetDimensions(dims[0], dims[1], dims[2]);
  dsVtk->SetPoints(points);
  dsVtk->GetPointData()->AddArray(pointField);
  dsVtk->GetCellData()->AddArray(cellField);

  vtkNew<vtkmDataSet> dsVtkm;
  dsVtkm->SetVtkmDataSet(dataset);

  TestDataSets(dsVtk, dsVtkm);
}

void TestExplicitDataSet()
{
  auto dataset = Maker.Make3DExplicitDataSetZoo();

  vtkNew<vtkPoints> points;
  CoordsCopy(dataset.GetCoordinateSystem(), points);

  auto& cellset = *dataset.GetCellSet().GetCellSetBase();
  vtkm::Id numCells = cellset.GetNumberOfCells();

  vtkNew<vtkUnsignedCharArray> shapes;
  vtkNew<vtkCellArray> connectivity;
  shapes->SetNumberOfComponents(1);
  shapes->SetNumberOfTuples(numCells);
  for (vtkm::Id i = 0; i < numCells; ++i)
  {
    shapes->SetValue(i, cellset.GetCellShape(i));

    vtkIdType ptIds[8];
    int count = cellset.GetNumberOfPointsInCell(i);
    cellset.GetCellPointIds(i, ptIds);
    connectivity->InsertNextCell(count, ptIds);
  }

  vtkNew<vtkFloatArray> pointField, cellField;
  FieldCopy(dataset.GetField("pointvar").GetData().Cast<vtkm::cont::ArrayHandle<float> >(),
    "pointvar", pointField);
  FieldCopy(dataset.GetField("cellvar").GetData().Cast<vtkm::cont::ArrayHandle<float> >(),
    "cellvar", cellField);

  vtkNew<vtkUnstructuredGrid> dsVtk;
  dsVtk->SetPoints(points);
  dsVtk->SetCells(shapes, connectivity);
  dsVtk->GetPointData()->AddArray(pointField);
  dsVtk->GetCellData()->AddArray(cellField);

  vtkNew<vtkmDataSet> dsVtkm;
  dsVtkm->SetVtkmDataSet(dataset);

  TestDataSets(dsVtk, dsVtkm);
}

} // anonymous namespace

//-----------------------------------------------------------------------------
int TestVTKMDataSet(int, char*[])
try
{
  std::cout << "Testing Uniform DataSet\n";
  TestUniformDataSet();
  std::cout << "Passed\n";

  std::cout << "Testing Curvilinear DataSet\n";
  TestCurvilinearDataSet();
  std::cout << "Passed\n";

  std::cout << "Testing Explicit DataSet\n";
  TestExplicitDataSet();
  std::cout << "Passed\n";

  return EXIT_SUCCESS;
}
catch (const TestError& e)
{
  e.PrintMessage(std::cout);
  return 1;
}
