/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVoxelWriter.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkVoxelWriter.hh"
#include "vtkBitScalars.hh"

vtkVoxelWriter::vtkVoxelWriter()
{
  this->Filename = NULL;
}

vtkVoxelWriter::~vtkVoxelWriter()
{
  if ( this->Filename ) delete [] this->Filename;
}

// Description:
// Specify the input data or filter.
void vtkVoxelWriter::SetInput(vtkStructuredPoints *input)
{
  if ( this->Input != input )
    {
    vtkDebugMacro(<<" setting Input to " << (void *)input);
    this->Input = (vtkDataSet *) input;
    this->Modified();
    }
}

// Description:
// Write voxel data out.
void vtkVoxelWriter::WriteData()
{
  FILE *fp;
  int i, j, k;
  vtkBitScalars *newScalars;
  int numPts, idx;
  int bitcount;
  unsigned char uc;
  int *dim;
  float *origin,*aspect;
  vtkStructuredPoints *input=(vtkStructuredPoints *)this->Input;

  vtkDebugMacro(<< "Writing Voxel model");

  dim = input->GetDimensions();
  origin = input->GetOrigin();
  aspect = input->GetAspectRatio();
  numPts = dim[0]*dim[1]*dim[2];

  newScalars = (vtkBitScalars *)input->GetPointData()->GetScalars();
  
  if ( this->Filename == NULL)
    {
    vtkErrorMacro(<< "Please specify filename to write");
    return;
    }

  fp = fopen(this->Filename,"w");
  if (!fp) 
    {
    vtkErrorMacro(<< "Couldn't open file: " << this->Filename << endl);
    return;
    }

  fprintf(fp,"Voxel Data File\n");
  fprintf(fp,"Origin: %f %f %f\n",origin[0],origin[1],origin[2]);
  fprintf(fp,"Aspect: %f %f %f\n",aspect[0],aspect[1],aspect[2]);
  fprintf(fp,"Dimensions: %i %i %i\n",dim[0],dim[1],dim[2]);

  // write out the data
  bitcount = 0;
  idx = 0;
  uc = 0x00;

  for (k = 0; k < dim[2]; k++)
    for (j = 0; j < dim[1]; j++)
      for (i = 0; i < dim[0]; i++)
	{
	if (newScalars->GetScalar(idx))
	  {
	  uc |= (0x80 >> bitcount);
	  }
	bitcount++;
	if (bitcount == 8)
	  {
	  fputc(uc,fp);
	  uc = 0x00;
	  bitcount = 0;
	  }
	idx++;
	}
  if (bitcount)
    {
    fputc(uc,fp);
    }

  fclose(fp);
}

void vtkVoxelWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkWriter::PrintSelf(os,indent);

  os << indent << "Filename: " 
     << (this->Filename ? this->Filename : "(none)") << "\n";
}


