/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRRandomTableSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkRRandomTableSource.h"
#include "vtkRInterface.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStdString.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkDoubleArray.h"
#include "vtkVariant.h"

#include <map>
#include <string>
#include <sstream>

#include "R.h"
#include "Rmath.h"

vtkStandardNewMacro(vtkRRandomTableSource);

class ColumnStatsInfo
{
public:

  std::string name;
  vtkRRandomTableSource::StatDistType t;
  double param1;
  double param2;
  double param3;

};

class vtkRrtsimplementation
{
public:

  vtkRInterface* ri;
  std::vector<ColumnStatsInfo> col_list;

};

vtkAbstractArray* CreateRandomArray(const char* name,
                                    vtkRRandomTableSource::StatDistType t,
                                    double p1,
                                    double p2,
                                    double p3,
                                    int ntuples)
{

  int i;
  double x;
  vtkDoubleArray* arr = vtkDoubleArray::New();

  arr->SetNumberOfComponents(1);
  arr->SetNumberOfValues(ntuples);
  arr->SetName(name);

  switch(t)
    {
    case vtkRRandomTableSource::WILCOXONRANKSUM:
      GetRNGstate();
      for(i=0;i<ntuples;i++)
        {
        x = rwilcox(p1,p2);
        arr->SetValue(i,x);
        }
      PutRNGstate();
      break;
    case vtkRRandomTableSource::WILCOXONSIGNEDRANK:
      GetRNGstate();
      for(i=0;i<ntuples;i++)
        {
        x = rsignrank(p1);
        arr->SetValue(i,x);
        }
      PutRNGstate();
      break;
    case vtkRRandomTableSource::LOGISTIC:
      GetRNGstate();
      for(i=0;i<ntuples;i++)
        {
        x = rlogis(p1,p2);
        arr->SetValue(i,x);
        }
      PutRNGstate();
      break;
    case vtkRRandomTableSource::WEIBULL:
      GetRNGstate();
      for(i=0;i<ntuples;i++)
        {
        x = rweibull(p1,p2);
        arr->SetValue(i,x);
        }
      PutRNGstate();
      break;
    case vtkRRandomTableSource::POISSON:
      GetRNGstate();
      for(i=0;i<ntuples;i++)
        {
        x = rpois(p1);
        arr->SetValue(i,x);
      }
      PutRNGstate();
      break;
    case vtkRRandomTableSource::NEGBINOMIAL:
      GetRNGstate();
      for(i=0;i<ntuples;i++)
        {
        x = rnbinom(p1,p2);
        arr->SetValue(i,x);
        }
      PutRNGstate();
      break;
    case vtkRRandomTableSource::HYPERGEOM:
      GetRNGstate();
      for(i=0;i<ntuples;i++)
        {
        x = rhyper(p1,p2,p3);
        arr->SetValue(i,x);
        }
      PutRNGstate();
      break;
    case vtkRRandomTableSource::GEOM:
      GetRNGstate();
      for(i=0;i<ntuples;i++)
        {
        x = rgeom(p1);
        arr->SetValue(i,x);
        }
      PutRNGstate();
      break;
    case vtkRRandomTableSource::EXP:
      GetRNGstate();
      for(i=0;i<ntuples;i++)
        {
        x = rexp(p1);
        arr->SetValue(i,x);
        }
      PutRNGstate();
      break;
    case vtkRRandomTableSource::CAUCHY:
      GetRNGstate();
      for(i=0;i<ntuples;i++)
        {
        x = rcauchy(p1,p2);
        arr->SetValue(i,x);
        }
      PutRNGstate();
      break;
    case vtkRRandomTableSource::T:
      GetRNGstate();
      for(i=0;i<ntuples;i++)
        {
        x = rt(p1);
        arr->SetValue(i,x);
        }
      PutRNGstate();
      break;
    case vtkRRandomTableSource::F:
      GetRNGstate();
      for(i=0;i<ntuples;i++)
        {
        x = rf(p1,p2);
        arr->SetValue(i,x);
        }
      PutRNGstate();
      break;
    case vtkRRandomTableSource::LOGNORMAL:
      GetRNGstate();
      for(i=0;i<ntuples;i++)
        {
        x = rlnorm(p1,p2);
        arr->SetValue(i,x);
        }
      PutRNGstate();
      break;
    case vtkRRandomTableSource::GAMMA:
       GetRNGstate();
        for(i=0;i<ntuples;i++)
        {
        x = rgamma(p1,p2);
        arr->SetValue(i,x);
        }
      PutRNGstate();
      break;
    case vtkRRandomTableSource::UNIF:
      GetRNGstate();
      for(i=0;i<ntuples;i++)
        {
        x = runif(p1,p2);
        arr->SetValue(i,x);
        }
      PutRNGstate();
      break;
    case vtkRRandomTableSource::BETA:
      GetRNGstate();
      for(i=0;i<ntuples;i++)
        {
        x = rbeta(p1,p2);
        arr->SetValue(i,x);
        }
      PutRNGstate();
      break;
    case vtkRRandomTableSource::BINOMIAL:
      GetRNGstate();
      for(i=0;i<ntuples;i++)
        {
        x = rbinom(p1,p2);
        arr->SetValue(i,x);
        }
      PutRNGstate();
      break;
    case vtkRRandomTableSource::NORMAL:
      GetRNGstate();
      for(i=0;i<ntuples;i++)
        {
        x = rnorm(p1,p2);
        arr->SetValue(i,x);
        }
      PutRNGstate();
      break;
    case vtkRRandomTableSource::CHISQUARE:
      GetRNGstate();
      for(i=0;i<ntuples;i++)
        {
        x = rchisq(p1);
        arr->SetValue(i,x);
        }
      PutRNGstate();
      break;
    default:
      GetRNGstate();
      x = rnorm(0.0,1.0);
      for(i=0;i<ntuples;i++)
        {
        arr->SetValue(i,x);
        }
      PutRNGstate();
      break;
    }

  return(arr);

}

//----------------------------------------------------------------------------
vtkRRandomTableSource::vtkRRandomTableSource()
{

  this->impl = new vtkRrtsimplementation;
  this->impl->ri = vtkRInterface::New();
  this->impl->col_list.clear();
  this->NumberOfRows = 0;

  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  
}

//----------------------------------------------------------------------------
vtkRRandomTableSource::~vtkRRandomTableSource()
{

  this->impl->col_list.clear();
  this->impl->ri->Delete();
  delete this->impl;

}

//----------------------------------------------------------------------------
void vtkRRandomTableSource::SetRandGenSeed(const int seed)
{

  this->impl->ri->EvalRcommand("set.seed", seed);

}

//----------------------------------------------------------------------------
int vtkRRandomTableSource::GetNumberOfColumns()
{

  return(this->impl->col_list.size());

}

//----------------------------------------------------------------------------
void vtkRRandomTableSource::SetNumberOfRows(int nrows)
{

  if(nrows > 0)
    {
    this->NumberOfRows = nrows;
    this->Modified();
    }

}

//----------------------------------------------------------------------------
int vtkRRandomTableSource::GetNumberOfRows()
{

  return(this->NumberOfRows);

}

void vtkRRandomTableSource::ClearTableOutput()
{

  this->impl->col_list.clear();
  this->Modified();

}


//----------------------------------------------------------------------------
void vtkRRandomTableSource::SetStatisticalDistributionForColumn(int StatDist,
                                                                double param1,
                                                                double param2,
                                                                double param3,
                                                                const char* ColumnName,
                                                                int column_index)
{

  SetStatisticalDistributionForColumn((StatDistType) StatDist,
                                      param1,
                                      param2,
                                      param3,
                                      ColumnName,
                                      column_index);

}

//----------------------------------------------------------------------------
void vtkRRandomTableSource::SetStatisticalDistributionForColumn(vtkRRandomTableSource::StatDistType t,
                                                                double param1,
                                                                double param2,
                                                                double param3,
                                                                const char* ColumnName,
                                                                int column_index)
{

  if( (column_index < 0) || (column_index > (int) this->impl->col_list.size() ) )
    {
    return;
    }

  if(column_index  < (int) this->impl->col_list.size())
    {
    this->impl->col_list[column_index].t = t;
    this->impl->col_list[column_index].param1 = param1;
    this->impl->col_list[column_index].param2 = param2;
    this->impl->col_list[column_index].param3 = param3;
    this->impl->col_list[column_index].name = ColumnName;
    }
  else
    {
    ColumnStatsInfo csi;
    csi.t = t;
    csi.param1 = param1;
    csi.param2 = param2;
    csi.param3 = param3;
    csi.name = ColumnName;
    this->impl->col_list.push_back(csi);
    }

}

//----------------------------------------------------------------------------
int vtkRRandomTableSource::RequestData(
  vtkInformation*, 
  vtkInformationVector** vtkNotUsed(inputVector), 
  vtkInformationVector* outputVector)
{

  // Get output table
  vtkInformation* outputInfo1 = outputVector->GetInformationObject(0);
  vtkTable* output1 = vtkTable::SafeDownCast(
  outputInfo1->Get(vtkDataObject::DATA_OBJECT()));

  std::vector<ColumnStatsInfo>::iterator it;
  vtkAbstractArray* arr;

  output1->Initialize();

  for(it = this->impl->col_list.begin(); it != this->impl->col_list.end(); it++)
    {
    arr = CreateRandomArray((*it).name.c_str(),
                            (*it).t,
                            (*it).param1,
                            (*it).param2,
                            (*it).param3,
                            this->NumberOfRows);
    output1->AddColumn(arr);

    arr->Delete();
    }

  return 1;

}

void vtkRRandomTableSource::PrintSelf(ostream& os, vtkIndent indent)
{

  this->Superclass::PrintSelf(os, indent);
  os << indent << "NumberOfRows: " << this->NumberOfRows << endl;
  os << indent << "NumberOfColumns: " << this->impl->col_list.size() << endl;

}

