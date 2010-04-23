/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSectorSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

     =========================================================================*/
#include "vtkSectorSource.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
//#include "vtkDiskSource.h"
#include "vtkLineSource.h"
#include "vtkRotationalExtrusionFilter.h"
#include "vtkMath.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name)                                  \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

vtkStandardNewMacro(vtkSectorSource);

vtkSectorSource::vtkSectorSource()
{
  this->InnerRadius = 1.0;
  this->OuterRadius = 2.0;
  this->ZCoord = 0.0;
  this->StartAngle = 0.0;
  this->EndAngle = 90.0;
  this->RadialResolution = 1;
  this->CircumferentialResolution = 6;
  
  this->SetNumberOfInputPorts(0);
}

int vtkSectorSource::RequestData(
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
  piece = output->GetUpdatePiece();
  numPieces = output->GetUpdateNumberOfPieces();
  ghostLevel = output->GetUpdateGhostLevel();
  
//   if( (this->StartAngle == 0. && this->EndAngle == 360.) ||
//       (this->StartAngle == 360. && this->EndAngle == 0. ) )
//   {
//       //use vtkDiskSource
//     VTK_CREATE(vtkDiskSource, diskSource );
//     diskSource->SetCircumferentialResolution( this->CircumferentialResolution );
//     diskSource->SetRadialResolution( this->RadialResolution );
//     diskSource->SetInnerRadius( this->InnerRadius );
//     diskSource->SetOuterRadius( this->OuterRadius );
  
//     if (output->GetUpdatePiece() == 0 && numPieces > 0)
//     {
//       diskSource->Update();
//       output->ShallowCopy(diskSource->GetOutput());
//     }
//     output->SetUpdatePiece(piece);
//     output->SetUpdateNumberOfPieces(numPieces);
//     output->SetUpdateGhostLevel(ghostLevel);
//   }
//   else
//   {
  VTK_CREATE(vtkLineSource, lineSource);
  lineSource->SetResolution( this->RadialResolution );
  
  //set vertex 1, adjust for start angle
  //set vertex 2, adjust for start angle
  double x1[3], x2[3];
  x1[0] = this->InnerRadius * cos( vtkMath::RadiansFromDegrees( this->StartAngle ) );
  x1[1] = this->InnerRadius * sin( vtkMath::RadiansFromDegrees( this->StartAngle ) );
  x1[2] = this->ZCoord;
  
  x2[0] = this->OuterRadius * cos( vtkMath::RadiansFromDegrees( this->StartAngle ) );
  x2[1] = this->OuterRadius * sin( vtkMath::RadiansFromDegrees( this->StartAngle ) );
  x2[2] = this->ZCoord;
  
  lineSource->SetPoint1(x1);
  lineSource->SetPoint2(x2);
  lineSource->Update();
  
  VTK_CREATE(vtkRotationalExtrusionFilter, rotateFilter);
  rotateFilter->SetResolution( this->CircumferentialResolution );
  rotateFilter->SetInput(lineSource->GetOutput());
  rotateFilter->SetAngle( this->EndAngle - this->StartAngle );
  
  if (output->GetUpdatePiece() == 0 && numPieces > 0)
    {
    rotateFilter->Update();
    output->ShallowCopy(rotateFilter->GetOutput());
    }
  output->SetUpdatePiece(piece);
  output->SetUpdateNumberOfPieces(numPieces);
  output->SetUpdateGhostLevel(ghostLevel);
//  }
  
  return 1;
}

void vtkSectorSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "InnerRadius: " << this->InnerRadius << "\n";
  os << indent << "OuterRadius: " << this->OuterRadius << "\n";
  os << indent << "ZCoord: " << this->ZCoord << "\n";
  os << indent << "StartAngle: " << this->StartAngle << "\n";
  os << indent << "EndAngle: " << this->EndAngle << "\n";;
  os << indent << "CircumferentialResolution: " << this->CircumferentialResolution << "\n";
  os << indent << "RadialResolution: " << this->RadialResolution << "\n";
}
