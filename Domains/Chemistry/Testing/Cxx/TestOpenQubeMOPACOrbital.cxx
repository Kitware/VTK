/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

#include "vtkAbstractElectronicData.h"
#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkImageData.h"
#include "vtkImageShiftScale.h"
#include "vtkSimpleBondPerceiver.h"
#include "vtkMolecule.h"
#include "vtkMoleculeMapper.h"
#include "vtkNew.h"
#include "vtkOpenQubeMoleculeSource.h"
#include "vtkPiecewiseFunction.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSimpleBondPerceiver.h"
#include "vtkSmartPointer.h"
#include "vtkSmartVolumeMapper.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"

#include <openqube/basissetloader.h>
#include <openqube/basisset.h>

int TestOpenQubeMOPACOrbital(int argc, char *argv[])
{
  char* fname = vtkTestUtilities::ExpandDataFileName(
    argc, argv, "Data/2h2o.out");

  vtkNew<vtkOpenQubeMoleculeSource> oq;
  oq->SetFileName(fname);
  oq->Update();

  delete [] fname;

  vtkSmartPointer<vtkMolecule> mol = vtkSmartPointer<vtkMolecule>::New();
  mol = oq->GetOutput();

  // If there aren't any bonds, attempt to perceive them
  if (mol->GetNumberOfBonds() == 0)
  {
    cout << "No bonds found. Running simple bond perception...\n";
    vtkNew<vtkSimpleBondPerceiver> bonder;
    bonder->SetInputData(mol);
    bonder->Update();
    mol = bonder->GetOutput();
    cout << "Bonds found: " << mol->GetNumberOfBonds() << "\n";
  }

  vtkNew<vtkMoleculeMapper> molMapper;
  molMapper->SetInputData(mol);
  molMapper->UseLiquoriceStickSettings();
  molMapper->SetBondRadius(0.1);
  molMapper->SetAtomicRadiusScaleFactor(0.1);

  vtkNew<vtkActor> molActor;
  molActor->SetMapper(molMapper.GetPointer());

  vtkAbstractElectronicData *edata = oq->GetOutput()->GetElectronicData();
  if (!edata)
  {
    cout << "NULL vtkAbstractElectronicData returned from "
            "vtkOpenQubeElectronicData.\n";
    return EXIT_FAILURE;
  }

  cout << "Num electrons: " << edata->GetNumberOfElectrons() << "\n";

  vtkSmartPointer<vtkImageData> data = vtkSmartPointer<vtkImageData>::New();
  data = edata->GetMO(4);
  if (!data)
  {
    cout << "NULL vtkImageData returned from vtkOpenQubeElectronicData.\n";
    return EXIT_FAILURE;
  }

  double range[2];
  data->GetScalarRange(range);
  cout << "ImageData range: " << range[0] <<" "<< range[1] << "\n";
  double maxAbsVal = (fabs(range[0]) > fabs(range[1])) ? fabs(range[0])
                                                       : fabs(range[1]);

  vtkNew<vtkImageShiftScale> t;
  t->SetInputData(data.GetPointer());
  t->SetShift(maxAbsVal);
  double magnitude = maxAbsVal + maxAbsVal;
  if(fabs(magnitude) < 1e-10)
    magnitude = 1.0;
  t->SetScale(255.0/magnitude);
  t->SetOutputScalarTypeToDouble();

  cout << "magnitude: " << magnitude << "\n";

  t->Update();
  t->GetOutput()->GetScalarRange(range);
  cout << "Shifted min/max: " << range[0] << " " << range[1] << "\n";

  vtkNew<vtkPiecewiseFunction> compositeOpacity;
  compositeOpacity->AddPoint(  0.00, 1.0);
  compositeOpacity->AddPoint( 63.75, 0.8);
  compositeOpacity->AddPoint(127.50, 0.0);
  compositeOpacity->AddPoint(191.25, 0.8);
  compositeOpacity->AddPoint(255.00, 1.0);

  vtkNew<vtkColorTransferFunction> color;
  color->AddRGBSegment( 0.00, 1.0, 0.0, 0.0, 127.0, 1.0, 0.0, 0.0);
  color->AddRGBSegment(128.0, 0.0, 0.0, 1.0, 255.0, 0.0, 0.0, 1.0);

  vtkNew<vtkSmartVolumeMapper> volumeMapper;
  volumeMapper->SetInputConnection(t->GetOutputPort());
  volumeMapper->SetBlendModeToComposite();

  vtkNew<vtkVolumeProperty> volumeProperty;
  volumeProperty->ShadeOff();
  volumeProperty->SetInterpolationTypeToLinear();
  volumeProperty->SetScalarOpacity(compositeOpacity.GetPointer());
  volumeProperty->SetColor(color.GetPointer());

  vtkNew<vtkVolume> volume;
  volume->SetMapper(volumeMapper.GetPointer());
  volume->SetProperty(volumeProperty.GetPointer());

  vtkNew<vtkRenderer> ren;
  vtkNew<vtkRenderWindow> win;
  win->AddRenderer(ren.GetPointer());
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(win.GetPointer());

  ren->AddActor(volume.GetPointer());
  ren->AddActor(molActor.GetPointer());

  ren->SetBackground(0.0, 0.0, 0.0);
  win->SetSize(450,450);
  win->Render();
  ren->GetActiveCamera()->Zoom(2.4);

  // Finally render the scene and compare the image to a reference image
  win->SetMultiSamples(0);
  win->GetInteractor()->Initialize();
  win->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
