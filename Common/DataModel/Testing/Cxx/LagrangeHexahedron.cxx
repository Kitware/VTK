#include "vtkLagrangeCurve.h"
#include "vtkLagrangeHexahedron.h"
#include "vtkLagrangeInterpolation.h"
#include "vtkLagrangeQuadrilateral.h"

#include "vtkMultiBaselineRegressionTest.h"

#include "vtkDoubleArray.h"
#include "vtkNew.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"

#include "vtkCell.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkIncrementalOctreePointLocator.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkXMLPolyDataWriter.h"

#include "vtkVector.h"
#include "vtkVectorOperators.h"

#include "vtkAxis.h"
#include "vtkChartXY.h"
#include "vtkColor.h"
#include "vtkColorSeries.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkFloatArray.h"
#include "vtkPlot.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTable.h"

#include <sstream>
#include <vector>

#include "vtkTestConditionals.txx"

using namespace vtk;

static int expectedDOFIndices1[] = {
  0, 1, //
  3, 2, //
  4, 5, //
  7, 6  //
};

static int expectedDOFIndices2[] = {
  0, 8, 1,   //
  11, 24, 9, //
  3, 10, 2,  //

  16, 22, 17, //
  20, 26, 21, //
  19, 23, 18, //

  4, 12, 5,   //
  15, 25, 13, //
  7, 14, 6    //
};

static int expectedDOFIndices3[] = {
  0, 8, 9, 1,     //
  14, 48, 49, 10, //
  15, 50, 51, 11, //
  3, 12, 13, 2,   //

  24, 40, 41, 26, //
  32, 56, 57, 36, //
  33, 58, 59, 37, //
  30, 44, 45, 28, //

  25, 42, 43, 27, //
  34, 60, 61, 38, //
  35, 62, 63, 39, //
  31, 46, 47, 29, //

  4, 16, 17, 5,   //
  22, 52, 53, 18, //
  23, 54, 55, 19, //
  7, 20, 21, 6    //
};

static const double expectedFacePoints333[96][3] = {
  { 0, 1, 0 },
  { 0, 0, 0 },
  { 0, 0, 1 },
  { 0, 1, 1 },
  { 0, 0.666667, 0 },
  { 0, 0.333333, 0 },
  { 0, 0, 0.333333 },
  { 0, 0, 0.666667 },
  { 0, 0.666667, 1 },
  { 0, 0.333333, 1 },
  { 0, 1, 0.333333 },
  { 0, 1, 0.666667 },
  { 0, 0.666667, 0.333333 },
  { 0, 0.333333, 0.333333 },
  { 0, 0.666667, 0.666667 },
  { 0, 0.333333, 0.666667 },

  { 1, 0, 0 },
  { 1, 1, 0 },
  { 1, 1, 1 },
  { 1, 0, 1 },
  { 1, 0.333333, 0 },
  { 1, 0.666667, 0 },
  { 1, 1, 0.333333 },
  { 1, 1, 0.666667 },
  { 1, 0.333333, 1 },
  { 1, 0.666667, 1 },
  { 1, 0, 0.333333 },
  { 1, 0, 0.666667 },
  { 1, 0.333333, 0.333333 },
  { 1, 0.666667, 0.333333 },
  { 1, 0.333333, 0.666667 },
  { 1, 0.666667, 0.666667 },

  { 0, 0, 0 },
  { 1, 0, 0 },
  { 1, 0, 1 },
  { 0, 0, 1 },
  { 0.333333, 0, 0 },
  { 0.666667, 0, 0 },
  { 1, 0, 0.333333 },
  { 1, 0, 0.666667 },
  { 0.333333, 0, 1 },
  { 0.666667, 0, 1 },
  { 0, 0, 0.333333 },
  { 0, 0, 0.666667 },
  { 0.333333, 0, 0.333333 },
  { 0.666667, 0, 0.333333 },
  { 0.333333, 0, 0.666667 },
  { 0.666667, 0, 0.666667 },

  { 1, 1, 0 },
  { 0, 1, 0 },
  { 0, 1, 1 },
  { 1, 1, 1 },
  { 0.666667, 1, 0 },
  { 0.333333, 1, 0 },
  { 0, 1, 0.333333 },
  { 0, 1, 0.666667 },
  { 0.666667, 1, 1 },
  { 0.333333, 1, 1 },
  { 1, 1, 0.333333 },
  { 1, 1, 0.666667 },
  { 0.666667, 1, 0.333333 },
  { 0.333333, 1, 0.333333 },
  { 0.666667, 1, 0.666667 },
  { 0.333333, 1, 0.666667 },

  { 1, 0, 0 },
  { 0, 0, 0 },
  { 0, 1, 0 },
  { 1, 1, 0 },
  { 0.666667, 0, 0 },
  { 0.333333, 0, 0 },
  { 0, 0.333333, 0 },
  { 0, 0.666667, 0 },
  { 0.666667, 1, 0 },
  { 0.333333, 1, 0 },
  { 1, 0.333333, 0 },
  { 1, 0.666667, 0 },
  { 0.666667, 0.333333, 0 },
  { 0.333333, 0.333333, 0 },
  { 0.666667, 0.666667, 0 },
  { 0.333333, 0.666667, 0 },

  { 0, 0, 1 },
  { 1, 0, 1 },
  { 1, 1, 1 },
  { 0, 1, 1 },
  { 0.333333, 0, 1 },
  { 0.666667, 0, 1 },
  { 1, 0.333333, 1 },
  { 1, 0.666667, 1 },
  { 0.333333, 1, 1 },
  { 0.666667, 1, 1 },
  { 0, 0.333333, 1 },
  { 0, 0.666667, 1 },
  { 0.333333, 0.333333, 1 },
  { 0.666667, 0.333333, 1 },
  { 0.333333, 0.666667, 1 },
  { 0.666667, 0.666667, 1 },
};

static const double expectedEdgePoints333[48][3] = {
  { 0, 0, 0 },
  { 1, 0, 0 },
  { 0.333333, 0, 0 },
  { 0.666667, 0, 0 },

  { 1, 0, 0 },
  { 1, 1, 0 },
  { 1, 0.333333, 0 },
  { 1, 0.666667, 0 },

  { 0, 1, 0 },
  { 1, 1, 0 },
  { 0.333333, 1, 0 },
  { 0.666667, 1, 0 },

  { 0, 0, 0 },
  { 0, 1, 0 },
  { 0, 0.333333, 0 },
  { 0, 0.666667, 0 },

  { 0, 0, 1 },
  { 1, 0, 1 },
  { 0.333333, 0, 1 },
  { 0.666667, 0, 1 },

  { 1, 0, 1 },
  { 1, 1, 1 },
  { 1, 0.333333, 1 },
  { 1, 0.666667, 1 },

  { 0, 1, 1 },
  { 1, 1, 1 },
  { 0.333333, 1, 1 },
  { 0.666667, 1, 1 },

  { 0, 0, 1 },
  { 0, 1, 1 },
  { 0, 0.333333, 1 },
  { 0, 0.666667, 1 },

  { 0, 0, 0 },
  { 0, 0, 1 },
  { 0, 0, 0.333333 },
  { 0, 0, 0.666667 },

  { 1, 0, 0 },
  { 1, 0, 1 },
  { 1, 0, 0.333333 },
  { 1, 0, 0.666667 },

  { 1, 1, 0 },
  { 1, 1, 1 },
  { 1, 1, 0.333333 },
  { 1, 1, 0.666667 },

  { 0, 1, 0 },
  { 0, 1, 1 },
  { 0, 1, 0.333333 },
  { 0, 1, 0.666667 },
};

static bool SnapFace(vtkPoints* pts)
{
  int face1PtIds[] = { 1, 2, 5, 6, 9, 13, 17, 18, 21 };
  double face1PtDeltaX[] = { -0.10, -0.10, -0.10, -0.10, -0.05, -0.05, -0.05, -0.05, 0.0 };
  double face1PtDeltaY[] = { -0.10, +0.10, -0.10, +0.10, 0.00, 0.00, -0.05, +0.05, 0.0 };
  double face1PtDeltaZ[] = { -0.10, -0.10, +0.10, +0.10, -0.05, +0.05, 0.00, 0.00, 0.0 };
  vtkVector3d xx;
  for (unsigned ii = 0; ii < sizeof(face1PtIds) / sizeof(face1PtIds[0]); ++ii)
  {
    pts->GetPoint(face1PtIds[ii], xx.GetData());
    xx[0] += face1PtDeltaX[ii];
    xx[1] += face1PtDeltaY[ii];
    xx[2] += face1PtDeltaZ[ii];
    pts->SetPoint(face1PtIds[ii], xx.GetData());
  }
  return true;
}

static vtkSmartPointer<vtkLagrangeHexahedron> CreateCell(const vtkVector3i& testOrder)
{
  // Create a hex cell:
  vtkSmartPointer<vtkLagrangeHexahedron> hex = vtkSmartPointer<vtkLagrangeHexahedron>::New();
  hex->SetOrder(testOrder[0], testOrder[1], testOrder[2]);
  vtkSmartPointer<vtkPoints> pts = vtkSmartPointer<vtkPoints>::New();
  vtkLagrangeInterpolation::AppendHexahedronCollocationPoints(pts, testOrder.GetData());
  if (testOrder[0] == 2)
  {
    SnapFace(pts);
  }
  vtkIdType npts = pts->GetNumberOfPoints();
  std::cout << "Creating hex order " << testOrder << " with " << npts << " vertices\n";
  std::vector<vtkIdType> conn(npts);
  for (int c = 0; c < npts; ++c)
  {
    conn[c] = c;

    /*
    vtkVector3d pt;
    pts->GetPoint(c, pt.GetData());
    std::cout << "  " << c << "   " << pt << "\n";
    */
  }
  vtkCell* hexc = hex.GetPointer();
  hexc->Initialize(npts, &conn[0], pts);

  return hex;
}

template <typename T>
bool TestDOFIndices(T hex, const int* expectedDOFIndices)
{
  // A. Test DOF index lookup
  const int* testOrder = hex->GetOrder();

  std::cout << "Test index conversion for order (" << testOrder[0] << " " << testOrder[1] << " "
            << testOrder[2] << "):\n";
  int ee = 0;
  bool ok = true;
  for (int kk = 0; kk <= testOrder[2]; ++kk)
  {
    for (int jj = 0; jj <= testOrder[1]; ++jj)
    {
      for (int ii = 0; ii <= testOrder[0]; ++ii)
      {
        std::ostringstream tname;
        tname << "  PointIndexFromIJK(" << ii << ", " << jj << ", " << kk
              << ") == " << expectedDOFIndices[ee];
        // std::cout << tname.str() << " " << hex->PointIndexFromIJK(ii, jj, kk) << "\n";
        ok &= testEqual(hex->PointIndexFromIJK(ii, jj, kk), expectedDOFIndices[ee++], tname.str());
      }
    }
  }
  std::cout << "\n";
  return ok;
}

template <typename T>
bool TestGetFace(T hex, const double expected[][3])
{
  bool ok = true;
  int nn = 0;
  for (int faceId = 0; faceId < hex->GetNumberOfFaces(); ++faceId)
  {
    vtkLagrangeQuadrilateral* qq = vtkLagrangeQuadrilateral::SafeDownCast(hex->GetFace(faceId));
    testNotNull(qq, "GetFace: returns a non-NULL Lagrange quadrilateral");
    vtkIdType npts = qq->GetPointIds()->GetNumberOfIds();
    for (int pp = 0; pp < npts; ++pp)
    {
      vtkVector3d pt;
      // qq->GetPoints()->GetPoint(qq->GetPointIds()->GetId(pp), pt.GetData());
      qq->GetPoints()->GetPoint(pp, pt.GetData());
      std::ostringstream tname;
      tname << "  GetFace(" << faceId << ") point " << pp << " = " << pt;
      ok &= testNearlyEqual(pt, vtkVector3d(expected[nn++]), tname.str(), 1e-5);
    }
  }
  return ok;
}

template <typename T>
bool TestGetEdge(T hex, const double expected[][3])
{
  bool ok = true;
  int nn = 0;
  for (int edgeId = 0; edgeId < hex->GetNumberOfEdges(); ++edgeId)
  {
    vtkLagrangeCurve* cc = vtkLagrangeCurve::SafeDownCast(hex->GetEdge(edgeId));
    testNotNull(cc, "GetEdge: returns a non-NULL Lagrange curve");
    vtkIdType npts = cc->GetPointIds()->GetNumberOfIds();
    for (int pp = 0; pp < npts; ++pp)
    {
      vtkVector3d pt;
      // cc->GetPoints()->GetPoint(cc->GetPointIds()->GetId(pp), pt.GetData());
      cc->GetPoints()->GetPoint(pp, pt.GetData());
      std::ostringstream tname;
      tname << "  GetEdge(" << edgeId << ") point " << pp << " = " << pt;
      testNearlyEqual(pt, vtkVector3d(expected[nn++]), tname.str(), 1e-5);
    }
  }
  return ok;
}

template <typename T>
bool TestEvaluation(T& hex)
{
  bool ok = true;

  // A. EvaluateLocation: Convert parametric to world coordinates.
  int subId = -100;
  vtkVector3d param(1., 1., 1.);
  vtkVector3d posn;
  std::vector<double> shape(hex->GetPoints()->GetNumberOfPoints());
  hex->EvaluateLocation(subId, param.GetData(), posn.GetData(), &shape[0]);
  std::cout << "\nEvaluateLocation" << param << " -> " << posn << "\n";
  ok &= testEqual(subId, 0, "EvaluateLocation: subId should be 0");
  vtkVector3d p6;
  hex->GetPoints()->GetPoint(6, p6.GetData());
  ok &= testNearlyEqual(posn, p6, "EvaluateLocation: interpolate point coordinates");

  // B. EvaluatePosition: Convert world to parametric coordinates.
  vtkVector3d closest;
  double minDist2 = -1.0; // invalid
  int result = hex->EvaluatePosition(
    posn.GetData(), closest.GetData(), subId, param.GetData(), minDist2, &shape[0]);
  std::cout << "\nEvaluatePosition" << posn << " -> " << param << " dist " << minDist2 << " subid "
            << subId << " status " << result << "\n";
  ok &= testEqual(result, 1, "EvaluatePosition: proper return code for interior point");
  ok &= testNearlyEqual(
    param, vtkVector3d(1., 1., 1.), "EvaluatePosition: returned parametric coordinates");
  ok &= testNearlyEqual(closest, posn, "EvaluatePosition: test point interior to hex");
  ok &= testEqual(minDist2, 0.0, "EvaluatePosition: squared minimum distance should be 0");
  ok &= testEqual(subId, 7, "EvaluatePosition: point should be in last sub-hex");

  return ok;
}

template <typename T>
bool TestIntersection(T hex)
{
  bool ok = true;
  double testLines[][3] = {
    { +2., +2., +2. },
    { 0., 0., 0. },
    { +1.5, 0., +1. },
    { 0., 0., 0. },
    { -2., -2., -2. },
    { -3., -2., -1. },
  };
  int testLineStatus[] = {
    1,
    1,
    0,
  };
  /*
  double testLineParam[] = {
    0.571429,
    0.0714286,
    VTK_DOUBLE_MAX
  };
  */
  for (unsigned tl = 0; tl < sizeof(testLines) / sizeof(testLines[0]) / 2; ++tl)
  {
    vtkVector3d p0(testLines[2 * tl]);
    vtkVector3d p1(testLines[2 * tl + 1]);
    double tol = 1e-7;
    double t;
    vtkVector3d x;
    vtkVector3d p;
    int subId = -1;
    int stat =
      hex->IntersectWithLine(p0.GetData(), p1.GetData(), tol, t, x.GetData(), p.GetData(), subId);
    std::cout << "\nIntersectWithLine " << p0 << " -- " << p1 << " stat " << stat << " t " << t
              << "\n       "
              << " subId " << subId << " x " << x << " p " << p << "\n";
    std::ostringstream tname;
    tname << "IntersectWithLine: status should be " << testLineStatus[tl];
    ok &= testEqual(stat, testLineStatus[tl], tname.str());
    // Commented out until we can validate:
    // ok &= testNearlyEqual(t, testLineParam[tl], "IntersectWithLine: line parameter",
    // testLineParam[tl]*1e-5);
  }
  return ok;
}

template <typename T>
bool TestContour(T hex)
{
  bool ok = true;
  double testPlanes[][6] = {
    { 0., 0., 0., 1., 0., 0. },
    { 0., 0., 0., 0., 1., 0. },
    { 0., 0., 0., 0., 0., 1. },
  };
  int testContourPointCount[] = {
    (hex->GetOrder()[0] + 1) * (hex->GetOrder()[1] + 1),
    (hex->GetOrder()[1] + 1) * (hex->GetOrder()[2] + 1),
    (hex->GetOrder()[2] + 1) * (hex->GetOrder()[0] + 1),
  };
  for (unsigned tp = 0; tp < sizeof(testPlanes) / sizeof(testPlanes[0]); ++tp)
  {
    vtkVector3d origin(testPlanes[tp]);
    vtkVector3d normal(testPlanes[tp] + 3);
    int np = hex->GetNumberOfPoints();
    vtkNew<vtkDoubleArray> contourScalars;
    vtkNew<vtkPoints> contourPoints;
    vtkNew<vtkIncrementalOctreePointLocator> locator;
    vtkNew<vtkCellArray> verts;
    vtkNew<vtkCellArray> lines;
    vtkNew<vtkCellArray> polys;
    vtkNew<vtkPointData> inPd;
    vtkNew<vtkPointData> outPd;
    vtkNew<vtkCellData> inCd;
    vtkNew<vtkCellData> outCd;
    contourScalars->SetNumberOfTuples(np);
    locator->InitPointInsertion(contourPoints, hex->GetBounds());
    for (vtkIdType ii = 0; ii < np; ++ii)
    {
      vtkVector3d pt(hex->GetPoints()->GetPoint(ii));
      double distance = normal.Dot(origin - pt);
      contourScalars->SetTuple1(ii, distance);
    }
    hex->Contour(
      0.0, contourScalars, locator, verts, lines, polys, inPd, outPd, inCd, /* cellId */ 0, outCd);

    int stat = static_cast<int>(contourPoints->GetNumberOfPoints());
    std::cout << "\nContour planar function: orig " << origin << " norm " << normal << "\n";
    std::ostringstream tname;
    tname << "Contour: num points out should be " << testContourPointCount[tp];
    ok &= testEqual(stat, testContourPointCount[tp], tname.str());
    for (int pp = 0; pp < stat; ++pp)
    {
      vtkVector3d pt(contourPoints->GetPoint(pp));
      double distance = normal.Dot(origin - pt);
      std::ostringstream testName;
      testName << "  Contour point " << pp << ": distance ";
      ok &= testNearlyEqual(distance, 0.0, testName.str(), 1e-5);
    }
  }
  return ok;
}

int LagrangeHexahedron(int argc, char* argv[])
{
  (void)argc;
  (void)argv;
  bool ok = true;

  vtkVector3i testOrder1(1, 1, 1);
  vtkSmartPointer<vtkLagrangeHexahedron> hex1 = CreateCell(testOrder1);

  vtkVector3i testOrder2(2, 2, 2);
  vtkSmartPointer<vtkLagrangeHexahedron> hex2 = CreateCell(testOrder2);

  vtkVector3i testOrder3(3, 3, 3);
  vtkSmartPointer<vtkLagrangeHexahedron> hex3 = CreateCell(testOrder3);

  // I. Low-level methods
  ok &= TestDOFIndices(hex1, expectedDOFIndices1);
  ok &= TestDOFIndices(hex2, expectedDOFIndices2);
  ok &= TestDOFIndices(hex3, expectedDOFIndices3);
  ok &= TestGetFace(hex3, expectedFacePoints333);
  ok &= TestGetEdge(hex3, expectedEdgePoints333);

  // II. High-level methods
  ok &= TestEvaluation(hex2);
  ok &= TestIntersection(hex1);
  ok &= TestIntersection(hex2);
  ok &= TestIntersection(hex3);
  ok &= TestContour(hex1);
  ok &= TestContour(hex2);
  ok &= TestContour(hex3);

  return ok ? 0 : 1;
}
