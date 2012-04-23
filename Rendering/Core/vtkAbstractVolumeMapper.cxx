/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractVolumeMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAbstractVolumeMapper.h"

#include "vtkDataSet.h"
#include "vtkExecutive.h"
#include "vtkInformation.h"
#include "vtkMath.h"


// Construct a vtkAbstractVolumeMapper
vtkAbstractVolumeMapper::vtkAbstractVolumeMapper()
{
  vtkMath::UninitializeBounds(this->Bounds);
  this->Center[0] = this->Center[1] = this->Center[2] = 0.0;

  this->ScalarMode = VTK_SCALAR_MODE_DEFAULT;

  this->ArrayName = new char[1];
  this->ArrayName[0] = '\0';
  this->ArrayId = -1;
  this->ArrayAccessMode = VTK_GET_ARRAY_BY_ID;
}

vtkAbstractVolumeMapper::~vtkAbstractVolumeMapper()
{
  delete[] this->ArrayName;
}

// Get the bounds for the input of this mapper as
// (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
double *vtkAbstractVolumeMapper::GetBounds()
{
  if ( ! this->GetDataSetInput() )
    {
    vtkMath::UninitializeBounds(this->Bounds);
    return this->Bounds;
    }
  else
    {
    this->Update();
    this->GetDataSetInput()->GetBounds(this->Bounds);
    return this->Bounds;
    }
}

vtkDataObject *vtkAbstractVolumeMapper::GetDataObjectInput()
{
  if (this->GetNumberOfInputConnections(0) < 1)
    {
    return 0;
    }
  return this->GetInputDataObject(0, 0);
}

vtkDataSet *vtkAbstractVolumeMapper::GetDataSetInput()
{
  if (this->GetNumberOfInputConnections(0) < 1)
    {
    return 0;
    }
  return vtkDataSet::SafeDownCast(this->GetInputDataObject(0, 0));
}

//----------------------------------------------------------------------------
int vtkAbstractVolumeMapper::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

void vtkAbstractVolumeMapper::SelectScalarArray(int arrayNum)
{
  if (   (this->ArrayId == arrayNum)
      && (this->ArrayAccessMode == VTK_GET_ARRAY_BY_ID) )
    {
    return;
    }
  this->Modified();

  this->ArrayId = arrayNum;
  this->ArrayAccessMode = VTK_GET_ARRAY_BY_ID;
}

void vtkAbstractVolumeMapper::SelectScalarArray(const char *arrayName)
{
  if (   !arrayName
      || (   (strcmp(this->ArrayName, arrayName) == 0)
          && (this->ArrayAccessMode == VTK_GET_ARRAY_BY_NAME) ) )
    {
    return;
    }
  this->Modified();

  delete[] this->ArrayName;
  this->ArrayName = new char[strlen(arrayName) + 1];
  strcpy(this->ArrayName, arrayName);
  this->ArrayAccessMode = VTK_GET_ARRAY_BY_NAME;
}

// Return the method for obtaining scalar data.
const char *vtkAbstractVolumeMapper::GetScalarModeAsString(void)
{
  if ( this->ScalarMode == VTK_SCALAR_MODE_USE_CELL_DATA )
    {
    return "UseCellData";
    }
  else if ( this->ScalarMode == VTK_SCALAR_MODE_USE_POINT_DATA )
    {
    return "UsePointData";
    }
  else if ( this->ScalarMode == VTK_SCALAR_MODE_USE_POINT_FIELD_DATA )
    {
    return "UsePointFieldData";
    }
  else if ( this->ScalarMode == VTK_SCALAR_MODE_USE_CELL_FIELD_DATA )
    {
    return "UseCellFieldData";
    }
  else
    {
    return "Default";
    }
}

// Print the vtkAbstractVolumeMapper
void vtkAbstractVolumeMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "ScalarMode: " << this->GetScalarModeAsString() << endl;

  if ( this->ScalarMode == VTK_SCALAR_MODE_USE_POINT_FIELD_DATA ||
       this->ScalarMode == VTK_SCALAR_MODE_USE_CELL_FIELD_DATA )
    {
    if (this->ArrayAccessMode == VTK_GET_ARRAY_BY_ID)
      {
      os << indent << "ArrayId: " << this->ArrayId << endl;
      }
    else
      {
      os << indent << "ArrayName: " << this->ArrayName << endl;
      }
    }
}

