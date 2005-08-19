/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataArray.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDataArray.h"
#include "vtkBitArray.h"
#include "vtkCharArray.h"
#include "vtkCriticalSection.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkIntArray.h"
#include "vtkIdTypeArray.h"
#include "vtkIdList.h"
#include "vtkLookupTable.h"
#include "vtkLongArray.h"
#include "vtkMath.h"
#include "vtkShortArray.h"
#include "vtkSignedCharArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedShortArray.h"

#if defined(VTK_TYPE_USE_LONG_LONG)
# include "vtkLongLongArray.h"
# include "vtkUnsignedLongLongArray.h"
#endif

#if defined(VTK_TYPE_USE___INT64)
# include "vtk__Int64Array.h"
# if defined(VTK_TYPE_CONVERT_UI64_TO_DOUBLE)
#  include "vtkUnsigned__Int64Array.h"
# endif
#endif

vtkCxxRevisionMacro(vtkDataArray, "1.69");

//----------------------------------------------------------------------------
// Construct object with default tuple dimension (number of components) of 1.
vtkDataArray::vtkDataArray(vtkIdType numComp)
{
  this->Range[0] = 0;
  this->Range[1] = 1;

  this->Size = 0;
  this->MaxId = -1;
  this->LookupTable = NULL;

  this->NumberOfComponents = (numComp < 1 ? 1 : numComp);
  this->Name = 0;
}

//----------------------------------------------------------------------------
vtkDataArray::~vtkDataArray()
{
  if ( this->LookupTable )
    {
    this->LookupTable->Delete();
    }
  this->SetName(0);
}

//----------------------------------------------------------------------------
template <class IT, class OT>
void vtkDeepCopyArrayOfDifferentType(IT *input, OT *output,
                                     int numTuples, int nComp)
{
  int i, j;
  for (i=0; i<numTuples; i++)
    {
    for (j=0; j<nComp; j++)
      {
      output[i*nComp+j] = static_cast<OT>(input[i*nComp+j]);
      }
    }
}

//----------------------------------------------------------------------------
template <class IT>
void vtkDeepCopySwitchOnOutput(IT *input, vtkDataArray *da,
                                      int numTuples, int nComp)
{
  void *output = da->GetVoidPointer(0);

  switch (da->GetDataType())
    {
    vtkTemplateMacro(
      vtkDeepCopyArrayOfDifferentType (input,
                                       (VTK_TT*)output,
                                       numTuples,
                                       nComp) );

    default:
      vtkGenericWarningMacro("Unsupported data type " << da->GetDataType()
                             <<"!");
    }
}

//----------------------------------------------------------------------------
//Normally subclasses will do this when the input and output type of the
//DeepCopy are the same. When they are not the same, then we use the
//templated code below.
void vtkDataArray::DeepCopy(vtkDataArray *da)
{
  // Match the behavior of the old AttributeData
  if ( da == NULL )
    {
    return;
    }

  if ( this != da )
    {
    int numTuples = da->GetNumberOfTuples();
    this->NumberOfComponents = da->NumberOfComponents;
    this->SetNumberOfTuples(numTuples);
    void *input=da->GetVoidPointer(0);

    switch (da->GetDataType())
      {
      vtkTemplateMacro(
        vtkDeepCopySwitchOnOutput((VTK_TT*)input,
                                  this,
                                  numTuples,
                                  this->NumberOfComponents));

      case VTK_BIT:
        {//bit not supported, using generic double API
        for (int i=0; i < numTuples; i++)
          {
          this->SetTuple(i, da->GetTuple(i));
          }
        break;
        }

      default:
        vtkErrorMacro("Unsupported data type " << da->GetDataType() << "!");
      }

    this->SetLookupTable(0);
    if (da->LookupTable)
      {
      this->LookupTable = da->LookupTable->NewInstance();
      this->LookupTable->DeepCopy(da->LookupTable);
      }
    }
}

//----------------------------------------------------------------------------
// These can be overridden for more efficiency
double vtkDataArray::GetComponent(vtkIdType i, int j)
{
  double *tuple=new double[this->NumberOfComponents], c;

  this->GetTuple(i,tuple);
  c =  tuple[j];
  delete [] tuple;

  return c;
}

//----------------------------------------------------------------------------
void vtkDataArray::SetComponent(vtkIdType i, int j, double c)
{
  double *tuple=new double[this->NumberOfComponents];

  if ( i < this->GetNumberOfTuples() )
    {
    this->GetTuple(i,tuple);
    }
  else
    {
    for (int k=0; k<this->NumberOfComponents; k++)
      {
      tuple[k] = 0.0;
      }
    }

  tuple[j] = c;
  this->SetTuple(i,tuple);

  delete [] tuple;
}

//----------------------------------------------------------------------------
void vtkDataArray::InsertComponent(vtkIdType i, int j, double c)
{
  double *tuple=new double[this->NumberOfComponents];

  if ( i < this->GetNumberOfTuples() )
    {
    this->GetTuple(i,tuple);
    }
  else
    {
    for (int k=0; k<this->NumberOfComponents; k++)
      {
      tuple[k] = 0.0;
      }
    }

  tuple[j] = c;
  this->InsertTuple(i,tuple);

  delete [] tuple;
}

//----------------------------------------------------------------------------
void vtkDataArray::GetData(vtkIdType tupleMin, vtkIdType tupleMax, int compMin,
                           int compMax, vtkDoubleArray* data)
{
  int i, j;
  int numComp=this->GetNumberOfComponents();
  double *tuple=new double[numComp];
  double *ptr=data->WritePointer(0,(tupleMax-tupleMin+1)*(compMax-compMin+1));
  
  for (j=tupleMin; j <= tupleMax; j++)
    {
    this->GetTuple(j,tuple);
    for (i=compMin; i <= compMax; i++)
      {
      *ptr++ = tuple[i];
      }
    }
  delete [] tuple;
}

//----------------------------------------------------------------------------
void vtkDataArray::CreateDefaultLookupTable()
{
  if ( this->LookupTable )
    {
    this->LookupTable->UnRegister(this);
    }
  this->LookupTable = vtkLookupTable::New();
  // make sure it is built 
  // otherwise problems with InsertScalar trying to map through 
  // non built lut
  this->LookupTable->Build();
}

//----------------------------------------------------------------------------
void vtkDataArray::SetLookupTable(vtkLookupTable* lut)
{
  if ( this->LookupTable != lut ) 
    {
    if ( this->LookupTable )
      {
      this->LookupTable->UnRegister(this);
      }
    this->LookupTable = lut;
    this->LookupTable->Register(this);
    this->Modified();
    }
}

//----------------------------------------------------------------------------
double* vtkDataArray::GetTupleN(vtkIdType i, int n)
{
  int numComp = this->GetNumberOfComponents();
  if (numComp != n)
    {
    vtkErrorMacro("The number of components do not match the number requested: "
                  << numComp << " != " << n);
    }
  return this->GetTuple(i);
}

//----------------------------------------------------------------------------
double vtkDataArray::GetTuple1(vtkIdType i)
{
  int numComp = this->GetNumberOfComponents();
  if (numComp != 1)
    {
    vtkErrorMacro("The number of components do not match the number requested: "
                  << numComp << " != 1");
    }
  return *(this->GetTuple(i));
}

//----------------------------------------------------------------------------
double* vtkDataArray::GetTuple2(vtkIdType i)
{
  return this->GetTupleN(i, 2);
}
//----------------------------------------------------------------------------
double* vtkDataArray::GetTuple3(vtkIdType i)
{
  return this->GetTupleN(i, 3);
}
//----------------------------------------------------------------------------
double* vtkDataArray::GetTuple4(vtkIdType i)
{
  return this->GetTupleN(i, 4);
}
//----------------------------------------------------------------------------
double* vtkDataArray::GetTuple9(vtkIdType i)
{
  return this->GetTupleN(i, 9);
}

//----------------------------------------------------------------------------
void vtkDataArray::SetTuple1(vtkIdType i, double value)
{
  int numComp = this->GetNumberOfComponents();
  if (numComp != 1)
    {
    vtkErrorMacro("The number of components do not match the number requested: "
                  << numComp << " != 1");
    }
  this->SetTuple(i, &value);
}
//----------------------------------------------------------------------------
void vtkDataArray::SetTuple2(vtkIdType i, double val0, double val1)
{
  double tuple[2];
  int numComp = this->GetNumberOfComponents();
  if (numComp != 2)
    {
    vtkErrorMacro("The number of components do not match the number requested: "
                  << numComp << " != 2");
    }
  tuple[0] = val0;
  tuple[1] = val1;
  this->SetTuple(i, tuple);
}
//----------------------------------------------------------------------------
void vtkDataArray::SetTuple3(vtkIdType i, double val0, double val1, 
                             double val2)
{
  double tuple[3];
  int numComp = this->GetNumberOfComponents();
  if (numComp != 3)
    {
    vtkErrorMacro("The number of components do not match the number requested: "
                  << numComp << " != 3");
    }
  tuple[0] = val0;
  tuple[1] = val1;
  tuple[2] = val2;
  this->SetTuple(i, tuple);
}
//----------------------------------------------------------------------------
void vtkDataArray::SetTuple4(vtkIdType i, double val0, double val1, 
                             double val2, double val3)
{
  double tuple[4];
  int numComp = this->GetNumberOfComponents();
  if (numComp != 4)
    {
    vtkErrorMacro("The number of components do not match the number requested: "
                  << numComp << " != 4");
    }
  tuple[0] = val0;
  tuple[1] = val1;
  tuple[2] = val2;
  tuple[3] = val3;
  this->SetTuple(i, tuple);
}
//----------------------------------------------------------------------------
void vtkDataArray::SetTuple9(vtkIdType i, double val0, double val1, 
                             double val2, double val3, double val4, 
                             double val5, double val6, double val7, double val8)
{
  double tuple[9];
  int numComp = this->GetNumberOfComponents();
  if (numComp != 9)
    {
    vtkErrorMacro("The number of components do not match the number requested: "
                  << numComp << " != 9");
    }
  tuple[0] = val0;
  tuple[1] = val1;
  tuple[2] = val2;
  tuple[3] = val3;
  tuple[4] = val4;
  tuple[5] = val5;
  tuple[6] = val6;
  tuple[7] = val7;
  tuple[8] = val8;
  this->SetTuple(i, tuple);
}

//----------------------------------------------------------------------------
void vtkDataArray::InsertTuple1(vtkIdType i, double value)
{
  int numComp = this->GetNumberOfComponents();
  if (numComp != 1)
    {
    vtkErrorMacro("The number of components do not match the number requested: "
                  << numComp << " != 1");
    }
  this->InsertTuple(i, &value);
}
//----------------------------------------------------------------------------
void vtkDataArray::InsertTuple2(vtkIdType i, double val0, double val1)
{
  double tuple[2];
  int numComp = this->GetNumberOfComponents();
  if (numComp != 2)
    {
    vtkErrorMacro("The number of components do not match the number requested: "
                  << numComp << " != 2");
    }
  tuple[0] = val0;
  tuple[1] = val1;
  this->InsertTuple(i, tuple);
}
//----------------------------------------------------------------------------
void vtkDataArray::InsertTuple3(vtkIdType i, double val0, double val1, 
                                double val2)
{
  double tuple[3];
  int numComp = this->GetNumberOfComponents();
  if (numComp != 3)
    {
    vtkErrorMacro("The number of components do not match the number requested: "
                  << numComp << " != 3");
    }
  tuple[0] = val0;
  tuple[1] = val1;
  tuple[2] = val2;
  this->InsertTuple(i, tuple);
}
//----------------------------------------------------------------------------
void vtkDataArray::InsertTuple4(vtkIdType i, double val0, double val1, 
                                double val2, double val3)
{
  double tuple[4];
  int numComp = this->GetNumberOfComponents();
  if (numComp != 4)
    {
    vtkErrorMacro("The number of components do not match the number requested: "
                  << numComp << " != 4");
    }
  tuple[0] = val0;
  tuple[1] = val1;
  tuple[2] = val2;
  tuple[3] = val3;
  this->InsertTuple(i, tuple);
}
//----------------------------------------------------------------------------
void vtkDataArray::InsertTuple9(vtkIdType i, double val0, double val1, 
                                double val2,  double val3, double val4, 
                                double val5, double val6,double val7, double val8)
{
  double tuple[9];
  int numComp = this->GetNumberOfComponents();
  if (numComp != 9)
    {
    vtkErrorMacro("The number of components do not match the number requested: "
                  << numComp << " != 9");
    }
  tuple[0] = val0;
  tuple[1] = val1;
  tuple[2] = val2;
  tuple[3] = val3;
  tuple[4] = val4;
  tuple[5] = val5;
  tuple[6] = val6;
  tuple[7] = val7;
  tuple[8] = val8;
  this->InsertTuple(i, tuple);
}

//----------------------------------------------------------------------------
void vtkDataArray::InsertNextTuple1(double value)
{
  int numComp = this->GetNumberOfComponents();
  if (numComp != 1)
    {
    vtkErrorMacro("The number of components do not match the number requested: "
                  << numComp << " != 1");
    }
  this->InsertNextTuple(&value);
}
//----------------------------------------------------------------------------
void vtkDataArray::InsertNextTuple2(double val0, double val1)
{
  double tuple[2];
  int numComp = this->GetNumberOfComponents();
  if (numComp != 2)
    {
    vtkErrorMacro("The number of components do not match the number requested: "
                  << numComp << " != 2");
    }
  tuple[0] = val0;
  tuple[1] = val1;
  this->InsertNextTuple(tuple);
}
//----------------------------------------------------------------------------
void vtkDataArray::InsertNextTuple3(double val0, double val1, 
                                    double val2)
{
  double tuple[3];
  int numComp = this->GetNumberOfComponents();
  if (numComp != 3)
    {
    vtkErrorMacro("The number of components do not match the number requested: "
                  << numComp << " != 3");
    }
  tuple[0] = val0;
  tuple[1] = val1;
  tuple[2] = val2;
  this->InsertNextTuple(tuple);
}
//----------------------------------------------------------------------------
void vtkDataArray::InsertNextTuple4(double val0, double val1, 
                                    double val2, double val3)
{
  double tuple[4];
  int numComp = this->GetNumberOfComponents();
  if (numComp != 4)
    {
    vtkErrorMacro("The number of components do not match the number requested: "
                  << numComp << " != 4");
    }
  tuple[0] = val0;
  tuple[1] = val1;
  tuple[2] = val2;
  tuple[3] = val3;
  this->InsertNextTuple(tuple);
}
//----------------------------------------------------------------------------
void vtkDataArray::InsertNextTuple9(double val0, double val1, 
                                    double val2,  double val3, double val4, 
                                    double val5, double val6,double val7, 
                                    double val8)
{
  double tuple[9];
  int numComp = this->GetNumberOfComponents();
  if (numComp != 9)
    {
    vtkErrorMacro("The number of components do not match the number requested: "
                  << numComp << " != 9");
    }
  tuple[0] = val0;
  tuple[1] = val1;
  tuple[2] = val2;
  tuple[3] = val3;
  tuple[4] = val4;
  tuple[5] = val5;
  tuple[6] = val6;
  tuple[7] = val7;
  tuple[8] = val8;
  this->InsertNextTuple(tuple);
}

//----------------------------------------------------------------------------
template <class T>
unsigned long vtkDataArrayGetDataTypeSize(T*)
{
  return sizeof(T);
}

//----------------------------------------------------------------------------
unsigned long vtkDataArray::GetDataTypeSize(int type)
{
  switch (type)
    {
    vtkTemplateMacro(
      return vtkDataArrayGetDataTypeSize(static_cast<VTK_TT*>(0))
      );
    case VTK_BIT:
      return 1;
      break;
    default:
      vtkGenericWarningMacro("Unsupported data type " << type << "!");
    }
  return 1;
}

//----------------------------------------------------------------------------
unsigned long vtkDataArray::GetActualMemorySize()
{
  unsigned long numPrims;
  double size = 0.0;
  // The allocated array may be larger than the number of primatives used.
  //numPrims = this->GetNumberOfTuples() * this->GetNumberOfComponents();
  numPrims = this->GetSize();

  size = vtkDataArray::GetDataTypeSize(this->GetDataType());

  return (unsigned long)ceil((size * numPrims)/1000.0); //kilobytes
}

//----------------------------------------------------------------------------
vtkDataArray* vtkDataArray::CreateDataArray(int dataType)
{
  switch (dataType)
    {
    case VTK_BIT:
      return vtkBitArray::New();

    case VTK_CHAR:
      return vtkCharArray::New();

    case VTK_SIGNED_CHAR:
      return vtkSignedCharArray::New();

    case VTK_UNSIGNED_CHAR:
      return vtkUnsignedCharArray::New();

    case VTK_SHORT:
      return vtkShortArray::New();

    case VTK_UNSIGNED_SHORT:
      return vtkUnsignedShortArray::New();

    case VTK_INT:
      return vtkIntArray::New();

    case VTK_UNSIGNED_INT:
      return vtkUnsignedIntArray::New();

    case VTK_LONG:
      return vtkLongArray::New();

    case VTK_UNSIGNED_LONG:
      return vtkUnsignedLongArray::New();

#if defined(VTK_TYPE_USE_LONG_LONG)
    case VTK_LONG_LONG:
      return vtkLongLongArray::New();

    case VTK_UNSIGNED_LONG_LONG:
      return vtkUnsignedLongLongArray::New();
#endif

#if defined(VTK_TYPE_USE___INT64)
    case VTK___INT64:
      return vtk__Int64Array::New();
      break;

# if defined(VTK_TYPE_CONVERT_UI64_TO_DOUBLE)
    case VTK_UNSIGNED___INT64:
      return vtkUnsigned__Int64Array::New();
      break;
# endif
#endif

    case VTK_FLOAT:
      return vtkFloatArray::New();

    case VTK_DOUBLE:
      return vtkDoubleArray::New();

    case VTK_ID_TYPE:
      return vtkIdTypeArray::New();

    default:
      vtkGenericWarningMacro("Unsupported data type " << dataType
                             << "! Setting to VTK_DOUBLE");
      return vtkDoubleArray::New();
    }
}

//----------------------------------------------------------------------------
template <class IT, class OT>
void vtkCopyTuples(IT* input, OT* output, int nComp, vtkIdList* ptIds )
{
  int i, j;
  int num=ptIds->GetNumberOfIds();
  for (i=0; i<num; i++)
    {
    for (j=0; j<nComp; j++)
      {
      output[i*nComp+j] = static_cast<OT>(input[ptIds->GetId(i)*nComp+j]);
      }
    }
}

//----------------------------------------------------------------------------
template <class IT>
void vtkCopyTuples1(IT* input, vtkDataArray* output, vtkIdList* ptIds)
{
  switch (output->GetDataType())
    {
    vtkTemplateMacro(vtkCopyTuples(input, 
                                   (VTK_TT *)output->GetVoidPointer(0), 
                                   output->GetNumberOfComponents(), ptIds) );

    default:
      vtkGenericWarningMacro("Sanity check failed: Unsupported data type "
                             << output->GetDataType() << ".");
      return;
    }
}

//----------------------------------------------------------------------------
void vtkDataArray::GetTuples(vtkIdList *ptIds, vtkDataArray *da)
{

  if ((da->GetNumberOfComponents() != this->GetNumberOfComponents()))
    {
    vtkWarningMacro("Number of components for input and output do not match");
    return;
    }

  switch (this->GetDataType())
    {
    vtkTemplateMacro(vtkCopyTuples1 ((VTK_TT *)this->GetVoidPointer(0), da,
                                     ptIds ));
    // This is not supported by the template macro.
    // Switch to using the double interface.
    case VTK_BIT:
      {
      vtkIdType num=ptIds->GetNumberOfIds();
      for (vtkIdType i=0; i<num; i++)
        {
        da->SetTuple(i,this->GetTuple(ptIds->GetId(i)));
        }
      }
      break;
    default:
      vtkErrorMacro("Sanity check failed: Unsupported data type "
                    << this->GetDataType() << ".");
      return;
    }
}

//----------------------------------------------------------------------------
template <class IT, class OT>
void vtkCopyTuples(IT* input, OT* output, int nComp, 
                   vtkIdType p1, vtkIdType p2)
{
  int i, j;
  int num=p2-p1+1;
  for (i=0; i<num; i++)
    {
    for (j=0; j<nComp; j++)
      {
      output[i*nComp+j] = static_cast<OT>(input[(p1+i)*nComp+j]);
      }
    }
}

//----------------------------------------------------------------------------
template <class IT>
void vtkCopyTuples1(IT* input, vtkDataArray* output, 
                    vtkIdType p1, vtkIdType p2)
{
  switch (output->GetDataType())
    {
    vtkTemplateMacro(vtkCopyTuples( input, 
                                    (VTK_TT *)output->GetVoidPointer(0), 
                                    output->GetNumberOfComponents(), p1, p2) );

    default:
      vtkGenericWarningMacro("Sanity check failed: Unsupported data type "
                             << output->GetDataType() << ".");
      return;
    }
}


//----------------------------------------------------------------------------
void vtkDataArray::GetTuples(vtkIdType p1, vtkIdType p2, vtkDataArray *da)
{

  if ((da->GetNumberOfComponents() != this->GetNumberOfComponents()))
    {
    vtkWarningMacro("Number of components for input and output do not match");
    return;
    }

  switch (this->GetDataType())
    {
    vtkTemplateMacro(vtkCopyTuples1( (VTK_TT *)this->GetVoidPointer(0), da,
                                     p1, p2 ) );
    // This is not supported by the template macro.
    // Switch to using the double interface.
    case VTK_BIT:
      {
      vtkIdType num=p2-p1+1;
      for (vtkIdType i=0; i<num; i++)
        {
        da->SetTuple(i,this->GetTuple(p1+i));
        }
      }
      break;
    default:
      vtkErrorMacro("Sanity check failed: Unsupported data type "
                    << this->GetDataType() << ".");
      return;
    }
}

//----------------------------------------------------------------------------
void vtkDataArray::FillComponent(int j, double c)
{
  if (j < 0 || j >= this->GetNumberOfComponents())
    {
    vtkErrorMacro(<< "Specified component " << j << " is not in [0, "
    << this->GetNumberOfComponents() << ")" );
    return;
    }
  
  vtkIdType i;

  for (i = 0; i < this->GetNumberOfTuples(); i++)
    {
    this->SetComponent(i, j, c);
    }
}


//----------------------------------------------------------------------------
void vtkDataArray::CopyComponent(int j, vtkDataArray *from,
                                 int fromComponent)
{
  if (this->GetNumberOfTuples() != from->GetNumberOfTuples())
    {
    vtkErrorMacro(<< "Number of tuples in 'from' ("
    << from->GetNumberOfTuples() << ") and 'to' ("
    << this->GetNumberOfTuples() << ") do not match.");
    return;
    }

  if (j < 0 || j >= this->GetNumberOfComponents())
    {
    vtkErrorMacro(<< "Specified component " << j << " in 'to' array is not in [0, "
    << this->GetNumberOfComponents() << ")" );
    return;
    }

  if (fromComponent < 0 || fromComponent >= from->GetNumberOfComponents())
    {
    vtkErrorMacro(<< "Specified component " << fromComponent << " in 'from' array is not in [0, "
    << from->GetNumberOfComponents() << ")" );
    return;
    }

  vtkIdType i;
  for (i = 0; i < this->GetNumberOfTuples(); i++)
    {
    this->SetComponent(i, j, from->GetComponent(i, fromComponent));
    }
}

//----------------------------------------------------------------------------
double vtkDataArray::GetMaxNorm()
{
  vtkIdType i;
  double norm, maxNorm;
  int nComponents = this->GetNumberOfComponents();

  maxNorm = 0.0;
  for (i=0; i<this->GetNumberOfTuples(); i++)
    {
    norm = vtkMath::Norm(this->GetTuple(i), nComponents);
    if ( norm > maxNorm )
      {
      maxNorm = norm;
      }
    }

  return maxNorm;
}

//----------------------------------------------------------------------------
void vtkDataArray::ComputeRange(int comp)
{
  double s,t;
  vtkIdType numTuples;

  if (comp < 0 && this->NumberOfComponents == 1)
    {
    comp = 0;
    }

  int idx = comp;
  idx = (idx<0)?(this->NumberOfComponents):(idx);
  
  if (idx >= VTK_MAXIMUM_NUMBER_OF_CACHED_COMPONENT_RANGES || 
       (this->GetMTime() > this->ComponentRangeComputeTime[idx]) )
    {
    numTuples=this->GetNumberOfTuples();
    this->Range[0] =  VTK_DOUBLE_MAX;
    this->Range[1] =  VTK_DOUBLE_MIN;

    for (vtkIdType i=0; i<numTuples; i++)
      {
      if (comp >= 0)
        {
        s = this->GetComponent(i,comp);
        }
      else
        { // Compute range of vector magnitude.
        s = 0.0;
        for (int j=0; j < this->NumberOfComponents; ++j)
          {
          t = this->GetComponent(i,j);
          s += t*t;
          }
        s = sqrt(s);
        }
      if ( s < this->Range[0] )
        {
        this->Range[0] = s;
        }
      if ( s > this->Range[1] )
        {
        this->Range[1] = s;
        }
      }
    if (idx < VTK_MAXIMUM_NUMBER_OF_CACHED_COMPONENT_RANGES)
      {
      this->ComponentRangeComputeTime[idx].Modified();
      this->ComponentRange[idx][0] = this->Range[0];
      this->ComponentRange[idx][1] = this->Range[1];
      }
    }
  else
    {
    this->Range[0] = this->ComponentRange[idx][0];
    this->Range[1] = this->ComponentRange[idx][1];    
    }
}

//----------------------------------------------------------------------------
void vtkDataArray::GetDataTypeRange(double range[2])
{
  vtkDataArray::GetDataTypeRange(this->GetDataType(), range);
}

//----------------------------------------------------------------------------
double vtkDataArray::GetDataTypeMin()
{
  return vtkDataArray::GetDataTypeMin(this->GetDataType());
}

//----------------------------------------------------------------------------
double vtkDataArray::GetDataTypeMax()
{
  return vtkDataArray::GetDataTypeMax(this->GetDataType());
}

//----------------------------------------------------------------------------
void vtkDataArray::GetDataTypeRange(int type, double range[2])
{
  range[0] = vtkDataArray::GetDataTypeMin(type);
  range[1] = vtkDataArray::GetDataTypeMax(type);
}

//----------------------------------------------------------------------------
double vtkDataArray::GetDataTypeMin(int type)
{
  switch (type)
    {
    case VTK_BIT:                return (double)VTK_BIT_MIN;
    case VTK_SIGNED_CHAR:        return (double)VTK_SIGNED_CHAR_MIN;
    case VTK_UNSIGNED_CHAR:      return (double)VTK_UNSIGNED_CHAR_MIN;
    case VTK_CHAR:               return (double)VTK_CHAR_MIN;
    case VTK_UNSIGNED_SHORT:     return (double)VTK_UNSIGNED_SHORT_MIN;
    case VTK_SHORT:              return (double)VTK_SHORT_MIN;
    case VTK_UNSIGNED_INT:       return (double)VTK_UNSIGNED_INT_MIN;
    case VTK_INT:                return (double)VTK_INT_MIN;
    case VTK_UNSIGNED_LONG:      return (double)VTK_UNSIGNED_LONG_MIN;
    case VTK_LONG:               return (double)VTK_LONG_MIN;
#if defined(VTK_TYPE_USE_LONG_LONG)
    case VTK_UNSIGNED_LONG_LONG: return (double)VTK_UNSIGNED_LONG_LONG_MIN;
    case VTK_LONG_LONG:          return (double)VTK_LONG_LONG_MIN;
#endif
#if defined(VTK_TYPE_USE___INT64)
    case VTK___INT64:            return (double)VTK___INT64_MIN;
# if defined(VTK_TYPE_CONVERT_UI64_TO_DOUBLE)
    case VTK_UNSIGNED___INT64:   return (double)VTK_UNSIGNED___INT64_MIN;
# endif
#endif
    case VTK_FLOAT:              return (double)VTK_FLOAT_MIN;
    case VTK_DOUBLE:             return (double)VTK_DOUBLE_MIN;
    default: return 0;
    }
}

//----------------------------------------------------------------------------
double vtkDataArray::GetDataTypeMax(int type)
{
  switch (type)
    {
    case VTK_BIT:                return (double)VTK_BIT_MAX;
    case VTK_SIGNED_CHAR:        return (double)VTK_SIGNED_CHAR_MAX;
    case VTK_UNSIGNED_CHAR:      return (double)VTK_UNSIGNED_CHAR_MAX;
    case VTK_CHAR:               return (double)VTK_CHAR_MAX;
    case VTK_UNSIGNED_SHORT:     return (double)VTK_UNSIGNED_SHORT_MAX;
    case VTK_SHORT:              return (double)VTK_SHORT_MAX;
    case VTK_UNSIGNED_INT:       return (double)VTK_UNSIGNED_INT_MAX;
    case VTK_INT:                return (double)VTK_INT_MAX;
    case VTK_UNSIGNED_LONG:      return (double)VTK_UNSIGNED_LONG_MAX;
    case VTK_LONG:               return (double)VTK_LONG_MAX;
#if defined(VTK_TYPE_USE_LONG_LONG)
    case VTK_UNSIGNED_LONG_LONG: return (double)VTK_UNSIGNED_LONG_LONG_MAX;
    case VTK_LONG_LONG:          return (double)VTK_LONG_LONG_MAX;
#endif
#if defined(VTK_TYPE_USE___INT64)
    case VTK___INT64:            return (double)VTK___INT64_MAX;
# if defined(VTK_TYPE_CONVERT_UI64_TO_DOUBLE)
    case VTK_UNSIGNED___INT64:   return (double)VTK_UNSIGNED___INT64_MAX;
# endif
#endif
    case VTK_FLOAT:              return (double)VTK_FLOAT_MAX;
    case VTK_DOUBLE:             return (double)VTK_DOUBLE_MAX;
    default: return 1;
    }
}

//----------------------------------------------------------------------------
void vtkDataArray::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  const char* name = this->GetName();
  if (name)
    {
    os << indent << "Name: " << name << "\n";
    }
  else
    {
    os << indent << "Name: (none)\n";
    }
  os << indent << "Number Of Components: " << this->NumberOfComponents << "\n";
  os << indent << "Number Of Tuples: " << this->GetNumberOfTuples() << "\n";
  os << indent << "Size: " << this->Size << "\n";
  os << indent << "MaxId: " << this->MaxId << "\n";
  if ( this->LookupTable )
    {
    os << indent << "Lookup Table:\n";
    this->LookupTable->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "LookupTable: (none)\n";
    }
}
