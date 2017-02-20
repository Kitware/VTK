/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/


// PURPOSE: FiberSurface test cases output generator (new vtkFiberSurface filter)

#include <cstring>
#include <sstream>
#include <string>

#include "vtkFiberSurface.h"
#include "vtkTestUtilities.h"
#include "vtkActor.h"
#include "vtkCellArray.h"
#include "vtkCleanPolyData.h"
#include "vtkFloatArray.h"
#include "vtkPolyData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataReader.h"
#include "vtkPolyDataWriter.h"
#include "vtkSmartPointer.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridReader.h"

int TestFiberSurface(int argc, char* argv[])
{
  bool pass = true;

  /************** Input File paths using vtkTestUtilities ****************/
  const char* inputDataFiles[3] = { vtkTestUtilities::ExpandDataFileName(
                                      argc, argv, "Data/FiberSurface/one_cube.vtk"),
    vtkTestUtilities::ExpandDataFileName(argc, argv,
                                      "Data/FiberSurface/one_cube_both_forking.vtk"),
    vtkTestUtilities::ExpandDataFileName(argc, argv,
                                      "Data/FiberSurface/one_cube_closed.vtk") };

  /************** FSCP File paths using vtkTestUtilities ****************/
  const char* inputFSCPFiles[15] = { vtkTestUtilities::ExpandDataFileName(
                                       argc, argv, "Data/FiberSurface/line_01.vtk"),
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/FiberSurface/line_02.vtk"),
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/FiberSurface/line_03.vtk"),
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/FiberSurface/line_04.vtk"),
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/FiberSurface/line_05.vtk"),
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/FiberSurface/line_11.vtk"),
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/FiberSurface/line_12.vtk"),
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/FiberSurface/line_13.vtk"),
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/FiberSurface/line_14.vtk"),
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/FiberSurface/line_15.vtk"),
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/FiberSurface/line_21.vtk"),
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/FiberSurface/line_22.vtk"),
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/FiberSurface/line_23.vtk"),
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/FiberSurface/line_24.vtk"),
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/FiberSurface/line_25.vtk") };
  /**********************************************/

  /********************* Desired output arrays *********************/
  std::string dataToCompare[15] = { "0.779,0.000,0.000,0.659,0.000,0.341,1.000,0.000,0.624,1.000,0."
                                    "797,0.000,0.659,0.659,1.000,1.000,1.000,0.203,1.000,0.376,1."
                                    "000,0.779,1.000,1.000",
    "0.775,0.000,0.775,0.798,0.000,1.000,0.768,0.232,0.768,0.874,0.000,0.000,0.889,0.000,0.111,0."
    "918,0.918,0.082,0.889,0.889,1.000,0.874,1.000,1.000,0.775,0.225,1.000",
    "0.000,0.800,0.000,0.000,0.000,0.667,0.286,0.000,0.286,0.200,0.000,0.000,0.000,0.333,1.000,0."
    "200,1.000,1.000,0.286,0.714,1.000,0.000,1.000,0.200",
    "0.331,0.000,0.331,0.167,0.833,0.167,0.202,0.000,1.000,0.285,0.000,0.000,0.169,0.000,0.831,0."
    "210,0.210,0.790,0.169,0.169,1.000,0.285,1.000,1.000,0.331,0.669,1.000",
    "0.650,0.000,1.000,0.500,0.500,0.500,0.000,0.053,0.000,0.000,0.000,0.029,1.000,0.000,0.478,0."
    "057,0.000,0.000,1.000,0.868,0.000,0.931,0.931,0.069,1.000,0.522,1.000,1.000,1.000,0.132,0.000,"
    "0.971,1.000,0.057,1.000,1.000,0.000,1.000,0.947",
    "0.500,0.500,1.000,1.000,1.000,0.714,1.000,0.667,1.000,0.714,1.000,1.000",
    "0.000,0.901,0.000,0.233,0.767,0.233,0.000,0.271,0.729,0.886,0.000,0.000,0.241,0.000,0.759,1."
    "000,0.000,0.263,1.000,0.201,0.000,0.064,0.064,1.000,1.000,1.000,0.460,1.000,0.376,1.000,0.469,"
    "1.000,1.000,0.000,1.000,0.172",
    "0.000,0.571,0.000,0.000,0.000,0.667,1.000,0.000,1.000,0.571,0.000,0.000,0.667,0.667,0.333,1."
    "000,0.000,1.000,1.000,0.750,0.000,1.000,1.000,0.143,0.000,0.667,1.000,0.143,1.000,1.000,1.000,"
    "0.000,1.000,0.000,1.000,0.750",
    "0.000,0.250,0.000,0.000,0.000,0.167,0.250,0.000,0.250,0.100,0.000,0.000,0.000,0.833,1.000,0."
    "100,1.000,1.000,0.250,0.750,1.000,0.000,1.000,0.750",
    "0.333,0.000,0.000,0.333,0.000,0.667,1.000,0.000,0.667,0.333,0.333,0.667,1.000,0.333,0.667,1."
    "000,1.000,0.667",
    "0.000,0.000,0.300,0.300,0.000,0.300,0.300,0.700,0.300,0.000,0.700,0.300,0.700,0.000,0.300,1."
    "000,0.000,0.300,0.700,0.700,0.300,1.000,0.700,0.300,1.000,0.700,1.000,0.700,0.700,1.000,0.300,"
    "0.700,1.000,0.000,0.700,1.000",
    "0.800,0.200,0.800,0.800,0.000,0.800,0.000,0.000,0.800,0.000,0.200,0.800,0.200,0.000,0.800,1."
    "000,0.000,0.800,1.000,0.200,0.800,0.200,0.200,0.800,0.200,0.200,1.000,1.000,0.200,1.000,0.800,"
    "0.200,1.000,0.000,0.200,1.000",
    "0.828,0.000,0.828,0.737,0.000,1.000,0.856,0.144,0.856,1.000,0.000,0.828,1.000,0.144,0.856,1."
    "000,0.172,1.000,0.856,0.144,1.000",
    "0.000,0.739,0.000,0.000,0.000,0.146,0.427,0.000,0.427,0.854,0.000,0.146,1.000,0.000,0.427,1."
    "000,0.739,0.000,0.854,0.854,1.000,1.000,1.000,0.261,1.000,0.573,1.000,0.261,1.000,1.000",
    "0.977,0.023,0.977,0.980,0.000,0.980,0.000,0.000,0.671,0.000,0.363,0.637,0.329,0.000,0.671,1."
    "000,0.000,0.980,1.000,0.023,0.977,0.363,0.363,0.637,0.329,0.329,1.000,1.000,0.020,1.000,0.977,"
    "0.023,1.000,0.000,0.363,1.000" };

  std::string outputString;
  char firstFieldName[] = "f1";
  char secondFieldName[] = "f2";
  int cmpIndex = 0;

  /************** output to error file ****************/
  /****************************************************/

  printf("FiberSurface test cases");
  /****************************************************/

  for (int i = 0; i < 3; i++) // loop for three input files
  {

    for (int j = 0; j < 5; j++) // loop for fifteen fscp files
    {
      // read and load .vtk input data file
      vtkSmartPointer<vtkUnstructuredGridReader> mesh_reader =
        vtkSmartPointer<vtkUnstructuredGridReader>::New();
      mesh_reader->SetFileName(inputDataFiles[i]);
      mesh_reader->Update();

      // read and load .vtu file containing control polyline
      vtkSmartPointer<vtkPolyDataReader> polyline_reader =
        vtkSmartPointer<vtkPolyDataReader>::New();
      polyline_reader->SetFileName(inputFSCPFiles[cmpIndex]);
      polyline_reader->Update();

      // extract polydata surface
      vtkSmartPointer<vtkFiberSurface> fs = vtkSmartPointer<vtkFiberSurface>::New();
      fs->SetInputData(0, mesh_reader->GetOutput());
      fs->SetInputData(1, polyline_reader->GetOutput());
      fs->SetField1(firstFieldName);
      fs->SetField2(secondFieldName);
      fs->Update();

      /********************* cleaning and comparing FS Soup to desired output ********************/

      vtkPolyData* polyData = fs->GetOutput();

      /**** Duplicate Ponts ****/
      int numOfPoints = polyData->GetNumberOfPoints();
      int index = 0;
      double* coords;

      /**** NON Duplicate Ponts ****/
      vtkSmartPointer<vtkCleanPolyData> clean = vtkSmartPointer<vtkCleanPolyData>::New();
      clean->SetInputData(polyData);
      clean->Update();
      numOfPoints = clean->GetOutput()->GetNumberOfPoints();
      std::cout << ".";

      index = 0;
      outputString = "";
      std::stringstream ss;

      // out non duplicate points
      while (index < numOfPoints)
      {
        coords = clean->GetOutput()->GetPoint(index);
        ss << std::fixed << std::setprecision(3) << coords[0] << "," << std::fixed
           << std::setprecision(3) << coords[1] << "," << std::fixed << std::setprecision(3)
           << coords[2];
        if (index > 0)
          outputString.append(",");
        outputString.append(ss.str());
        ss.str(std::string());
        ss.clear();
        index++;
      } // end while

      /**********************************************/
      /*********** comparing the results ************/

      if (strcmp(outputString.c_str(), dataToCompare[cmpIndex].c_str()) != 0)
      {
        pass = false;
        { // writing to file
          printf("\n\n/**************************************/");
          printf("\n/********Test Unsuccessful*************/");
          printf("\nInput  Data: %s", (std::string(inputDataFiles[i]).substr(6, 31)).c_str());
          printf(
            "\nInput  FSCP: %s", (std::string(inputFSCPFiles[cmpIndex]).substr(6, 30)).c_str());
          printf("\nString to compare: %s", dataToCompare[cmpIndex].c_str());
          printf("\nOutput String     : %s", outputString.c_str());
          printf("\n/**************************************/");
        } // end of writing
      }
      cmpIndex++;
    } // end for
  }   // end for

  for (int i = 0; i <  15; ++i)
  {
    delete [] inputFSCPFiles[i];
  }
  delete [] inputDataFiles[0];
  delete [] inputDataFiles[1];
  delete [] inputDataFiles[2];

  if (pass)
  {
    cout << "\nTest Successful!!!" << endl;
    return EXIT_SUCCESS;
  }

  cout << "\nTest Unsuccessful." << endl;
  return EXIT_FAILURE;
}
