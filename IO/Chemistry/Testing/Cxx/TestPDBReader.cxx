/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPDBReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPDBReader.h"
#include "vtkPolyData.h"
#include "vtkTestUtilities.h"

int TestPdb(
  const char* pdbFileName, unsigned int validNumberOfAtoms, unsigned int validNumberOfModels);

/**
 * @brief Test entry point.
 */
int TestPDBReader(int argc, char* argv[])
{
  // Validation data
  constexpr unsigned int VALID_NUMBER_OF_ATOMS_6VWW = 18027;
  constexpr unsigned int VALID_NUMBER_OF_MODELS_6VWW = 3;

  // Test PDB 6vww
  const char* fileName6vww = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/6VWW.pdb");
  const int testResult6vww =
    TestPdb(fileName6vww, VALID_NUMBER_OF_ATOMS_6VWW, VALID_NUMBER_OF_MODELS_6VWW);

  delete[] fileName6vww;

  if (testResult6vww != 0)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

/**
 * @brief Test a PDB file.
 * @param pdbFileName File name of the PDB file to read for testing.
 * @param validNumberOfAtoms The amount of atoms the PDB should have to be valid.
 * @param validNumberOfModels The amount of models the PDB should have to be valid.
 */
int TestPdb(
  const char* pdbFileName, unsigned int validNumberOfAtoms, unsigned int validNumberOfModels)
{
  const auto pdbReader = vtkSmartPointer<vtkPDBReader>::New();
  pdbReader->SetFileName(pdbFileName);
  pdbReader->Update();

  const unsigned int numberOfAtoms = pdbReader->GetNumberOfAtoms();
  const unsigned int numberOfModels = pdbReader->GetNumberOfModels();

  // Check the number of atoms
  if (numberOfAtoms != validNumberOfAtoms)
  {
    std::cerr << "Invalid number of atoms for " << pdbFileName << '.' << std::endl;
    std::cerr << "Found " << numberOfAtoms << ", but " << validNumberOfAtoms << " required."
              << std::endl;
    return EXIT_FAILURE;
  }

  // Check the number of models
  if (numberOfModels != validNumberOfModels)
  {
    std::cerr << "Invalid number of models for " << pdbFileName << std::endl;
    std::cerr << "Found " << numberOfModels << ", but " << validNumberOfModels << " required."
              << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
