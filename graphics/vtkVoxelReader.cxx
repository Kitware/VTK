/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVoxelReader.cxx
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
#include <ctype.h>
#include "vtkVoxelReader.h"

vtkVoxelReader::vtkVoxelReader()
{
  this->Filename = NULL;
}

void vtkVoxelReader::Execute()
{
  FILE *fp;
  vtkBitScalars *newScalars;
  int i,numPts;
  int bitcount;
  unsigned char uc;
  float f[3];
  int ti[3];
  vtkStructuredPoints *output=(vtkStructuredPoints *)this->Output;

  // Initialize

  if ((fp = fopen(this->Filename, "r")) == NULL)
    {
    vtkErrorMacro(<< "File " << this->Filename << " not found");
    return;
    }

  // read the header
  fscanf(fp,"Voxel Data File\n");

  // read in the 
  fscanf(fp,"Origin: %f %f %f\n",f,f+1,f+2);
  output->SetOrigin(f);

  fscanf(fp,"Aspect: %f %f %f\n",f,f+1,f+2);
  output->SetAspectRatio(f);

  fscanf(fp,"Dimensions: %i %i %i\n",ti,ti+1,ti+2);
  output->SetDimensions(ti);

  numPts = ti[0] * ti[1] * ti[2];
  newScalars = new vtkBitScalars(numPts);

  bitcount = 0;
  for (i=0; i<numPts;) 
    {
    uc = fgetc(fp);
    newScalars->SetScalar(i++,(int)(uc&0x80));
    newScalars->SetScalar(i++,(int)(uc&0x40));
    newScalars->SetScalar(i++,(int)(uc&0x20));
    newScalars->SetScalar(i++,(int)(uc&0x10));
    newScalars->SetScalar(i++,(int)(uc&0x08));
    newScalars->SetScalar(i++,(int)(uc&0x04));
    newScalars->SetScalar(i++,(int)(uc&0x02));
    newScalars->SetScalar(i++,(int)(uc&0x01));
    }

  fclose( fp );

  vtkDebugMacro(<< "Read " << numPts<< " points");

  output->GetPointData()->SetScalars(newScalars);
}

void vtkVoxelReader::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkStructuredPointsSource::PrintSelf(os,indent);

  os << indent << "Filename: " 
     << (this->Filename ? this->Filename : "(none)") << "\n";
}
