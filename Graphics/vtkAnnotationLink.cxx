/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAnnotationLink.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkAnnotationLink.h"

#include "vtkCommand.h"
#include "vtkDataObjectCollection.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkAnnotationLayers.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"

vtkCxxRevisionMacro(vtkAnnotationLink, "1.2");
vtkStandardNewMacro(vtkAnnotationLink);
vtkCxxSetObjectMacro(vtkAnnotationLink, AnnotationLayers, vtkAnnotationLayers);
//----------------------------------------------------------------------------
vtkAnnotationLink::vtkAnnotationLink()
{
  this->SetNumberOfInputPorts(0);
  this->AnnotationLayers = vtkAnnotationLayers::New();
}

//----------------------------------------------------------------------------
vtkAnnotationLink::~vtkAnnotationLink()
{
  if (this->AnnotationLayers)
    {
    this->AnnotationLayers->Delete();
    }
}

//----------------------------------------------------------------------------
int vtkAnnotationLink::RequestData(
  vtkInformation *vtkNotUsed(info),
  vtkInformationVector **vtkNotUsed(inVector),
  vtkInformationVector *outVector)
{
  vtkInformation *outInfo = outVector->GetInformationObject(0);
  vtkAnnotationLayers* output = vtkAnnotationLayers::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  
  if (this->AnnotationLayers)
    {
    output->ShallowCopy(this->AnnotationLayers);
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkAnnotationLink::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "AnnotationLayers: ";
  if (this->AnnotationLayers)
    {
    os << "\n";
    this->AnnotationLayers->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)\n";
    }
}

