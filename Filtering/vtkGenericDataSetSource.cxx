/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericDataSetSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGenericDataSetSource.h"

#include "vtkObjectFactory.h"
#include "vtkGenericDataSet.h"

#if VTK_MAJOR_VERSION>4 || (VTK_MAJOR_VERSION==4 && VTK_MINOR_VERSION>4)
#include "vtkInformation.h"
#endif

vtkCxxRevisionMacro(vtkGenericDataSetSource, "1.3");

//----------------------------------------------------------------------------
vtkGenericDataSetSource::vtkGenericDataSetSource()
{
#if VTK_MAJOR_VERSION>4 || (VTK_MAJOR_VERSION==4 && VTK_MINOR_VERSION>4)
  this->SetNumberOfOutputPorts(1);
#endif
  
  // We can create abtract data set. to be postpone in the concrete sources.
  //  this->vtkSource::SetNthOutput(0, vtkGenericDataSet::New());
}

//----------------------------------------------------------------------------
vtkGenericDataSet *vtkGenericDataSetSource::GetOutput()
{
  if (this->NumberOfOutputs < 1)
    {
    return NULL;
    }
  
  return static_cast<vtkGenericDataSet *>(this->Outputs[0]);
}

//----------------------------------------------------------------------------
vtkGenericDataSet *vtkGenericDataSetSource::GetOutput(int i)
{
  if ( (i<0) || ( i>=this->NumberOfOutputs) )
    {
    return NULL;
    }
  
  return static_cast<vtkGenericDataSet *>(this->Outputs[i]);
}

//----------------------------------------------------------------------------
void vtkGenericDataSetSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
#if VTK_MAJOR_VERSION>4 || (VTK_MAJOR_VERSION==4 && VTK_MINOR_VERSION>4)
//----------------------------------------------------------------------------
int vtkGenericDataSetSource::FillOutputPortInformation(int port,
                                                       vtkInformation* info)
{
  if(!this->Superclass::FillOutputPortInformation(port, info))
    {
    return 0;
    }
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkGenericDataSet");
  return 1;
}
#endif
