/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractIterator.txx
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

#ifndef __vtkAbstractIterator_txx
#define __vtkAbstractIterator_txx

#include "vtkAbstractIterator.h"
#include "vtkDebugLeaks.h"

template<class KeyType, class DataType>
void vtkAbstractIterator<KeyType,DataType>::Register()
{
  this->ReferenceCount++;
}

template<class KeyType, class DataType>
void vtkAbstractIterator<KeyType,DataType>::UnRegister()
{
  if (--this->ReferenceCount <= 0)
    {
    delete this;
    }
}

template<class KeyType, class DataType>
vtkAbstractIterator<KeyType,DataType>::vtkAbstractIterator() 
{ 
  this->ReferenceCount = 1;
  this->Container = 0;
}

template<class KeyType, class DataType>
vtkAbstractIterator<KeyType,DataType>::~vtkAbstractIterator() 
{
  this->SetContainer(0);
}

template<class KeyType, class DataType>
void vtkAbstractIterator<KeyType,DataType>::SetContainer(vtkContainer* container)
{
  if ( this->Container == container )
    {
    return;
    }
  if ( this->Container )
    {
    this->Container->UnRegister(0);
    this->Container = 0;
    }
  this->Container = container;
  if ( this->Container )
    {
    this->Container->Register(0);
    }
}


#if defined ( _MSC_VER )
template <class KeyType,class DataType>
vtkAbstractIterator<KeyType,DataType>::vtkAbstractIterator(const vtkAbstractIterator<KeyType,DataType>&){}
template <class KeyType,class DataType>
void vtkAbstractIterator<KeyType,DataType>::operator=(const vtkAbstractIterator<KeyType,DataType>&){}
#endif

#endif
