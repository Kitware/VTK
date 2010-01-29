/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractPolyDataReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAbstractPolyDataReader.h"

#include "vtkByteSwap.h"
#include "vtkDataArray.h"
#include "vtkPolyData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"

#include <sys/stat.h>

vtkCxxRevisionMacro(vtkAbstractPolyDataReader, "1.1");
vtkStandardNewMacro(vtkAbstractPolyDataReader);

#ifdef read
#undef read
#endif

#ifdef close
#undef close
#endif

//----------------------------------------------------------------------------
vtkAbstractPolyDataReader::vtkAbstractPolyDataReader()
{
  this->FileName = NULL;
}

//----------------------------------------------------------------------------
vtkAbstractPolyDataReader::~vtkAbstractPolyDataReader()
{
}


//----------------------------------------------------------------------------
void vtkAbstractPolyDataReader::PrintSelf(ostream& os, vtkIndent indent)
{
  int idx;
  
  this->Superclass::PrintSelf(os,indent);

  // this->File, this->Colors need not be printed  
  os << indent << "FileName: " <<
    (this->FileName ? this->FileName : "(none)") << "\n";
}

#if 0
//----------------------------------------------------------------------------
template <class T>
unsigned long vtkAbstractPolyDataReaderGetSize(T*)
{
  return sizeof(T);
}
#endif
