/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataArray.cxx
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
#include "vtkDataArray.h"
#include "vtkBitArray.h"
#include "vtkCharArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkShortArray.h"
#include "vtkUnsignedShortArray.h"
#include "vtkIntArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkLongArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkLookupTable.h"
#include "vtkCriticalSection.h"
#include "vtkIdList.h"

unsigned long vtkDataArray::ArrayNamePostfix = 0;

static vtkSimpleCriticalSection DataArrayCritSec;

// Construct object with default tuple dimension (number of components) of 1.
vtkDataArray::vtkDataArray(vtkIdType numComp)
{
  this->Size = 0;
  this->MaxId = -1;
  this->LookupTable = NULL;

  this->NumberOfComponents = (numComp < 1 ? 1 : numComp);
  this->Name = 0;
  DataArrayCritSec.Lock();
  ostrstream buf;
  buf << "Array_";
  buf << vtkDataArray::ArrayNamePostfix << ends; 
  vtkDataArray::ArrayNamePostfix++;
  this->SetName(buf.str());
  delete[] buf.str();
  DataArrayCritSec.Unlock();
}

vtkDataArray::~vtkDataArray()
{
  if ( this->LookupTable )
    {
    this->LookupTable->Delete();
    }
  delete[] this->Name;
}

void vtkDataArray::SetName(const char* name)
{
  if (!name)
    {
    vtkWarningMacro("Array name can not be NULL.");
    return;
    }
  delete[] this->Name;
  this->Name = 0;
  int size = strlen(name);
  this->Name = new char[size+1];
  strcpy(this->Name, name);
}

const char* vtkDataArray::GetName()
{
  return this->Name;
}

void vtkDataArray::DeepCopy(vtkDataArray *da)
{
  if ( this != da )
    {
    int numTuples = da->GetNumberOfTuples();
    this->NumberOfComponents = da->NumberOfComponents;
    this->SetNumberOfTuples(numTuples);

    for (int i=0; i < numTuples; i++)
      {
      this->SetTuple(i, da->GetTuple(i));
      }
    }
}

// These can be overridden for more efficiency
float vtkDataArray::GetComponent(const vtkIdType i, const int j)
{
  float *tuple=new float[this->NumberOfComponents], c;

  this->GetTuple(i,tuple);
  c =  tuple[j];
  delete [] tuple;

  return c;
}

void vtkDataArray::SetComponent(const vtkIdType i, const int j, const float c)
{
  float *tuple=new float[this->NumberOfComponents];

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

void vtkDataArray::InsertComponent(const vtkIdType i, const int j,
                                   const float c)
{
  float *tuple=new float[this->NumberOfComponents];

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

void vtkDataArray::GetData(vtkIdType tupleMin, vtkIdType tupleMax, int compMin,
			   int compMax, vtkFloatArray* data)
{
  int i, j;
  int numComp=this->GetNumberOfComponents();
  float *tuple=new float[numComp];
  float *ptr=data->WritePointer(0,(tupleMax-tupleMin+1)*(compMax-compMin+1));
  
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
  this->LookupTable->Register(this);
}

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

// default double behaviour
void vtkDataArray::GetTuple(const vtkIdType i, double * tuple)
{
  int c;
  int numComp=this->GetNumberOfComponents();
  float *ftuple=new float[numComp];
  this->GetTuple(i,ftuple);
  for (c = 0; c < numComp;  c++)
    {
    tuple[c] = ftuple[c];
    }
  delete [] ftuple;
}

void vtkDataArray::SetTuple(const vtkIdType i, const double * tuple)
{
  int c;
  int numComp=this->GetNumberOfComponents();
  float *ftuple=new float[numComp];
  for (c = 0; c < numComp;  c++)
    {
    ftuple[c] = (float)(tuple[c]);
    }
  this->SetTuple(i,ftuple);
  delete [] ftuple;
}

void vtkDataArray::InsertTuple(const vtkIdType i, const double * tuple)
{
  int c;
  int numComp=this->GetNumberOfComponents();
  float *ftuple=new float[numComp];
  for (c = 0; c < numComp;  c++)
    {
    ftuple[c] = (float)(tuple[c]);
    }
  this->InsertTuple(i,ftuple);
  delete [] ftuple;
}

vtkIdType vtkDataArray::InsertNextTuple(const double * tuple)
{
  int c;
  int numComp=this->GetNumberOfComponents();
  float *ftuple=new float[numComp];
  for (c = 0; c < numComp;  c++)
    {
    ftuple[c] = (float)(tuple[c]);
    }
  int ret = this->InsertNextTuple(ftuple);
  delete [] ftuple;
  return ret;
}

unsigned long vtkDataArray::GetActualMemorySize()
{
  unsigned long numPrims;
  float size = 0.0;
  numPrims = this->GetNumberOfTuples() * this->GetNumberOfComponents();

  switch (this->GetDataType())
    {
    case VTK_BIT:
      size = (float)sizeof(char)/8.0;
      break;

    case VTK_CHAR:
      size = (float)sizeof(char);
      break;

    case VTK_UNSIGNED_CHAR:
      size = (float)sizeof(unsigned char);
      break;

    case VTK_SHORT:
      size = (float)sizeof(short);
      break;

    case VTK_UNSIGNED_SHORT:
      size = (float)sizeof(unsigned short);
      break;

    case VTK_INT:
      size = (float)sizeof(int);
      break;

    case VTK_UNSIGNED_INT:
      size = (float)sizeof(unsigned int);
      break;

    case VTK_LONG:
      size = (float)sizeof(long);
      break;

    case VTK_UNSIGNED_LONG:
      size = (float)sizeof(unsigned long);
      break;

    case VTK_FLOAT:
      size = (float)sizeof(float);
      break;

    case VTK_DOUBLE:
      size = (float)sizeof(double);
      break;

    case VTK_ID_TYPE:
      size = (float)sizeof(vtkIdType);
      break;

    default:
      vtkErrorMacro(<<"Unsupported data type!");
    }

  return (unsigned long)ceil((size * numPrims)/1000.0); //kilobytes
}

vtkDataArray* vtkDataArray::CreateDataArray(int dataType)
{
  switch (dataType)
    {
    case VTK_BIT:
      return vtkBitArray::New();

    case VTK_CHAR:
      return vtkCharArray::New();

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

    case VTK_FLOAT:
      return vtkFloatArray::New();

    case VTK_DOUBLE:
      return vtkDoubleArray::New();

    case VTK_ID_TYPE:
      return vtkIdTypeArray::New();

    default:
      vtkGenericWarningMacro(<<"Unsupported data type! Setting to VTK_FLOAT");
      return vtkFloatArray::New();
    }
}

template <class IT, class OT>
static void CopyTuples(IT* input, OT* output, int nComp, vtkIdList* ptIds )
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

template <class IT>
static void CopyTuples1(IT* input, vtkDataArray* output, vtkIdList* ptIds)
{
  switch (output->GetDataType())
    {
    vtkTemplateMacro4(CopyTuples, input, (VTK_TT *)output->GetVoidPointer(0), 
		      output->GetNumberOfComponents(), ptIds );

    default:
      vtkGenericWarningMacro(<<"Sanity check failed: Unsupported data type.");
      return;
    }
}

void vtkDataArray::GetTuples(vtkIdList *ptIds, vtkDataArray *da)
{

  if ((da->GetNumberOfComponents() != this->GetNumberOfComponents()))
    {
    vtkWarningMacro("Number of components for input and output do not match");
    return;
    }

  switch (this->GetDataType())
    {
    vtkTemplateMacro3(CopyTuples1, (VTK_TT *)this->GetVoidPointer(0), da,
		      ptIds );
    // This is not supported by the template macro.
    // Switch to using the float interface.
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
      vtkErrorMacro(<<"Sanity check failed: Unsupported data type.");
      return;
    }
}


void vtkDataArray::FillComponent(const int j, const float c)
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


void vtkDataArray::CopyComponent(const int j, vtkDataArray *from,
                                 const int fromComponent)
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

void vtkDataArray::GetRange(float range[2], int comp)
{
  float s;
  vtkIdType numTuples=this->GetNumberOfTuples();

  range[0] =  VTK_LARGE_FLOAT;
  range[1] =  -VTK_LARGE_FLOAT;
  for (vtkIdType i=0; i<numTuples; i++)
    {
    s = this->GetComponent(i,comp);
    if ( s < range[0] )
      {
      range[0] = s;
      }
    if ( s > range[1] )
      {
      range[1] = s;
      }
    }

}

void vtkDataArray::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

  os << indent << "Name: " << this->Name << "\n";
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
