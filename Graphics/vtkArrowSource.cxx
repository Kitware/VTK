/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArrowSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkArrowSource.h"

#include "vtkAppendPolyData.h"
#include "vtkConeSource.h"
#include "vtkCylinderSource.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkTransform.h"
#include "vtkTransformFilter.h"

vtkStandardNewMacro(vtkArrowSource);

vtkArrowSource::vtkArrowSource()
{
  this->TipResolution = 6;
  this->TipRadius = 0.1;
  this->TipLength = 0.35;
  this->ShaftResolution = 6;
  this->ShaftRadius = 0.03;
  this->Invert = false;
  
  this->SetNumberOfInputPorts(0);
}

int vtkArrowSource::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the ouptut
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  int piece, numPieces, ghostLevel;
  vtkCylinderSource *cyl = vtkCylinderSource::New();
  vtkTransform *trans0 = vtkTransform::New();
  vtkTransformFilter *tf0 = vtkTransformFilter::New();
  vtkConeSource *cone = vtkConeSource::New();
  vtkTransform *trans1 = vtkTransform::New();
  vtkTransform *trans2 = vtkTransform::New();
  vtkTransformFilter *tf1 = vtkTransformFilter::New();
  vtkTransformFilter *tf2 = vtkTransformFilter::New();
  vtkAppendPolyData *append = vtkAppendPolyData::New();

  piece = output->GetUpdatePiece();
  numPieces = output->GetUpdateNumberOfPieces();
  ghostLevel = output->GetUpdateGhostLevel();

  cyl->SetResolution(this->ShaftResolution);
  cyl->SetRadius(this->ShaftRadius);
  cyl->SetHeight(1.0 - this->TipLength);
  cyl->SetCenter(0, (1.0-this->TipLength)*0.5, 0.0);
  cyl->CappingOn();

  trans0->RotateZ(-90.0);
  tf0->SetTransform(trans0);
  tf0->SetInput(cyl->GetOutput());

  cone->SetResolution(this->TipResolution);
  cone->SetHeight(this->TipLength);
  cone->SetRadius(this->TipRadius);

  trans1->Translate(1.0-this->TipLength*0.5, 0.0, 0.0);
  tf1->SetTransform(trans1);
  tf1->SetInput(cone->GetOutput());

  append->AddInput(tf0->GetPolyDataOutput());
  append->AddInput(tf1->GetPolyDataOutput());

 // used only when this->Invert is true.
 trans2->Translate(1, 0, 0);
 trans2->Scale(-1, 1, 1);
 tf2->SetTransform(trans2);
 tf2->SetInputConnection(append->GetOutputPort());

  if (output->GetUpdatePiece() == 0 && numPieces > 0)
    {
    if (this->Invert)
      {
      tf2->Update();
      output->ShallowCopy(tf2->GetOutput());
      }
    else
      {
      append->Update();
      output->ShallowCopy(append->GetOutput());
      }
    }
  output->SetUpdatePiece(piece);
  output->SetUpdateNumberOfPieces(numPieces);
  output->SetUpdateGhostLevel(ghostLevel);

  cone->Delete();
  trans0->Delete();
  tf0->Delete();
  cyl->Delete();
  trans1->Delete();
  tf1->Delete();
  append->Delete();
  tf2->Delete();
  trans2->Delete();

  return 1;
}

void vtkArrowSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "TipResolution: " << this->TipResolution << "\n";
  os << indent << "TipRadius: " << this->TipRadius << "\n";
  os << indent << "TipLength: " << this->TipLength << "\n";

  os << indent << "ShaftResolution: " << this->ShaftResolution << "\n";
  os << indent << "ShaftRadius: " << this->ShaftRadius << "\n";

  os << indent << "Invert: " << this->Invert << "\n";
}
