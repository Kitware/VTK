/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuadraturePointsGenerator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkQuadraturePointsGenerator.h"

#include "vtkUnstructuredGrid.h"
#include "vtkPolyData.h"
#include "vtkPoints.h"
#include "vtkType.h"
#include "vtkDoubleArray.h"
#include "vtkDataArray.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkCellArray.h"
#include "vtkIdTypeArray.h"
#include "vtkIntArray.h"
#include "vtkUnstructuredGridAlgorithm.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkInformationQuadratureSchemeDefinitionVectorKey.h"
#include "vtkObjectFactory.h"
#include "vtkDataSetAttributes.h"

#include "vtksys/ios/sstream"
using vtksys_ios::ostringstream;

#include "vtkQuadratureSchemeDefinition.h"
#include "vtkQuadraturePointsUtilities.hxx"



vtkCxxRevisionMacro(vtkQuadraturePointsGenerator, "1.11");
vtkStandardNewMacro(vtkQuadraturePointsGenerator);

//-----------------------------------------------------------------------------
vtkQuadraturePointsGenerator::vtkQuadraturePointsGenerator()
{
  this->SourceArrayName=0;
  this->HasSourceArrayName=0;
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//-----------------------------------------------------------------------------
vtkQuadraturePointsGenerator::~vtkQuadraturePointsGenerator()
{
  this->Clear();
}

//-----------------------------------------------------------------------------
int vtkQuadraturePointsGenerator::FillInputPortInformation(
        int port,
        vtkInformation *info)
{
  switch (port)
    {
    case 0:
      info->Set(vtkDataObject::DATA_TYPE_NAME(),"vtkUnstructuredGrid");
      break;
    }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkQuadraturePointsGenerator::FillOutputPortInformation(
        int port,
        vtkInformation *info)
{
  switch (port)
    {
    case 0:
      info->Set(vtkDataObject::DATA_TYPE_NAME(),"vtkPolyData");
      break;
    }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkQuadraturePointsGenerator::RequestData(
        vtkInformation *,
        vtkInformationVector **input,
        vtkInformationVector *output)
{
  vtkDataObject *tmpDataObj;
  // Get the input.
  tmpDataObj
    = input[0]->GetInformationObject(0)->Get(vtkDataObject::DATA_OBJECT());
  vtkUnstructuredGrid *usgIn
    = vtkUnstructuredGrid::SafeDownCast(tmpDataObj);
  // Get the output.
  tmpDataObj
    = output->GetInformationObject(0)->Get(vtkDataObject::DATA_OBJECT());
  vtkPolyData *pdOut
    = vtkPolyData::SafeDownCast(tmpDataObj);

  // Quick sanity check.
  if (usgIn==NULL || pdOut==NULL
     || usgIn->GetNumberOfCells()==0
     || usgIn->GetNumberOfPoints()==0
     || usgIn->GetPointData()==NULL
     || usgIn->GetPointData()->GetNumberOfArrays()==0)
    {
    vtkWarningMacro("Filter data has not been configured correctly. Aborting.");
    return 1;
    }

  // If we don't already have an array set, then
  // paraview may be trying to set one via vtkAlgorithm
  if (!this->HasSourceArrayName)
    {
    this->GetSourceArrayNameFromAlgorithm(input);
    }

  // Generate points for the selected data array.
  this->Generate(usgIn,this->SourceArrayName,pdOut);

  return 1;
}

//-----------------------------------------------------------------------------
void vtkQuadraturePointsGenerator::SetSourceArrayName(const char* _arg)
{
  if ( this->SourceArrayName == NULL && _arg == NULL) { return;} 
  if ( this->SourceArrayName && _arg && (!strcmp(this->SourceArrayName,_arg))) { return; }
  if (this->SourceArrayName) { delete [] this->SourceArrayName; } 
  if (_arg) 
    {
    size_t n = strlen(_arg) + 1;
    char *cp1 =  new char[n];
    const char *cp2 = (_arg);
    this->SourceArrayName = cp1; 
    do { *cp1++ = *cp2++; } while ( --n );
    }
   else
    {
    this->SourceArrayName = NULL;
    }
  this->Modified();
  this->HasSourceArrayName=1;
}


//-----------------------------------------------------------------------------
int vtkQuadraturePointsGenerator::GetSourceArrayNameFromAlgorithm(
        vtkInformationVector **inputVector)
{
  vtkDataArray *da = this->GetInputArrayToProcess(0,inputVector);
  if (da!=NULL)
    {
    char *name=da->GetName();
    int n=static_cast<int>(strlen(name));
    if (this->SourceArrayName!=NULL)
      {
      delete this->SourceArrayName;
      }
    this->SourceArrayName=new char [n+1];
    for (int i=0; i<n; ++i)
      {
      this->SourceArrayName[i]=name[i];
      }
    this->SourceArrayName[n]='\0';
    }
  else
    {
    vtkWarningMacro("Could not get array name.");
    }
  return 1;
}

//-----------------------------------------------------------------------------
void vtkQuadraturePointsGenerator::Clear()
{
  this->SetSourceArrayName(0);
  this->HasSourceArrayName=0;
}

//-----------------------------------------------------------------------------
int vtkQuadraturePointsGenerator::Generate(
        vtkUnstructuredGrid *usgIn,
        char *sourceArrayName,
        vtkPolyData *pdOut)
{
  // Grab the interpolated data from FieldData, and set it
  // as our point data.
  ostringstream interpolatedName;
  interpolatedName << sourceArrayName << "_QP_Interpolated";
  vtkDoubleArray *V_int
   = vtkDoubleArray::SafeDownCast(usgIn->GetFieldData()->GetArray(interpolatedName.str().c_str()));
  if (V_int==0)
    {
    // This is not requisite data, but we want to warn
    vtkWarningMacro("Could not access array:" 
                      << interpolatedName.str() << ". "
                      << "Points will be generated with out point data.");
    }
  else
    {
    pdOut->GetPointData()->AddArray(V_int);
    }

  // Extract info we need for all cells.
  vtkIdType nCells=usgIn->GetNumberOfCells();
  // Get the dictionary associated with this array.We are going 
  // to make a copy for efficiency.
  vtkDataArray *V=usgIn->GetPointData()->GetArray(this->SourceArrayName);
  if (V==NULL)
    {
    vtkWarningMacro("Could not access source array:" << this->SourceArrayName
                    << "Aborting.");
    return 0;
    }
  vtkInformation *info=V->GetInformation();
  vtkInformationQuadratureSchemeDefinitionVectorKey *key=vtkQuadratureSchemeDefinition::DICTIONARY();
  if (!key->Has(info))
    {
    vtkWarningMacro("Dictionary is not present. Aborting.");
    return 0;
    }
  int dictSize= key->Size(info);
  vtkQuadratureSchemeDefinition **dict=new vtkQuadratureSchemeDefinition *[dictSize];
  key->GetRange(info,dict,0,0,dictSize);

   // Grab the point set.
  vtkDataArray *X=usgIn->GetPoints()->GetData();
  int X_type=X->GetDataType();

  // Create the result array.
  vtkDoubleArray *qPts=vtkDoubleArray::New();
  qPts->Allocate(3*nCells); // Expect at least one point per cell
  qPts->SetNumberOfComponents(3);

  // For all cells interpolate.
  switch (X_type)
    {
    case VTK_DOUBLE:
      {
      vtkDoubleArray *X_d=static_cast<vtkDoubleArray *>(X);
      double *pX_d=X_d->GetPointer(0);
      if (!Interpolate(usgIn,nCells,X_d,pX_d,3,dict,qPts,0))
        {
        vtkWarningMacro("Failed to interpolate cell vertices "
                          "to quadrature points. Aborting.");
        return 0;
        }
      break;
      }
    case VTK_FLOAT:
      {
      vtkFloatArray *X_f=static_cast<vtkFloatArray *>(X);
      float *pX_f=X_f->GetPointer(0);
      if (!Interpolate(usgIn,nCells,X_f,pX_f,3,dict,qPts,0))
        {
        vtkWarningMacro("Failed to interpolate cell vertices "
                          "to quadrature points. Aborting.");
        return 0;
        }
      break;
      }
    }

  delete [] dict;

  // Add the interpolated quadrature points to the output
  vtkPoints *p=vtkPoints::New();
  p->SetDataTypeToDouble();
  p->SetData(qPts);
  qPts->Delete();
  pdOut->SetPoints(p);
  p->Delete();
  // Generate vertices at the quadrature points
  int nVerts=qPts->GetNumberOfTuples();
  vtkIdTypeArray *va=vtkIdTypeArray::New();
  va->SetNumberOfTuples(2*nVerts);
  vtkIdType *verts=va->GetPointer(0);
  for (int i=0; i<nVerts; ++i)
    {
    verts[0]=1;
    verts[1]=i;
    verts+=2;
    }
  vtkCellArray *cells=vtkCellArray::New();
  cells->SetCells(static_cast<vtkIdType>(nVerts),va);
  pdOut->SetVerts(cells);
  cells->Delete();
  va->Delete();

  return 1;
}

//-----------------------------------------------------------------------------
void vtkQuadraturePointsGenerator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "SourceArrayName:" 
     << (this->SourceArrayName?this->SourceArrayName:"\"\"") << endl;
}
