/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFieldDataToAttributeDataFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkFieldDataToAttributeDataFilter.h"

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

  this->Output = NULL;
  
  for (i=0; i<4; i++)
    {
    if ( this->ScalarArrays[i] != NULL )
      delete [] this->ScalarArrays[i];
    }
  
  for (i=0; i<3; i++)
    {
    if ( this->VectorArrays[i] != NULL )
      delete [] this->VectorArrays[i];
    }
  
  for (i=0; i<3; i++)
    {
    if ( this->NormalArrays[i] != NULL )
      delete [] this->NormalArrays[i];
    }
  
  for (i=0; i<3; i++)
    {
    if ( this->TCoordArrays[i] != NULL )
      delete [] this->TCoordArrays[i];
    }
  
  for (i=0; i<9; i++)
    {
    if ( this->TensorArrays[i] != NULL )
      delete [] this->TensorArrays[i];
    }
  
}

// Stuff related to filter interface------------------------------------------
//
void vtkFieldDataToAttributeDataFilter::Execute()
{
  int num;
  vtkDataSetAttributes *attr;
  vtkFieldData *fd;

  vtkDebugMacro(<<"Generating attribute data from field data");

  if ( this->OutputAttributeData == VTK_CELL_DATA )
    {
    attr = ((vtkDataSet *)this->Output)->GetCellData();
    num = ((vtkDataSet *)this->Input)->GetNumberOfCells();
    }
  else
    {
    attr = ((vtkDataSet *)this->Output)->GetPointData();
    num = ((vtkDataSet *)this->Input)->GetNumberOfPoints();
    }
    
  if ( num < 1 )
    {
    vtkErrorMacro(<<"No input points/cells to create attribute data for");
    return;
    }

  fd = NULL;
  if ( this->InputField == VTK_DATA_OBJECT_FIELD )
    {
    fd = this->GetInput()->GetFieldData();
    }
  else if ( this->InputField == VTK_POINT_DATA_FIELD )
    {
    fd = this->GetInput()->GetPointData()->GetFieldData();
    }
  else if ( this->InputField == VTK_CELL_DATA_FIELD )
    {
    fd = this->GetInput()->GetCellData()->GetFieldData();
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
void vtkFieldDataToAttributeDataFilter::SetScalarComponent(int comp, char *arrayName, 
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

int vtkFieldDataToAttributeDataFilter::GetScalarComponentNormailzeFlag(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 3 ? 3 : comp));
  return this->ScalarNormalize[comp];
}

void vtkFieldDataToAttributeDataFilter::ConstructScalars(int num, vtkFieldData *fd, 
                                                         vtkDataSetAttributes *attr,
                                                         int componentRange[4][2],
                                                         char *arrays[4], int arrayComp[4],
                                                         int normalize[4], int numComp)
{
  int i;
  vtkDataArray *fieldArray[4];
  
  if ( numComp < 1 ) return;
  for (i=0; i<numComp; i++) if ( arrays[i] == NULL ) return;
  
  for ( i=0; i < numComp; i++ )
    {
    fieldArray[i] = this->GetFieldArray(fd, arrays[i], arrayComp[i]);

    if ( fieldArray[i] == NULL )
      {
      vtkErrorMacro(<<"Can't find array/component requested");
      return;
      }
    }
  
  for (i=0; i < numComp; i++)
    {
    this->UpdateComponentRange(fieldArray[i], componentRange[i]);
    if ( num != (componentRange[i][1] - componentRange[i][0] + 1) )
      {
      vtkErrorMacro(<<"Number of scalars not consistent");
      return;
      }
    }
  
  vtkScalars *newScalars = vtkScalars::New();
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
  
  attr->SetScalars(newScalars);
  newScalars->Delete();
}

// Stuff related to vectors --------------------------------------------
//
void vtkFieldDataToAttributeDataFilter::SetVectorComponent(int comp, char *arrayName, 
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

int vtkFieldDataToAttributeDataFilter::GetVectorComponentNormailzeFlag(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 2 ? 2 : comp));
  return this->VectorNormalize[comp];
}

void vtkFieldDataToAttributeDataFilter::ConstructVectors(int num, vtkFieldData *fd, 
                                                         vtkDataSetAttributes *attr,
                                                         int componentRange[3][2], 
                                                         char *arrays[3],
                                                         int arrayComp[3], int normalize[3])
{
  int i;
  vtkDataArray *fieldArray[3];

  for (i=0; i<3; i++) if ( arrays[i] == NULL ) return;

  for ( i=0; i < 3; i++ )
    {
    fieldArray[i] = this->GetFieldArray(fd, arrays[i], arrayComp[i]);

    if ( fieldArray[i] == NULL )
      {
      vtkErrorMacro(<<"Can't find array requested");
      return;
      }
    }
  
  this->UpdateComponentRange(fieldArray[0], componentRange[0]);
  this->UpdateComponentRange(fieldArray[1], componentRange[1]);
  this->UpdateComponentRange(fieldArray[2], componentRange[2]);

  if ( num != (componentRange[0][1] - componentRange[0][0] + 1) ||
       num != (componentRange[1][1] - componentRange[1][0] + 1) ||
       num != (componentRange[2][1] - componentRange[2][0] + 1) )
    {
    vtkErrorMacro(<<"Number of vectors not consistent");
    return;
    }
  
  vtkVectors *newVectors = vtkVectors::New();
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
  
  attr->SetVectors(newVectors);
  newVectors->Delete();
}

// Stuff related to normals --------------------------------------------
//
void vtkFieldDataToAttributeDataFilter::SetNormalComponent(int comp, char *arrayName, 
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

int vtkFieldDataToAttributeDataFilter::GetNormalComponentNormailzeFlag(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 2 ? 2 : comp));
  return this->NormalNormalize[comp];
}

void vtkFieldDataToAttributeDataFilter::ConstructNormals(int num, vtkFieldData *fd, 
                                                         vtkDataSetAttributes *attr,
                                                         int componentRange[3][2], 
                                                         char *arrays[3], int arrayComp[3], 
                                                         int normalize[3])
{
  int i;
  vtkDataArray *fieldArray[3];

  for (i=0; i<3; i++) if ( arrays[i] == NULL ) return;
  
  for ( i=0; i < 3; i++ )
    {
    fieldArray[i] = this->GetFieldArray(fd, arrays[i], arrayComp[i]);

    if ( fieldArray[i] == NULL )
      {
      vtkErrorMacro(<<"Can't find array requested");
      return;
      }
    }
  
  this->UpdateComponentRange(fieldArray[0], componentRange[0]);
  this->UpdateComponentRange(fieldArray[1], componentRange[1]);
  this->UpdateComponentRange(fieldArray[2], componentRange[2]);

  if ( num != (componentRange[0][1] - componentRange[0][0] + 1) ||
       num != (componentRange[1][1] - componentRange[1][0] + 1) ||
       num != (componentRange[2][1] - componentRange[2][0] + 1) )
    {
    vtkErrorMacro(<<"Number of normals not consistent");
    return;
    }
  
  vtkNormals *newNormals = vtkNormals::New();
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
  
  attr->SetNormals(newNormals);
  newNormals->Delete();
}

// Stuff related to texture coords --------------------------------------------
//
void vtkFieldDataToAttributeDataFilter::SetTCoordComponent(int comp, char *arrayName, 
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

int vtkFieldDataToAttributeDataFilter::GetTCoordComponentNormailzeFlag(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 2 ? 2 : comp));
  return this->TCoordNormalize[comp];
}

void vtkFieldDataToAttributeDataFilter::ConstructTCoords(int num, vtkFieldData *fd, 
                                                         vtkDataSetAttributes *attr,
                                                         int componentRange[3][2], 
                                                         char *arrays[3], int arrayComp[3], 
                                                         int normalize[3], int numComp)
{
  int i;
  vtkDataArray *fieldArray[3];
  
  if ( numComp < 1 ) return;
  for (i=0; i<numComp; i++) if ( arrays[i] == NULL ) return;
  
  for ( i=0; i < numComp; i++ )
    {
    fieldArray[i] = this->GetFieldArray(fd, arrays[i], arrayComp[i]);

    if ( fieldArray[i] == NULL )
      {
      vtkErrorMacro(<<"Can't find array/component requested");
      return;
      }
    }
  
  for (i=0; i < numComp; i++)
    {
    this->UpdateComponentRange(fieldArray[i], componentRange[i]);
    if ( num != (componentRange[i][1] - componentRange[i][0] + 1) )
      {
      vtkErrorMacro(<<"Number of texture coords not consistent");
      return;
      }
    }
  
  vtkTCoords *newTCoords = vtkTCoords::New();
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
  
  attr->SetTCoords(newTCoords);
  newTCoords->Delete();
}

// Stuff related to tensors --------------------------------------------
//
void vtkFieldDataToAttributeDataFilter::SetTensorComponent(int comp, char *arrayName, 
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

int vtkFieldDataToAttributeDataFilter::GetTensorComponentNormailzeFlag(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 8 ? 8 : comp));
  return this->TensorNormalize[comp];
}

void vtkFieldDataToAttributeDataFilter::ConstructTensors(int num, vtkFieldData *fd, 
                                                         vtkDataSetAttributes *attr,
                                                         int componentRange[9][2], 
                                                         char *arrays[9], int arrayComp[9], 
                                                         int normalize[9])
{
  int i;
  vtkDataArray *fieldArray[9];

  for (i=0; i<9; i++) if ( arrays[i] == NULL ) return;
  
  for ( i=0; i < 9; i++ )
    {
    fieldArray[i] = this->GetFieldArray(fd, arrays[i], arrayComp[i]);

    if ( fieldArray[i] == NULL )
      {
      vtkErrorMacro(<<"Can't find array requested");
      return;
      }
    }
  
  for (i=0; i < 9; i++)
    {
    this->UpdateComponentRange(fieldArray[i], componentRange[i]);
    if ( num != (componentRange[i][1] - componentRange[i][0] + 1) )
      {
      vtkErrorMacro(<<"Number of tensors not consistent");
      return;
      }
    }
  
  vtkTensors *newTensors = vtkTensors::New();
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
  
  attr->SetTensors(newTensors);
  newTensors->Delete();
}

// Stuff related to fields --------------------------------------------
//
void vtkFieldDataToAttributeDataFilter::ConstructFieldData(int num, vtkDataSetAttributes *attr)
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
    if ( compValue < minValue ) minValue = compValue;
    if ( compValue > maxValue ) maxValue = compValue;
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
  vtkDataArray *da;
  int numComp;

  if ( name != NULL )
    {
    if ( (da = fd->GetArray(name)) == NULL ) return NULL;
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

void vtkFieldDataToAttributeDataFilter::SetArrayName(vtkObject *self, char* &name, char *newName)
{
  if ( name && newName && (!strcmp(name,newName))) return;
  if (name) delete [] name;
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

void vtkFieldDataToAttributeDataFilter::UpdateComponentRange(vtkDataArray *da, 
                                                             int compRange[2])
{
  if ( compRange[0] == -1 )
    {
    compRange[0] = 0;
    compRange[1] = da->GetNumberOfTuples() - 1;
    }
}

