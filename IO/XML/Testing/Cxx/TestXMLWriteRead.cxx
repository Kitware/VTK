/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestXMLWriteRead.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPointSource.h"
#include "vtkXMLPolyDataReader.h"
#include "vtkXMLPolyDataWriter.h"

#include "vtkPointSource.h"
#include "vtkPoints.h"

#include "vtkTestUtilities.h"
#include <string>

namespace
{
template <typename T>
int TestConvertType(const std::string& type, const std::string& fileName);
}
int TestXMLWriteRead(int argc, char* argv[])
{
  char* tempDir =
    vtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary");
  std::string fileName(tempDir);
  delete[] tempDir;

  int statusFloat, statusDouble;

  statusFloat = TestConvertType<float>("float", fileName + "/XMLReadWriteFloat.vtp");
  statusDouble = TestConvertType<double>("double", fileName + "/XMLReadWriteDouble.vtp");
  if (statusFloat == EXIT_FAILURE || statusDouble == EXIT_FAILURE)
  {
    return EXIT_FAILURE;
  }
  else
  {
    return EXIT_SUCCESS;
  }
}

namespace
{
template <typename T>
int TestConvertType(const std::string& type, const std::string& fileName)
{
  std::cout << "Testing type " << type << std::endl;

  // Create a polydata with points, write the polydata, read the polydata
  // and compare the point values
  vtkSmartPointer<vtkPointSource> source = vtkSmartPointer<vtkPointSource>::New();
  source->SetCenter(0.0, 0.0, 0.0);
  source->SetNumberOfPoints(5000);
  source->SetRadius(5.0);
  if (type == "float")
  {
    source->SetOutputPointsPrecision(vtkAlgorithm::SINGLE_PRECISION);
  }
  else
  {
    source->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);
  }
  source->Update();
  std::cout << "Write to " << fileName << std::endl;

  // write the polydata
  vtkSmartPointer<vtkXMLPolyDataWriter> writer = vtkSmartPointer<vtkXMLPolyDataWriter>::New();
  writer->SetInputData(source->GetOutput());
  writer->SetFileName(fileName.c_str());
  writer->SetDataModeToAscii();
  writer->Write();

  // read back the polydata
  vtkSmartPointer<vtkXMLPolyDataReader> reader = vtkSmartPointer<vtkXMLPolyDataReader>::New();
  reader->SetFileName(fileName.c_str());
  reader->Update();

  vtkPoints* originalPoints = source->GetOutput()->GetPoints();
  vtkPoints* readPoints = reader->GetOutput()->GetPoints();

  unsigned int numberOfMismatches = 0;
  for (auto i = 0; i < originalPoints->GetNumberOfPoints(); ++i)
  {
    T* original;
    original = static_cast<T*>(originalPoints->GetVoidPointer(i * 3));
    T* read;
    read = static_cast<T*>(readPoints->GetVoidPointer(i * 3));
    for (auto j = 0; j < 3; ++j)
    {
      if (original[j] != read[j])
      {
        std::cout << std::setprecision(19) << "WARNING: point mismatch. PointId: " << i << " "
                  << "original[" << j << "] != read[" << j << "] " << original[j]
                  << " != " << read[j] << std::endl;
        // The epsRatio measures the error in multiples of the units in the last place (ULP)
        T epsRatio = std::abs((original[j] - read[j]) / std::numeric_limits<T>::epsilon());
        std::cout << " eps ratio is: " << epsRatio << std::endl;

        // Accept a few insignificant mismatches
        if (epsRatio < 4)
        {
          ++numberOfMismatches;
        }
      }
    }
  }
  if (numberOfMismatches > 5)
  {
    return EXIT_FAILURE;
  }
  else
  {
    return EXIT_SUCCESS;
  }
}
}
