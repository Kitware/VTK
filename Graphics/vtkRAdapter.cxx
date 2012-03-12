
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRAdapter.cxx

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

#include "vtkObjectFactory.h"
#include "vtkRAdapter.h"
#include "vtkAbstractArray.h"
#include "vtkDoubleArray.h"
#include "vtkIntArray.h"
#include "vtkStdString.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkArray.h"
#include "vtkArrayExtents.h"
#include "vtkArrayCoordinates.h"
#include "vtkTypedArray.h"
#include "vtkVariantArray.h"
#include "vtkDoubleArray.h"
#include "vtkIntArray.h"
#include "vtkStringArray.h"
#include "vtkDataArrayCollection.h"
#include "vtkArrayData.h"
#include "vtkDataObjectCollection.h"

#include <map>

#include <stdio.h>
#include <assert.h>

#define R_NO_REMAP /* AVOID SOME SERIOUS STUPIDITY. DO NOT REMOVE. */

#include "R.h"
#include "Rdefines.h"
#include "R_ext/Parse.h"
#include "R_ext/Rdynload.h"


vtkStandardNewMacro(vtkRAdapter);

namespace
{

int R_FindArrayIndex(vtkArrayCoordinates& coordinates, const vtkArrayExtents& extents)
{

  vtkIdType i;
  int ret = 0;
  vtkIdType divisor = 1;
  vtkIdType d = coordinates.GetDimensions();

  for(i = 0; i < d; ++i)
    {
    ret = ret + coordinates[i]*divisor;
    divisor *= extents[i].GetSize();
    }

  return(ret);

}

} // End anonymous namespace

//----------------------------------------------------------------------------
vtkRAdapter::vtkRAdapter()
{

  this->vad =  vtkArrayData::New();
  this->vdoc = vtkDataObjectCollection::New();
  this->vdac = vtkDataArrayCollection::New();

}

//----------------------------------------------------------------------------
vtkRAdapter::~vtkRAdapter()
{

  if(this->vad)
    {
    this->vad->Delete();
    }

  if(this->vdoc)
    {
    this->vdoc->Delete();
    }

  if(this->vdac)
    {
    this->vdac->Delete();
    }

}

vtkDataArray* vtkRAdapter::RToVTKDataArray(SEXP variable)
{

  int i;
  int j;
  int nr;
  int nc;
  vtkDoubleArray* result;
  double* data;

  if( Rf_isMatrix(variable) || Rf_isVector(variable) )
    {
    nc = Rf_ncols(variable);
    nr = Rf_nrows(variable);

    result = vtkDoubleArray::New(); 

    result->SetNumberOfTuples(nr);
    result->SetNumberOfComponents(nc);

    data = new double[nc];

    for(i=0;i<nr;i++)
      {
      for(j=0;j<nc;j++)
        {
        if ( isReal(variable) )
          {
          data[j] = REAL(variable)[j*nr + i];
          }
        else if ( isInteger(variable) )
          {
          data[j] = static_cast<double>(INTEGER(variable)[j*nr + i]);
          }
        else
          {
          vtkErrorMacro(<< "Bad return variable, tried REAL and INTEGER.");
          }
        result->InsertTuple(i,data);
        }
      }

    delete [] data;
    this->vdac->AddItem(result);
    result->Delete();
    return(result);
    }
  else
    {
    return(0);
    }

}


SEXP vtkRAdapter::VTKDataArrayToR(vtkDataArray* da)
{

  SEXP a;
  int nr;
  int nc;
  int i;
  int j;
  double* data;

  nr = da->GetNumberOfTuples();
  nc = da->GetNumberOfComponents();

  PROTECT(a = Rf_allocMatrix(REALSXP,nr,nc));

  for(i=0;i<nr;i++)
    {
    for(j=0;j<nc;j++)
      {
      data = da->GetTuple(i);
      REAL(a)[j*nr + i] = data[j];
      }
    }

  return(a);

}

vtkArray* vtkRAdapter::RToVTKArray(SEXP variable)
{

  vtkArray::SizeT i;
  vtkArray::DimensionT j;
  vtkArray::DimensionT ndim;
  SEXP dims;
  vtkArrayExtents extents;
  vtkTypedArray<double>* da;
  da = vtkTypedArray<double>::SafeDownCast(vtkArray::CreateArray(vtkArray::DENSE, VTK_DOUBLE));

  dims = getAttrib(variable, R_DimSymbol);
  ndim = static_cast<vtkArray::DimensionT>(length(dims));

  if (!isMatrix(variable)&&!isArray(variable)&&isVector(variable))
    {
    ndim = 1;
    }

  extents.SetDimensions(ndim);

  if (isMatrix(variable)||isArray(variable))
    {
    for(j=0;j<ndim;j++)
      {
      extents[j] = vtkArrayRange(0,INTEGER(dims)[j]);
      }
    }
  else
    {
    extents[0] = vtkArrayRange(0,length(variable));
    }

  da->Resize(extents);

  vtkArrayCoordinates index;

  index.SetDimensions(ndim);

  for(i=0;i<da->GetSize();i++)
    {
    da->GetCoordinatesN(i,index);
    if ( isReal(variable) )
      {
      da->SetVariantValue(index,REAL(variable)[i]);
      }
    else if ( isInteger(variable) )
      {
      da->SetVariantValue(index,static_cast<double>(INTEGER(variable)[i]));
      }
    else
      {
      vtkErrorMacro(<< "Bad return variable, tried REAL and INTEGER.");
      }
    }

  this->vad->AddArray(da);
  da->Delete();
  return(da);

}

SEXP vtkRAdapter::VTKArrayToR(vtkArray* da)
{

  SEXP a;
  SEXP dim;
  vtkArray::SizeT i;
  vtkArray::DimensionT j;
  vtkArrayCoordinates coords;

  PROTECT(dim = Rf_allocVector(INTSXP, da->GetDimensions()));

  assert(da->GetExtents().ZeroBased());
  for(j=0;j<da->GetDimensions();j++)
    {
    INTEGER(dim)[j] = da->GetExtents()[j].GetSize();
    }

  PROTECT(a = Rf_allocArray(REALSXP, dim));

  for(i=0;i<da->GetSize();i++)
    {
    REAL(a)[i] = 0.0;
    }

  assert(da->GetExtents().ZeroBased());
  for(i=0;i<da->GetNonNullSize();i++)
    {
    da->GetCoordinatesN(i,coords);
    REAL(a)[R_FindArrayIndex(coords,da->GetExtents())] = da->GetVariantValue(coords).ToDouble();
    }

  UNPROTECT(1);

  return(a);

}

SEXP vtkRAdapter::VTKTableToR(vtkTable* table)
{

  SEXP a;
  SEXP b;
  SEXP names;
  int i;
  int j;
  int nr = table->GetNumberOfRows();
  int nc = table->GetNumberOfColumns();
  vtkVariant data;

  PROTECT(a = allocVector(VECSXP, nc));
  PROTECT(names = allocVector(STRSXP, nc));

  for(j=0;j<nc;j++)
    {
    SET_STRING_ELT(names,j,mkChar(table->GetColumn(j)->GetName()));
    if(vtkDataArray::SafeDownCast(table->GetColumn(j)))
      {
      PROTECT(b = allocVector(REALSXP,nr));
      SET_VECTOR_ELT(a,j,b);
      for(i=0;i<nr;i++)
        {
        data = table->GetValue(i,j);
        REAL(b)[i] = data.ToDouble();
        }
      }
    else
      {
      PROTECT(b = allocVector(STRSXP,nr));
      SET_VECTOR_ELT(a,j,b);
      for(i=0;i<nr;i++)
        {
        data = table->GetValue(i,j);
        SET_STRING_ELT(b,i,mkChar(data.ToString().c_str()));
        }
      }
    }

  setAttrib(a,R_NamesSymbol,names);

  return(a);

}

vtkTable* vtkRAdapter::RToVTKTable(SEXP variable)
{

  int i;
  int j;
  int nr;
  int nc;
  SEXP names;
  vtkVariant v;
  vtkTable* result;

  if( isMatrix(variable) )
    {
    nc = Rf_ncols(variable);
    nr = Rf_nrows(variable);
    result = vtkTable::New();

    for(j=0;j<nc;j++)
      {
      vtkDoubleArray* da = vtkDoubleArray::New();
      da->SetNumberOfComponents(1);
      names = getAttrib(variable, R_DimNamesSymbol);
      if(!isNull(names))
        {
        da->SetName(CHAR(STRING_ELT(VECTOR_ELT(names,1),j)));
        }
      else
        {
        v = j;
        da->SetName(v.ToString().c_str());
        }
      for(i=0;i<nr;i++)
        {
        da->InsertNextValue(REAL(variable)[j*nr + i]);
        }
      result->AddColumn(da);
      da->Delete();
      }
    }
  else if( isNewList(variable) )
    {
    nc = length(variable);
    nr = length(VECTOR_ELT(variable,0));
    for(j=1;j<nc;j++)
      {
      if(isReal(VECTOR_ELT(variable,j)) ||
         isInteger(VECTOR_ELT(variable,j)) ||
         isString(VECTOR_ELT(variable,j)) )
        {
        if(length(VECTOR_ELT(variable,j)) != nr)
          {
          vtkGenericWarningMacro(<<"Cannot convert R data type to vtkTable");
          return(0);
          }
        }
      else
        {
        vtkGenericWarningMacro(<<"Cannot convert R data type to vtkTable");
        return(0);
        }
      }

    result = vtkTable::New();
    names = getAttrib(variable, R_NamesSymbol);
    vtkAbstractArray *aa;
    for(j=0;j<nc;j++)
      {
      if(isReal(VECTOR_ELT(variable,j)))
        {
        vtkDoubleArray* da = vtkDoubleArray::New();
        da->SetNumberOfComponents(1);
        aa = da;
        for(i=0;i<nr;i++)
          {
          da->InsertNextValue(REAL(VECTOR_ELT(variable,j))[i]);
          }
        }
      else if(isInteger(VECTOR_ELT(variable,j)))
        {
        vtkIntArray* da = vtkIntArray::New();
        da->SetNumberOfComponents(1);
        aa = da;
        for(i=0;i<nr;i++)
          {
          da->InsertNextValue(INTEGER(VECTOR_ELT(variable,j))[i]);
          }
        }
      else
        {
        vtkStringArray* da = vtkStringArray::New();
        da->SetNumberOfComponents(1);
        aa = da;
        for(i=0;i<nr;i++)
          {
          da->InsertNextValue(CHAR(STRING_ELT(VECTOR_ELT(variable,j),i)));
          }
        }

      if(!isNull(names))
        {
        aa->SetName(CHAR(STRING_ELT(names,j)));
        }
      else
        {
        v = j;
        aa->SetName(v.ToString().c_str());
        }
      result->AddColumn(aa);
      aa->Delete();
      }
    }
  else
    {
    vtkGenericWarningMacro(<<"Cannot convert R data type to vtkTable");
    return(0);
    }

  this->vdoc->AddItem(result);
  result->Delete();
  return(result);

}

void vtkRAdapter::PrintSelf(ostream& os, vtkIndent indent)
{

  this->Superclass::PrintSelf(os,indent);

  if(this->vad)
    {
    this->vad->PrintSelf(os,indent);
    }

  if(this->vdoc)
    {
    this->vdoc->PrintSelf(os,indent);
    }

  if(this->vdac)
    {
    this->vdac->PrintSelf(os,indent);
    }


}

