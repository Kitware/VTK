/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFieldDataToAttributeDataFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkFieldDataToAttributeDataFilter.h"
#include "vtkObjectFactory.h"

//--------------------------------------------------------------------------
vtkFieldDataToAttributeDataFilter* vtkFieldDataToAttributeDataFilter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkFieldDataToAttributeDataFilter");
  if(ret)
    {
    return (vtkFieldDataToAttributeDataFilter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkFieldDataToAttributeDataFilter;
}

// Instantiate object with no input and no defined output.
vtkFieldDataToAttributeDataFilter::vtkFieldDataToAttributeDataFilter()
{
  int i;
   
  this->InputField = VTK_DATA_OBJECT_FIELD;
  this->OutputAttributeData = VTK_POINT_DATA;
  this->DefaultNormalize = 0;

  this->NumberOfScalarComponents = 0;
  for (i=0; i < 4; i++)
    {
    this->ScalarArrays[i] = NULL;
    this->ScalarArrayComponents[i] = -1; //uninitialized
    this->ScalarComponentRange[i][0] = this->ScalarComponentRange[i][1] = -1;
    this->ScalarNormalize[i] = 1; //yes, normalize
    }
  
  for (i=0; i < 3; i++)
    {
    this->VectorArrays[i] = NULL;
    this->VectorArrayComponents[i] = -1; //uninitialized
    this->VectorComponentRange[i][0] = this->VectorComponentRange[i][1] = -1;
    this->VectorNormalize[i] = 1; //yes, normalize
    }

  for (i=0; i < 3; i++)
    {
    this->NormalArrays[i] = NULL;
    this->NormalArrayComponents[i] = -1; //uninitialized
    this->NormalComponentRange[i][0] = this->NormalComponentRange[i][1] = -1;
    this->NormalNormalize[i] = 1; //yes, normalize
    }

  this->NumberOfTCoordComponents = 0;
  for (i=0; i < 3; i++)
    {
    this->TCoordArrays[i] = NULL;
    this->TCoordArrayComponents[i] = -1; //uninitialized
    this->TCoordComponentRange[i][0] = this->TCoordComponentRange[i][1] = -1;
    this->TCoordNormalize[i] = 1; //yes, normalize
    }

  for (i=0; i < 9; i++)
    {
    this->TensorArrays[i] = NULL;
    this->TensorArrayComponents[i] = -1; //uninitialized
    this->TensorComponentRange[i][0] = this->TensorComponentRange[i][1] = -1;
    this->TensorNormalize[i] = 1; //yes, normalize
    }
}

vtkFieldDataToAttributeDataFilter::~vtkFieldDataToAttributeDataFilter()
{
  int i;

  for (i=0; i<4; i++)
    {
    if ( this->ScalarArrays[i] != NULL )
      {
      delete [] this->ScalarArrays[i];
      }
    }
  
  for (i=0; i<3; i++)
    {
    if ( this->VectorArrays[i] != NULL )
      {
      delete [] this->VectorArrays[i];
      }
    }
  
  for (i=0; i<3; i++)
    {
    if ( this->NormalArrays[i] != NULL )
      {
      delete [] this->NormalArrays[i];
      }
    }
  
  for (i=0; i<3; i++)
    {
    if ( this->TCoordArrays[i] != NULL )
      {
      delete [] this->TCoordArrays[i];
      }
    }
  
  for (i=0; i<9; i++)
    {
    if ( this->TensorArrays[i] != NULL )
      {
      delete [] this->TensorArrays[i];
      }
    }
  
}

// Stuff related to filter interface------------------------------------------
//
void vtkFieldDataToAttributeDataFilter::Execute()
{

  int num;
  vtkDataSetAttributes *attr;
  vtkFieldData *fd;
  vtkDataSet *input = this->GetInput();
  vtkDataSet *output = this->GetOutput();

  vtkDebugMacro(<<"Generating attribute data from field data");

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );

  // Pass here so that the attributes/fields can be over-written later
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());

  if ( this->OutputAttributeData == VTK_CELL_DATA )
    {
    attr = output->GetCellData();
    num = input->GetNumberOfCells();
    }
  else
    {
    attr = output->GetPointData();
    num = input->GetNumberOfPoints();
    }
    
  if ( num < 1 )
    {
    vtkErrorMacro(<<"No input points/cells to create attribute data for");
    return;
    }

  fd = NULL;
  if ( this->InputField == VTK_DATA_OBJECT_FIELD )
    {
    fd = input->GetFieldData();
    }
  else if ( this->InputField == VTK_POINT_DATA_FIELD )
    {
    fd = input->GetPointData();
    }
  else if ( this->InputField == VTK_CELL_DATA_FIELD )
    {
    fd = input->GetCellData();
    }
  if ( fd == NULL )
    {
    vtkErrorMacro(<<"No field data available");
    return;
    }

  this->ConstructScalars(num, fd, attr, this->ScalarComponentRange,
                         this->ScalarArrays, 
                         this->ScalarArrayComponents, 
                         this->ScalarNormalize, this->NumberOfScalarComponents);
  this->ConstructVectors(num, fd, attr, this->VectorComponentRange,
                         this->VectorArrays, 
                         this->VectorArrayComponents, 
                         this->VectorNormalize);
  this->ConstructTensors(num, fd, attr, this->TensorComponentRange,
                         this->TensorArrays, 
                         this->TensorArrayComponents, 
                         this->TensorNormalize);
  this->ConstructTCoords(num, fd, attr, this->TCoordComponentRange,
                         this->TCoordArrays, 
                         this->TCoordArrayComponents, 
                         this->TCoordNormalize, this->NumberOfTCoordComponents);
  this->ConstructNormals(num, fd, attr, this->NormalComponentRange,
                         this->NormalArrays, 
                         this->NormalArrayComponents, 
                         this->NormalNormalize);
  this->ConstructFieldData(num, attr);
  
}

void vtkFieldDataToAttributeDataFilter::PrintSelf(ostream& os, 
                                                  vtkIndent indent)
{
  vtkDataSetToDataSetFilter::PrintSelf(os,indent);

  os << indent << "Input Field: ";
  if ( this->InputField == VTK_DATA_OBJECT_FIELD )
    {
    os << "DataObjectField\n";
    }
  else if ( this->InputField == VTK_POINT_DATA_FIELD )
    {
    os << "PointDataField\n";
    }
  else //if ( this->InputField == VTK_CELL_DATA_FIELD )
    {
    os << "CellDataField\n";
    }

  os << indent << "Default Normalize: " 
     << (this->DefaultNormalize ? "On\n" : "Off\n");

  os << indent << "Output Attribute Data: ";
  if ( this->OutputAttributeData == VTK_CELL_DATA )
    {
    os << "CellData\n";
    }
  else //if ( this->OutputAttributeData == VTK_POINT_DATA )
    {
    os << "PointData\n";
    }
}

// Stuff related to scalars --------------------------------------------
//
void vtkFieldDataToAttributeDataFilter::SetScalarComponent(int comp, const char *arrayName, 
                              int arrayComp, int min, int max, int normalize)
{
  if ( comp < 0 || comp > 3 )
    {
    vtkErrorMacro(<<"Scalar component must be between (0,3)");
    return;
    }
  
  if ( comp >= this->NumberOfScalarComponents ) 
    {
    this->NumberOfScalarComponents = comp + 1;
    }
  this->SetArrayName(this, this->ScalarArrays[comp], arrayName);
  if ( this->ScalarArrayComponents[comp] != arrayComp )
    {
    this->ScalarArrayComponents[comp] = arrayComp;
    this->Modified();
    }
  if ( this->ScalarComponentRange[comp][0] != min )
    {
    this->ScalarComponentRange[comp][0] = min;
    this->Modified();
    }
  if ( this->ScalarComponentRange[comp][1] != max )
    {
    this->ScalarComponentRange[comp][1] = max;
    this->Modified();
    }
  if ( this->ScalarNormalize[comp] != normalize )
    {
    this->ScalarNormalize[comp] = normalize;  
    this->Modified();
    }
}

const char *vtkFieldDataToAttributeDataFilter::GetScalarComponentArrayName(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 3 ? 3 : comp));
  return this->ScalarArrays[comp];
}

int vtkFieldDataToAttributeDataFilter::GetScalarComponentArrayComponent(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 3 ? 3 : comp));
  return this->ScalarArrayComponents[comp];
}

int vtkFieldDataToAttributeDataFilter::GetScalarComponentMinRange(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 3 ? 3 : comp));
  return this->ScalarComponentRange[comp][0];
}

int vtkFieldDataToAttributeDataFilter::GetScalarComponentMaxRange(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 3 ? 3 : comp));
  return this->ScalarComponentRange[comp][1];
}

int vtkFieldDataToAttributeDataFilter::GetScalarComponentNormalizeFlag(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 3 ? 3 : comp));
  return this->ScalarNormalize[comp];
}

void vtkFieldDataToAttributeDataFilter::ConstructScalars(int num, vtkFieldData *fd, 
                                                         vtkDataSetAttributes *attr,
                                                         vtkIdType componentRange[4][2],
                                                         char *arrays[4], int arrayComp[4],
                                                         int normalize[4], int numComp)
{
  int i, normalizeAny, updated=0;
  vtkDataArray *fieldArray[4];
  
  if ( numComp < 1 )
    {
    return;
    }
  for (i=0; i<numComp; i++)
    {
    if ( arrays[i] == NULL )
      {
      return;
      }
    }
  
  for ( i=0; i < numComp; i++ )
    {
    fieldArray[i] = this->GetFieldArray(fd, arrays[i], arrayComp[i]);

    if ( fieldArray[i] == NULL )
      {
      vtkErrorMacro(<<"Can't find array/component requested");
      return;
      }
    }
  
  for (normalizeAny=i=0; i < numComp; i++)
    {
    updated |= this->UpdateComponentRange(fieldArray[i], componentRange[i]);
    if ( num != (componentRange[i][1] - componentRange[i][0] + 1) )
      {
      vtkErrorMacro(<<"Number of scalars not consistent");
      return;
      }
    normalizeAny |= normalize[i];
    }
  
  vtkScalars *newScalars = vtkScalars::New();
  for (i=1; i < numComp; i++) //see whether all the data is from the same array
    {
    if ( fieldArray[i] != fieldArray[i-1] )
      {
      break;
      }
    }
  
  // see whether we can reuse the data array from the field
  if ( i >= numComp && fieldArray[0]->GetNumberOfComponents() == numComp && 
       fieldArray[0]->GetNumberOfTuples() == num && !normalizeAny )
    {
    newScalars->SetData(fieldArray[0]);
    }
  else //have to copy data into created array
    {
    newScalars->SetNumberOfComponents(numComp);
    newScalars->SetDataType(this->GetComponentsType(numComp, fieldArray));
    newScalars->SetNumberOfScalars(num);
  
    for ( i=0; i < numComp; i++ )
      {
      if ( this->ConstructArray(newScalars->GetData(), i, fieldArray[i], arrayComp[i],
                                componentRange[i][0], componentRange[i][1],
                                normalize[i]) == 0 )
        {
        newScalars->Delete();
        return;
        }
      }
    }
  
  attr->SetScalars(newScalars);
  newScalars->Delete();
  if ( updated ) //reset for next execution pass
    {
    for (i=0; i < numComp; i++)
      {
      componentRange[i][0] = componentRange[i][1] = -1;
      }
    }
}

// Stuff related to vectors --------------------------------------------
//
void vtkFieldDataToAttributeDataFilter::SetVectorComponent(int comp, const char *arrayName, 
                              int arrayComp, int min, int max, int normalize)
{
  if ( comp < 0 || comp > 2 )
    {
    vtkErrorMacro(<<"Vector component must be between (0,2)");
    return;
    }
  
  this->SetArrayName(this, this->VectorArrays[comp], arrayName);
  if ( this->VectorArrayComponents[comp] != arrayComp )
    {
    this->VectorArrayComponents[comp] = arrayComp;
    this->Modified();
    }
  if ( this->VectorComponentRange[comp][0] != min )
    {
    this->VectorComponentRange[comp][0] = min;
    this->Modified();
    }
  if ( this->VectorComponentRange[comp][1] != max )
    {
    this->VectorComponentRange[comp][1] = max;
    this->Modified();
    }
  if ( this->VectorNormalize[comp] != normalize )
    {
    this->VectorNormalize[comp] = normalize;  
    this->Modified();
    }
}

const char *vtkFieldDataToAttributeDataFilter::GetVectorComponentArrayName(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 2 ? 2 : comp));
  return this->VectorArrays[comp];
}

int vtkFieldDataToAttributeDataFilter::GetVectorComponentArrayComponent(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 2 ? 2 : comp));
  return this->VectorArrayComponents[comp];
}

int vtkFieldDataToAttributeDataFilter::GetVectorComponentMinRange(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 2 ? 2 : comp));
  return this->VectorComponentRange[comp][0];
}

int vtkFieldDataToAttributeDataFilter::GetVectorComponentMaxRange(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 2 ? 2 : comp));
  return this->VectorComponentRange[comp][1];
}

int vtkFieldDataToAttributeDataFilter::GetVectorComponentNormalizeFlag(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 2 ? 2 : comp));
  return this->VectorNormalize[comp];
}

void vtkFieldDataToAttributeDataFilter::ConstructVectors(int num, vtkFieldData *fd, 
                                                         vtkDataSetAttributes *attr,
                                                         vtkIdType componentRange[3][2], 
                                                         char *arrays[3],
                                                         int arrayComp[3], int normalize[3])
{
  int i, updated;
  vtkDataArray *fieldArray[3];

  for (i=0; i<3; i++)
    {
    if ( arrays[i] == NULL )
      {
      return;
      }
    }

  for ( i=0; i < 3; i++ )
    {
    fieldArray[i] = this->GetFieldArray(fd, arrays[i], arrayComp[i]);

    if ( fieldArray[i] == NULL )
      {
      vtkErrorMacro(<<"Can't find array requested");
      return;
      }
    }
  
  updated = this->UpdateComponentRange(fieldArray[0], componentRange[0]);
  updated |= this->UpdateComponentRange(fieldArray[1], componentRange[1]);
  updated |= this->UpdateComponentRange(fieldArray[2], componentRange[2]);

  if ( num != (componentRange[0][1] - componentRange[0][0] + 1) ||
       num != (componentRange[1][1] - componentRange[1][0] + 1) ||
       num != (componentRange[2][1] - componentRange[2][0] + 1) )
    {
    vtkErrorMacro(<<"Number of vectors not consistent");
    return;
    }
  
  vtkVectors *newVectors = vtkVectors::New();
  if ( fieldArray[0]->GetNumberOfComponents() == 3 && 
       fieldArray[0] == fieldArray[1] && fieldArray[1] == fieldArray[2] &&
       fieldArray[0]->GetNumberOfTuples() == num &&
       !normalize[0] && !normalize[1] && !normalize[2] )
    {
    newVectors->SetData(fieldArray[0]);
    }
  else //have to copy data into created array
    {
    newVectors->SetDataType(this->GetComponentsType(3, fieldArray));
    newVectors->SetNumberOfVectors(num);

    for ( i=0; i < 3; i++ )
      {
      if ( this->ConstructArray(newVectors->GetData(), i, fieldArray[i], arrayComp[i],
                                componentRange[i][0], componentRange[i][1],
                                normalize[i]) == 0 )
        {
        newVectors->Delete();
        return;
        }
      }
    }
  
  attr->SetVectors(newVectors);
  newVectors->Delete();
  if ( updated ) //reset for next execution pass
    {
    for (i=0; i < 3; i++)
      {
      componentRange[i][0] = componentRange[i][1] = -1;
      }
    }
}


// Stuff related to normals --------------------------------------------
//
void vtkFieldDataToAttributeDataFilter::SetNormalComponent(int comp, const char *arrayName, 
                              int arrayComp, int min, int max, int normalize)
{
  if ( comp < 0 || comp > 2 )
    {
    vtkErrorMacro(<<"Normal component must be between (0,2)");
    return;
    }
  
  this->SetArrayName(this, this->NormalArrays[comp], arrayName);
  if ( this->NormalArrayComponents[comp] != arrayComp )
    {
    this->NormalArrayComponents[comp] = arrayComp;
    this->Modified();
    }
  if ( this->NormalComponentRange[comp][0] != min )
    {
    this->NormalComponentRange[comp][0] = min;
    this->Modified();
    }
  if ( this->NormalComponentRange[comp][1] != max )
    {
    this->NormalComponentRange[comp][1] = max;
    this->Modified();
    }
  if ( this->NormalNormalize[comp] != normalize )
    {
    this->NormalNormalize[comp] = normalize;  
    this->Modified();
    }
}

const char *vtkFieldDataToAttributeDataFilter::GetNormalComponentArrayName(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 2 ? 2 : comp));
  return this->NormalArrays[comp];
}

int vtkFieldDataToAttributeDataFilter::GetNormalComponentArrayComponent(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 2 ? 2 : comp));
  return this->NormalArrayComponents[comp];
}

int vtkFieldDataToAttributeDataFilter::GetNormalComponentMinRange(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 2 ? 2 : comp));
  return this->NormalComponentRange[comp][0];
}

int vtkFieldDataToAttributeDataFilter::GetNormalComponentMaxRange(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 2 ? 2 : comp));
  return this->NormalComponentRange[comp][1];
}

int vtkFieldDataToAttributeDataFilter::GetNormalComponentNormalizeFlag(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 2 ? 2 : comp));
  return this->NormalNormalize[comp];
}

void vtkFieldDataToAttributeDataFilter::ConstructNormals(int num, vtkFieldData *fd, 
                                                         vtkDataSetAttributes *attr,
                                                         vtkIdType componentRange[3][2], 
                                                         char *arrays[3], int arrayComp[3], 
                                                         int normalize[3])
{
  int i, updated;
  vtkDataArray *fieldArray[3];

  for (i=0; i<3; i++)
    {
    if ( arrays[i] == NULL )
      {
      return;
      }
    }
  
  for ( i=0; i < 3; i++ )
    {
    fieldArray[i] = this->GetFieldArray(fd, arrays[i], arrayComp[i]);

    if ( fieldArray[i] == NULL )
      {
      vtkErrorMacro(<<"Can't find array requested");
      return;
      }
    }
  
  updated = this->UpdateComponentRange(fieldArray[0], componentRange[0]);
  updated |= this->UpdateComponentRange(fieldArray[1], componentRange[1]);
  updated |= this->UpdateComponentRange(fieldArray[2], componentRange[2]);

  if ( num != (componentRange[0][1] - componentRange[0][0] + 1) ||
       num != (componentRange[1][1] - componentRange[1][0] + 1) ||
       num != (componentRange[2][1] - componentRange[2][0] + 1) )
    {
    vtkErrorMacro(<<"Number of normals not consistent");
    return;
    }
  
  vtkNormals *newNormals = vtkNormals::New();
  if ( fieldArray[0]->GetNumberOfComponents() == 3 && 
       fieldArray[0] == fieldArray[1] && fieldArray[1] == fieldArray[2] &&
       fieldArray[0]->GetNumberOfTuples() == num &&
       !normalize[0] && !normalize[1] && !normalize[2] )
    {
    newNormals->SetData(fieldArray[0]);
    }
  else //have to copy data into created array
    {
    newNormals->SetDataType(this->GetComponentsType(3, fieldArray));
    newNormals->SetNumberOfNormals(num);

    for ( i=0; i < 3; i++ )
      {
      if ( this->ConstructArray(newNormals->GetData(), i, fieldArray[i], arrayComp[i],
                                componentRange[i][0], componentRange[i][1],
                                normalize[i]) == 0 )
        {
        newNormals->Delete();
        return;
        }
      }
    }
  
  attr->SetNormals(newNormals);
  newNormals->Delete();
  if ( updated ) //reset for next execution pass
    {
    for (i=0; i < 3; i++)
      {
      componentRange[i][0] = componentRange[i][1] = -1;
      }
    }
}

// Stuff related to texture coords --------------------------------------------
//
void vtkFieldDataToAttributeDataFilter::SetTCoordComponent(int comp, const char *arrayName, 
                              int arrayComp, int min, int max, int normalize)
{
  if ( comp < 0 || comp > 2 )
    {
    vtkErrorMacro(<<"TCoord component must be between (0,2)");
    return;
    }
  
  if ( comp >= this->NumberOfTCoordComponents ) 
    {
    this->NumberOfTCoordComponents = comp + 1;
    }
  this->SetArrayName(this, this->TCoordArrays[comp], arrayName);
  if ( this->TCoordArrayComponents[comp] != arrayComp )
    {
    this->TCoordArrayComponents[comp] = arrayComp;
    this->Modified();
    }
  if ( this->TCoordComponentRange[comp][0] != min )
    {
    this->TCoordComponentRange[comp][0] = min;
    this->Modified();
    }
  if ( this->TCoordComponentRange[comp][1] != max )
    {
    this->TCoordComponentRange[comp][1] = max;
    this->Modified();
    }
  if ( this->TCoordNormalize[comp] != normalize )
    {
    this->TCoordNormalize[comp] = normalize;  
    this->Modified();
    }
}

const char *vtkFieldDataToAttributeDataFilter::GetTCoordComponentArrayName(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 2 ? 2 : comp));
  return this->TCoordArrays[comp];
}

int vtkFieldDataToAttributeDataFilter::GetTCoordComponentArrayComponent(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 2 ? 2 : comp));
  return this->TCoordArrayComponents[comp];
}

int vtkFieldDataToAttributeDataFilter::GetTCoordComponentMinRange(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 2 ? 2 : comp));
  return this->TCoordComponentRange[comp][0];
}

int vtkFieldDataToAttributeDataFilter::GetTCoordComponentMaxRange(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 2 ? 2 : comp));
  return this->TCoordComponentRange[comp][1];
}

int vtkFieldDataToAttributeDataFilter::GetTCoordComponentNormalizeFlag(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 2 ? 2 : comp));
  return this->TCoordNormalize[comp];
}

void vtkFieldDataToAttributeDataFilter::ConstructTCoords(int num, vtkFieldData *fd, 
                                                         vtkDataSetAttributes *attr,
                                                         vtkIdType componentRange[3][2], 
                                                         char *arrays[3], int arrayComp[3], 
                                                         int normalize[3], int numComp)
{
  int i, normalizeAny, updated=0;
  vtkDataArray *fieldArray[3];
  
  if ( numComp < 1 )
    {
    return;
    }
  for (i=0; i<numComp; i++)
    {
    if ( arrays[i] == NULL )
      {
      return;
      }
    }
  
  for ( normalizeAny=i=0; i < numComp; i++ )
    {
    fieldArray[i] = this->GetFieldArray(fd, arrays[i], arrayComp[i]);

    if ( fieldArray[i] == NULL )
      {
      vtkErrorMacro(<<"Can't find array/component requested");
      return;
      }
    normalizeAny |= normalize[i];
    }
  
  for (i=0; i < numComp; i++)
    {
    updated |= this->UpdateComponentRange(fieldArray[i], componentRange[i]);
    if ( num != (componentRange[i][1] - componentRange[i][0] + 1) )
      {
      vtkErrorMacro(<<"Number of texture coords not consistent");
      return;
      }
    }
  
  vtkTCoords *newTCoords = vtkTCoords::New();
  for (i=1; i < numComp; i++) //see whether all the data is from the same array
    {
    if ( fieldArray[i] != fieldArray[i-1] )
      {
      break;
      }
    }
  
  // see whether we can reuse the data array from the field
  if ( i >= numComp && fieldArray[0]->GetNumberOfComponents() == numComp && 
       fieldArray[0]->GetNumberOfTuples() == num && !normalizeAny )
    {
    newTCoords->SetData(fieldArray[0]);
    }
  else //have to copy data into created array
    {
    newTCoords->SetNumberOfComponents(numComp);
    newTCoords->SetDataType(this->GetComponentsType(numComp, fieldArray));
    newTCoords->SetNumberOfTCoords(num);

    for ( i=0; i < numComp; i++ )
      {
      if ( this->ConstructArray(newTCoords->GetData(), i, fieldArray[i], arrayComp[i],
                                componentRange[i][0], componentRange[i][1],
                                normalize[i]) == 0 )
        {
        newTCoords->Delete();
        return;
        }
      }
    }
  
  attr->SetTCoords(newTCoords);
  newTCoords->Delete();
  if ( updated ) //reset for next execution pass
    {
    for (i=0; i < numComp; i++)
      {
      componentRange[i][0] = componentRange[i][1] = -1;
      }
    }
}

// Stuff related to tensors --------------------------------------------
//
void vtkFieldDataToAttributeDataFilter::SetTensorComponent(int comp, const char *arrayName, 
                              int arrayComp, int min, int max, int normalize)
{
  if ( comp < 0 || comp > 8 )
    {
    vtkErrorMacro(<<"Tensor component must be between (0,8)");
    return;
    }
  
  this->SetArrayName(this, this->TensorArrays[comp], arrayName);
  if ( this->TensorArrayComponents[comp] != arrayComp )
    {
    this->TensorArrayComponents[comp] = arrayComp;
    this->Modified();
    }
  if ( this->TensorComponentRange[comp][0] != min )
    {
    this->TensorComponentRange[comp][0] = min;
    this->Modified();
    }
  if ( this->TensorComponentRange[comp][1] != max )
    {
    this->TensorComponentRange[comp][1] = max;
    this->Modified();
    }
  if ( this->TensorNormalize[comp] != normalize )
    {
    this->TensorNormalize[comp] = normalize;  
    this->Modified();
    }
}

const char *vtkFieldDataToAttributeDataFilter::GetTensorComponentArrayName(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 8 ? 8 : comp));
  return this->TensorArrays[comp];
}

int vtkFieldDataToAttributeDataFilter::GetTensorComponentArrayComponent(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 8 ? 8 : comp));
  return this->TensorArrayComponents[comp];
}

int vtkFieldDataToAttributeDataFilter::GetTensorComponentMinRange(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 8 ? 8 : comp));
  return this->TensorComponentRange[comp][0];
}

int vtkFieldDataToAttributeDataFilter::GetTensorComponentMaxRange(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 8 ? 8 : comp));
  return this->TensorComponentRange[comp][1];
}

int vtkFieldDataToAttributeDataFilter::GetTensorComponentNormalizeFlag(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 8 ? 8 : comp));
  return this->TensorNormalize[comp];
}

void vtkFieldDataToAttributeDataFilter::ConstructTensors(int num, vtkFieldData *fd, 
                                                         vtkDataSetAttributes *attr,
                                                         vtkIdType componentRange[9][2], 
                                                         char *arrays[9], int arrayComp[9], 
                                                         int normalize[9])
{
  int i, normalizeAny, updated=0;
  vtkDataArray *fieldArray[9];

  for (i=0; i<9; i++)
    {
    if ( arrays[i] == NULL )
      {
      return;
      }
    }
  
  for ( i=0; i < 9; i++ )
    {
    fieldArray[i] = this->GetFieldArray(fd, arrays[i], arrayComp[i]);

    if ( fieldArray[i] == NULL )
      {
      vtkErrorMacro(<<"Can't find array requested");
      return;
      }
    }
  
  for (normalizeAny=i=0; i < 9; i++)
    {
    updated |= this->UpdateComponentRange(fieldArray[i], componentRange[i]);
    if ( num != (componentRange[i][1] - componentRange[i][0] + 1) )
      {
      vtkErrorMacro(<<"Number of tensors not consistent");
      return;
      }
    normalizeAny |= normalize[i];
    }
  
  vtkTensors *newTensors = vtkTensors::New();
  for (i=1; i < 9; i++) //see whether all the data is from the same array
    {
    if ( fieldArray[i] != fieldArray[i-1] )
      {
      break;
      }
    }
  
  // see whether we can reuse the data array from the field
  if ( i >= 9 && fieldArray[0]->GetNumberOfComponents() == 9 && 
       fieldArray[0]->GetNumberOfTuples() == num && !normalizeAny )
    {
    newTensors->SetData(fieldArray[0]);
    }
  else //have to copy data into created array
    {
    newTensors->SetDataType(this->GetComponentsType(9, fieldArray));
    newTensors->SetNumberOfTensors(num);

    for ( i=0; i < 9; i++ )
      {
      if ( this->ConstructArray(newTensors->GetData(), i, fieldArray[i], arrayComp[i],
                                componentRange[i][0], componentRange[i][1],
                                normalize[i]) == 0 )
        {
        newTensors->Delete();
        return;
        }
      }
    }
  
  attr->SetTensors(newTensors);
  newTensors->Delete();
  if ( updated ) //reset for next execution pass
    {
    for (i=0; i < 9; i++)
      {
      componentRange[i][0] = componentRange[i][1] = -1;
      }
    }
}

// Stuff related to fields --------------------------------------------
//
void
vtkFieldDataToAttributeDataFilter::ConstructFieldData(int vtkNotUsed(num),
                                      vtkDataSetAttributes *vtkNotUsed(attr))
{
}

// Stuff related to helper methods ---------------------------------------
//
int vtkFieldDataToAttributeDataFilter::ConstructArray(vtkDataArray *da, int comp, 
                                          vtkDataArray *fieldArray, int fieldComp,
                                          int min, int max, int normalize)
{
  int i, n=max-min+1;
  float minValue=VTK_LARGE_FLOAT;
  float maxValue= -VTK_LARGE_FLOAT;
  float compRange, compValue;

  if ( fieldComp >= fieldArray->GetNumberOfComponents() )
    {
    vtkGenericWarningMacro(<<"Trying to access component out of range");
    return 0;
    }

  for (i=0; i < n; i++)
    {
    compValue = fieldArray->GetComponent(min+i, fieldComp);
    if ( compValue < minValue )
      {
      minValue = compValue;
      }
    if ( compValue > maxValue )
      {
      maxValue = compValue;
      }
    da->SetComponent(i, comp, compValue);
    }
  
  if ( normalize )
    {
    compRange = maxValue - minValue;
    if ( compRange != 0.0 )
      {
      for (i=0; i < n; i++)
        {
        compValue = da->GetComponent(i, comp);
        compValue = (compValue - minValue) / compRange;
        da->SetComponent(i, comp, compValue);
        }
      }
    }

  return 1;
}

int vtkFieldDataToAttributeDataFilter::GetComponentsType(int numComp, vtkDataArray **arrays)
{
  int type, mostComplexType=VTK_VOID;

  for (int i=0; i < numComp; i++)
    {
    type = arrays[i]->GetDataType();
    if ( type > mostComplexType ) 
      {
      mostComplexType = type;
      }
    }

  return mostComplexType;
}

vtkDataArray *vtkFieldDataToAttributeDataFilter::GetFieldArray(vtkFieldData *fd, 
                                                               char *name, int comp)
{
  vtkDataArray *da = NULL;
  int numComp;
  int found=0;

  if ( name != NULL )
    {
    vtkDataSetAttributes* dsa;
    if ((dsa=vtkDataSetAttributes::SafeDownCast(fd)))
      {
      found=1;
      if(!strcmp("PointScalars", name) || !strcmp("CellScalars", name))
	{
	da = dsa->GetActiveScalars();
	}
      else if(!strcmp("PointVectors", name) || !strcmp("CellVectors", name))
	{
	da = dsa->GetActiveVectors();
	}
      else if(!strcmp("PointTensors", name) || !strcmp("CellTensors", name))
	{
	da = dsa->GetActiveTensors();
	}
      else if(!strcmp("PointNormals", name) || !strcmp("CellNormals", name))
	{
	da = dsa->GetActiveNormals();
	}
      else if(!strcmp("PointTCoords", name) || !strcmp("CellTCoords", name))
	{
	da = dsa->GetActiveTCoords();
	}
      else
	{
	found=0;
	}
      }
    if (!found || !da)
      {
      da = fd->GetArray(name);
      }

    if ( da == NULL )
      {
      return NULL;
      }
    numComp = da->GetNumberOfComponents();
    if ( comp < 0 || comp >= numComp )
      {
      return NULL;
      }
    else
      {
      return da;
      }
    }

  return NULL;
}

void vtkFieldDataToAttributeDataFilter::SetArrayName(vtkObject *self, char* &name, const char *newName)
{
  if ( name && newName && (!strcmp(name,newName)))
    {
    return;
    }
  if (name)
    {
    delete [] name;
    } 
  if (newName)
    {
    name = new char[strlen(newName)+1];
    strcpy(name,newName);
    }
   else
    {
    name = NULL;
    }
  self->Modified();
}

int vtkFieldDataToAttributeDataFilter::UpdateComponentRange(vtkDataArray *da, 
                                                            vtkIdType compRange[2])
{
  if ( compRange[0] == -1 )
    {
    compRange[0] = 0;
    compRange[1] = da->GetNumberOfTuples() - 1;
    return 1;
    }
  else
    {
    return 0;
    }
}

