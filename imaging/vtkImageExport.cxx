/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageExport.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G. Gobbi who developed this class.


Copyright (c) 1993-1999 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include <ctype.h>
#include <string.h>
#include "vtkImageExport.h"



//----------------------------------------------------------------------------
vtkImageExport::vtkImageExport()
{
  this->ImageFlip = NULL;
  this->ImageLowerLeft = 1;
}



//----------------------------------------------------------------------------
vtkImageExport::~vtkImageExport()
{
  if (this->ImageFlip)
    {
    this->ImageFlip->UnRegister(this);
    this->ImageFlip = NULL;
    }
}


//----------------------------------------------------------------------------
void vtkImageExport::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkProcessObject::PrintSelf(os,indent);

  os << indent << "ImageLowerLeft: " 
     << (this->ImageLowerLeft ? "On\n" : "Off\n");
}

//----------------------------------------------------------------------------
void vtkImageExport::SetInput(vtkImageData *input)
{
  this->vtkProcessObject::SetInput(0, input);
}

//----------------------------------------------------------------------------
vtkImageData *vtkImageExport::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return (vtkImageData *)(this->Inputs[0]);
}

//----------------------------------------------------------------------------
int vtkImageExportGetDataTypeSize(int type)
{
  switch (type)
    {
    case VTK_VOID:
      return 0;
    case VTK_DOUBLE:
      return sizeof(double);
    case VTK_FLOAT:
      return sizeof(float);
    case VTK_LONG:
      return sizeof(long);
    case VTK_UNSIGNED_LONG:
      return sizeof(unsigned long);
    case VTK_INT:
      return sizeof(int);
    case VTK_UNSIGNED_INT:
      return sizeof(unsigned int);
    case VTK_SHORT:
      return sizeof(short);
    case VTK_UNSIGNED_SHORT:
      return sizeof(unsigned short); 
    case VTK_UNSIGNED_CHAR:
      return sizeof(unsigned char); 
    default:
      return 0; 
    }
}

//----------------------------------------------------------------------------
int vtkImageExport::GetDataMemorySize()
{
  this->GetInput()->UpdateInformation();
  int *extent = this->GetInput()->GetWholeExtent();

  int size = vtkImageExportGetDataTypeSize(this->GetInput()->GetScalarType());
  if (size == 0)
    {
    vtkErrorMacro(<< "GetDataMemorySize: Illegal ScalarType.");
    return 0; 
    }
  size *= this->GetInput()->GetNumberOfScalarComponents();
  size *= (extent[1] - extent[0] + 1);
  size *= (extent[3] - extent[2] + 1);
  size *= (extent[5] - extent[4] + 1);

  return size;
}


//----------------------------------------------------------------------------
void vtkImageExport::GetDataDimensions(int *dims)
{
  this->GetInput()->UpdateInformation();
  int *extent = this->GetInput()->GetWholeExtent();
  dims[0] = extent[1]-extent[0]+1;
  dims[1] = extent[3]-extent[2]+1;
  dims[2] = extent[5]-extent[4]+1;
}

//----------------------------------------------------------------------------
// Exports all the data from the input.
void vtkImageExport::Export(void *output)
{
  memcpy(output,this->GetPointerToData(),this->GetDataMemorySize());
}

//----------------------------------------------------------------------------
// Provides a valid pointer to the data (only valid until the next
// update, though)

void *vtkImageExport::GetPointerToData()
{
  // Error checking
  if ( this->GetInput() == NULL )
    {
    vtkErrorMacro(<<"Export: Please specify an input!");
    return 0;
    }

  vtkImageData *input = this->GetInput();

  // flip data if necessary
  if (this->ImageLowerLeft == 0)
    {
    if (this->ImageFlip == NULL)
      {
      this->ImageFlip = vtkImageFlip::New();
      this->ImageFlip->SetInput(input);
      this->ImageFlip->SetFilteredAxis(1);
      input = this->ImageFlip->GetOutput();
      }
    }
  else
    {
    if (this->ImageFlip)
      {
      this->ImageFlip->UnRegister(this);
      this->ImageFlip = NULL;
      }
    }

  if (this->GetDataMemorySize() > input->GetMemoryLimit())
    {
    input->SetMemoryLimit(this->GetDataMemorySize());
    }
  input->SetUpdateExtent(input->GetWholeExtent());
  input->ReleaseDataFlagOff();

  this->UpdateProgress(0.0);
  input->Update();
  this->UpdateProgress(1.0);

  return input->GetScalarPointer();
}
  
  





