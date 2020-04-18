#include <vtkActor.h>
#include <vtkAppendFilter.h>
#include <vtkBezierCurve.h>
#include <vtkBezierHexahedron.h>
#include <vtkCamera.h>
#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkDataArray.h>
#include <vtkDataSetMapper.h>
#include <vtkDoubleArray.h>
#include <vtkIdList.h>
#include <vtkNamedColors.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyhedron.h>
#include <vtkProperty.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkTessellatorFilter.h>
#include <vtkUnstructuredGrid.h>
#include <vtkXMLUnstructuredGridReader.h>
#include <vtkXMLUnstructuredGridWriter.h>

// The 3D cell with the maximum number of points is VTK_LAGRANGE_HEXAHEDRON.
// We support up to 6th order hexahedra.
static const int VTK_MAXIMUM_NUMBER_OF_POINTS = 216;

void img_compare(vtkUnstructuredGrid* ugrid2, bool tessellate = true)
{
  vtkSmartPointer<vtkNamedColors> colors = vtkSmartPointer<vtkNamedColors>::New();
  vtkSmartPointer<vtkTessellatorFilter> tess = vtkSmartPointer<vtkTessellatorFilter>::New();
  tess->SetMaximumNumberOfSubdivisions(3);
  tess->SetInputData(ugrid2);

  // Create a mapper and actor
  vtkSmartPointer<vtkDataSetMapper> mapper = vtkSmartPointer<vtkDataSetMapper>::New();
  if (tessellate)
  {
    mapper->SetInputConnection(tess->GetOutputPort());
  }
  else
  {
    mapper->SetInputData(ugrid2);
  }

  vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);
  actor->GetProperty()->SetColor(colors->GetColor3d("red").GetData());

  // Visualize
  vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
  renderWindow->SetWindowName("BezierHexahedron");
  renderWindow->AddRenderer(renderer);
  vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  renderWindowInteractor->SetRenderWindow(renderWindow);

  renderer->AddActor(actor);
  renderer->SetBackground(colors->GetColor3d("black").GetData());
  renderer->ResetCamera();
  renderer->GetActiveCamera()->Azimuth(0);
  renderer->GetActiveCamera()->Elevation(0);
  renderWindow->SetSize(1024, 1024);
  renderWindow->Render();
  // renderWindowInteractor->Start();  //Uncomment this line to stop the visualization
  int argc = 0;
  char* argv[] = { nullptr };
  int retVal = vtkRegressionTestImage(renderWindow);

  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
  }
}

void img_compare(const std::string& filename)
{
  vtkSmartPointer<vtkXMLUnstructuredGridReader> reader =
    vtkSmartPointer<vtkXMLUnstructuredGridReader>::New();
  reader->SetFileName(filename.c_str());
  // reader->SetFileName("wedge.vtu");
  // reader->SetFileName("lag_wedge_linear.vtu");
  // reader->SetFileName("tri_rat.vtu");
  reader->Update();
  vtkUnstructuredGrid* ugrid2 = reader->GetOutput();
  img_compare(ugrid2);
}

int TestBezier(int, char*[])
{
  vtkSmartPointer<vtkNamedColors> colors = vtkSmartPointer<vtkNamedColors>::New();

  ///////////////////////////// TEST VTK_BEZIER_CURVE_quadratic_quarter_circle
  /////////////////////////////////////////
  {
    vtkIdType pointIds[11] = { 0, 1, 2 };

    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkDoubleArray> rationalWeights = vtkSmartPointer<vtkDoubleArray>::New();
    const double sqrt2over2 = std::sqrt(2.) / 2.;
    rationalWeights->SetName("RationalWeights");
    rationalWeights->SetNumberOfComponents(1);
    rationalWeights->SetNumberOfTuples(3);
    points->InsertNextPoint(0.0, 1.0, 0.0);
    rationalWeights->SetValue(0, 1.0);
    points->InsertNextPoint(1.0, 0.0, 0.0);
    rationalWeights->SetValue(1, 1.0);
    points->InsertNextPoint(1.0, 1.0, 0.0);
    rationalWeights->SetValue(2, sqrt2over2);

    vtkSmartPointer<vtkUnstructuredGrid> ugrid = vtkSmartPointer<vtkUnstructuredGrid>::New();
    ugrid->SetPoints(points);
    // ugrid->InsertNextCell( VTK_LAGRANGE_CURVE, 3, pointIds );
    ugrid->InsertNextCell(VTK_BEZIER_CURVE, 3, pointIds);
    ugrid->GetPointData()->SetRationalWeights(rationalWeights);

    vtkCell* cell = ugrid->GetCell(0);
    int subId = 0;
    const double pcoords[3]{ 0.23, 0, 0 };
    double x[3];
    double weights[VTK_MAXIMUM_NUMBER_OF_POINTS];
    cell->EvaluateLocation(subId, pcoords, x, weights);

    vtkBezierCurve* cellBezier = dynamic_cast<vtkBezierCurve*>(cell);

    vtkDoubleArray* rationalWeightsCell = cellBezier->GetRationalWeights();

    int numPts = cell->Points->GetNumberOfPoints();
    int numWeights = cellBezier->GetRationalWeights()->GetNumberOfTuples();
    if (numWeights != numPts)
    {
      vtkGenericWarningMacro(
        "VTK_BEZIER_CURVE_quadratic_quarter_circle is failing. The number of rational weights "
        "should be the same as the number of point of the cell ( "
        << numPts << " ), but is " << numWeights);
      return EXIT_FAILURE;
    }
    else
    {
      for (int i = 0; i < numPts; i++)
      {
        if (rationalWeightsCell->GetTuple1(i) != rationalWeights->GetValue(i))
        {
          vtkGenericWarningMacro("VTK_BEZIER_CURVE_quadratic_quarter_circle is failing. The "
                                 "weights of the cell should be the same as the input");
          return EXIT_FAILURE;
        }
      }
    }

    if (std::abs(sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]) - 1) > 1e-12)
    {
      vtkGenericWarningMacro("VTK_BEZIER_CURVE_quadratic_quarter_circle is failing. Its radius is "
                             "supposed to be one and is instead "
        << sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]));
      return EXIT_FAILURE;
    }

    // Here we write out the cube.
    vtkSmartPointer<vtkXMLUnstructuredGridWriter> writer =
      vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
    writer->SetInputData(ugrid);
    writer->SetFileName("VTK_BEZIER_CURVE_quadratic_quarter_circle.vtu");
    writer->SetDataModeToAscii();
    writer->Update();
    img_compare(ugrid, false);
    img_compare(ugrid, true);
  }

  ///////////////////////////// TEST VTK_BEZIER_QUADRILATERAL_linearquadratic_quarter_disk
  /////////////////////////////////////////
  {
    vtkIdType pointIds[6] = { 0, 1, 2, 3, 4, 5 };

    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkDoubleArray> rationalWeights = vtkSmartPointer<vtkDoubleArray>::New();
    vtkSmartPointer<vtkDoubleArray> degrees = vtkSmartPointer<vtkDoubleArray>::New();
    const double sqrt2over2 = std::sqrt(2.) / 2.;
    rationalWeights->SetName("RationalWeights");
    rationalWeights->SetNumberOfComponents(1);
    rationalWeights->SetNumberOfTuples(6);
    points->InsertNextPoint(0.0, 1.0, 0.0);
    rationalWeights->SetValue(0, 1.0);
    points->InsertNextPoint(1.0, 0.0, 0.0);
    rationalWeights->SetValue(1, 1.0);
    points->InsertNextPoint(2.0, 0.0, 0.0);
    rationalWeights->SetValue(2, 1.0);
    points->InsertNextPoint(0.0, 2.0, 0.0);
    rationalWeights->SetValue(3, 1.0);

    points->InsertNextPoint(1.0, 1.0, 0.0);
    rationalWeights->SetValue(4, sqrt2over2);
    points->InsertNextPoint(2, 2, 0.0);
    rationalWeights->SetValue(5, sqrt2over2);

    vtkSmartPointer<vtkUnstructuredGrid> ugrid = vtkSmartPointer<vtkUnstructuredGrid>::New();
    ugrid->SetPoints(points);
    ugrid->InsertNextCell(VTK_BEZIER_QUADRILATERAL, 6, pointIds);
    degrees->SetName("HigherOrderDegrees");
    degrees->SetNumberOfComponents(3);
    degrees->InsertNextTuple3(2, 1, 0);

    ugrid->GetPointData()->SetRationalWeights(rationalWeights);
    ugrid->GetCellData()->SetHigherOrderDegrees(degrees);

    vtkCell* cell = ugrid->GetCell(0);
    int subId = 0;
    double pcoords[3]{ 0.23, 0, 0 };
    double x[3];
    double weights[VTK_MAXIMUM_NUMBER_OF_POINTS];
    cell->EvaluateLocation(subId, pcoords, x, weights);

    if (std::abs(sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]) - 1) > 1e-12)
    {
      vtkGenericWarningMacro("VTK_BEZIER_QUADRILATERAL_linearquadratic_quarter_disk is failing. "
                             "Its radius is supposed to be 1 and is instead "
        << sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]));
      return EXIT_FAILURE;
    }

    pcoords[1] = 1;
    cell->EvaluateLocation(subId, pcoords, x, weights);
    if (std::abs(sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]) - 2) > 1e-12)
    {
      vtkGenericWarningMacro("VTK_BEZIER_QUADRILATERAL_linearquadratic_quarter_disk is failing. "
                             "Its radius is supposed to be 2 and is instead "
        << sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]));
      return EXIT_FAILURE;
    }

    // Here we write out the cube.
    vtkSmartPointer<vtkXMLUnstructuredGridWriter> writer =
      vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
    writer->SetInputData(ugrid);
    writer->SetFileName("VTK_BEZIER_QUADRILATERAL_linearquadratic_quarter_disk.vtu");
    writer->SetDataModeToAscii();
    writer->Update();
    img_compare(ugrid, false);
    img_compare(ugrid, true);
  }
  ///////////////////////////// TEST VTK_BEZIER_TRIANGLE_quadratic_full_disk
  /////////////////////////////////////////

  {
    // create quadratic tri rat
    vtkIdType pointIds[11] = { 0, 1, 2, 3, 4, 5 };

    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkDoubleArray> rationalWeights = vtkSmartPointer<vtkDoubleArray>::New();
    const double sqrt3over3 = std::sqrt(3.) / 3.;
    rationalWeights->SetName("RationalWeights");
    rationalWeights->SetNumberOfComponents(1);
    rationalWeights->SetNumberOfTuples(6);
    points->InsertNextPoint(0, -sqrt3over3, 0);
    rationalWeights->SetValue(0, 1.0);
    points->InsertNextPoint(0.5, sqrt3over3 / 2.0, 0);
    rationalWeights->SetValue(1, 1.0);
    points->InsertNextPoint(-0.5, sqrt3over3 / 2.0, 0);
    rationalWeights->SetValue(2, 1.0);
    points->InsertNextPoint(1, -sqrt3over3, 0);
    rationalWeights->SetValue(3, 0.5);
    points->InsertNextPoint(0, 2 * sqrt3over3, 0);
    rationalWeights->SetValue(4, 0.5);
    points->InsertNextPoint(-1, -sqrt3over3, 0);
    rationalWeights->SetValue(5, 0.5);

    vtkSmartPointer<vtkUnstructuredGrid> ugrid = vtkSmartPointer<vtkUnstructuredGrid>::New();
    ugrid->SetPoints(points);
    ugrid->InsertNextCell(VTK_BEZIER_TRIANGLE, 6, pointIds);
    ugrid->GetPointData()->SetRationalWeights(rationalWeights);

    // Here we write out the cube.
    vtkSmartPointer<vtkXMLUnstructuredGridWriter> writer =
      vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
    writer->SetInputData(ugrid);
    writer->SetFileName("VTK_BEZIER_TRIANGLE_quadratic_full_disk.vtu");
    writer->SetDataModeToAscii();
    writer->Update();
    // img_compare(ugrid, false);
    img_compare(ugrid, true);

    vtkCell* cell = ugrid->GetCell(0);
    int subId = 0;
    double pcoords[3]{ 0.23, 0, 0 };
    double x[3];
    double weights[VTK_MAXIMUM_NUMBER_OF_POINTS];
    cell->EvaluateLocation(subId, pcoords, x, weights);

    const double radius = sqrt3over3;
    if (std::abs(sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]) - radius) > 1e-8)
    {
      vtkGenericWarningMacro(
        "VTK_BEZIER_TRIANGLE_quadratic_full_disk is failing 1. Its radius is supposed to be "
        << radius << " and is instead " << sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2])
        << " diff is " << sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]) - radius);
      return EXIT_FAILURE;
    }

    pcoords[0] = 0;
    pcoords[1] = 0.23;
    cell->EvaluateLocation(subId, pcoords, x, weights);
    if (std::abs(sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]) - radius) > 1e-8)
    {
      vtkGenericWarningMacro(
        "VTK_BEZIER_TRIANGLE_quadratic_full_disk is failing 2. Its radius is supposed to be "
        << radius << " and is instead " << sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2])
        << " diff is " << sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]) - radius);
      return EXIT_FAILURE;
    }

    pcoords[0] = 0.5;
    pcoords[1] = 0.5;
    cell->EvaluateLocation(subId, pcoords, x, weights);
    if (std::abs(sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]) - radius) > 1e-7)
    {
      vtkGenericWarningMacro(
        "VTK_BEZIER_TRIANGLE_quadratic_full_disk is failing 3. Its radius is supposed to be "
        << radius << " and is instead " << sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2])
        << " diff is " << sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]) - radius);
      return EXIT_FAILURE;
    }
  }

  ///////////////////////////// TEST VTK_BEZIER_TRIANGLE_quartic_sphereOctant
  /////////////////////////////////////////
  {
    const int nPoints = 15;

    vtkIdType pointIds[nPoints];
    for (int i = 0; i < nPoints; i++)
      pointIds[i] = i;

    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkDoubleArray> rationalWeights = vtkSmartPointer<vtkDoubleArray>::New();
    rationalWeights->SetName("RationalWeights");
    rationalWeights->SetNumberOfComponents(1);
    rationalWeights->SetNumberOfTuples(nPoints);
    points->SetNumberOfPoints(nPoints);

    // The points and weights can be found in this paper:
    // Farin, Piper and Worsey, 1987, Computer Aided Geometric Design
    // The octant of a sphere as a non-degenerate triangular Bézier patch

    const double c1 = (sqrt(3) - 1.) / sqrt(3);
    const double c2 = (sqrt(3) + 1.) / (2. * sqrt(3));
    const double c3 = 1. - (5. - sqrt(2)) * (7. - sqrt(3)) / 46.;

    points->SetPoint(0, 1, 0, 0);
    points->SetPoint(1, 0, 1, 0);
    points->SetPoint(2, 0, 0, 1);
    points->SetPoint(3, 1, c1, 0);
    points->SetPoint(4, c2, c2, 0);
    points->SetPoint(5, c1, 1, 0);
    points->SetPoint(6, 0, 1, c1);
    points->SetPoint(7, 0, c2, c2);
    points->SetPoint(8, 0, c1, 1);
    points->SetPoint(9, c1, 0, 1);
    points->SetPoint(10, c2, 0, c2);
    points->SetPoint(11, 1, 0, c1);
    points->SetPoint(12, 1, c3, c3);
    points->SetPoint(13, c3, 1, c3);
    points->SetPoint(14, c3, c3, 1);

    const double w1 = 4 * sqrt(3) * (sqrt(3) - 1.);
    const double w2 = 3 * sqrt(2);
    const double w3 = sqrt(2. / 3.) * (3. + 2. * sqrt(2) - sqrt(3));

    double rationalWeightsTps2[nPoints] = {
      w1,
      w1,
      w1,
      w2,
      4.,
      w2,
      w2,
      4.,
      w2,
      w2,
      4.,
      w2,
      w3,
      w3,
      w3,
    };
    for (int i = 0; i < nPoints; i++)
    {
      rationalWeights->SetValue(i, rationalWeightsTps2[i]);
    }

    vtkSmartPointer<vtkUnstructuredGrid> ugrid = vtkSmartPointer<vtkUnstructuredGrid>::New();
    ugrid->SetPoints(points);
    ugrid->InsertNextCell(VTK_BEZIER_TRIANGLE, nPoints, pointIds);
    ugrid->GetPointData()->SetRationalWeights(rationalWeights);

    // Here we write out the cube.
    vtkSmartPointer<vtkXMLUnstructuredGridWriter> writer =
      vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
    writer->SetInputData(ugrid);
    writer->SetFileName("VTK_BEZIER_TRIANGLE_quartic_sphereOctant.vtu");
    writer->SetDataModeToAscii();
    writer->Update();

    img_compare(ugrid, false);
    // img_compare(ugrid, true);

    vtkCell* cell = ugrid->GetCell(0);
    int subId = 0;
    double pcoords[3]{ 0.23, 0.654, 0 };
    double x[3];
    double weights[VTK_MAXIMUM_NUMBER_OF_POINTS];
    cell->EvaluateLocation(subId, pcoords, x, weights);

    double radius = 1;
    if (std::abs(sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]) - radius) > 1e-8)
    {
      vtkGenericWarningMacro(
        "VTK_BEZIER_TRIANGLE_quartic_sphereOctant is failing 1. Its radius is supposed to be "
        << radius << " and is instead " << sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2])
        << " diff is " << sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]) - radius);
      return EXIT_FAILURE;
    }
  }

  ///////////////////////////// TEST VTK_BEZIER_HEXAHEDRON_triquadratic_cube
  /////////////////////////////////////////
  {
    vtkIdType pointIds[27] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
      20, 21, 22, 23, 24, 25, 26 };

    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
    const int nPoints = 27;
    vtkSmartPointer<vtkDoubleArray> rationalWeights = vtkSmartPointer<vtkDoubleArray>::New();
    rationalWeights->SetName("RationalWeights");
    rationalWeights->SetNumberOfComponents(1);
    rationalWeights->SetNumberOfTuples(nPoints);
    points->SetNumberOfPoints(nPoints);
    points->SetPoint(0, 0.0, 0.0, 0.0);
    rationalWeights->SetValue(0, 1.0);
    points->SetPoint(1, 1.0, 0.0, 0.0);
    rationalWeights->SetValue(1, 1.0);
    points->SetPoint(2, 1.0, 1.0, 0.0);
    rationalWeights->SetValue(2, 1.0);
    points->SetPoint(3, 0.0, 1.0, 0.0);
    rationalWeights->SetValue(3, 1.0);
    points->SetPoint(4, 0.0, 0.0, 1.0);
    rationalWeights->SetValue(4, 1.0);
    points->SetPoint(5, 1.0, 0.0, 1.0);
    rationalWeights->SetValue(5, 1.0);
    points->SetPoint(6, 1.0, 1.0, 1.0);
    rationalWeights->SetValue(6, 1.0);
    points->SetPoint(7, 0.0, 1.0, 1.0);
    rationalWeights->SetValue(7, 1.0);

    points->SetPoint(8, 0.5, 0.0, 0.0);
    rationalWeights->SetValue(8, 1.0);
    points->SetPoint(9, 1.0, 0.5, 0.0);
    rationalWeights->SetValue(9, 1.0);
    points->SetPoint(10, 0.5, 1.0, 0.0);
    rationalWeights->SetValue(10, 1.0);
    points->SetPoint(11, 0.0, 0.5, 0.0);
    rationalWeights->SetValue(11, 1.0);

    points->SetPoint(12, 0.5, 0.0, 1.0);
    rationalWeights->SetValue(12, 1.0);
    points->SetPoint(13, 1.0, 0.5, 1.0);
    rationalWeights->SetValue(13, 1.0);
    points->SetPoint(14, 0.5, 1.0, 1.0);
    rationalWeights->SetValue(14, 1.0);
    points->SetPoint(15, 0.0, 0.5, 1.0);
    rationalWeights->SetValue(15, 1.0);

    points->SetPoint(16, 0.0, 0.0, 0.5);
    rationalWeights->SetValue(16, 1.0);
    points->SetPoint(17, 1.0, 0.0, 0.5);
    rationalWeights->SetValue(17, 1.0);
    points->SetPoint(18, 1.0, 1.0, 0.5);
    rationalWeights->SetValue(18, 1.0);
    points->SetPoint(19, 0.0, 1.0, 0.5);
    rationalWeights->SetValue(19, 1.0);

    points->SetPoint(20, 0.0, 0.5, 0.5);
    rationalWeights->SetValue(20, 1.2);
    points->SetPoint(21, 1.0, 0.5, 0.5);
    rationalWeights->SetValue(21, 1.2);
    points->SetPoint(22, 0.5, 0.0, 0.5);
    rationalWeights->SetValue(22, 1.2);
    points->SetPoint(23, 0.5, 1.0, 0.5);
    rationalWeights->SetValue(23, 1.2);
    points->SetPoint(24, 0.5, 0.5, 0.0);
    rationalWeights->SetValue(24, 1.2);
    points->SetPoint(25, 0.5, 0.5, 1.0);
    rationalWeights->SetValue(25, 1.2);
    points->SetPoint(26, 0.5, 0.5, 0.5);
    rationalWeights->SetValue(26, 1.2);
    vtkSmartPointer<vtkUnstructuredGrid> ugrid = vtkSmartPointer<vtkUnstructuredGrid>::New();
    ugrid->GetPointData()->SetRationalWeights(rationalWeights);
    ugrid->SetPoints(points);
    ugrid->InsertNextCell(VTK_BEZIER_HEXAHEDRON, 27, pointIds);

    // Here we write out the cube.
    vtkSmartPointer<vtkXMLUnstructuredGridWriter> writer =
      vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
    writer->SetInputData(ugrid);
    writer->SetFileName("VTK_BEZIER_HEXAHEDRON_triquadratic_cube.vtu");
    writer->SetDataModeToAscii();
    writer->Update();
    img_compare(ugrid, false);
    // img_compare(ugrid, true);
  }

  ///////////////////////////// TEST VTK_BEZIER_HEXAHEDRON_triquartic_full_sphere
  /////////////////////////////////////////
  {
    const int nPoints = 125;

    vtkIdType pointIds[nPoints];
    for (int i = 0; i < nPoints; i++)
      pointIds[i] = i;

    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkDoubleArray> rationalWeights = vtkSmartPointer<vtkDoubleArray>::New();
    rationalWeights->SetName("RationalWeights");
    rationalWeights->SetNumberOfComponents(1);
    rationalWeights->SetNumberOfTuples(nPoints);
    points->SetNumberOfPoints(nPoints);

    double pointsTps[3 * nPoints] = { -0.5773502588272095, -0.5773502588272095, -0.5773502588272095,
      0.5773502588272095, -0.5773502588272095, -0.5773502588272095, 0.5773502588272095,
      0.5773502588272095, -0.5773502588272095, -0.5773502588272095, 0.5773502588272095,
      -0.5773502588272095, -0.5773502588272095, -0.5773502588272095, 0.5773502588272095,
      0.5773502588272095, -0.5773502588272095, 0.5773502588272095, 0.5773502588272095,
      0.5773502588272095, 0.5773502588272095, -0.5773502588272095, 0.5773502588272095,
      0.5773502588272095, -0.3128761947154999, -0.7095873355865479, -0.7095873355865479,
      1.573255109901025e-14, -0.7540208101272583, -0.7540208101272583, 0.3128761947154999,
      -0.7095873355865479, -0.7095873355865479, 0.7095873355865479, -0.3128761947154999,
      -0.7095873355865479, 0.7540208101272583, 2.990022576862712e-14, -0.7540208101272583,
      0.7095873355865479, 0.3128761947154999, -0.7095873355865479, -0.3128761947154999,
      0.7095873355865479, -0.7095873355865479, 2.731565719601113e-14, 0.7540208101272583,
      -0.7540208101272583, 0.3128761947154999, 0.7095873355865479, -0.7095873355865479,
      -0.7095873355865479, -0.3128761947154999, -0.7095873355865479, -0.7540208101272583,
      6.634061992783456e-15, -0.7540208101272583, -0.7095873355865479, 0.3128761947154999,
      -0.7095873355865479, -0.3128761947154999, -0.7095873355865479, 0.7095873355865479,
      2.5118773909287538e-14, -0.7540208101272583, 0.7540208101272583, 0.3128761947154999,
      -0.7095873355865479, 0.7095873355865479, 0.7095873355865479, -0.3128761947154999,
      0.7095873355865479, 0.7540208101272583, 1.9901176315077242e-14, 0.7540208101272583,
      0.7095873355865479, 0.3128761947154999, 0.7095873355865479, -0.3128761947154999,
      0.7095873355865479, 0.7095873355865479, 1.2922842016047011e-14, 0.7540208101272583,
      0.7540208101272583, 0.3128761947154999, 0.7095873355865479, 0.7095873355865479,
      -0.7095873355865479, -0.3128761947154999, 0.7095873355865479, -0.7540208101272583,
      1.697738459961306e-14, 0.7540208101272583, -0.7095873355865479, 0.3128761947154999,
      0.7095873355865479, -0.7095873355865479, -0.7095873355865479, -0.3128761947154999,
      -0.7540208101272583, -0.7540208101272583, 1.5042995976210626e-15, -0.7095873355865479,
      -0.7095873355865479, 0.3128761947154999, 0.7095873355865479, -0.7095873355865479,
      -0.3128761947154999, 0.7540208101272583, -0.7540208101272583, 4.377612808110099e-15,
      0.7095873355865479, -0.7095873355865479, 0.3128761947154999, 0.7095873355865479,
      0.7095873355865479, -0.3128761947154999, 0.7540208101272583, 0.7540208101272583,
      1.3068224210014898e-14, 0.7095873355865479, 0.7095873355865479, 0.3128761947154999,
      -0.7095873355865479, 0.7095873355865479, -0.3128761947154999, -0.7540208101272583,
      0.7540208101272583, 8.529075442599825e-15, -0.7095873355865479, 0.7095873355865479,
      0.3128761947154999, -1, -0.41336411237716675, -0.41336411237716675, -1.1200461387634277,
      1.356080266796348e-14, -0.45730409026145935, -1, 0.41336411237716675, -0.41336411237716675,
      -1.1200461387634277, -0.45730409026145935, 4.0291368467526914e-14, -1.279833197593689,
      6.071910959052837e-14, -1.1453314391460159e-14, -1.1200461387634277, 0.45730409026145935,
      2.2303634498099453e-14, -1, -0.41336411237716675, 0.41336411237716675, -1.1200461387634277,
      -1.0080084248580753e-14, 0.45730409026145935, -1, 0.41336411237716675, 0.41336411237716675, 1,
      -0.41336411237716675, -0.41336411237716675, 1.1200461387634277, -6.3338459877057465e-15,
      -0.45730409026145935, 1, 0.41336411237716675, -0.41336411237716675, 1.1200461387634277,
      -0.45730409026145935, 3.954791409719466e-14, 1.279833197593689, 3.969379688763437e-14,
      -9.989605713814353e-14, 1.1200461387634277, 0.45730409026145935, 3.9856884611298715e-14, 1,
      -0.41336411237716675, 0.41336411237716675, 1.1200461387634277, -4.3255533161389026e-15,
      0.45730409026145935, 1, 0.41336411237716675, 0.41336411237716675, -0.41336411237716675, -1,
      -0.41336411237716675, -6.174534336920263e-15, -1.1200461387634277, -0.45730409026145935,
      0.41336411237716675, -1, -0.41336411237716675, -0.45730409026145935, -1.1200461387634277,
      2.9226451716667795e-14, 6.681789588855805e-14, -1.279833197593689, -9.117169570845539e-15,
      0.45730409026145935, -1.1200461387634277, 2.8000232612113846e-14, -0.41336411237716675, -1,
      0.41336411237716675, 6.005567357569028e-15, -1.1200461387634277, 0.45730409026145935,
      0.41336411237716675, -1, 0.41336411237716675, -0.41336411237716675, 1, -0.41336411237716675,
      -1.0504915317342042e-14, 1.1200461387634277, -0.45730409026145935, 0.41336411237716675, 1,
      -0.41336411237716675, -0.45730409026145935, 1.1200461387634277, 4.402795267038559e-14,
      8.600322771925174e-15, 1.279833197593689, -1.5215955868875218e-14, 0.45730409026145935,
      1.1200461387634277, 2.7498160608892437e-14, -0.41336411237716675, 1, 0.41336411237716675,
      1.977395704859733e-14, 1.1200461387634277, 0.45730409026145935, 0.41336411237716675, 1,
      0.41336411237716675, -0.41336411237716675, -0.41336411237716675, -1, -7.815927064087382e-15,
      -0.45730409026145935, -1.1200461387634277, 0.41336411237716675, -0.41336411237716675, -1,
      -0.45730409026145935, 2.13236258484557e-14, -1.1200461387634277, 1.0651168938806063e-13,
      3.849471333057866e-14, -1.279833197593689, 0.45730409026145935, -1.86732580624311e-14,
      -1.1200461387634277, -0.41336411237716675, 0.41336411237716675, -1, -8.515932879390228e-15,
      0.45730409026145935, -1.1200461387634277, 0.41336411237716675, 0.41336411237716675, -1,
      -0.41336411237716675, -0.41336411237716675, 1, -5.252457658671021e-15, -0.45730409026145935,
      1.1200461387634277, 0.41336411237716675, -0.41336411237716675, 1, -0.45730409026145935,
      -2.3790543238763964e-14, 1.1200461387634277, 5.1932718244046897e-14, 6.483320282545113e-14,
      1.279833197593689, 0.45730409026145935, -3.336855463709036e-14, 1.1200461387634277,
      -0.41336411237716675, 0.41336411237716675, 1, 8.96007460528302e-15, 0.45730409026145935,
      1.1200461387634277, 0.41336411237716675, 0.41336411237716675, 1, -0.6340351700782776,
      -0.6340351700782776, -0.6340351700782776, 7.311288212222614e-14, -0.7453672289848328,
      -0.7453672289848328, 0.6340351700782776, -0.6340351700782776, -0.6340351700782776,
      -0.7453672289848328, -4.57577235447198e-15, -0.7453672289848328, -1.792698205594495e-13,
      1.457957425577902e-13, -0.8993141055107117, 0.7453672289848328, 1.4125209519839227e-13,
      -0.7453672289848328, -0.6340351700782776, 0.6340351700782776, -0.6340351700782776,
      1.8541824976445187e-13, 0.7453672289848328, -0.7453672289848328, 0.6340351700782776,
      0.6340351700782776, -0.6340351700782776, -0.7453672289848328, -0.7453672289848328,
      -3.460676569529836e-13, 2.0284624403354296e-13, -0.8993141055107117, 5.85852004929277e-13,
      0.7453672289848328, -0.7453672289848328, -3.0697463342978237e-13, -0.8993141055107117,
      2.124102658357125e-14, 5.956384474190002e-13, -1.2616546262583794e-13,
      -1.5974175378730832e-13, -7.702197712088077e-13, 0.8993141055107117, 1.6014177695511042e-14,
      5.324714193871705e-13, -0.7453672289848328, 0.7453672289848328, -3.1751879771280134e-13,
      4.6263177750499596e-14, 0.8993141055107117, 3.5275675067635015e-13, 0.7453672289848328,
      0.7453672289848328, -2.880747127387978e-13, -0.6340351700782776, -0.6340351700782776,
      0.6340351700782776, 7.679339613210837e-14, -0.7453672289848328, 0.7453672289848328,
      0.6340351700782776, -0.6340351700782776, 0.6340351700782776, -0.7453672289848328,
      4.794613396087201e-14, 0.7453672289848328, 1.2455471258608717e-14, -1.4234823714730227e-13,
      0.8993141055107117, 0.7453672289848328, 6.04797719609522e-14, 0.7453672289848328,
      -0.6340351700782776, 0.6340351700782776, 0.6340351700782776, 1.2255111205353164e-13,
      0.7453672289848328, 0.7453672289848328, 0.6340351700782776, 0.6340351700782776,
      0.6340351700782776 };

    double rationalWeightsTps[nPoints] = { 1.0000000000000004, 1.0000000000000038,
      1.0000000000000182, 1.0000000000000064, 0.9999999999999962, 1.0000000000000053,
      1.0000000000000222, 1.0000000000000113, 0.8912112036083922, 0.8591167563965603,
      0.891211203608377, 0.8912112036084067, 0.8591167563965719, 0.8912112036083736,
      0.8912112036084004, 0.8591167563965604, 0.8912112036083729, 0.8912112036083879,
      0.8591167563965556, 0.8912112036083712, 0.8912112036084329, 0.8591167563965191,
      0.8912112036084098, 0.8912112036084323, 0.8591167563965256, 0.8912112036084059,
      0.8912112036084165, 0.8591167563965465, 0.891211203608397, 0.8912112036084232,
      0.8591167563965371, 0.8912112036083985, 0.8912112036083935, 0.8591167563965474,
      0.8912112036083979, 0.8912112036084096, 0.8591167563965341, 0.8912112036084131,
      0.8912112036083994, 0.8591167563965553, 0.8912112036083869, 0.8912112036084061,
      0.8591167563965514, 0.8912112036083917, 0.7622595264192012, 0.7186651735399952,
      0.7622595264192108, 0.7186651735400831, 0.6712724315919532, 0.7186651735400142,
      0.7622595264191386, 0.7186651735400778, 0.7622595264191658, 0.7622595264191585,
      0.7186651735400396, 0.7622595264191763, 0.7186651735401408, 0.6712724315918481,
      0.7186651735400713, 0.7622595264191132, 0.7186651735401441, 0.7622595264191405,
      0.7622595264191835, 0.7186651735400215, 0.7622595264191755, 0.718665173540074,
      0.6712724315918527, 0.7186651735400587, 0.76225952641914, 0.7186651735401088,
      0.7622595264191026, 0.762259526419186, 0.718665173539988, 0.7622595264191874,
      0.718665173540024, 0.671272431591901, 0.7186651735400487, 0.7622595264191483,
      0.7186651735400567, 0.7622595264191343, 0.7622595264191981, 0.7186651735400303,
      0.7622595264191959, 0.7186651735400273, 0.6712724315919835, 0.7186651735399883,
      0.7622595264191977, 0.7186651735400158, 0.7622595264191645, 0.7622595264191123,
      0.7186651735401346, 0.7622595264191119, 0.7186651735401413, 0.6712724315917651,
      0.7186651735401233, 0.7622595264191341, 0.718665173540117, 0.7622595264191148,
      0.6131449684323388, 0.5580507098859488, 0.6131449684323087, 0.558050709885957,
      0.499158062270623, 0.5580507098860268, 0.613144968432233, 0.5580507098859713,
      0.6131449684321943, 0.5580507098858419, 0.49915806227084414, 0.5580507098857239,
      0.4991580622706119, 0.43646702558565864, 0.49915806227082715, 0.5580507098859069,
      0.49915806227063975, 0.5580507098858827, 0.6131449684325461, 0.5580507098855338,
      0.613144968432479, 0.5580507098856315, 0.4991580622710732, 0.5580507098856855,
      0.6131449684324657, 0.5580507098857228, 0.613144968432394 };

    for (int i = 0; i < nPoints; i++)
    {
      points->SetPoint(i, pointsTps[3 * i], pointsTps[3 * i + 1], pointsTps[3 * i + 2]);
      rationalWeights->SetValue(i, rationalWeightsTps[i]);
    }

    vtkSmartPointer<vtkUnstructuredGrid> ugrid = vtkSmartPointer<vtkUnstructuredGrid>::New();
    ugrid->GetPointData()->SetRationalWeights(rationalWeights);
    ugrid->SetPoints(points);
    ugrid->InsertNextCell(VTK_BEZIER_HEXAHEDRON, nPoints, pointIds);

    // Here we write out the cube.
    vtkSmartPointer<vtkXMLUnstructuredGridWriter> writer =
      vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
    writer->SetInputData(ugrid);
    writer->SetFileName("VTK_BEZIER_HEXAHEDRON_triquartic_full_sphere.vtu");
    writer->SetDataModeToAscii();
    writer->Update();

    img_compare(ugrid, false);
    img_compare(ugrid, true);

    vtkCell* cell = ugrid->GetCell(0);
    int subId = 0;
    double pcoords[3]{ 0.23, 0, 0 };
    double x[3];
    double weights[VTK_MAXIMUM_NUMBER_OF_POINTS];
    cell->EvaluateLocation(subId, pcoords, x, weights);

    const double radius = 1;
    if (std::abs(sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]) - radius) > 1e-7)
    {
      vtkGenericWarningMacro(
        "VTK_BEZIER_HEXAHEDRON_triquartic_full_sphere is failing 1. Its radius is supposed to be "
        << radius << " and is instead " << sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2])
        << " diff is " << sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]) - radius);
      return EXIT_FAILURE;
    }

    pcoords[0] = 0;
    pcoords[1] = 0.23;
    cell->EvaluateLocation(subId, pcoords, x, weights);
    if (std::abs(sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]) - radius) > 1e-7)
    {
      vtkGenericWarningMacro(
        "VTK_BEZIER_HEXAHEDRON_triquartic_full_sphere is failing 2. Its radius is supposed to be "
        << radius << " and is instead " << sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2])
        << " diff is " << sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]) - radius);
      return EXIT_FAILURE;
    }

    pcoords[0] = 1.0;
    pcoords[1] = 0.5;
    cell->EvaluateLocation(subId, pcoords, x, weights);
    if (std::abs(sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]) - radius) > 1e-7)
    {
      vtkGenericWarningMacro(
        "VTK_BEZIER_HEXAHEDRON_triquartic_full_sphere is failing 3. Its radius is supposed to be "
        << radius << " and is instead " << sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2])
        << " diff is " << sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]) - radius);
      return EXIT_FAILURE;
    }
  }

  ///////////////////////////// TEST
  /// VTK_BEZIER_HEXAHEDRON_bilinearquadratic_quarteRingWithSquareSection
  /////////////////////////////////////////
  {
    const int nPoints = 12;
    vtkIdType pointIds[nPoints] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };

    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkDoubleArray> rationalWeights = vtkSmartPointer<vtkDoubleArray>::New();
    vtkSmartPointer<vtkDoubleArray> degrees = vtkSmartPointer<vtkDoubleArray>::New();
    const double sqrt2over2 = std::sqrt(2.) / 2.;
    rationalWeights->SetName("RationalWeights");
    rationalWeights->SetNumberOfComponents(1);
    rationalWeights->SetNumberOfTuples(nPoints);
    points->InsertNextPoint(0.0, 1.0, 0.0);
    rationalWeights->SetValue(0, 1.0);
    points->InsertNextPoint(1.0, 0.0, 0.0);
    rationalWeights->SetValue(1, 1.0);
    points->InsertNextPoint(2.0, 0.0, 0.0);
    rationalWeights->SetValue(2, 1.0);
    points->InsertNextPoint(0.0, 2.0, 0.0);
    rationalWeights->SetValue(3, 1.0);

    points->InsertNextPoint(0.0, 1.0, 1.0);
    rationalWeights->SetValue(4, 1.0);
    points->InsertNextPoint(1.0, 0.0, 1.0);
    rationalWeights->SetValue(5, 1.0);
    points->InsertNextPoint(2.0, 0.0, 1.0);
    rationalWeights->SetValue(6, 1.0);
    points->InsertNextPoint(0.0, 2.0, 1.0);
    rationalWeights->SetValue(7, 1.0);

    points->InsertNextPoint(1.0, 1.0, 0.0);
    rationalWeights->SetValue(8, sqrt2over2);
    points->InsertNextPoint(2.0, 2.0, 0.0);
    rationalWeights->SetValue(9, sqrt2over2);
    points->InsertNextPoint(1.0, 1.0, 1.0);
    rationalWeights->SetValue(10, sqrt2over2);
    points->InsertNextPoint(2.0, 2.0, 1.0);
    rationalWeights->SetValue(11, sqrt2over2);

    degrees->SetName("HigherOrderDegrees");
    degrees->SetNumberOfComponents(3);
    degrees->InsertNextTuple3(2, 1, 1);

    vtkSmartPointer<vtkUnstructuredGrid> ugrid = vtkSmartPointer<vtkUnstructuredGrid>::New();
    ugrid->SetPoints(points);
    ugrid->InsertNextCell(VTK_BEZIER_HEXAHEDRON, nPoints, pointIds);
    ugrid->GetPointData()->SetRationalWeights(rationalWeights);
    ugrid->GetCellData()->SetHigherOrderDegrees(degrees);

    // Here we write out the cube.
    vtkSmartPointer<vtkXMLUnstructuredGridWriter> writer =
      vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
    writer->SetInputData(ugrid);
    writer->SetFileName("VTK_BEZIER_HEXAHEDRON_bilinearquadratic_quarteRingWithSquareSection.vtu");
    writer->SetDataModeToAscii();
    writer->Update();
    img_compare(ugrid, false);
    // img_compare(ugrid, true);

    vtkCell* cell = ugrid->GetCell(0);
    int subId = 0;
    double pcoords[3]{ 0.23, 0, 0 };
    double x[3];
    double weights[VTK_MAXIMUM_NUMBER_OF_POINTS];
    cell->EvaluateLocation(subId, pcoords, x, weights);

    double radius = 1;
    if (std::abs(sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]) - radius) > 1e-10)
    {
      vtkGenericWarningMacro("VTK_BEZIER_HEXAHEDRON_bilinearquadratic_quarteRingWithSquareSection "
                             "is failing 1. Its radius is supposed to be "
        << radius << " and is instead " << sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2])
        << " diff is " << sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]) - radius);
      return EXIT_FAILURE;
    }

    pcoords[0] = 0.6546;
    pcoords[1] = 1;
    radius = 2;
    cell->EvaluateLocation(subId, pcoords, x, weights);
    if (std::abs(sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]) - radius) > 1e-10)
    {
      vtkGenericWarningMacro("VTK_BEZIER_HEXAHEDRON_bilinearquadratic_quarteRingWithSquareSection "
                             "is failing 2. Its radius is supposed to be "
        << radius << " and is instead " << sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2])
        << " diff is " << sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]) - radius);
      return EXIT_FAILURE;
    }
  }

  ///////////////////////////// TEST VTK_BEZIER_TETRA_quadratic
  /////////////////////////////////////////
  {
    vtkIdType pointIds[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };

    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkDoubleArray> rationalWeights = vtkSmartPointer<vtkDoubleArray>::New();
    const double sqrt2over2 = 1.0;
    // const double sqrt2over2 = std::sqrt( 2. ) / 2.;
    rationalWeights->SetName("RationalWeights");
    rationalWeights->SetNumberOfComponents(1);
    rationalWeights->SetNumberOfTuples(10);
    points->InsertNextPoint(0.0, 0.0, 0.0);
    rationalWeights->SetValue(0, 1.0);
    points->InsertNextPoint(1, 0.0, 0.0);
    rationalWeights->SetValue(1, 1.0);
    points->InsertNextPoint(0.5, 0.86602540378, 0.0);
    rationalWeights->SetValue(2, 1.0);
    points->InsertNextPoint(0.5, 0.86602540378 / 2, 0.86602540378);
    rationalWeights->SetValue(3, 1.0);
    points->InsertNextPoint(0.5, 0.0, 0.0);
    rationalWeights->SetValue(4, 1.0);
    points->InsertNextPoint(0.75, 0.86602540378 / 2, 0.0);
    rationalWeights->SetValue(5, 1.0 * sqrt2over2);
    points->InsertNextPoint(0.25, 0.86602540378 / 2, 0.0);
    rationalWeights->SetValue(6, 1.0);
    points->InsertNextPoint(0.25, 0.86602540378 / 4, 0.86602540378 / 2);
    rationalWeights->SetValue(7, 1.0);
    points->InsertNextPoint(0.75, 0.86602540378 / 4, 0.86602540378 / 2);
    rationalWeights->SetValue(8, 1.0);
    points->InsertNextPoint(0.5, 0.86602540378 * 3 / 4, 0.86602540378 / 2);
    rationalWeights->SetValue(9, 1.0);

    vtkSmartPointer<vtkUnstructuredGrid> ugrid = vtkSmartPointer<vtkUnstructuredGrid>::New();
    ugrid->SetPoints(points);
    ugrid->InsertNextCell(VTK_BEZIER_TETRAHEDRON, 10, pointIds);
    ugrid->GetPointData()->SetRationalWeights(rationalWeights);

    // Here we write out the cube.
    vtkSmartPointer<vtkXMLUnstructuredGridWriter> writer =
      vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
    writer->SetInputData(ugrid);
    writer->SetFileName("VTK_BEZIER_TETRA_quadratic.vtu");
    writer->SetDataModeToAscii();
    writer->Update();

    img_compare(ugrid, false);
    img_compare(ugrid, true);
  }

  ///////////////////////////// TEST VTK_BEZIER_TETRA_quartic_solidSphereOctant
  /////////////////////////////////////////
  {
    const int nPoints = 35;

    vtkIdType pointIds[nPoints];
    for (int i = 0; i < nPoints; i++)
      pointIds[i] = i;

    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkDoubleArray> rationalWeights = vtkSmartPointer<vtkDoubleArray>::New();
    rationalWeights->SetName("RationalWeights");
    rationalWeights->SetNumberOfComponents(1);
    rationalWeights->SetNumberOfTuples(nPoints);

    points->SetNumberOfPoints(nPoints);

    const double c1 = (sqrt(3) - 1.) / sqrt(3);
    const double c2 = (sqrt(3) + 1.) / (2. * sqrt(3));
    const double c3 = 1. - (5. - sqrt(2)) * (7. - sqrt(3)) / 46.;

    const double c1o3 = 0.25;
    const double c2o3 = 0.5;

    const double c1o4 = 1. / 4.;
    const double c2o4 = 2. / 4.;
    const double c3o4 = 3. / 4.;

    points->SetPoint(0, 1, 0, 0);
    points->SetPoint(1, 0, 1, 0);
    points->SetPoint(2, 0, 0, 1);
    points->SetPoint(3, 0, 0, 0);
    points->SetPoint(4, 1, c1, 0);
    points->SetPoint(5, c2, c2, 0);
    points->SetPoint(6, c1, 1, 0);
    points->SetPoint(7, 0, 1, c1);
    points->SetPoint(8, 0, c2, c2);
    points->SetPoint(9, 0, c1, 1);
    points->SetPoint(10, c1, 0, 1);
    points->SetPoint(11, c2, 0, c2);
    points->SetPoint(12, 1, 0, c1);
    points->SetPoint(13, c3o4, 0, 0);
    points->SetPoint(14, c2o4, 0, 0);
    points->SetPoint(15, c1o4, 0, 0);
    points->SetPoint(16, 0, c3o4, 0);
    points->SetPoint(17, 0, c2o4, 0);
    points->SetPoint(18, 0, c1o4, 0);
    points->SetPoint(19, 0, 0, c3o4);
    points->SetPoint(20, 0, 0, c2o4);
    points->SetPoint(21, 0, 0, c1o4);

    points->SetPoint(22, c2o3, c1o3, 0);
    points->SetPoint(23, c1o3, c2o3, 0);
    points->SetPoint(24, c1o3, c1o3, 0);

    points->SetPoint(25, 0, c1o3, c2o3);
    points->SetPoint(26, 0, c1o3, c1o3);
    points->SetPoint(27, 0, c2o3, c1o3);

    points->SetPoint(28, c2o3, 0, c1o3);
    points->SetPoint(29, c1o3, 0, c1o3);
    points->SetPoint(30, c1o3, 0, c2o3);

    points->SetPoint(31, 1, c3, c3);
    points->SetPoint(32, c3, c3, 1);
    points->SetPoint(33, c3, 1, c3);

    points->SetPoint(34, 0.3, 0.3, 0.3);

    const double w1 = 4 * sqrt(3) * (sqrt(3) - 1.);
    const double w2 = 3 * sqrt(2) / w1;
    const double w3 = sqrt(2. / 3.) * (3. + 2. * sqrt(2) - sqrt(3)) / w1;

    double rationalWeightsTps2[nPoints] = { 1, 1, 1, 1, w2, 4. / w1, w2, w2, 4. / w1, w2, w2,
      4. / w1, w2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, w3, w3, w3, 1 };
    for (int i = 0; i < nPoints; i++)
    {
      rationalWeights->SetValue(i, rationalWeightsTps2[i]);
    }

    vtkSmartPointer<vtkUnstructuredGrid> ugrid = vtkSmartPointer<vtkUnstructuredGrid>::New();
    ugrid->SetPoints(points);
    ugrid->InsertNextCell(VTK_BEZIER_TETRAHEDRON, nPoints, pointIds);
    ugrid->GetPointData()->SetRationalWeights(rationalWeights);

    vtkCell* cell = ugrid->GetCell(0);
    int subId = 0;
    double pcoords[3]{ 0., 0., 0. };
    double x[3];
    double weights[VTK_MAXIMUM_NUMBER_OF_POINTS];

    pcoords[0] = 0.6;
    pcoords[1] = 0;
    pcoords[2] = 0.4;
    cell->EvaluateLocation(subId, pcoords, x, weights);
    if ((std::abs(x[0] - 0.) > 1e-7) || (std::abs(x[1] - 0.6) > 1e-7) ||
      (std::abs(x[2] - 0.) > 1e-7))
    {
      vtkGenericWarningMacro(
        "VTK_BEZIER_TETRA_quartic_solidSphereOctant is failing, the points are "
        << x[0] << " " << x[1] << " " << x[2]);
    }

    pcoords[0] = 0;
    pcoords[1] = 0.6;
    pcoords[2] = 0.4;
    cell->EvaluateLocation(subId, pcoords, x, weights);
    if ((std::abs(x[0] - 0.) > 1e-7) || (std::abs(x[1] - 0.) > 1e-7) ||
      (std::abs(x[2] - 0.6) > 1e-7))
    {
      vtkGenericWarningMacro(
        "VTK_BEZIER_TETRA_quartic_solidSphereOctant is failing, the points are "
        << x[0] << " " << x[1] << " " << x[2]);
      return EXIT_FAILURE;
    }

    pcoords[0] = 0.23654;
    pcoords[1] = 0.1845;
    pcoords[2] = 0.;
    cell->EvaluateLocation(subId, pcoords, x, weights);
    double radius = 1;
    if (std::abs(sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]) - radius) > 1e-8)
    {
      vtkGenericWarningMacro("VTK_BEZIER_TETRA_quartic_solidSphereOctant is failing 1. Its "
                             "radius is supposed to be "
        << radius << " and is instead " << sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2])
        << " diff is " << sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]) - radius);
      return EXIT_FAILURE;
    }

    // Here we write out the cube.
    vtkSmartPointer<vtkXMLUnstructuredGridWriter> writer =
      vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
    writer->SetInputData(ugrid);
    writer->SetFileName("VTK_BEZIER_TETRA_quartic_solidSphereOctant.vtu");
    writer->SetDataModeToAscii();
    writer->Update();

    img_compare(ugrid, false);
    // img_compare(ugrid, true);
  }

  ///////////////////////////// TEST VTK_BEZIER_WEDGE_biquadratic_quarterCylinder
  /////////////////////////////////////////
  {
    vtkIdType pointIds[18] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17 };

    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkDoubleArray> rationalWeights = vtkSmartPointer<vtkDoubleArray>::New();
    const double sqrt2over2 = std::sqrt(2.) / 2.;
    rationalWeights->SetName("RationalWeights");
    rationalWeights->SetNumberOfComponents(1);
    rationalWeights->SetNumberOfTuples(18);
    points->InsertNextPoint(0.0, 0.0, 0.0);
    rationalWeights->SetValue(0, 1.0);
    points->InsertNextPoint(1.0, 0.0, 0.0);
    rationalWeights->SetValue(1, 1.0);
    points->InsertNextPoint(0.0, 1.0, 0.0);
    rationalWeights->SetValue(2, 1.0);

    points->InsertNextPoint(0.0, 0.0, 3.0);
    rationalWeights->SetValue(3, 1.0);
    points->InsertNextPoint(1.0, 0.0, 3.0);
    rationalWeights->SetValue(4, 1.0);
    points->InsertNextPoint(0.0, 1.0, 3.0);
    rationalWeights->SetValue(5, 1.0);

    points->InsertNextPoint(0.5, 0.0, 0.0);
    rationalWeights->SetValue(6, 1.0);
    points->InsertNextPoint(1.0 * sqrt2over2, 1.0 * sqrt2over2, 0.0 * sqrt2over2);
    rationalWeights->SetValue(7, 1.0 * sqrt2over2);
    points->InsertNextPoint(0.0, 0.5, 0.0);
    rationalWeights->SetValue(8, 1.0);

    points->InsertNextPoint(0.5, 0.0, 3.0);
    rationalWeights->SetValue(9, 1.0);
    points->InsertNextPoint(1.0 * sqrt2over2, 1.0 * sqrt2over2, 3.0 * sqrt2over2);
    rationalWeights->SetValue(10, 1.0 * sqrt2over2);
    points->InsertNextPoint(0.0, 0.5, 3.0);
    rationalWeights->SetValue(11, 1.0);

    points->InsertNextPoint(0.0, 0.0, 1.5);
    rationalWeights->SetValue(12, 1.0);
    points->InsertNextPoint(1, 0.0, 1.5);
    rationalWeights->SetValue(13, 1.0);
    points->InsertNextPoint(0.0, 1.0, 1.5);
    rationalWeights->SetValue(14, 1.0);

    points->InsertNextPoint(0.5, 0.0, 1.5);
    rationalWeights->SetValue(15, 1.0);
    points->InsertNextPoint(1.0 * sqrt2over2, 1.0 * sqrt2over2, 1.5 * sqrt2over2);
    rationalWeights->SetValue(16, 1.0 * sqrt2over2);
    points->InsertNextPoint(0.0, 0.5, 1.5);
    rationalWeights->SetValue(17, 1.0);

    vtkSmartPointer<vtkUnstructuredGrid> ugrid = vtkSmartPointer<vtkUnstructuredGrid>::New();
    ugrid->SetPoints(points);
    ugrid->InsertNextCell(VTK_BEZIER_WEDGE, 18, pointIds);
    ugrid->GetPointData()->SetRationalWeights(rationalWeights);

    // Here we write out the cube.
    vtkSmartPointer<vtkXMLUnstructuredGridWriter> writer =
      vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
    writer->SetInputData(ugrid);
    writer->SetFileName("VTK_BEZIER_WEDGE_biquadratic_quarterCylinder.vtu");
    writer->SetDataModeToAscii();
    writer->Update();

    img_compare(ugrid, false);
    img_compare(ugrid, true);
  }

  ///////////////////////////// TEST VTK_BEZIER_WEDGE_quarticLinear_thickSphereOctant
  /////////////////////////////////////////
  {
    const int nPoints = 30;

    vtkIdType pointIds[nPoints];
    for (int i = 0; i < nPoints; i++)
      pointIds[i] = i;

    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkDoubleArray> rationalWeights = vtkSmartPointer<vtkDoubleArray>::New();
    vtkSmartPointer<vtkDoubleArray> degrees = vtkSmartPointer<vtkDoubleArray>::New();
    rationalWeights->SetName("RationalWeights");
    rationalWeights->SetNumberOfComponents(1);
    rationalWeights->SetNumberOfTuples(nPoints);
    points->SetNumberOfPoints(nPoints);

    // The points and weights can be found in this paper:
    // Farin, Piper and Worsey, 1987, Computer Aided Geometric Design
    // The octant of a sphere as a non-degenerate triangular Bézier patch

    const double c1 = (sqrt(3) - 1.) / sqrt(3);
    const double c2 = (sqrt(3) + 1.) / (2. * sqrt(3));
    const double c3 = 1. - (5. - sqrt(2)) * (7. - sqrt(3)) / 46.;
    const double c1_2 = c1 / 2.;
    const double c2_2 = c2 / 2.;
    const double c3_2 = c3 / 2.;

    points->SetPoint(0, 1, 0, 0);
    points->SetPoint(1, 0, 1, 0);
    points->SetPoint(2, 0, 0, 1);
    points->SetPoint(3, 0.5, 0, 0);
    points->SetPoint(4, 0, 0.5, 0);
    points->SetPoint(5, 0, 0, 0.5);
    points->SetPoint(6, 1, c1, 0);
    points->SetPoint(7, c2, c2, 0);
    points->SetPoint(8, c1, 1, 0);
    points->SetPoint(9, 0, 1, c1);
    points->SetPoint(10, 0, c2, c2);
    points->SetPoint(11, 0, c1, 1);
    points->SetPoint(12, c1, 0, 1);
    points->SetPoint(13, c2, 0, c2);
    points->SetPoint(14, 1, 0, c1);
    points->SetPoint(15, 0.5, c1_2, 0);
    points->SetPoint(16, c2_2, c2_2, 0);
    points->SetPoint(17, c1_2, 0.5, 0);
    points->SetPoint(18, 0, 0.5, c1_2);
    points->SetPoint(19, 0, c2_2, c2_2);
    points->SetPoint(20, 0, c1_2, 0.5);
    points->SetPoint(21, c1_2, 0, 0.5);
    points->SetPoint(22, c2_2, 0, c2_2);
    points->SetPoint(23, 0.5, 0, c1_2);
    points->SetPoint(24, 1, c3, c3);
    points->SetPoint(25, c3, 1, c3);
    points->SetPoint(26, c3, c3, 1);
    points->SetPoint(27, 0.5, c3_2, c3_2);
    points->SetPoint(28, c3_2, 0.5, c3_2);
    points->SetPoint(29, c3_2, c3_2, 0.5);

    const double w1 = 4 * sqrt(3) * (sqrt(3) - 1.);
    const double w2 = 3 * sqrt(2);
    const double w3 = sqrt(2. / 3.) * (3. + 2. * sqrt(2) - sqrt(3));

    double rationalWeightsTps2[nPoints] = { w1, w1, w1, w1, w1, w1, w2, 4., w2, w2, 4., w2, w2, 4.,
      w2, w2, 4., w2, w2, 4., w2, w2, 4., w2, w3, w3, w3, w3, w3, w3 };
    for (int i = 0; i < nPoints; i++)
    {
      rationalWeights->SetValue(i, rationalWeightsTps2[i]);
    }

    degrees->SetName("HigherOrderDegrees");
    degrees->SetNumberOfComponents(3);
    degrees->InsertNextTuple3(4, 4, 1);

    vtkSmartPointer<vtkUnstructuredGrid> ugrid = vtkSmartPointer<vtkUnstructuredGrid>::New();
    ugrid->SetPoints(points);
    ugrid->InsertNextCell(VTK_BEZIER_WEDGE, nPoints, pointIds);
    ugrid->GetPointData()->SetRationalWeights(rationalWeights);
    ugrid->GetCellData()->SetHigherOrderDegrees(degrees);

    // Here we write out the cube.
    vtkSmartPointer<vtkXMLUnstructuredGridWriter> writer =
      vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
    writer->SetInputData(ugrid);
    writer->SetFileName("VTK_BEZIER_WEDGE_quarticLinear_thickSphereOctant.vtu");
    writer->SetDataModeToAscii();
    writer->Update();

    img_compare(ugrid, false);
    // img_compare(ugrid, true);

    vtkCell* cell = ugrid->GetCell(0);
    int subId = 0;
    double pcoords[3]{ 0.23, 0.654, 0 };
    double x[3];
    double weights[VTK_MAXIMUM_NUMBER_OF_POINTS];
    cell->EvaluateLocation(subId, pcoords, x, weights);

    double radius = 1;
    if (std::abs(sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]) - radius) > 1e-8)
    {
      vtkGenericWarningMacro("VTK_BEZIER_WEDGE_quarticLinear_thickSphereOctant is failing 1. Its "
                             "radius is supposed to be "
        << radius << " and is instead " << sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2])
        << " diff is " << sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]) - radius);
      return EXIT_FAILURE;
    }

    pcoords[0] = 0.2446;
    pcoords[1] = 0.687496;
    pcoords[2] = 1;
    radius = 0.5;
    cell->EvaluateLocation(subId, pcoords, x, weights);
    if (std::abs(sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]) - radius) > 1e-8)
    {
      vtkGenericWarningMacro("VTK_BEZIER_WEDGE_quarticLinear_thickSphereOctant is failing 2. Its "
                             "radius is supposed to be "
        << radius << " and is instead " << sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2])
        << " diff is " << sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]) - radius);
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
