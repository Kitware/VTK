/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuadraturePointStatistics.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkQuadraturePointStatistics.h"

#include "vtkUnstructuredGrid.h"
#include "vtkTable.h"
#include "vtkType.h"
#include "vtkDoubleArray.h"
#include "vtkStringArray.h"
#include "vtkDataArray.h"
#include "vtkPointData.h"
#include "vtkFieldData.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkTableWriter.h"

#include "vtksys/ios/sstream"
using vtksys_ios::ostringstream;
#include "vtkstd/vector"
using vtkstd::vector;
#include "vtkstd/string"
using vtkstd::string;

#include "vtkQuadraturePointsUtilities.hxx"


//*****************************************************************************
void ComputeScalarStatistics(
        vtkDoubleArray *input,
        string name,
        vector<vtkDoubleArray*> &stats)
{
  const int nAtts=3;

  vtkDoubleArray *column=vtkDoubleArray::New();
  column->SetName(name.c_str());
  column->SetNumberOfTuples(nAtts);

  vtkIdType nTups=input->GetNumberOfTuples();
  double *pV=input->GetPointer(0);
  // initialize.
  double min=pV[0];
  double max=pV[0];
  double mean=pV[0];
  ++pV;
  // rest.
  for (vtkIdType i=1; i<nTups; ++i)
    {
    mean+=pV[0];
    min=pV[0]<min?pV[0]:min;
    max=pV[0]>max?pV[0]:max;
    ++pV;
    }
  mean/=nTups;
  column->SetValue(0,min);
  column->SetValue(1,max);
  column->SetValue(2,mean);

  stats.push_back(column);
}

//*****************************************************************************
void ComputeVectorStatistics(
        vtkDoubleArray *input,
        string name,
        vector<vtkDoubleArray*> &stats)
{
  const int nAtts=3;

  int nComps=input->GetNumberOfComponents();
  vtkIdType nTups=input->GetNumberOfTuples();
  double *pV=input->GetPointer(0);
  double mod=sqrt(pV[0]*pV[0]+pV[1]*pV[1]+pV[2]*pV[2]);
  double min[4]={mod,pV[0],pV[1],pV[2]};
  double max[4]={mod,pV[0],pV[1],pV[2]};
  double mean[4]={mod,pV[0],pV[1],pV[2]};
  pV+=nComps;
  for (vtkIdType i=1; i<nTups; ++i)
    {
    mod=sqrt(pV[0]*pV[0]+pV[1]*pV[1]+pV[2]*pV[2]);
    //
    min[0]=mod<min[0]?mod:min[0];
    min[1]=pV[0]<min[1]?pV[0]:min[1];
    min[2]=pV[1]<min[2]?pV[1]:min[2];
    min[3]=pV[2]<min[3]?pV[2]:min[3];
    //
    max[0]=mod>max[0]?mod:max[0];
    max[1]=pV[0]>max[1]?pV[0]:max[1];
    max[2]=pV[1]>max[2]?pV[1]:max[2];
    max[3]=pV[2]>max[3]?pV[2]:max[3];
    //
    mean[0]+=mod;
    mean[1]+=pV[0];
    mean[2]+=pV[1];
    mean[3]+=pV[2];

    pV+=nComps;
    }
  mean[0]/=nTups;
  mean[1]/=nTups;
  mean[2]/=nTups;
  mean[3]/=nTups;
  // add 4 arrays, 1 for the L2 norm, 3 for the comps.
  vtkDoubleArray *column;
  ostringstream compM;
  compM << "|" << name << "|";
  column=vtkDoubleArray::New();
  column->SetName(compM.str().c_str());
  column->SetNumberOfTuples(nAtts);
  column->SetValue(0,min[0]);
  column->SetValue(1,max[0]);
  column->SetValue(2,mean[0]);
  stats.push_back(column);
  //
  ostringstream compX;
  compX << name << "_X";
  column=vtkDoubleArray::New();
  column->SetName(compX.str().c_str());
  column->SetNumberOfTuples(nAtts);
  column->SetValue(0,min[1]);
  column->SetValue(1,max[1]);
  column->SetValue(2,mean[1]);
  stats.push_back(column);
  //
  ostringstream compY;
  compY << name << "_Y";
  column=vtkDoubleArray::New();
  column->SetName(compY.str().c_str());
  column->SetNumberOfTuples(nAtts);
  column->SetValue(0,min[2]);
  column->SetValue(1,max[2]);
  column->SetValue(2,mean[2]);
  stats.push_back(column);
  //
  ostringstream compZ;
  compZ << name << "_Z";
  column=vtkDoubleArray::New();
  column->SetName(compZ.str().c_str());
  column->SetNumberOfTuples(nAtts);
  column->SetValue(0,min[3]);
  column->SetValue(1,max[3]);
  column->SetValue(2,mean[3]);
  stats.push_back(column);
}






vtkCxxRevisionMacro(vtkQuadraturePointStatistics, "1.3");
vtkStandardNewMacro(vtkQuadraturePointStatistics);

//-----------------------------------------------------------------------------
vtkQuadraturePointStatistics::vtkQuadraturePointStatistics()
{
  this->Clear();
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//-----------------------------------------------------------------------------
vtkQuadraturePointStatistics::~vtkQuadraturePointStatistics()
{
  this->Clear();
}

//-----------------------------------------------------------------------------
int vtkQuadraturePointStatistics::FillInputPortInformation(
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
int vtkQuadraturePointStatistics::FillOutputPortInformation(
        int port,
        vtkInformation *info)
{
  switch (port)
    {
    case 0:
      info->Set(vtkDataObject::DATA_TYPE_NAME(),"vtkTable");
      break;
    }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkQuadraturePointStatistics::RequestData(
        vtkInformation *,
        vtkInformationVector **input,
        vtkInformationVector *output)
{
  vtkDataObject *tmpDataObj;
  // Get the inputs
  tmpDataObj
    = input[0]->GetInformationObject(0)->Get(vtkDataObject::DATA_OBJECT());
  vtkUnstructuredGrid *usgIn
    = vtkUnstructuredGrid::SafeDownCast(tmpDataObj);
  // Get the outputs
  tmpDataObj
    = output->GetInformationObject(0)->Get(vtkDataObject::DATA_OBJECT());
  vtkTable *tabOut
    = vtkTable::SafeDownCast(tmpDataObj);

  // Quick sanity check.
  if (usgIn==NULL || tabOut==NULL
     || usgIn->GetNumberOfCells()==0
     || usgIn->GetNumberOfPoints()==0
     || usgIn->GetPointData()->GetNumberOfArrays()==0)
    {
    vtkWarningMacro("Filter data has not been configured correctly. Aborting.");
    return 1;
    }

  // Interpolate the data arrays, but no points. Results
  // are stored in field data arrays.
  this->ComputeStatistics(usgIn,tabOut);

  return 1;
}

//-----------------------------------------------------------------------------
void vtkQuadraturePointStatistics::Clear()
{
  // Nothing to do
}

//-----------------------------------------------------------------------------
int vtkQuadraturePointStatistics::ComputeStatistics(
        vtkUnstructuredGrid *usgIn,
        vtkTable *results)
{
  // Each valid array on the input, produces one column for each of 
  // its components on the output.
  vector<vtkDoubleArray *> columns;

  // Look at the arrays in FieldData for fields interpolated to
  // quadrature points.
  int nArrays
    = usgIn->GetPointData()->GetNumberOfArrays();
  for (int arrayId=0; arrayId<nArrays; ++arrayId)
    {
    // Proccess it only if we have floating point data.
    vtkDataArray *V=usgIn->GetPointData()->GetArray(arrayId);
    int V_type=V->GetDataType();
    if (! ((V_type==VTK_FLOAT)||(V_type==VTK_DOUBLE)))
      {
      continue;
      }
    vtkDataArray *tmpDa;
    // Get the array with the interpolated values.
    ostringstream interpolatedName;
    interpolatedName << V->GetName() << "_QP_Interpolated";
    tmpDa=usgIn->GetFieldData()->GetArray(interpolatedName.str().c_str());
    vtkDoubleArray *interpolated=vtkDoubleArray::SafeDownCast(tmpDa);
    // Not found, try next array.
    if (!interpolated)
      {
      // cerr << "Skipping: " << V->GetName() << endl;
      continue;
      }

    // Process arrays, by the number of components they
    // have, because we want to name stuff like V_X,V_Y,V_Z
    // If there are more than three components we'll have to do
    // something else.
    int nComps=interpolated->GetNumberOfComponents();
    switch (nComps)
      {
      case 1:
        ComputeScalarStatistics(interpolated,V->GetName(),columns);
        break;
      case 3:
        ComputeVectorStatistics(interpolated,V->GetName(),columns);
        break;
      default:
        vtkWarningMacro("Unsupported number of components.");
        break;
      }
    }
  // Add the processed columns to the table.
  if (columns.size())
    {
    vtkStringArray *rowLabels=vtkStringArray::New();
    rowLabels->SetName(" ");
    rowLabels->SetNumberOfTuples(3);
    rowLabels->SetValue(0,"min");
    rowLabels->SetValue(1,"max");
    rowLabels->SetValue(2,"mean");
    results->AddColumn(rowLabels);
    rowLabels->Delete();
    size_t nCols=columns.size();
    for (size_t colId=0; colId<nCols; ++colId)
      {
      results->AddColumn(columns[colId]);
      //cerr << "Adding " << columns[colId]->GetName() << " to table." << endl;
      columns[colId]->Delete();
      }
    }
  // Clean out garbage that is added by default.
  results->GetFieldData()->Initialize();
  return static_cast<int>(columns.size());
}

//-----------------------------------------------------------------------------
void vtkQuadraturePointStatistics::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "No state." << endl;
}


