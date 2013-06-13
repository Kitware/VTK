/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestvtkAMRInterpolatedVelocityField.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <vtkAMRInterpolatedVelocityField.h>
#include <vtkAMRGaussianPulseSource.h>
#include <vtkNew.h>
#include <vtkGradientFilter.h>
#include <vtkOverlappingAMR.h>
#include <vtkMath.h>
#include <vtkCompositeDataPipeline.h>

#define RETURNONFALSE(b)\
  if(!(b)) \
    {\
    vtkAlgorithm::SetDefaultExecutivePrototype(NULL);\
    return EXIT_FAILURE;\
    }

int TestAMRInterpolatedVelocityField(int, char*[])
{
  vtkNew<vtkCompositeDataPipeline> cexec;
  vtkAlgorithm::SetDefaultExecutivePrototype(cexec.GetPointer());

  char name[100] = "Gaussian-Pulse";
  vtkNew<vtkAMRGaussianPulseSource> imageSource;
  vtkNew<vtkGradientFilter> gradientFilter;
  gradientFilter->SetInputConnection(imageSource->GetOutputPort());
  gradientFilter->SetInputScalars( vtkDataObject::FIELD_ASSOCIATION_CELLS,name);
  gradientFilter->SetResultArrayName("Gradient");
  gradientFilter->Update();

  vtkOverlappingAMR* amrGrad = vtkOverlappingAMR::SafeDownCast(gradientFilter->GetOutputDataObject(0));
  amrGrad->GenerateParentChildInformation();

  vtkNew<vtkAMRInterpolatedVelocityField> func;
  func->SetAMRData(amrGrad);
  func->SelectVectors(vtkDataObject::FIELD_ASSOCIATION_CELLS,"Gradient");

  double Points[4][3] =
    {{-2.1,-0.51,1},
     {-1.9,-0.51,1},
     {-0.9,-0.51,1},
     {-0.1,-0.51,1}
    };

  double v[3];
  bool res;
  unsigned int level, id;
  res = func->FunctionValues(Points[0],v)!=0;
  RETURNONFALSE(!res);
  res = func->FunctionValues(Points[1],v)!=0;
  RETURNONFALSE(res);
  func->GetLastDataSetLocation(level,id);
  RETURNONFALSE(level==1)
  res = func->FunctionValues(Points[2],v)!=0;
  RETURNONFALSE(res);
  func->GetLastDataSetLocation(level,id);
  RETURNONFALSE(level==0)
  res = func->FunctionValues(Points[3],v)!=0;
  RETURNONFALSE(res);
  func->GetLastDataSetLocation(level,id);
  RETURNONFALSE(level==1)

  vtkAlgorithm::SetDefaultExecutivePrototype(NULL);
  return EXIT_SUCCESS;
}
