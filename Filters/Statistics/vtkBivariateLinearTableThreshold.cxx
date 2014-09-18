/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkBivariateLinearTableThreshold.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
#include "vtkBivariateLinearTableThreshold.h"

#include "vtkDataArrayCollection.h"
#include "vtkDataObject.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkTable.h"

#include <map>

vtkStandardNewMacro(vtkBivariateLinearTableThreshold);

class vtkBivariateLinearTableThreshold::Internals
{
public:
  std::vector<vtkIdType> ColumnsToThreshold;
  std::vector<vtkIdType> ColumnComponentsToThreshold;
};

vtkBivariateLinearTableThreshold::vtkBivariateLinearTableThreshold()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(2);

  this->Implementation = new Internals;

  this->Initialize();
}

vtkBivariateLinearTableThreshold::~vtkBivariateLinearTableThreshold()
{
  delete this->Implementation;
}

void vtkBivariateLinearTableThreshold::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << "ColumnRanges: " << this->ColumnRanges[0] << " " << this->ColumnRanges[1] << endl;
  os << "UseNormalizedDistance: " << this->UseNormalizedDistance << endl;
  os << "Inclusive: " << this->Inclusive << endl;
  os << "DistanceThreshold: " << this->DistanceThreshold << endl;
  os << "LinearThresholdType: " << this->LinearThresholdType << endl;
}

void vtkBivariateLinearTableThreshold::Initialize()
{
  this->Inclusive = 0;
  this->Implementation->ColumnsToThreshold.clear();
  this->Implementation->ColumnComponentsToThreshold.clear();

  this->DistanceThreshold = 1.0;
  this->ColumnRanges[0] = 1.0;
  this->ColumnRanges[1] = 1.0;
  this->UseNormalizedDistance = 0;
  this->NumberOfLineEquations = 0;
  this->LinearThresholdType = BLT_NEAR;

  this->LineEquations = vtkSmartPointer<vtkDoubleArray>::New();
  this->LineEquations->SetNumberOfComponents(3);
  this->Modified();
}

int vtkBivariateLinearTableThreshold::RequestData(vtkInformation* vtkNotUsed(request),
                                              vtkInformationVector** inputVector,
                                              vtkInformationVector* outputVector)
{
  vtkTable* inTable = vtkTable::GetData( inputVector[0], 0);
  vtkTable* outRowIdsTable = vtkTable::GetData( outputVector, OUTPUT_ROW_IDS );
  vtkTable* outRowDataTable = vtkTable::GetData( outputVector, OUTPUT_ROW_DATA );

  if (!inTable || this->GetNumberOfColumnsToThreshold() != 2)
    {
    return 1;
    }

  if (!outRowIdsTable)
    {
    vtkErrorMacro(<<"No output table, for some reason.");
    return 0;
    }

  vtkSmartPointer<vtkIdTypeArray> outIds = vtkSmartPointer<vtkIdTypeArray>::New();
  if (!ApplyThreshold(inTable,outIds))
    {
    vtkErrorMacro(<<"Error during threshold application.");
    return 0;
    }

  outRowIdsTable->Initialize();
  outRowIdsTable->AddColumn(outIds);

  outRowDataTable->Initialize();
  int numColumns = inTable->GetNumberOfColumns();
  for (int i=0; i<numColumns; i++)
    {
    vtkDataArray* a = vtkDataArray::CreateDataArray(inTable->GetColumn(i)->GetDataType());
    a->SetNumberOfComponents(inTable->GetColumn(i)->GetNumberOfComponents());
    a->SetName(inTable->GetColumn(i)->GetName());
    outRowDataTable->AddColumn(a);
    a->Delete();
    }

  for (int i=0; i<outIds->GetNumberOfTuples(); i++)
    {
    outRowDataTable->InsertNextRow(inTable->GetRow(outIds->GetValue(i)));
    }

  return 1;
}

int vtkBivariateLinearTableThreshold::FillInputPortInformation( int port, vtkInformation* info )
{
  if (port == 0)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
    return 1;
    }

  return 0;
}

int vtkBivariateLinearTableThreshold::FillOutputPortInformation( int port, vtkInformation* info )
{
  if ( port == OUTPUT_ROW_IDS ||
       port == OUTPUT_ROW_DATA)
    {
    info->Set( vtkDataObject::DATA_TYPE_NAME(), "vtkTable" );
    return 1;
    }

  return 0;
}

void vtkBivariateLinearTableThreshold::AddColumnToThreshold(vtkIdType column, vtkIdType component)
{
  this->Implementation->ColumnsToThreshold.push_back(column);
  this->Implementation->ColumnComponentsToThreshold.push_back(component);
  this->Modified();
}

int vtkBivariateLinearTableThreshold::GetNumberOfColumnsToThreshold()
{
  return (int)this->Implementation->ColumnsToThreshold.size();
}

void vtkBivariateLinearTableThreshold::GetColumnToThreshold(vtkIdType idx, vtkIdType& column, vtkIdType& component)
{
  if (idx < 0 || idx >= (int)this->Implementation->ColumnsToThreshold.size())
    {
    column = -1;
    component = -1;
    }
  else
    {
    column = this->Implementation->ColumnsToThreshold[idx];
    component = this->Implementation->ColumnComponentsToThreshold[idx];
    }
}

void vtkBivariateLinearTableThreshold::ClearColumnsToThreshold()
{
  this->Implementation->ColumnsToThreshold.clear();
  this->Implementation->ColumnComponentsToThreshold.clear();
}

vtkIdTypeArray* vtkBivariateLinearTableThreshold::GetSelectedRowIds(int selection)
{
  vtkTable* table = vtkTable::SafeDownCast(this->GetOutput());

  if (!table)
    return NULL;

  return vtkIdTypeArray::SafeDownCast(table->GetColumn(selection));
}

int vtkBivariateLinearTableThreshold::ApplyThreshold(vtkTable* tableToThreshold, vtkIdTypeArray* acceptedIds)
{
  // grab the first two arrays (and their components) to threshold
  vtkIdType column1,column2,component1,component2;

  if (this->GetNumberOfColumnsToThreshold() != 2)
    {
    vtkErrorMacro(<<"This threshold only works on two columns at a time.  Received: "<<this->GetNumberOfColumnsToThreshold());
    return 0;
    }

  this->GetColumnToThreshold(0,column1,component1);
  this->GetColumnToThreshold(1,column2,component2);

  vtkDataArray* a1 = vtkDataArray::SafeDownCast(tableToThreshold->GetColumn(column1));
  vtkDataArray* a2 = vtkDataArray::SafeDownCast(tableToThreshold->GetColumn(column2));

  if (!a1 || !a2)
    {
    vtkErrorMacro(<<"Wrong number of arrays received.");
    return 0;
    }

  if (a1->GetNumberOfTuples() != a2->GetNumberOfTuples())
    {
    vtkErrorMacro(<<"Two arrays to threshold must have the same number of tuples.");
    return 0;
    }

  int (vtkBivariateLinearTableThreshold::*thresholdFunc)(double,double) = NULL;
  switch (this->LinearThresholdType)
    {
    case vtkBivariateLinearTableThreshold::BLT_ABOVE:
      thresholdFunc = &vtkBivariateLinearTableThreshold::ThresholdAbove;
      break;
    case vtkBivariateLinearTableThreshold::BLT_BELOW:
      thresholdFunc = &vtkBivariateLinearTableThreshold::ThresholdBelow;
      break;
    case vtkBivariateLinearTableThreshold::BLT_NEAR:
      thresholdFunc = &vtkBivariateLinearTableThreshold::ThresholdNear;
      break;
    case vtkBivariateLinearTableThreshold::BLT_BETWEEN:
      thresholdFunc = &vtkBivariateLinearTableThreshold::ThresholdBetween;
      break;
    default:
      vtkErrorMacro(<<"Threshold type not defined: "<<this->LinearThresholdType);
      return 0;
    }

  acceptedIds->Initialize();
  int numTuples = a1->GetNumberOfTuples();
  double v1,v2;
  for (int i=0; i<numTuples; i++)
    {
    v1 = a1->GetComponent(i,component1);
    v2 = a2->GetComponent(i,component2);

    if ((this->*thresholdFunc)(v1,v2))
      {
      acceptedIds->InsertNextValue(i);
      }
    }

  return 1;
}

void vtkBivariateLinearTableThreshold::AddLineEquation(double* p1, double* p2)
{
  double a = p1[1]-p2[1];
  double b = p2[0]-p1[0];
  double c = p1[0]*p2[1]-p2[0]*p1[1];

  this->AddLineEquation(a,b,c);
}

void vtkBivariateLinearTableThreshold::AddLineEquation(double* p, double slope)
{
  double p2[2] = {p[0]+1.0,p[1]+slope};
  this->AddLineEquation(p,p2);
}

void vtkBivariateLinearTableThreshold::AddLineEquation(double a, double b, double c)
{
  double norm = sqrt(a*a+b*b);
  a /= norm;
  b /= norm;
  c /= norm;

  this->LineEquations->InsertNextTuple3(a,b,c);
  this->NumberOfLineEquations++;
}

void vtkBivariateLinearTableThreshold::ClearLineEquations()
{
  this->LineEquations->Initialize();
  this->NumberOfLineEquations=0;
}

void vtkBivariateLinearTableThreshold::ComputeImplicitLineFunction(double* p1, double* p2, double* abc)
{
  abc[0] = p1[1]-p2[1];
  abc[1] = p2[0]-p1[0];
  abc[2] = p1[0]*p2[1]-p2[0]*p1[1];
}

void vtkBivariateLinearTableThreshold::ComputeImplicitLineFunction(double* p, double slope, double* abc)
{
  double p2[2] = {p[0]+1.0,p[1]+slope};
  vtkBivariateLinearTableThreshold::ComputeImplicitLineFunction(p,p2,abc);
}

int vtkBivariateLinearTableThreshold::ThresholdAbove(double x, double y)
{
  double* c,v;
  for (int i=0; i<this->NumberOfLineEquations; i++)
    {
    c = this->LineEquations->GetTuple3(i);
    v = c[0]*x + c[1]*y + c[2];

    if ((this->GetInclusive() && v >= 0) ||
        (!this->GetInclusive() && v > 0))
      {
      return 1;
      }
    }
  return 0;
}

int vtkBivariateLinearTableThreshold::ThresholdBelow(double x, double y)
{
  double* c,v;
  for (int i=0; i<this->NumberOfLineEquations; i++)
    {
    c = this->LineEquations->GetTuple3(i);
    v = c[0]*x + c[1]*y + c[2];

    if ((this->GetInclusive() && v <= 0) ||
        (!this->GetInclusive() && v < 0))
      {
      return 1;
      }
    }
  return 0;
}

int vtkBivariateLinearTableThreshold::ThresholdNear(double x, double y)
{
  double* c,v;
  for (int i=0; i<this->NumberOfLineEquations; i++)
    {
    c = this->LineEquations->GetTuple3(i);

    if (this->UseNormalizedDistance)
      {
      double dx = fabs(x-(-c[1]*y-c[2])/c[0]);
      double dy = fabs(y-(-c[0]*x-c[2])/c[1]);

      double dxn = dx/this->ColumnRanges[0];
      double dyn = dy/this->ColumnRanges[1];

      v = sqrt(dxn*dxn+dyn*dyn);
      }
    else
      {
      v = fabs(c[0]*x + c[1]*y + c[2]);
      }

    if ((this->GetInclusive() && v <= this->DistanceThreshold) ||
        (!this->GetInclusive() && v < this->DistanceThreshold))
      {
      return 1;
      }
    }

  return 0;
}

int vtkBivariateLinearTableThreshold::ThresholdBetween(double x, double y)
{
  return (this->ThresholdAbove(x,y) && this->ThresholdBelow(x,y));
}
