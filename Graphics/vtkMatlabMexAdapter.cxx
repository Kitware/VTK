
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMatlabMexAdapter.cxx

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

#include "vtkMatlabMexAdapter.h"
#include "vtkSmartPointer.h"
#include "vtkObjectFactory.h"
#include "vtkDataArray.h"
#include "vtkCharArray.h"
#include "vtkUnsignedShortArray.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkShortArray.h"
#include "vtkIntArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkLongArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkGraph.h"
#include "vtkDirectedGraph.h"
#include "vtkAdjacentVertexIterator.h"
#include "vtkVertexListIterator.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkMutableUndirectedGraph.h"
#include "vtkTypeInt8Array.h"
#include "vtkTypeInt16Array.h"
#include "vtkTypeInt32Array.h"
#include "vtkTypeInt64Array.h"
#include "vtkTypeUInt8Array.h"
#include "vtkTypeUInt16Array.h"
#include "vtkTypeUInt32Array.h"
#include "vtkTypeUInt64Array.h"
#include "vtkArray.h"
#include "vtkTypedArray.h"
#include "vtkSparseArray.h"
#include "vtkDenseArray.h"
#include "vtkArrayExtents.h"
#include "vtkArrayCoordinates.h"
#include "vtkDataArrayCollection.h"
#include "vtkArrayData.h"
#include "vtkDataObjectCollection.h"
#include <vtkstd/vector>
#include <vtkstd/algorithm>
#include <assert.h>

#define VTK_CREATE(classname, varname) vtkSmartPointer<classname> varname = vtkSmartPointer<classname>::New()


vtkStandardNewMacro(vtkMatlabMexAdapter);

namespace
{

// Get (i,j) index value from an mxArray matrix

double mxArrayGetValue(int i, int j, mxArray* mxa)
{

  mwIndex *ir;
  mwIndex *jc;
  double* pr;
  int row_index;
  int row_start_index;
  int row_stop_index;

  pr = mxGetPr(mxa);

  if( mxIsSparse(mxa) )
    {
    ir = mxGetIr(mxa);
    jc = mxGetJc(mxa);

    row_start_index = jc[j];
    row_stop_index = jc[j+1];

    if(row_start_index == row_stop_index)
      return(0.0);
    else
      {
      for(row_index = row_start_index; row_index < row_stop_index ;row_index++)
        {
        if(i == (int) ir[row_index])
          return(pr[row_index]);
        }

      return(0.0);
      }

    }
  else
    {
    return(pr[j*mxGetM(mxa) + i]);
    }

}


int FindArrayIndex(vtkArrayCoordinates& coordinates, const vtkArrayExtents& extents)
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

template<typename T> mxArray* CopyVTKArrayTomxArray(vtkTypedArray<T>* da, mxClassID mt)
{

  mxArray* output;
  mwSize* dims;
  int i;
  vtkArrayCoordinates coords;
  T* dest;

  dims = new mwSize[da->GetDimensions()];

  assert(da->GetExtents().ZeroBased());

  for(i=0;i<da->GetDimensions();i++)
    {
    dims[i] = da->GetExtents()[i].GetSize();
    }

  output = mxCreateNumericArray(da->GetDimensions(),dims,mt,mxREAL);

  dest = (T*) mxGetData(output);

  for(i=0;i<da->GetSize();i++)
    {
    dest[i] = 0;
    }

  assert(da->GetExtents().ZeroBased());
  for(i=0;i<da->GetNonNullSize();i++)
    {
    da->GetCoordinatesN(i,coords);
    dest[FindArrayIndex(coords,da->GetExtents())] = da->GetValue(coords);
    }

  delete [] dims;
  return(output);

};

} // End anonymous namespace

template<typename T> vtkArray* vtkMatlabMexAdapter::CopymxArrayToVTKArray(mxArray* mxa, int ValueType)
{

  vtkTypedArray<T>* da;
  vtkArrayExtents extents;
  int i;
  T* source;

  da = vtkTypedArray<T>::SafeDownCast(vtkArray::CreateArray(vtkArray::DENSE, ValueType));

  mwSize mxndim = mxGetNumberOfDimensions(mxa);
  const mwSize* mxdims = mxGetDimensions(mxa);

// Set Array Extents

  extents.SetDimensions(mxndim);

  for(i=0;i< (int) mxndim;i++)
    {
    extents[i] = vtkArrayRange(0,(int) mxdims[i]);
    }

  da->Resize(extents);

  vtkArrayCoordinates index;

  index.SetDimensions(mxndim);

  source = (T*) mxGetData(mxa);

  for(i=0;i<da->GetSize();i++)
    {
    da->GetCoordinatesN(i,index);
    da->SetVariantValue(index,source[i]);
    }

  this->vad->AddArray(da);
  da->Delete();
  return(da);

};



mxClassID vtkMatlabMexAdapter::GetMatlabDataType(vtkDataArray* da)
{

  switch(da->GetDataType())
    {
    case VTK_BIT:
      return(mxUINT8_CLASS);
      break;
    case VTK_CHAR:
      return(mxINT8_CLASS);
      break;
    case VTK_SIGNED_CHAR:
      return(mxCHAR_CLASS);
      break;
    case VTK_UNSIGNED_CHAR:
      return(mxUINT8_CLASS);
      break;
    case VTK_SHORT:
      return(mxINT16_CLASS);
      break;
    case VTK_UNSIGNED_SHORT:
      return(mxUINT16_CLASS);
      break;
    case VTK_INT:
      return(mxINT32_CLASS);
      break;
    case VTK_ID_TYPE:
      return(mxINT32_CLASS);
      break;
    case VTK_UNSIGNED_INT:
      return(mxUINT32_CLASS);
      break;
    case VTK_LONG:
      return(mxINT64_CLASS);
      break;
    case VTK_UNSIGNED_LONG:
      return(mxUINT64_CLASS);
      break;
    case VTK_LONG_LONG:
      return(mxINT64_CLASS);
      break;
    case VTK_UNSIGNED_LONG_LONG:
      return(mxUINT64_CLASS);
      break;
    case VTK_FLOAT:
      return(mxSINGLE_CLASS);
      break;
    case VTK_DOUBLE:
      return(mxDOUBLE_CLASS);
      break;
    default:
      return(mxDOUBLE_CLASS);
      break;
    }

}

vtkDataArray* vtkMatlabMexAdapter::GetVTKDataType(mxClassID cid)
{

  switch(cid)
    {
    case mxCHAR_CLASS:
      return(vtkCharArray::New());
      break;
    case mxLOGICAL_CLASS:
      return(vtkUnsignedShortArray::New());
      break;
    case mxDOUBLE_CLASS:
      return(vtkDoubleArray::New());
      break;
    case mxSINGLE_CLASS:
      return(vtkFloatArray::New());
      break;
    case mxINT8_CLASS:
      return(vtkTypeInt8Array::New());
      break;
    case mxUINT8_CLASS:
      return(vtkTypeUInt8Array::New());
      break;
    case mxINT16_CLASS:
      return(vtkTypeInt16Array::New());
      break;
    case mxUINT16_CLASS:
      return(vtkTypeUInt16Array::New());
      break;
    case mxINT32_CLASS:
      return(vtkTypeInt32Array::New());
      break;
    case mxUINT32_CLASS:
      return(vtkTypeUInt32Array::New());
      break;
    case mxINT64_CLASS:
      return(vtkTypeInt64Array::New());
      break;
    case mxUINT64_CLASS:
      return(vtkTypeUInt64Array::New());
      break;
    default:
      return(vtkDoubleArray::New());
      break;
    }

}


//----------------------------------------------------------------------------
vtkMatlabMexAdapter::vtkMatlabMexAdapter()
{

  this->vad =  vtkArrayData::New();
  this->vdoc = vtkDataObjectCollection::New();
  this->vdac = vtkDataArrayCollection::New();

}

//----------------------------------------------------------------------------
vtkMatlabMexAdapter::~vtkMatlabMexAdapter()
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

// Create a deep copy Matlab mxArray of the input vtkDataArray.

mxArray* vtkMatlabMexAdapter::vtkDataArrayToMxArray(vtkDataArray* aa, bool ShallowCopy)
{

  int ntuples;
  int ncomp;
  mwSize dims[2];
  mxClassID mdtype;
  void* dp;
  int i,j,k;
  int nbytes;
  unsigned char* dest;
  unsigned char* source;

  if( aa == NULL )
    {
    vtkGenericWarningMacro(<<"NULL input to vtkDataArrayToMxArray()");
    return(NULL);
    }

  mdtype = vtkMatlabMexAdapter::GetMatlabDataType(aa);

  ntuples = aa->GetNumberOfTuples();

  ncomp = aa->GetNumberOfComponents();

  if(ShallowCopy)
    {
    dims[0] = 0;
    dims[1] = 0;

    mxArray* mxa = mxCreateNumericArray(2,dims,mdtype,mxREAL);


    dims[0] = ncomp;
    dims[1] = ntuples;

    mxSetDimensions(mxa,dims,2);

    mxSetData(mxa, aa->GetVoidPointer(0));

    return(mxa);
    }

  dims[0] = ntuples;

  dims[1] = ncomp;

  mxArray* mxa = mxCreateNumericArray(2, dims, mdtype, mxREAL);

  nbytes = mxGetElementSize(mxa);

  if (nbytes != aa->GetElementComponentSize())
    {
    vtkGenericWarningMacro(<<"Data size mismatch between Matlab and VTK");
    return(NULL);
    }

  dp = mxGetData(mxa);

  dest = (unsigned char*) dp;

  for(i=0;i<ntuples;i++)
    {
    for(j=0;j<ncomp;j++)
      {
      source = (unsigned char*) aa->GetVoidPointer(i*ncomp + j);

      for(k=0;k<nbytes;k++)
        {
        dest[j*(ntuples*nbytes) + i*nbytes + k] = source[k];
        }
      }

    }

  return(mxa);

}

// Create a deep copy vtkDataArray of the input mxArray.

vtkDataArray* vtkMatlabMexAdapter::mxArrayTovtkDataArray(const mxArray* mxa, bool ShallowCopy)
{

  vtkDataArray* da;
  void *dp;
  double* tuple;
  int nr;
  int nc;
  int i,j,k;
  int nbytes;
  unsigned char* dest;
  unsigned char* source;

  if( mxa == NULL )
    {
    vtkGenericWarningMacro(<<"NULL input to mxArrayTovtkDataArray()");
    return(NULL);
    }

  if( mxGetNumberOfDimensions(mxa) > 2 )
    {
    vtkGenericWarningMacro(<<"Input to mxArrayTovtkDataArray() has more than two dimensions, cannot convert to vtkDataArray");
    return(NULL);
    }

  if( mxIsCell(mxa) )
    {
    vtkGenericWarningMacro(<<"Input to mxArrayTovtkDataArray() is a Cell Array, cannot convert to vtkDataArray");
    return(NULL);
    }

  if( mxIsSparse(mxa) )
    {
    vtkGenericWarningMacro(<<"Input to mxArrayTovtkDataArray() is a Sparse Array, cannot convert to vtkDataArray");
    return(NULL);
    }

  nr = mxGetM(mxa);
  nc = mxGetN(mxa);

  da = vtkMatlabMexAdapter::GetVTKDataType(mxGetClassID(mxa));

  nbytes = mxGetElementSize(mxa);

  if (nbytes != da->GetElementComponentSize())
    {
    da->Delete();
    vtkGenericWarningMacro(<<"Data size mismatch between Matlab and VTK");
    return(NULL);
    }

  da->SetNumberOfTuples(nr);
  da->SetNumberOfComponents(nc);

  if(ShallowCopy)
    {
    da->SetVoidArray(mxGetData(mxa),(vtkIdType) nr*nc, 1);
    return(da);
    }

  tuple = (double*) mxMalloc(sizeof(double)*nc);
  dp = mxGetData(mxa);
  source = (unsigned char*) dp;

  for(i=0;i<nr;i++)
    {
    da->InsertTuple(i,tuple);

    for(j=0;j<nc;j++)
      {
      dest = (unsigned char*) da->GetVoidPointer(i*nc + j);

      for(k=0;k<nbytes;k++)
        {
        dest[k] = source[j*(nr*nbytes) + i*nbytes + k];
        }
      }
    }

  mxFree(tuple);
  this->vdac->AddItem(da);
  da->Delete();
  return(da);

}


// Create a mxArray from a vtkArray (Allocates memory)

mxArray* vtkMatlabMexAdapter::vtkArrayToMxArray(vtkArray* va)
{

  if( va == NULL )
    {
    vtkGenericWarningMacro(<<"NULL input to vtkArrayToMxArray()");
    }

  if(vtkTypedArray<unsigned char>::SafeDownCast(va))
    {
    return(CopyVTKArrayTomxArray<unsigned char>(vtkTypedArray<unsigned char>::SafeDownCast(va),
           mxUINT8_CLASS));
    }
  else if(vtkTypedArray<char>::SafeDownCast(va))
    {
    return(CopyVTKArrayTomxArray<char>(vtkTypedArray<char>::SafeDownCast(va),
           mxINT8_CLASS));
    }
  else if(vtkTypedArray<short>::SafeDownCast(va))
    {
    return(CopyVTKArrayTomxArray<short>(vtkTypedArray<short>::SafeDownCast(va),
           mxINT16_CLASS));
    }
  else if(vtkTypedArray<unsigned short>::SafeDownCast(va))
    {
    return(CopyVTKArrayTomxArray<unsigned short>(vtkTypedArray<unsigned short>::SafeDownCast(va),
           mxUINT16_CLASS));
    }
  else if(vtkTypedArray<int>::SafeDownCast(va))
    {
    return(CopyVTKArrayTomxArray<int>(vtkTypedArray<int>::SafeDownCast(va),
           mxINT32_CLASS));
    }
  else if(vtkTypedArray<vtkIdType>::SafeDownCast(va))
    {
    return(CopyVTKArrayTomxArray<vtkIdType>(vtkTypedArray<vtkIdType>::SafeDownCast(va),
           mxINT32_CLASS));
    }
  else if(vtkTypedArray<unsigned int>::SafeDownCast(va))
    {
    return(CopyVTKArrayTomxArray<unsigned int>(vtkTypedArray<unsigned int>::SafeDownCast(va),
           mxUINT32_CLASS));
    }
  else if(vtkTypedArray<long>::SafeDownCast(va))
    {
    return(CopyVTKArrayTomxArray<long>(vtkTypedArray<long>::SafeDownCast(va),
           mxINT64_CLASS));
    }
  else if(vtkTypedArray<unsigned long>::SafeDownCast(va))
    {
    return(CopyVTKArrayTomxArray<unsigned long>(vtkTypedArray<unsigned long>::SafeDownCast(va),
           mxUINT64_CLASS));
    }
  else if(vtkTypedArray<float>::SafeDownCast(va))
    {
    return(CopyVTKArrayTomxArray<float>(vtkTypedArray<float>::SafeDownCast(va),
           mxSINGLE_CLASS));
    }
  else if(vtkTypedArray<double>::SafeDownCast(va))
    {
    return(CopyVTKArrayTomxArray<double>(vtkTypedArray<double>::SafeDownCast(va),
           mxDOUBLE_CLASS));
    }
  else
    {
    return(CopyVTKArrayTomxArray<double>(vtkTypedArray<double>::SafeDownCast(va),
           mxDOUBLE_CLASS));
    }

}


// Create a vtkArray from an mxArray (Allocates memory)

vtkArray* vtkMatlabMexAdapter::mxArrayTovtkArray(mxArray* mxa)
{

  if( mxa == NULL )
    {
    vtkGenericWarningMacro(<<"NULL input to mxArrayTovtkArray()");
    }

  if(mxGetClassID(mxa) == mxCELL_CLASS)
    {
    vtkGenericWarningMacro(<<" Unable to convert input mwArray cell array to vtArray in mxArrayTovtkArray()");
    return(NULL);
    }

  switch(mxGetClassID(mxa))
    {
    case mxCHAR_CLASS:
      return(CopymxArrayToVTKArray<char>(mxa,VTK_CHAR));
      break;
    case mxLOGICAL_CLASS:
      return(CopymxArrayToVTKArray<unsigned char>(mxa, VTK_BIT));
      break;
    case mxDOUBLE_CLASS:
      return(CopymxArrayToVTKArray<double>(mxa,VTK_DOUBLE));
      break;
    case mxSINGLE_CLASS:
      return(CopymxArrayToVTKArray<float>(mxa,VTK_FLOAT));
      break;
    case mxINT8_CLASS:
      return(CopymxArrayToVTKArray<short>(mxa,VTK_SHORT));
      break;
    case mxUINT8_CLASS:
      return(CopymxArrayToVTKArray<unsigned char>(mxa,VTK_UNSIGNED_CHAR));
      break;
    case mxINT16_CLASS:
      return(CopymxArrayToVTKArray<short>(mxa,VTK_SHORT));
      break;
    case mxUINT16_CLASS:
      return(CopymxArrayToVTKArray<unsigned short>(mxa,VTK_UNSIGNED_SHORT));
      break;
    case mxINT32_CLASS:
      return(CopymxArrayToVTKArray<int>(mxa,VTK_INT));
      break;
    case mxUINT32_CLASS:
      return(CopymxArrayToVTKArray<unsigned int>(mxa,VTK_UNSIGNED_INT));
      break;
    case mxINT64_CLASS:
      return(CopymxArrayToVTKArray<long>(mxa,VTK_LONG));
      break;
    case mxUINT64_CLASS:
      return(CopymxArrayToVTKArray<unsigned long>(mxa,VTK_UNSIGNED_LONG));
      break;
    default:
      return(CopymxArrayToVTKArray<double>(mxa,VTK_DOUBLE));
      break;
    }

}

// Create a mxArray from a vtkGraph (Allocates memory)

mxArray* vtkMatlabMexAdapter::vtkGraphToMxArray(vtkGraph* ga)
{

  mxArray* output;
  double* pr;
  bool isDirected = false;

// Use Matlab mxArray sparse or dense matrix format to represent the input vtkGraph

  if( ga == NULL )
    {
    vtkGenericWarningMacro(<<"NULL input to vtkGraphToMxArray()");
    }

  if(vtkDirectedGraph::SafeDownCast(ga))
    {
    isDirected = true;
    }

  int numvert = ga->GetNumberOfVertices();
  int numedges = ga->GetNumberOfEdges();

  vtkSmartPointer<vtkVertexListIterator> vl = vtkSmartPointer<vtkVertexListIterator>::New();
  vtkSmartPointer<vtkAdjacentVertexIterator> av = vtkSmartPointer<vtkAdjacentVertexIterator>::New();

// Create a sparse matrix if there is a low density of edges in the graph, else create a dense matrix.

  if( numedges < (numvert*numvert) )
    {
    mwIndex *ir, *jc;
    int index;
    vtkstd::vector<int> vertlist;
    vtkstd::vector<int>::iterator vli;

    if(isDirected)
      output = mxCreateSparse(numvert, numvert, numedges, mxREAL);
    else
      output = mxCreateSparse(numvert, numvert, 2*numedges, mxREAL);

    pr = mxGetPr(output);
    ir = mxGetIr(output);
    jc = mxGetJc(output);

    ga->GetVertices(vl);
    index = 0;

    while(vl->HasNext())
      {
      vtkIdType vid = vl->Next();
      ga->GetAdjacentVertices(vid, av);

      vertlist.clear();

      while(av->HasNext())
        {
        vertlist.push_back(av->Next());
        }

      vtkstd::sort( vertlist.begin(), vertlist.end() );
      jc[vid] = index;

      if(!vertlist.empty())
        {
        int lvv = *(vertlist.begin());

        for(vli = vertlist.begin(); vli != vertlist.end(); ++vli)
          {
          if( (*vli != lvv) || (vli == vertlist.begin()) )
            {
            pr[index]++;
            ir[index] = *vli;
            index++;
            }
          else
            {
            pr[index-1]++;
            }

          lvv = *vli;
          }
        }
      }

    jc[numvert] = index;
    return(output);
    }
  else // Create a dense matrix
    {
    output = mxCreateDoubleMatrix(numvert, numvert, mxREAL);
    pr = mxGetPr(output);
    ga->GetVertices(vl);

    while(vl->HasNext())
      {
      vtkIdType vid = vl->Next();
      ga->GetAdjacentVertices(vid, av);

      while(av->HasNext())
        {
        pr[ vid*numvert + av->Next() ]++;
        }
      }
    return(output);
    }

}

// Create a vtkGraph from an mxArray (Allocates memory)

vtkGraph* vtkMatlabMexAdapter::mxArrayTovtkGraph(mxArray* mxa)
{

  int nr;
  int nc;
  int i;
  int j;
  int k;
  bool isDirected = false;

  if( mxa == NULL )
    {
    vtkGenericWarningMacro(<<"NULL input to mxArrayTovtkGraph()");
    }

  if( mxGetNumberOfFields(mxa) > 1 )
    {
    vtkGenericWarningMacro(<<"Input to mxArrayTovtkGraph() has multiple fields, cannot convert to vtkGraph");
    }

  if( mxGetNumberOfDimensions(mxa) != 2 )
    {
    vtkGenericWarningMacro(<<"Input to mxArrayTovtkGraph() does not have two dimensions, cannot convert to vtkGraph");
    }

  nr = mxGetM(mxa);
  nc = mxGetN(mxa);

  if( nr != nc )
    {
    vtkGenericWarningMacro(<<"Input to mxArrayTovtkGraph() is not square, cannot convert to vtkGraph");
    }

// Check input matrix for symmetry, if symmetric create an instance vtkMutableUndirectedGraph()

  for(i=0;i<nr;i++)
    {
    for(j=0;j<nc;j++)
      {
      if( mxArrayGetValue(i,j,mxa) != mxArrayGetValue(j,i,mxa) )
        {
        isDirected = true;
        break;
        }
      }
    if(isDirected)
      break;
    }

  if(isDirected)
    {
    vtkMutableDirectedGraph* dg = vtkMutableDirectedGraph::New();

    for(i=0;i<nr;i++)
      {
      dg->AddVertex();
      }

    for(i=0;i<nr;i++)
      {
      for(j=0;j<nc;j++)
        {
        for(k=0;k<mxArrayGetValue(i,j,mxa);k++)
          {
          dg->AddEdge(j,i);
          }
        }
      }

    this->vdoc->AddItem(dg);
    dg->Delete();
    return(dg);
    }
  else
    {
    vtkMutableUndirectedGraph* ug = vtkMutableUndirectedGraph::New();

    for(i=0;i<nr;i++)
      {
      ug->AddVertex();
      }

    for(i=0;i<nr;i++)
      {
      for(j=0;j<=i;j++)
        {
        for(k=0;k<mxArrayGetValue(i,j,mxa);k++)
          {
          ug->AddEdge(i,j);
          }
        }
      }
    this->vdoc->AddItem(ug);
    ug->Delete();
    return(ug);
    }
}

void vtkMatlabMexAdapter::PrintSelf(ostream& os, vtkIndent indent)
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
