/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPNMWriter.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkPNMWriter.hh"
#include "vtkColorScalars.hh"

vtkPNMWriter::vtkPNMWriter()
{
  this->Filename = NULL;
}

vtkPNMWriter::~vtkPNMWriter()
{
  if ( this->Filename ) delete [] this->Filename;
}

// Description:
// Specify the input data or filter.
void vtkPNMWriter::SetInput(vtkStructuredPoints *input)
{
  if ( this->Input != input )
    {
    vtkDebugMacro(<<" setting Input to " << (void *)input);
    this->Input = (vtkDataSet *) input;
    this->Modified();
    }
}

// Description:
// Write PNM data out.
void vtkPNMWriter::WriteData()
{
  FILE *fp;
  vtkColorScalars *newScalars;
  int *dims;
  int numPts;
  vtkStructuredPoints *input=(vtkStructuredPoints *)this->Input;
  vtkPointData *pd = input->GetPointData();
  int bpp, i;
  unsigned char *buffer;
  
  vtkDebugMacro(<< "Writing PNM file");

  dims = input->GetDimensions();
  numPts = dims[0]*dims[1]*dims[2];

  if (strcmp(pd->GetScalars()->GetScalarType(),"ColorScalar"))
    {
    vtkWarningMacro(<< "Scalars must be of type ColorScalar.");
    return;
    }

  newScalars = (vtkColorScalars *)pd->GetScalars();

  bpp = newScalars->GetNumberOfValuesPerScalar();

  if (!(bpp % 2))
    {
    vtkWarningMacro(<< "Scalars must have one or three bytes per pixel");
    return;
    }
  
  if ( this->Filename == NULL)
    {
    vtkErrorMacro(<< "Please specify filename to write");
    return;
    }

  fp = fopen(this->Filename,"wb");
  if (!fp) 
    {
    vtkErrorMacro(<< "Couldn't open file: " << this->Filename << endl);
    return;
    }

  buffer = newScalars->GetPtr(0);
  
  if (bpp == 1)
    {
    fprintf(fp,"P5\n");
    fprintf(fp,"# pgm file written by the visualization toolkit\n");
    fprintf(fp,"%i %i\n255\n",dims[0],dims[1]);
    // now write the binary info 
    for (i = dims[1]-1; i >= 0; i--)
      {
      fwrite(buffer + i*dims[0],1,dims[0],fp);
      }
    }
  else
    {
    fprintf(fp,"P6\n");
    fprintf(fp,"# ppm file written by the visualization toolkit\n");
    fprintf(fp,"%i %i\n255\n",dims[0],dims[1]);
    // now write the binary info 
    for (i = dims[1]-1; i >= 0; i--)
      {
      fwrite(buffer + i*dims[0]*3,1,dims[0]*3,fp);
      }
    }
  
  fclose(fp);
}

void vtkPNMWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkWriter::PrintSelf(os,indent);

  os << indent << "Filename: " 
     << (this->Filename ? this->Filename : "(none)") << "\n";
}


