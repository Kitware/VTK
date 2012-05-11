/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCompositer.h"
#include "vtkObjectFactory.h"
#include "vtkToolkits.h"
#include "vtkDataArray.h"
#include "vtkFloatArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkMultiProcessController.h"

vtkStandardNewMacro(vtkCompositer);

//-------------------------------------------------------------------------
vtkCompositer::vtkCompositer()
{
  this->Controller = vtkMultiProcessController::GetGlobalController();
  this->NumberOfProcesses = 1;
  if (this->Controller)
    {
    this->Controller->Register(this);
    this->NumberOfProcesses = this->Controller->GetNumberOfProcesses();
    }
}

//-------------------------------------------------------------------------
vtkCompositer::~vtkCompositer()
{
  this->SetController(NULL);
}


//-------------------------------------------------------------------------
void vtkCompositer::SetController(vtkMultiProcessController *mpc)
{
  if (this->Controller == mpc)
    {
    return;
    }
  if (mpc)
    {
    mpc->Register(this);
    this->NumberOfProcesses = mpc->GetNumberOfProcesses();
    }
  if (this->Controller)
    {
    this->Controller->UnRegister(this);
    }
  this->Controller = mpc;
}

//-------------------------------------------------------------------------
void vtkCompositer::CompositeBuffer(vtkDataArray *pBuf, vtkFloatArray *zBuf,
                                    vtkDataArray *pTmp, vtkFloatArray *zTmp)
{
  (void)pBuf;
  (void)zBuf;
  (void)pTmp;
  (void)zTmp;
}

//-------------------------------------------------------------------------
void vtkCompositer::ResizeFloatArray(vtkFloatArray* fa, int numComp,
                                     vtkIdType size)
{
  fa->SetNumberOfComponents(numComp);

#ifdef MPIPROALLOC
  vtkIdType fa_size = fa->GetSize();
  if ( fa_size < size*numComp )
    {
    float* ptr = fa->GetPointer(0);
    if (ptr)
      {
      MPI_Free_mem(ptr);
      }
    char* tptr;
    MPI_Alloc_mem(size*numComp*sizeof(float), NULL, &tptr);
    ptr = (float*)tptr;
    fa->SetArray(ptr, size*numComp, 1);
    }
  else
    {
    fa->SetNumberOfTuples(size);
    }
#else
  fa->SetNumberOfTuples(size);
#endif
}

void vtkCompositer::ResizeUnsignedCharArray(vtkUnsignedCharArray* uca,
                                            int numComp, vtkIdType size)
{
  uca->SetNumberOfComponents(numComp);
#ifdef MPIPROALLOC
  vtkIdType uca_size = uca->GetSize();

  if ( uca_size < size*numComp )
    {
    unsigned char* ptr = uca->GetPointer(0);
    if (ptr)
      {
      MPI_Free_mem(ptr);
      }
    char* tptr;
    MPI_Alloc_mem(size*numComp*sizeof(unsigned char), NULL, &tptr);
    ptr = (unsigned char*)tptr;
    uca->SetArray(ptr, size*numComp, 1);
    }
  else
    {
    uca->SetNumberOfTuples(size);
    }
#else
  uca->SetNumberOfTuples(size);
#endif
}

void vtkCompositer::DeleteArray(vtkDataArray* da)
{
#ifdef MPIPROALLOC
  void* ptr = da->GetVoidPointer(0);
  if (ptr)
    {
    MPI_Free_mem(ptr);
    }
#endif
  da->Delete();
}

//-------------------------------------------------------------------------
void vtkCompositer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: (" << this->Controller << ")\n";
  os << indent << "NumberOfProcesses: " << this->NumberOfProcesses << endl;
}



