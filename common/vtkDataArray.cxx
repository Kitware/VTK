/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataArray.cxx
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
#include "vtkDataArray.h"
#include "vtkFloatArray.h"

// Construct object with default tuple dimension (number of components) of 1.
vtkDataArray::vtkDataArray(int numComp)
{
  this->Size = 0;
  this->MaxId = -1;
  this->Extend = 1000;

  this->NumberOfComponents = (numComp < 1 ? 1 : numComp);
}

void vtkDataArray::DeepCopy(vtkDataArray& da)
{
  if ( this != &da )
    {
    int numTuples = da.GetNumberOfTuples();
    this->NumberOfComponents = da.NumberOfComponents;
    this->SetNumberOfTuples(numTuples);

    for (int i=0; i < numTuples; i++)
      {
      this->SetTuple(i, da.GetTuple(i));
      }
    }
}

// These can be overridden for more efficiency
float vtkDataArray::GetComponent(const int i, const int j)
{
  float *tuple=new float[this->NumberOfComponents], c;

  this->GetTuple(i,tuple);
  c =  tuple[j];
  delete [] tuple;

  return c;
}

void vtkDataArray::SetComponent(const int i, const int j, const float c)
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

void vtkDataArray::InsertComponent(const int i, const int j, const float c)
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

void vtkDataArray::GetData(int tupleMin, int tupleMax, int compMin, int compMax, 
			   vtkFloatArray &data)
{
  int i, j;
  int numComp=this->GetNumberOfComponents();
  float *tuple=new float[numComp];
  float *ptr=data.WritePointer(0,(tupleMax-tupleMin+1)*(compMax-compMin+1));
  
  for (j=tupleMin; j <= tupleMax; j++)
    {
    this->GetTuple(j,tuple);
    for (i=compMin; i <= compMax; i++)
      {
      *ptr++ = tuple[i];
      }
    }
}

void vtkDataArray::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkReferenceCount::PrintSelf(os,indent);

  os << indent << "Number Of Components: " << this->NumberOfComponents << "\n";
  os << indent << "Number Of Tuples: " << this->GetNumberOfTuples() << "\n";
  os << indent << "Size: " << this->Size << "\n";
  os << indent << "MaxId: " << this->MaxId << "\n";
  os << indent << "Extend size: " << this->Extend << "\n";
}
