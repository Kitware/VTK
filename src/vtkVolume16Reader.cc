/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolume16Reader.cc
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
#include "vtkVolume16Reader.hh"
#include "vtkShortScalars.hh"

// Description:
// Construct object with NULL file prefix; file pattern "%s.%d"; image range 
// set to (1,1); data origin (0,0,0); data aspect ratio (1,1,1); no data mask;
// header size 0; and byte swapping turned off.
vtkVolume16Reader::vtkVolume16Reader()
{
  this->DataMask = 0x0000;
  this->HeaderSize = 0;
  this->SwapBytes = 0;
}

void vtkVolume16Reader::Execute()
{
  vtkScalars *newScalars;
  int first, last;
  int numberSlices;
  int *dim;
  int dimensions[3];
  vtkStructuredPoints *output=(vtkStructuredPoints *)this->Output;

  // Validate instance variables
  if (this->FilePrefix == NULL) 
    {
    vtkErrorMacro(<< "FilePrefix is NULL");
    return;
    }

  if (this->HeaderSize < 0) 
    {
    vtkErrorMacro(<< "HeaderSize " << this->HeaderSize << " must be >= 0");
    return;
    }

  dim = this->DataDimensions;

  if (dim[0] <= 0 || dim[1] <= 0) 
    {
    vtkErrorMacro(<< "x, y dimensions " << dim[0] << ", " << dim[1] 
                  << "must be greater than 0.");
    return;
    } 

  if ( (this->ImageRange[1]-this->ImageRange[0]) <= 0 )
    {
    numberSlices = 1;
    newScalars = this->ReadImage(this->ImageRange[0]);
    }
  else
    {
    first = this->ImageRange[0];
    last = this->ImageRange[1];
    numberSlices = last - first + 1;
    newScalars = this->ReadVolume(first, last);
    }

  dimensions[0] = dim[0]; dimensions[1] = dim[1];
  dimensions[2] = numberSlices;
  output->SetDimensions(dimensions);
  output->SetAspectRatio(this->DataAspectRatio);
  output->SetOrigin(this->DataOrigin);
  if ( newScalars ) 
    {
    output->GetPointData()->SetScalars(newScalars);
    newScalars->Delete();
    }
}

vtkStructuredPoints *vtkVolume16Reader::GetImage(int vtkNotUsed(ImageNumber))
{
  vtkScalars *newScalars;
  int *dim;
  int dimensions[3];
  vtkStructuredPoints *result;

  // Validate instance variables
  if (this->FilePrefix == NULL) 
    {
    vtkErrorMacro(<< "FilePrefix is NULL");
    return NULL;
    }

  if (this->HeaderSize < 0) 
    {
    vtkErrorMacro(<< "HeaderSize " << this->HeaderSize << " must be >= 0");
    return NULL;
    }

  dim = this->DataDimensions;

  if (dim[0] <= 0 || dim[1] <= 0) 
    {
    vtkErrorMacro(<< "x, y dimensions " << dim[0] << ", " << dim[1] 
                  << "must be greater than 0.");
    return NULL;
    } 
  
  result = new vtkStructuredPoints();
  newScalars = this->ReadImage(this->ImageRange[0]);
  dimensions[0] = dim[0]; dimensions[1] = dim[1];
  dimensions[2] = 1;
  result->SetDimensions(dimensions);
  result->SetAspectRatio(this->DataAspectRatio);
  result->SetOrigin(this->DataOrigin);
  if ( newScalars ) 
    {
    result->GetPointData()->SetScalars(newScalars);
    newScalars->Delete();
    }
  return result;
}

// Description:
// Read a slice of volume data.
vtkScalars *vtkVolume16Reader::ReadImage(int sliceNumber)
{
  vtkShortScalars *scalars = NULL;
  short *pixels;
  FILE *fp;
  int numPts;
  int status;
  char filename[1024];

  // build the file name
  sprintf (filename, this->FilePattern, this->FilePrefix, sliceNumber);
  if ( !(fp = fopen(filename,"r")) )
    {
    vtkErrorMacro(<<"Can't open file: " << filename);
    return NULL;
    }

  numPts = this->DataDimensions[0] * this->DataDimensions[1];

  // create the short scalars
  scalars = new vtkShortScalars(numPts);

  // get a pointer to the data
  pixels = scalars->WritePtr(0, numPts);

  // read the image data
  status = Read16BitImage (fp, pixels, this->DataDimensions[0], this->DataDimensions[1], this->HeaderSize, this->SwapBytes);

  // close the file
  fclose (fp);

  // check the status of the read
  if (status == 0) 
    {
    scalars->Delete();
    return NULL;
    }
  else 
    return scalars;
}

// Description:
// Read a volume of data.
vtkScalars *vtkVolume16Reader::ReadVolume(int first, int last)
{
  vtkShortScalars *scalars = NULL;
  short *pixels;
  FILE *fp;
  int numPts;
  int fileNumber;
  int status=0;
  int numberSlices = last - first + 1;
  char filename[1024];

  // calculate the number of points per image
  numPts = this->DataDimensions[0] * this->DataDimensions[1];

  // create the short scalars for all of the images
  scalars = new vtkShortScalars(numPts * numberSlices);
  vtkDebugMacro (<< "Creating scalars with " << numPts * numberSlices << " points.");

  // build each file name and read the data from the file
  for (fileNumber = first; fileNumber <= last; fileNumber++) 
    {
    // build the file name
    sprintf (filename, this->FilePattern, this->FilePrefix, fileNumber);
    if ( !(fp = fopen(filename,"r")) )
      {
      vtkErrorMacro(<<"Can't find file: " << filename);
      return NULL;
      }

    vtkDebugMacro ( << "Reading " << filename );
    // get a pointer to the data
    pixels = scalars->WritePtr((fileNumber - first) * numPts, numPts);

    // read the image data
    status = Read16BitImage (fp, pixels, this->DataDimensions[0], this->DataDimensions[1], this->HeaderSize, this->SwapBytes);

    fclose (fp);

    if (status == 0) break;
    }

  if (status == 0) 
    {
    scalars->Delete();
    return NULL;
    }
  else return scalars;
}

int vtkVolume16Reader:: Read16BitImage (FILE *fp, short *pixels, int xsize, 
                                        int ysize, int skip, int swapBytes)
{
  int numShorts = xsize * ysize;
  int status;

  if (skip) fseek (fp, skip, 0);

  status = fread (pixels, sizeof (short), numShorts, fp);

  if (status && swapBytes) 
    {
    unsigned char *bytes = (unsigned char *) pixels;
    unsigned char tmp;
    int i;
    for (i = 0; i < numShorts; i++, bytes += 2) 
      {
      tmp = *bytes; 
      *bytes = *(bytes + 1); 
      *(bytes + 1) = tmp;
      }
    }

  if (status && this->DataMask != 0x0000 )
    {
    short *dataPtr = pixels;
    int i;
    for (i = 0; i < numShorts; i++, dataPtr++) 
      {
      *dataPtr &= this->DataMask;
      }
    }

  return status;
}

void vtkVolume16Reader::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkVolumeReader::PrintSelf(os,indent);

  os << indent << "HeaderSize: " << this->HeaderSize << "\n";
  os << indent << "SwapBytes: " << this->SwapBytes << "\n";
  os << indent << "Data Dimensions: (" << this->DataDimensions[0] << ", "
                                   << this->DataDimensions[1] << ")\n";
}
