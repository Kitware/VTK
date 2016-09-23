/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkAbstractElectronicData.h"
#include "vtkDataSetCollection.h"
#include "vtkImageData.h"
#include "vtkMolecule.h"
#include "vtkNew.h"
#include "vtkOpenQubeElectronicData.h"
#include "vtkOpenQubeMoleculeSource.h"
#include "vtkTestUtilities.h"
#include "vtkWeakPointer.h"

#include <openqube/basissetloader.h>

int TestOpenQubeElectronicData(int argc, char *argv[])
{
  char* fname = vtkTestUtilities::ExpandDataFileName(
    argc, argv, "Data/2h2o.out");

  vtkNew<vtkOpenQubeMoleculeSource> oq;
  oq->SetFileName(fname);
  oq->Update();

  vtkWeakPointer<vtkOpenQubeElectronicData> oqed;
  oqed = vtkOpenQubeElectronicData::SafeDownCast(
        oq->GetOutput()->GetElectronicData());

  if (!oqed)
  {
    cerr << "Molecule's electronic data is not from OpenQube.\n";
    return EXIT_FAILURE;
  }

  // Set parameters to generate fast, low-res cubes
  const float lowResSpacing1 = 1.0;
  const float lowResSpacing2 = 1.5;
  const float lowResPadding1 = 1.0;
  const float lowResPadding2 = 1.5;
  vtkIdType expectedNumberOfImages = 0;

  // Calculate 4 cubes using all combinations of the above parameters.
  // Test that the number of cached images increases accordingly, and that
  // when a cached image is returned, it is the correct one.
  oqed->SetSpacing(lowResSpacing1);
  oqed->SetPadding(lowResPadding1);
  vtkImageData *testImage11 = oqed->GetHOMO();
  ++expectedNumberOfImages;
  if (oqed->GetImages()->GetNumberOfItems() != expectedNumberOfImages)
  {
    cerr << "Number of cached images ("
         << oqed->GetImages()->GetNumberOfItems() << ") not equal to the "
            "number of expected images (" << expectedNumberOfImages << ")\n";
    return EXIT_FAILURE;
  }
  if (oqed->GetHOMO() != testImage11)
  {
    cerr << "(Test11) New orbital calculated when cached image is "
            "available\n";
    return EXIT_FAILURE;
  }

  oqed->SetSpacing(lowResSpacing1);
  oqed->SetPadding(lowResPadding2);
  vtkImageData *testImage12 = oqed->GetHOMO();
  ++expectedNumberOfImages;
  if (oqed->GetImages()->GetNumberOfItems() != expectedNumberOfImages)
  {
    cerr << "Number of cached images ("
         << oqed->GetImages()->GetNumberOfItems() << ") not equal to the "
            "number of expected images (" << expectedNumberOfImages << ")\n";
    return EXIT_FAILURE;
  }
  if (oqed->GetHOMO() != testImage12)
  {
    cerr << "(Test12) New orbital calculated when cached image is "
            "available\n";
    return EXIT_FAILURE;
  }

  oqed->SetSpacing(lowResSpacing2);
  oqed->SetPadding(lowResPadding1);
  vtkImageData *testImage21 = oqed->GetHOMO();
  ++expectedNumberOfImages;
  if (oqed->GetImages()->GetNumberOfItems() != expectedNumberOfImages)
  {
    cerr << "Number of cached images ("
         << oqed->GetImages()->GetNumberOfItems() << ") not equal to the "
            "number of expected images (" << expectedNumberOfImages << ")\n";
    return EXIT_FAILURE;
  }
  if (oqed->GetHOMO() != testImage21)
  {
    cerr << "(Test21) New orbital calculated when cached image is "
            "available\n";
    return EXIT_FAILURE;
  }

  oqed->SetSpacing(lowResSpacing2);
  oqed->SetPadding(lowResPadding2);
  vtkImageData *testImage22 = oqed->GetHOMO();
  ++expectedNumberOfImages;
  if (oqed->GetImages()->GetNumberOfItems() != expectedNumberOfImages)
  {
    cerr << "Number of cached images ("
         << oqed->GetImages()->GetNumberOfItems() << ") not equal to the "
            "number of expected images (" << expectedNumberOfImages << ")\n";
    return EXIT_FAILURE;
  }
  if (oqed->GetHOMO() != testImage22)
  {
    cerr << "(Test22) New orbital calculated when cached image is "
            "available\n";
    return EXIT_FAILURE;
  }


  return EXIT_SUCCESS;
}
