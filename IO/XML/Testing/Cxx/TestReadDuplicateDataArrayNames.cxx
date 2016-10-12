/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestReadDuplicateDataArrayNames.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkPointData.h>
#include <vtkSmartPointer.h>
#include <vtkTestUtilities.h>
#include <vtkUnstructuredGrid.h>
#include <vtkXMLUnstructuredGridReader.h>
#include <vtkXMLWriterC.h>

#include <cmath>
#include <fstream>

#define NPOINTS 8
#define NTIMESTEPS 8
#define VTK_EPSILON 1.e-6

void generateDataSetWithTimesteps(std::string filename)
{
  int i,j;
  {
  vtkXMLWriterC* writer = vtkXMLWriterC_New();
  float points[3*NPOINTS] = {0, 0, 0,
                       1, 0, 0,
                       1, 1, 0,
                       0, 1, 0,
                       0, 0, 1,
                       1, 0, 1,
                       1, 1, 1,
                       0, 1, 1 };
  vtkIdType cellarray[] = {8, 0, 1, 2, 3, 4, 5, 6, 7};
  float pointdata[NTIMESTEPS][NPOINTS];
  /* Give different values for the pointdata: */
  for(i=0;i<NTIMESTEPS;i++)
  {
    for(j=0; j<NPOINTS;j++)
    {
      pointdata[i][j] = (float)i;
    }
  }

  /* #define VTK_UNSTRUCTURED_GRID               4 */
  vtkXMLWriterC_SetDataObjectType(writer, 4);
  vtkXMLWriterC_SetFileName(writer, filename.c_str() );
  /* #define VTK_FLOAT          10 */
  vtkXMLWriterC_SetPoints(writer, 10, points, NPOINTS);
  /* #define VTK_HEXAHEDRON    12 */
  vtkXMLWriterC_SetCellsWithType(writer, 12, 1, cellarray, 1+NPOINTS);

  /* for all timesteps: */
  vtkXMLWriterC_SetNumberOfTimeSteps(writer, NTIMESTEPS);
  vtkXMLWriterC_Start(writer);
  for(i=0; i<NTIMESTEPS; i++)
  {
    /* #define VTK_FLOAT          10 */
    vtkXMLWriterC_SetPointData(writer, "example data", 10, pointdata[i],
                               NPOINTS, 1, "SCALARS");
    vtkXMLWriterC_WriteNextTimeStep(writer, i);
  }
  vtkXMLWriterC_Stop(writer);
  vtkXMLWriterC_Delete(writer);
  }
}

void generateDataSetWithDuplicateArrayNames(std::string filename)
{
  std::string dataSet =
    "<VTKFile type=\"UnstructuredGrid\" version=\"0.1\" byte_order=\"LittleEndian\">\n"
    "<UnstructuredGrid>\n"
    " <Piece NumberOfPoints=\"4\" NumberOfCells=\"1\">\n"
    "   <PointData Scalars=\"scalars\">\n"
    "     <DataArray type=\"Float32\" Name=\"test123\" format=\"ascii\">\n"
    "        0.0 1.0 2.0 3.0 \n"
    "     <DataArray type=\"Float32\" Name=\"test123\" format=\"ascii\">\n"
    "        0.1 0.2 0.3 0.4\n"
    "     </DataArray>\n"
    "     </DataArray>\n"
    "   </PointData>\n"
    "   <Points>\n"
    "     <DataArray type=\"Float32\" NumberOfComponents=\"3\" format=\"ascii\">\n"
    "        0 0 0 0 0 1 0 1 0 1 0 0\n"
    "     </DataArray>\n"
    "   </Points>\n"
    "   <Cells>\n"
    "     <DataArray type=\"Int32\" Name=\"connectivity\" format=\"ascii\">\n"
    "        0 1 2 3\n"
    "     </DataArray>\n"
    "     <DataArray type=\"Int32\" Name=\"offsets\" format=\"ascii\">\n"
    "        4\n"
    "     </DataArray>\n"
    "     <DataArray type=\"UInt8\" Name=\"types\" format=\"ascii\">\n"
    "        10\n"
    "     </DataArray>\n"
    "   </Cells>\n"
    " </Piece>\n"
    "</UnstructuredGrid>\n"
    "</VTKFile>";

  ofstream myfile;
  myfile.open (filename.c_str());
  myfile << dataSet;
  myfile.close();
}

int TestReadDuplicateDataArrayNames(int argc, char* argv[])
{
  // These tests are designed to ensure that point/cell data arrays with the
  // same name are handled appropriately by the xml reader. The first test
  // creates multiple data arrays that have the same name but differ in
  // timestep. We test that the different timesteps are accessible. The second
  // test creates two data arrays that have the same name and no timestep
  // affiliation. In this case, we ensure that the reader reads the first data
  // array without crashing.

  char* temp_dir_c =
    vtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv,
                                           "VTK_TEMP_DIR",
                                           "Testing/Temporary");
  std::string temp_dir = std::string(temp_dir_c);
  delete [] temp_dir_c;

  if (temp_dir.empty())
    {
    std::cerr << "Could not determine temporary directory." << std::endl;
    return EXIT_FAILURE;
    }
  std::string filename = temp_dir + "/duplicateArrayNames.vtu";

  // Test 1
  {
  generateDataSetWithTimesteps(filename);

  vtkSmartPointer<vtkXMLUnstructuredGridReader> reader =
    vtkSmartPointer<vtkXMLUnstructuredGridReader>::New();
  reader->SetFileName(filename.c_str());

  for ( int i = 0; i < NTIMESTEPS; i++ )
    {
    reader->SetTimeStep(i);
    reader->Update();

    vtkUnstructuredGrid* ugrid = reader->GetOutput();
    double d = ugrid->GetPointData()->GetScalars()->GetTuple1(0);

    if (std::abs(static_cast<double>((d-static_cast<double>(i)))) > VTK_EPSILON)
      {
      std::cerr << "Different timesteps were not correctly read." << std::endl;
      return EXIT_FAILURE;
      }
    }
  }

  // Test 2
  {
  generateDataSetWithDuplicateArrayNames(filename);

  vtkSmartPointer<vtkXMLUnstructuredGridReader> reader =
    vtkSmartPointer<vtkXMLUnstructuredGridReader>::New();
  reader->SetFileName(filename.c_str());
  reader->Update();

  vtkUnstructuredGrid* ugrid = reader->GetOutput();
  for (int i = 0; i < 4; i++)
    {
    double d = ugrid->GetPointData()->GetScalars("test123")->GetTuple1(i);
    std::cout<<d<<std::endl;

    if (std::abs(static_cast<double>((d-static_cast<double>(i)))) > VTK_EPSILON)
      {
      std::cerr << "The first array with degenerate naming was not correctly read." << std::endl;
      return EXIT_FAILURE;
      }
    }
  }

  return EXIT_SUCCESS;
}
