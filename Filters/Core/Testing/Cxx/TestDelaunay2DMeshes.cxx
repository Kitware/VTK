/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDelaunay2DMeshes.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Test meshes obtained with vtkDelaunay2D.

#include "vtkCellArray.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataReader.h"
#include "vtkPolyDataWriter.h"
#include "vtkDelaunay2D.h"
#include "vtkCellArray.h"
#include "vtkTransform.h"
#include "vtkTriangle.h"
#include "vtkXMLPolyDataReader.h"
#include "vtkTestUtilities.h"
#include "vtkSmartPointer.h"

#define VTK_FAILURE 1

bool CompareMeshes(vtkPolyData* p1, vtkPolyData* p2)
{
  vtkIdType nbPoints1 = p1->GetNumberOfPoints();
  vtkIdType nbPoints2 = p2->GetNumberOfPoints();
  vtkIdType nbCells1 = p1->GetNumberOfCells();
  vtkIdType nbCells2 = p2->GetNumberOfCells();
  if (nbPoints1 != nbPoints2 || nbCells1 != nbCells2)
  {
    return false;
  }

  vtkCellArray* polys1 = p1->GetPolys();
  vtkCellArray* polys2 = p2->GetPolys();
  polys1->InitTraversal();
  polys2->InitTraversal();
  vtkIdType npts1, npts2, *pts1, *pts2;
  while (polys1->GetNextCell(npts1, pts1) && polys2->GetNextCell(npts2, pts2))
  {
    if (npts1 != npts2)
    {
      return false;
    }
    for (vtkIdType i = 0; i < npts1; i++)
    {
      if (pts1[i] != pts2[i])
      {
        return false;
      }
    }
  }

  return true;
}

void DumpMesh(vtkPolyData* mesh)
{
  vtkNew<vtkPolyDataWriter> writer;
  writer->SetInputData(mesh);
  writer->WriteToOutputStringOn();
  writer->Write();
  std::cerr << writer->GetOutputString() << std::endl;
}

bool TriangulationTest(const std::string& filePath)
{
  vtkNew<vtkPolyDataReader> inputReader;
  inputReader->SetFileName((filePath + "-Input.vtk").c_str());
  inputReader->Update();

  vtkNew<vtkDelaunay2D> delaunay2D;
  delaunay2D->SetInputConnection(inputReader->GetOutputPort());
  delaunay2D->SetSourceConnection(inputReader->GetOutputPort());
  delaunay2D->Update();

  vtkPolyData* obtainedMesh = delaunay2D->GetOutput();

  vtkNew<vtkPolyDataReader> outputReader;
  outputReader->SetFileName((filePath + "-Output.vtk").c_str());
  outputReader->Update();

  vtkPolyData* validMesh = outputReader->GetOutput();

  if (!CompareMeshes(validMesh, obtainedMesh))
  {
    std::cerr << "Obtained mesh is different from expected! "
      "Its VTK file follows:" << std::endl;
    DumpMesh(obtainedMesh);
    return false;
  }

  return true;
}

void GetTransform(vtkTransform *transform, vtkPoints *points)
{
  double zaxis[3] = { 0., 0., 1. };
  double pt0[3], pt1[3], pt2[3], normal[3];
  points->GetPoint(0, pt0);
  points->GetPoint(1, pt1);
  points->GetPoint(2, pt2);
  vtkTriangle::ComputeNormal(pt0, pt1, pt2, normal);

  double rotationAxis[3], center[3], rotationAngle;
  double dotZAxis = vtkMath::Dot(normal, zaxis);
  if (fabs(1.0 - dotZAxis) < 1e-6)
  {
    // Aligned with z-axis
    rotationAxis[0] = 1.0;
    rotationAxis[1] = 0.0;
    rotationAxis[2] = 0.0;
    rotationAngle   = 0.0;
  }
  else if (fabs(1.0 + dotZAxis) < 1e-6)
  {
    // Co-linear with z-axis, but reversed sense.
    // Aligned with z-axis
    rotationAxis[0] = 1.0;
    rotationAxis[1] = 0.0;
    rotationAxis[2] = 0.0;
    rotationAngle = 180.0;
  }
  else
  {
    // The general case
    vtkMath::Cross(normal, zaxis, rotationAxis);
    vtkMath::Normalize(rotationAxis);
    rotationAngle =
      vtkMath::DegreesFromRadians(acos(vtkMath::Dot(zaxis, normal)));
  }

  transform->PreMultiply();
  transform->Identity();
  transform->RotateWXYZ(rotationAngle,
    rotationAxis[0],
    rotationAxis[1],
    rotationAxis[2]);

  vtkTriangle::TriangleCenter(pt0, pt1, pt2, center);
  transform->Translate(-center[0], -center[1], -center[2]);
}

bool TessellationTestWithTransform(const std::string& dataPath)
{
  std::string transformFilePath = dataPath + "-Transform.vtp";
  std::string boundaryFilePath = dataPath + "-Input.vtp";

  vtkNew<vtkXMLPolyDataReader> reader;
  reader->SetFileName(transformFilePath.c_str());
  reader->Update();

  vtkNew<vtkTransform> transform;
  vtkPoints* points = reader->GetOutput()->GetPoints();
  GetTransform(transform.Get(), points);

  reader->SetFileName(boundaryFilePath.c_str());
  reader->Update();
  vtkPolyData* boundaryPoly = reader->GetOutput();

  vtkNew<vtkDelaunay2D> del2D;
  del2D->SetInputData(boundaryPoly);
  del2D->SetSourceData(boundaryPoly);
  del2D->SetTolerance(0.0);
  del2D->SetAlpha(0.0);
  del2D->SetOffset(0);
  del2D->SetProjectionPlaneMode(VTK_SET_TRANSFORM_PLANE);
  del2D->SetTransform(transform.Get());
  del2D->BoundingTriangulationOff();
  del2D->Update();

  vtkPolyData* outPoly = del2D->GetOutput();

  if (outPoly->GetNumberOfCells() != boundaryPoly->GetNumberOfPoints() - 2)
  {
    std::cerr << "Bad triangulation for " << dataPath << "!" << std::endl;
    std::cerr << "Output has " << outPoly->GetNumberOfCells() << " cells instead of "
      << boundaryPoly->GetNumberOfPoints() - 2 << std::endl;
    return false;
  }
  return true;
}

int TestDelaunay2DMeshes(int argc, char* argv[])
{

  char* data_dir = vtkTestUtilities::GetDataRoot(argc, argv);
  if (!data_dir)
  {
    cerr << "Could not determine data directory." << endl;
    return VTK_FAILURE;
  }

  std::string dataPath =
    std::string(data_dir) + "/Data/Delaunay/";
  delete[] data_dir;

  bool result = true;
  result &= TriangulationTest(dataPath + "DomainWithHole");
  result &= TessellationTestWithTransform(dataPath + "Test1");
  result &= TessellationTestWithTransform(dataPath + "Test2");
  result &= TessellationTestWithTransform(dataPath + "Test3");
  result &= TessellationTestWithTransform(dataPath + "Test4");
  result &= TessellationTestWithTransform(dataPath + "Test5");

  return result ? EXIT_SUCCESS : EXIT_FAILURE;
}
