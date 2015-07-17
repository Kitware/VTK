/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPistonToDataSet.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPistonToDataSet.h"

#include "vtkInformation.h"
#include "vtkDataObjectTypes.h"
#include "vtkDataSet.h"
#include "vtkInformationVector.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkPistonDataObject.h"
#include "vtkType.h"

//----------------------------------------------------------------------------
namespace vtkpiston {
  //forward declarations of methods defined in the cuda implementation
  void CopyFromGPU(vtkPistonDataObject *id, vtkImageData *od);
  void CopyFromGPU(vtkPistonDataObject *id, vtkPolyData *od);
}

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPistonToDataSet);

//----------------------------------------------------------------------------
vtkPistonToDataSet::vtkPistonToDataSet()
{
  VTK_LEGACY_BODY(vtkPistonToDataSet::vtkPistonToDataSet, "VTK 6.3");
  this->OutputDataSetType = VTK_POLY_DATA;
}

//----------------------------------------------------------------------------
vtkPistonToDataSet::~vtkPistonToDataSet()
{
}

//----------------------------------------------------------------------------
void vtkPistonToDataSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "OutputDataSetType: " << this->OutputDataSetType << endl;
}

//----------------------------------------------------------------------------
int
vtkPistonToDataSet
::FillOutputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(),
            vtkDataObjectTypes::GetClassNameFromTypeId
            (this->OutputDataSetType));
  return 1;
}

//----------------------------------------------------------------------------
vtkDataSet* vtkPistonToDataSet::GetDataSetOutput(int port)
{
  return vtkDataSet::SafeDownCast(this->GetOutputDataObject(port));
}

//----------------------------------------------------------------------------
int vtkPistonToDataSet::RequestDataObject(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* outputVector)
{
  const char *outTypeStr =
    vtkDataObjectTypes::GetClassNameFromTypeId(this->OutputDataSetType);

  // for each output
  for(int i=0; i < this->GetNumberOfOutputPorts(); ++i)
    {
    vtkInformation* info = outputVector->GetInformationObject(i);
    vtkDataObject *output = info->Get(vtkDataObject::DATA_OBJECT());
    if (!output || !output->IsA(outTypeStr))
      {
      vtkDataObject* newOutput =
        vtkDataObjectTypes::NewDataObject(this->OutputDataSetType);
      if (!newOutput)
        {
        vtkErrorMacro("Could not create chosen output data type: "
                      << outTypeStr);
        return 0;
        }
      this->GetOutputPortInformation(0)->Set(
        vtkDataObject::DATA_EXTENT_TYPE(), newOutput->GetExtentType());
      newOutput->Delete();
      }
    }

  return 1;
}

//------------------------------------------------------------------------------
int vtkPistonToDataSet::RequestData(vtkInformation *vtkNotUsed(request),
                                    vtkInformationVector** inputVector,
                                    vtkInformationVector* outputVector)
{
  vtkPistonDataObject *id = vtkPistonDataObject::GetData(inputVector[0]);
  switch (this->OutputDataSetType)
    {
    case VTK_UNIFORM_GRID:
    case VTK_STRUCTURED_POINTS:
    case VTK_IMAGE_DATA:
      {
      vtkImageData *od = vtkImageData::GetData(outputVector);
      vtkpiston::CopyFromGPU(id, od);
      }
      break;
    case VTK_POLY_DATA:
      {
      vtkPolyData *od = vtkPolyData::GetData(outputVector);
      vtkpiston::CopyFromGPU(id, od);
      od->BuildCells();
      }
      break;
    default:
      vtkWarningMacro(<< "I don't have a converter to "
                      << vtkDataObjectTypes::GetClassNameFromTypeId(this->OutputDataSetType)
                      << " yet.");
      return 0;
    }
  return 1;
}
