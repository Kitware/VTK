/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOBJPolyDataWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkActor.h>
#include <vtkJPEGReader.h>
#include <vtkMath.h>
#include <vtkNew.h>
#include <vtkOBJReader.h>
#include <vtkOBJWriter.h>
#include <vtkPNGReader.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkTestUtilities.h>
#include <vtkTexture.h>
#include <vtkTexturedSphereSource.h>

#include <vtkNumberToString.h>

#include <string>

int TestOBJPolyDataWriter(int argc, char* argv[])
{
  vtkNew<vtkTexturedSphereSource> sphereSource;
  sphereSource->SetThetaResolution(16);
  sphereSource->SetPhiResolution(16);

  vtkNew<vtkJPEGReader> textReader;
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/NE2_ps_bath_small.jpg");
  textReader->SetFileName(fname);
  delete[] fname;

  char* tname =
    vtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary");
  std::string tmpDir(tname);
  delete[] tname;
  std::string filename = tmpDir + "/TestOBJPolyDataWriter_write.obj";

  vtkNew<vtkOBJWriter> writer;
  writer->SetFileName(filename.c_str());
  writer->SetInputConnection(0, sphereSource->GetOutputPort());
  writer->SetInputConnection(1, textReader->GetOutputPort());
  writer->Write();

  vtkPolyData* polyInput = sphereSource->GetOutput();

  // read this file and compare with input
  vtkNew<vtkOBJReader> reader;
  reader->SetFileName(filename.c_str());
  reader->Update();
  vtkPolyData* polyOutput = reader->GetOutput();

  if (polyInput->GetNumberOfPoints() != polyOutput->GetNumberOfPoints())
  {
    cerr << "PolyData do not have the same number of points.\n";
    return EXIT_FAILURE;
  }

  vtkDataArray* positionsInput = polyInput->GetPoints()->GetData();
  vtkDataArray* positionsOutput = polyOutput->GetPoints()->GetData();
  vtkDataArray* normalsInput = polyInput->GetPointData()->GetNormals();
  vtkDataArray* normalsOutput = polyOutput->GetPointData()->GetNormals();
  vtkDataArray* tcoordsInput = polyInput->GetPointData()->GetTCoords();
  vtkDataArray* tcoordsOutput = polyOutput->GetPointData()->GetTCoords();

  if (!positionsInput || !positionsOutput || !normalsInput || !normalsOutput || !tcoordsInput ||
    !tcoordsOutput)
  {
    cerr << "One of the arrays is null.\n";
    return EXIT_FAILURE;
  }

  // check values
  vtkNumberToString convert;
  int numberOfDifferentPoints = 0;
  int numberOfDifferentNormals = 0;
  int numberOfDifferentTCoords = 0;
  for (vtkIdType i = 0; i < polyInput->GetNumberOfPoints(); i++)
  {
    double pi[3], po[3];

    // check positions
    positionsInput->GetTuple(i, pi);
    positionsOutput->GetTuple(i, po);
    if (vtkMath::Distance2BetweenPoints(pi, po) > 0.0)
    {
      cerr << "Point is different.\n";
      cerr << "  Input:  " << convert(pi[0]) << " " << convert(pi[1]) << " " << convert(pi[2])
           << "\n";
      cerr << "  Output: " << convert(po[0]) << " " << convert(po[1]) << " " << convert(po[2])
           << "\n";
      numberOfDifferentPoints++;
    }

    // check normals
    normalsInput->GetTuple(i, pi);
    normalsOutput->GetTuple(i, po);
    if (vtkMath::AngleBetweenVectors(pi, po) > 0)
    {
      cerr << "Normal is different:\n";
      cerr << "  Input:  " << convert(pi[0]) << " " << convert(pi[1]) << " " << convert(pi[2])
           << "\n";
      cerr << "  Output: " << convert(po[0]) << " " << convert(po[1]) << " " << convert(po[2])
           << "\n";
      numberOfDifferentNormals++;
    }

    // check texture coords
    tcoordsInput->GetTuple(i, pi);
    tcoordsOutput->GetTuple(i, po);
    pi[2] = po[2] = 0.0;
    if (vtkMath::Distance2BetweenPoints(pi, po) > 0.0)
    {
      cerr << "Texture coord is different:\n";
      cerr << "Input:  " << convert(pi[0]) << " " << convert(pi[1]) << "\n";
      cerr << "Output: " << convert(po[0]) << " " << convert(po[1]) << "\n";
      numberOfDifferentTCoords++;
    }
  }
  if (numberOfDifferentPoints != 0 || numberOfDifferentNormals != 0 ||
    numberOfDifferentTCoords != 0)
  {
    return EXIT_FAILURE;
  }

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(reader->GetOutputPort());

  // read png file and set up texture
  vtkNew<vtkPNGReader> pngReader;
  std::string pngFile = filename.replace(filename.length() - 3, 3, "png");
  pngReader->SetFileName(pngFile.c_str());

  vtkNew<vtkTexture> texture;
  texture->SetInputConnection(pngReader->GetOutputPort());

  // add mapper and texture in an actor
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->SetTexture(texture);

  // Standard rendering classes
  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  // set up the view
  renderer->SetBackground(0.5, 0.5, 0.5);
  renWin->SetSize(300, 300);

  renderer->AddActor(actor);
  renderer->ResetCamera();

  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
