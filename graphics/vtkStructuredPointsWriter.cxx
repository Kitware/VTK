/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredPointsWriter.cxx
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
#include "vtkStructuredPointsWriter.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkStructuredPointsWriter* vtkStructuredPointsWriter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkStructuredPointsWriter");
  if(ret)
    {
    return (vtkStructuredPointsWriter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkStructuredPointsWriter;
}




//----------------------------------------------------------------------------
// Specify the input data or filter.
void vtkStructuredPointsWriter::SetInput(vtkStructuredPoints *input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
vtkStructuredPoints *vtkStructuredPointsWriter::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return (vtkStructuredPoints *)(this->Inputs[0]);
}


void vtkStructuredPointsWriter::WriteData()
{
  ostream *fp;
  vtkStructuredPoints *input=this->GetInput();
  int dim[3];
  float spacing[3], origin[3];

  vtkDebugMacro(<<"Writing vtk structured points...");

  if ( !(fp=this->OpenVTKFile()) || !this->WriteHeader(fp) )
      {
      return;
      }
  //
  // Write structured points specific stuff
  //
  *fp << "DATASET STRUCTURED_POINTS\n";

  input->GetDimensions(dim);
  *fp << "DIMENSIONS " << dim[0] << " " << dim[1] << " " << dim[2] << "\n";

  input->GetSpacing(spacing);
  *fp << "SPACING " << spacing[0] << " " << spacing[1] << " " << spacing[2] << "\n";

  input->GetOrigin(origin);
  *fp << "ORIGIN " << origin[0] << " " << origin[1] << " " << origin[2] << "\n";

  this->WriteCellData(fp, input);
  this->WritePointData(fp, input);

  this->CloseVTKFile(fp);
}

void vtkStructuredPointsWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataWriter::PrintSelf(os,indent);
}
