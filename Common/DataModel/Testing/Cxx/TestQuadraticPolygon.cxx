/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestQuadraticPolygon.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkPolygon.h"
#include "vtkQuadraticPolygon.h"

class vtkQuadraticPolygonTest : public vtkQuadraticPolygon
{
public:
  static vtkQuadraticPolygonTest *New();
  vtkTypeMacro(vtkQuadraticPolygonTest, vtkQuadraticPolygon);

  bool IsClose(double d1, double d2);
  bool IsClose(double *point1, double *point2);

  void InitializeCircle();
  void InitializeSquare();
  void InitializeSquareWithQuadraticEdge();

  int TestGetSet();
  int TestGetPermutations();
  int TestInitializePolygon();
  int TestIntersectWithLine();
  int TestInterpolateFunctions();
  int TestInterpolateFunctionsUsingMVC();

  int TestAll();

protected:
  vtkQuadraticPolygonTest() : Tolerance(0.000001) {}
private:
  double Tolerance;
};

vtkStandardNewMacro(vtkQuadraticPolygonTest);

int TestQuadraticPolygon(int, char *[])
{
  vtkNew<vtkQuadraticPolygonTest> test;
  int result = test->TestAll();
  cout << ((result == EXIT_SUCCESS) ? "SUCCESS" : "FAILURE") << endl;
  return result;
}

int vtkQuadraticPolygonTest::TestAll()
{
  int result = EXIT_SUCCESS;

  this->InitializeSquareWithQuadraticEdge();
  result |= this->TestGetSet();
  result |= this->TestGetPermutations();
  result |= this->TestInitializePolygon();
  result |= this->TestIntersectWithLine();
  this->InitializeSquare();
  result |= this->TestInterpolateFunctions();
  result |= this->TestInterpolateFunctionsUsingMVC();

  return result;
}

bool vtkQuadraticPolygonTest::IsClose(double v1, double v2)
{
  return (v1 < v2 ? (v2-v1 < this->Tolerance) : (v1-v2 < this->Tolerance));
}

bool vtkQuadraticPolygonTest::IsClose(double *point1, double *point2)
{
  return (vtkMath::Distance2BetweenPoints(point1, point2) <
    this->Tolerance * this->Tolerance);
}

void vtkQuadraticPolygonTest::InitializeSquare()
{
  this->GetPointIds()->SetNumberOfIds(8);
  this->GetPointIds()->SetId(0,0);
  this->GetPointIds()->SetId(1,1);
  this->GetPointIds()->SetId(2,2);
  this->GetPointIds()->SetId(3,3);
  this->GetPointIds()->SetId(4,4);
  this->GetPointIds()->SetId(5,5);
  this->GetPointIds()->SetId(6,6);
  this->GetPointIds()->SetId(7,7);

  this->GetPoints()->SetNumberOfPoints(8);
  this->GetPoints()->SetPoint(0, 0.0, 0.0, 0.0);
  this->GetPoints()->SetPoint(1, 2.0, 0.0, 0.0);
  this->GetPoints()->SetPoint(2, 2.0, 2.0, 0.0);
  this->GetPoints()->SetPoint(3, 0.0, 2.0, 0.0);
  this->GetPoints()->SetPoint(4, 1.0, 0.0, 0.0);
  this->GetPoints()->SetPoint(5, 2.0, 1.0, 0.0);
  this->GetPoints()->SetPoint(6, 1.0, 2.0, 0.0);
  this->GetPoints()->SetPoint(7, 0.0, 1.0, 0.0);
}

void vtkQuadraticPolygonTest::InitializeSquareWithQuadraticEdge()
{
  this->InitializeSquare();
  this->GetPoints()->SetPoint(5, 3.0, 1.0, 0.0);
}

int vtkQuadraticPolygonTest::TestGetSet()
{
  if (this->GetCellType() != VTK_QUADRATIC_POLYGON)
  {
    cerr << "ERROR:  quadratic polygon type is " << this->GetCellType()
         << ", should be " << VTK_QUADRATIC_POLYGON << endl;
    return EXIT_FAILURE;
  }

  if (this->GetCellDimension() != 2)
  {
    cerr << "ERROR:  quadratic polygon dim is "
         << this->GetCellDimension()
         << ", should be 2" << endl;
    return EXIT_FAILURE;
  }

  if (this->GetNumberOfEdges() != 4)
  {
    cerr << "ERROR:  quadratic polygon edges number is "
         << this->GetNumberOfEdges()
         << ", should be 4" << endl;
    return EXIT_FAILURE;
  }

  if (this->GetNumberOfFaces() != 0)
  {
    cerr << "ERROR:  quadratic polygon faces number is "
         << this->GetNumberOfFaces()
         << ", should be 0" << endl;
    return EXIT_FAILURE;
  }

  if (this->GetFace(0) != 0)
  {
    cerr << "ERROR:  quadratic polygon face is " << this->GetFace(0)
         << ", should be 0" << endl;
    return EXIT_FAILURE;
  }

  if (this->GetEdge(0)->PointIds->GetId(0) != 0)
  {
    cerr << "ERROR:  quadratic polygon edge[0] point[0] id is "
         << this->GetEdge(0)->PointIds->GetId(0)
         << ", should be 0" << endl;
    return EXIT_FAILURE;
  }
  if (this->GetEdge(0)->PointIds->GetId(1) != 1)
  {
    cerr << "ERROR:  quadratic polygon edge[0] point[1] id is "
         << this->GetEdge(0)->PointIds->GetId(1)
         << ", should be 1" << endl;
    return EXIT_FAILURE;
  }
  if (this->GetEdge(0)->PointIds->GetId(2) != 4)
  {
    cerr << "ERROR:  quadratic polygon edge[0] point[2] id is "
         << this->GetEdge(0)->PointIds->GetId(2)
         << ", should be 4" << endl;
    return EXIT_FAILURE;
  }

  if (this->IsPrimaryCell() != 0)
  {
    cerr << "ERROR:  quadratic polygon primary boolean is "
         << this->IsPrimaryCell()
         << ", should be 0" << endl;
    return EXIT_FAILURE;
  }

  if (!this->GetUseMVCInterpolation())
  {
    cerr << "ERROR:  quadratic polygon MVC boolean is "
         << this->GetUseMVCInterpolation()
         << ", should be 1" << endl;
    return EXIT_FAILURE;
  }

  this->SetUseMVCInterpolation(false);
  if (this->GetUseMVCInterpolation())
  {
    cerr << "ERROR:  quadratic polygon MVC boolean is "
         << this->GetUseMVCInterpolation()
         << ", should be 0" << endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

int vtkQuadraticPolygonTest::TestGetPermutations()
{
  // reference permutation
  vtkIdType temp[] = { 0, 2, 4, 6, 1, 3, 5, 7 };
  vtkNew<vtkIdList> permutationToPolygonRef;
  permutationToPolygonRef->SetNumberOfIds(8);
  for (vtkIdType i = 0; i < 8; i++)
  {
    permutationToPolygonRef->SetId(i, temp[i]);
  }

  // computed permutation
  vtkNew<vtkIdList> permutationToPolygon;
  vtkQuadraticPolygon::GetPermutationToPolygon(8, permutationToPolygon.GetPointer());

  // reference permutation
  vtkIdType temp2[] = { 0, 4, 1, 5, 2, 6, 3, 7 };
  vtkNew<vtkIdList> permutationFromPolygonRef;
  permutationFromPolygonRef->SetNumberOfIds(8);
  for (vtkIdType i = 0; i < 8; i++)
  {
    permutationFromPolygonRef->SetId(i, temp2[i]);
  }

  // computed permutation
  vtkNew<vtkIdList> permutationFromPolygon;
  vtkQuadraticPolygon::GetPermutationFromPolygon(8,
    permutationFromPolygon.GetPointer());

  for (vtkIdType i = 0; i < 8; i++)
  {
    if (permutationToPolygonRef->GetId(i) != permutationToPolygon->GetId(i))
    {
      cerr << "ERROR:  permutation to polygon is wrong" << endl;
      return EXIT_FAILURE;
    }
    if (permutationFromPolygonRef->GetId(i) != permutationFromPolygon->GetId(i))
    {
      cerr << "ERROR:  permutation from polygon is wrong" << endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}

int vtkQuadraticPolygonTest::TestInitializePolygon()
{
  // reference permutation
  vtkIdType temp[] = { 0, 2, 4, 6, 1, 3, 5, 7 };
  vtkNew<vtkIdList> permutationToPolygonRef;
  permutationToPolygonRef->SetNumberOfIds(8);
  for (vtkIdType i = 0; i < 8; i++)
  {
    permutationToPolygonRef->SetId(i, temp[i]);
  }

  this->InitializePolygon();
  vtkPolygon *polygon = this->Polygon;

  for (vtkIdType i = 0; i < 8; i++)
  {
    if (this->GetPointIds()->GetId(i) !=
         polygon->GetPointIds()->GetId(permutationToPolygonRef->GetId(i)))
    {
      cerr << "ERROR:  quadratic polygon point id at index " << i
           << " is " << this->GetPointIds()->GetId(i)
           << ", should be "
           << polygon->GetPointIds()->GetId(permutationToPolygonRef->GetId(i))
           << endl;
      return EXIT_FAILURE;
    }
    for (int j = 0; j < 3; j++)
    {
      if (!this->IsClose( this->GetPoints()->GetPoint(i)[j] ,
                           polygon->GetPoints()->GetPoint(permutationToPolygonRef->GetId(i))[j]
                        ))
      {
        cerr << "ERROR:  quadratic polygon point at index " << i
             << " (coord " << j << ") is " << this->GetPoints()->GetPoint(i)[j]
             << ", should be "
             << polygon->GetPoints()->GetPoint(permutationToPolygonRef->GetId(i))[j]
             << endl;
        return EXIT_FAILURE;
      }
    }
  }

  return EXIT_SUCCESS;
}

int vtkQuadraticPolygonTest::TestIntersectWithLine()
{
  double t, x[3], pcoords[3];
  int subId;

  double p1[3] = { 2.5, 1.0, -1.0 };
  double p2[3] = { 2.5, 1.0, 1.0 };
  int intersect = this->IntersectWithLine(p1, p2, 0.0, t, x, pcoords, subId);

  if (x[0] != 2.5 || x[1] != 1.0 || x[2] != 0.0)
  {
    cerr << "ERROR:  vtkQuadraticPolygon::IntersectWithLine returns point ("
         << x[0] << "," << x[1] << "," << x[2] << ")"
         << ", should return point (2.5,1.0,0.0)" << endl;
    return EXIT_FAILURE;
  }

  if (!intersect)
  {
    cerr << "ERROR:  vtkQuadraticPolygon::IntersectWithLine returns " << intersect
         << ", should return 1" << endl;
    return EXIT_FAILURE;
  }

  p1[0] = 3.5;
  p2[0] = 3.5;
  intersect = this->IntersectWithLine(p1, p2, 0.0, t, x, pcoords, subId);

  if (intersect)
  {
    cerr << "ERROR:  vtkQuadraticPolygon::IntersectWithLine returns " << intersect
         << ", should return 0" << endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

int vtkQuadraticPolygonTest::TestInterpolateFunctions()
{
  int nbPoints = this->GetNumberOfPoints();

  double x[3] = { 1.0, 1.0, 0.0 };
  double *weights = new double[nbPoints];
  double w1 = 1. / 12.;
  double w2 = 1. / 6.;
  this->SetUseMVCInterpolation(false);
  this->InterpolateFunctions(x, weights);

  int i;
  for (i = 0; i < nbPoints/2; i++)
  {
    if (!this->IsClose(weights[i],w1))
    {
      cerr << "ERROR:  quadratic polygon weights is " << weights[i]
           << ", should be " << w1 << endl;
      delete [] weights;
      return EXIT_FAILURE;
    }
  }
  for ( ; i < nbPoints; i++)
  {
    if (!this->IsClose(weights[i],w2))
    {
      cerr << "ERROR:  quadratic polygon weights is " << weights[i]
           << ", should be " << w2 << endl;
      delete [] weights;
      return EXIT_FAILURE;
    }
  }

  delete [] weights;
  return EXIT_SUCCESS;
}

int vtkQuadraticPolygonTest::TestInterpolateFunctionsUsingMVC()
{
  int nbPoints = this->GetNumberOfPoints();

  double x[3] = { 1.0, 1.0, 0.0 };
  double *weights = new double[nbPoints];
  double w1 = (sqrt(2.) - 1.) / 4.;
  double w2 = (sqrt(2.) - 1.) / (2. * sqrt(2.));
  this->SetUseMVCInterpolation(true);
  this->InterpolateFunctions(x, weights);

  int i;
  for (i = 0; i < nbPoints/2; i++)
  {
    if (!this->IsClose(weights[i],w1))
    {
      cerr << "ERROR:  quadratic polygon weights is " << weights[i]
           << ", should be " << w1 << endl;
      delete [] weights;
      return EXIT_FAILURE;
    }
  }
  for ( ; i < nbPoints; i++)
  {
    if (!this->IsClose(weights[i],w2))
    {
      cerr << "ERROR:  quadratic polygon weights is " << weights[i]
           << ", should be " << w2 << endl;
      delete [] weights;
      return EXIT_FAILURE;
    }
  }

  delete [] weights;
  return EXIT_SUCCESS;
}
