/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolume16Reader.cxx
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
#include "vtkVolume16Reader.h"
#include "vtkUnsignedShortArray.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkVolume16Reader* vtkVolume16Reader::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkVolume16Reader");
  if(ret)
    {
    return (vtkVolume16Reader*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkVolume16Reader;
}




// Construct object with NULL file prefix; file pattern "%s.%d"; image range 
// set to (1,1); data origin (0,0,0); data spacing (1,1,1); no data mask;
// header size 0; and byte swapping turned off.
vtkVolume16Reader::vtkVolume16Reader()
{
  this->DataMask = 0x0000;
  this->HeaderSize = 0;
  this->SwapBytes = 0;
  this->DataDimensions[0] = this->DataDimensions[1] = 0;
  this->Transform = NULL;
}

vtkVolume16Reader::~vtkVolume16Reader()
{
  this->SetTransform(NULL);
}

void vtkVolume16Reader::SetDataByteOrderToBigEndian()
{
#ifndef VTK_WORDS_BIGENDIAN
  this->SwapBytesOn();
#else
  this->SwapBytesOff();
#endif
}

void vtkVolume16Reader::SetDataByteOrderToLittleEndian()
{
#ifdef VTK_WORDS_BIGENDIAN
  this->SwapBytesOn();
#else
  this->SwapBytesOff();
#endif
}

void vtkVolume16Reader::SetDataByteOrder(int byteOrder)
{
  if ( byteOrder == VTK_FILE_BYTE_ORDER_BIG_ENDIAN )
    {
    this->SetDataByteOrderToBigEndian();
    }
  else
    {
    this->SetDataByteOrderToLittleEndian();
    }
}

int vtkVolume16Reader::GetDataByteOrder()
{
#ifdef VTK_WORDS_BIGENDIAN
  if ( this->SwapBytes )
    {
    return VTK_FILE_BYTE_ORDER_LITTLE_ENDIAN;
    }
  else
    {
    return VTK_FILE_BYTE_ORDER_BIG_ENDIAN;
    }
#else
  if ( this->SwapBytes )
    {
    return VTK_FILE_BYTE_ORDER_BIG_ENDIAN;
    }
  else
    {
    return VTK_FILE_BYTE_ORDER_LITTLE_ENDIAN;
    }
#endif
}

const char *vtkVolume16Reader::GetDataByteOrderAsString()
{
#ifdef VTK_WORDS_BIGENDIAN
  if ( this->SwapBytes )
    {
    return "LittleEndian";
    }
  else
    {
    return "BigEndian";
    }
#else
  if ( this->SwapBytes )
    {
    return "BigEndian";
    }
  else
    {
    return "LittleEndian";
    }
#endif
}


void vtkVolume16Reader::ExecuteInformation()
{
  int dim[3];
  vtkStructuredPoints *output = this->GetOutput();
  
  this->ComputeTransformedDimensions(dim);
  output->SetWholeExtent(0, dim[0]-1, 0, dim[1]-1, 0, dim[2]-1);

  output->SetScalarType(VTK_UNSIGNED_SHORT);
  output->SetNumberOfScalarComponents(1);
  
  output->SetSpacing(this->DataSpacing);
  output->SetOrigin(this->DataOrigin);
  // spacing and origin ?
}
    
    
void vtkVolume16Reader::Execute()
{
  vtkDataArray *newScalars;
  int first, last;
  int *dim;
  int dimensions[3];
  float Spacing[3];
  float origin[3];

  vtkStructuredPoints *output = this->GetOutput();

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
    newScalars = this->ReadImage(this->ImageRange[0]);
    }
  else
    {
    first = this->ImageRange[0];
    last = this->ImageRange[1];
    newScalars = this->ReadVolume(first, last);
    }

  // calculate dimensions of output from data dimensions and transform
  this->ComputeTransformedDimensions (dimensions);
  output->SetDimensions(dimensions);

  // calculate spacing of output from data spacing and transform
  this->ComputeTransformedSpacing (Spacing);

  // calculate origin of output from data origin and transform
  this->ComputeTransformedOrigin (origin);

  // adjust spacing and origin if spacing is negative
  this->AdjustSpacingAndOrigin (dimensions, Spacing, origin);

  output->SetSpacing(Spacing);
  output->SetOrigin(origin);
  if ( newScalars ) 
    {
    output->GetPointData()->SetScalars(newScalars);
    newScalars->Delete();
    }
}

vtkStructuredPoints *vtkVolume16Reader::GetImage(int ImageNumber)
{
  vtkDataArray *newScalars;
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
  
  result = vtkStructuredPoints::New();
  newScalars = this->ReadImage(ImageNumber);
  dimensions[0] = dim[0]; dimensions[1] = dim[1];
  dimensions[2] = 1;
  result->SetDimensions(dimensions);
  result->SetSpacing(this->DataSpacing);
  result->SetOrigin(this->DataOrigin);
  if ( newScalars ) 
    {
    result->GetPointData()->SetScalars(newScalars);
    newScalars->Delete();
    }
  return result;
}

// Read a slice of volume data.
vtkUnsignedShortArray *vtkVolume16Reader::ReadImage(int sliceNumber)
{
  vtkUnsignedShortArray *scalars = NULL;
  unsigned short *pixels;
  FILE *fp;
  int numPts;
  int status;
  char filename[1024];

  // build the file name. if there is no prefix, just use the slice number
  if (this->FilePrefix)
    {
    sprintf (filename, this->FilePattern, this->FilePrefix, sliceNumber);
    }
  else
    {
    sprintf (filename, this->FilePattern, sliceNumber);
    }
  if ( !(fp = fopen(filename,"rb")) )
    {
    vtkErrorMacro(<<"Can't open file: " << filename);
    return NULL;
    }

  numPts = this->DataDimensions[0] * this->DataDimensions[1];

  // create the short scalars
  scalars = vtkUnsignedShortArray::New();
  scalars->Allocate(numPts);

  // get a pointer to the data
  pixels = scalars->WritePointer(0, numPts);

  // read the image data
  status = this->Read16BitImage (fp, pixels, this->DataDimensions[0], 
				 this->DataDimensions[1], this->HeaderSize, 
				 this->SwapBytes);

  // close the file
  fclose (fp);

  // check the status of the read
  if (status == 0) 
    {
    scalars->Delete();
    return NULL;
    }
  else 
    {
    return scalars;
    }
}

// Read a volume of data.
vtkUnsignedShortArray *vtkVolume16Reader::ReadVolume(int first, int last)
{
  vtkUnsignedShortArray *scalars = NULL;
  unsigned short *pixels;
  unsigned short *slice;
  FILE *fp;
  int numPts;
  int fileNumber;
  int status=0;
  int numberSlices = last - first + 1;
  char filename[1024];
  int dimensions[3];
  int bounds[6];

  // calculate the number of points per image
  numPts = this->DataDimensions[0] * this->DataDimensions[1];

  // compute transformed dimensions
  this->ComputeTransformedDimensions (dimensions);

  // compute transformed bounds
  this->ComputeTransformedBounds (bounds);

  // get memory for slice
  slice = new unsigned short[numPts];

  // create the short scalars for all of the images
  scalars = vtkUnsignedShortArray::New();
  scalars->Allocate(numPts * numberSlices);

  // get a pointer to the scalar data
  pixels = scalars->WritePointer(0, numPts*numberSlices);

  vtkDebugMacro (<< "Creating scalars with " << numPts * numberSlices 
                 << " points.");

  // build each file name and read the data from the file
  for (fileNumber = first; fileNumber <= last; fileNumber++) 
    {
    // build the file name. if there is no prefix, just use the slice number
    if (this->FilePattern)
      {
      sprintf (filename, this->FilePattern, this->FilePrefix, fileNumber);
      }
    else
      {
      sprintf (filename, this->FilePattern, fileNumber);
      }
    if ( !(fp = fopen(filename,"rb")) )
      {
      vtkErrorMacro(<<"Can't find file: " << filename);
      return NULL;
      }

    vtkDebugMacro ( << "Reading " << filename );

    // read the image data
    status = this->Read16BitImage (fp, slice, this->DataDimensions[0], 
                    this->DataDimensions[1], this->HeaderSize, this->SwapBytes);

    fclose (fp);

    if (status == 0)
      {
      break;
      }

    // transform slice
    this->TransformSlice (slice, pixels, fileNumber - first, dimensions, bounds);
    }

  delete []slice;
  if (status == 0) 
    {
    scalars->Delete();
    return NULL;
    }
  else
    {
    return scalars;
    }
}

int vtkVolume16Reader:: Read16BitImage (FILE *fp, unsigned short *pixels, int xsize, 
                                        int ysize, int skip, int swapBytes)
{
  unsigned short *shortPtr;
  int numShorts = xsize * ysize;

  if (skip)
    {
    fseek (fp, skip, 0);
    }

  shortPtr = pixels;
  shortPtr = shortPtr + xsize*(ysize - 1);
  for (int j=0; j<ysize; j++, shortPtr = shortPtr - xsize)
    {
    if ( ! fread(shortPtr,sizeof (unsigned short),xsize,fp) )
      {
      vtkErrorMacro(<<"Error reaading raw pgm data!");
      return 0;
      }
    }

  if (swapBytes) 
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

  if (this->DataMask != 0x0000 )
    {
    unsigned short *dataPtr = pixels;
    int i;
    for (i = 0; i < numShorts; i++, dataPtr++) 
      {
      *dataPtr &= this->DataMask;
      }
    }

  return 1;
}

void vtkVolume16Reader::ComputeTransformedSpacing (float Spacing[3])
{
  if (!this->Transform)
    {
    memcpy (Spacing, this->DataSpacing, 3 * sizeof (float));
    }
  else
    {
    float transformedSpacing[4];
    memcpy (transformedSpacing, this->DataSpacing, 3 * sizeof (float));
    transformedSpacing[3] = 1.0;
    this->Transform->MultiplyPoint (transformedSpacing, transformedSpacing);

    for (int i = 0; i < 3; i++)
      {
      Spacing[i] = transformedSpacing[i];
      }
    vtkDebugMacro("Transformed Spacing " << Spacing[0] << ", " << Spacing[1] << ", " << Spacing[2]);
    }
}

void vtkVolume16Reader::ComputeTransformedOrigin (float origin[3])
{
  if (!this->Transform)
    {
    memcpy (origin, this->DataOrigin, 3 * sizeof (float));
    }
  else
    {
    float transformedOrigin[4];
    memcpy (transformedOrigin, this->DataOrigin, 3 * sizeof (float));
    transformedOrigin[3] = 1.0;
    this->Transform->MultiplyPoint (transformedOrigin, transformedOrigin);

    for (int i = 0; i < 3; i++)
      {
      origin[i] = transformedOrigin[i];
      }
    vtkDebugMacro("Transformed Origin " << origin[0] << ", " << origin[1] << ", " << origin[2]);
    }
}

void vtkVolume16Reader::ComputeTransformedDimensions (int dimensions[3])
{
  float transformedDimensions[4];
  if (!this->Transform)
    {
    dimensions[0] = this->DataDimensions[0];
    dimensions[1] = this->DataDimensions[1];
    dimensions[2] = this->ImageRange[1] - this->ImageRange[0] + 1;
    }
  else
    {
    transformedDimensions[0] = this->DataDimensions[0];
    transformedDimensions[1] = this->DataDimensions[1];
    transformedDimensions[2] = this->ImageRange[1] - this->ImageRange[0] + 1;
    transformedDimensions[3] = 1.0;
    this->Transform->MultiplyPoint (transformedDimensions, transformedDimensions);
    dimensions[0] = (int) transformedDimensions[0];
    dimensions[1] = (int) transformedDimensions[1];
    dimensions[2] = (int) transformedDimensions[2];
    if (dimensions[0] < 0)
      {
      dimensions[0] = -dimensions[0];
      }
    if (dimensions[1] < 0)
      {
      dimensions[1] = -dimensions[1];
      }
    if (dimensions[2] < 0)
      {
      dimensions[2] = -dimensions[2];
      }
    vtkDebugMacro(<< "Transformed dimensions are:" << dimensions[0] << ", "
					     << dimensions[1] << ", "
					     << dimensions[2]);
    }
}

void vtkVolume16Reader::ComputeTransformedBounds (int bounds[6])
{
  float transformedBounds[4];

  if (!this->Transform)
    {
    bounds[0] = 0;
    bounds[1] = this->DataDimensions[0] - 1;
    bounds[2] = 0;
    bounds[3] = this->DataDimensions[1] - 1;
    bounds[4] = 0;
    bounds[5] = this->ImageRange[1] - this->ImageRange[0];
    }
  else
    {
    transformedBounds[0] = 0;
    transformedBounds[1] = 0;
    transformedBounds[2] = 0;
    transformedBounds[3] = 1.0;
    this->Transform->MultiplyPoint (transformedBounds, transformedBounds);
    bounds[0] = (int) transformedBounds[0];
    bounds[2] = (int) transformedBounds[1];
    bounds[4] = (int) transformedBounds[2];
    transformedBounds[0] = this->DataDimensions[0] - 1;
    transformedBounds[1] = this->DataDimensions[1] - 1;
    transformedBounds[2] = this->ImageRange[1] - this->ImageRange[0];
    transformedBounds[3] = 1.0;
    this->Transform->MultiplyPoint (transformedBounds, transformedBounds);
    bounds[1] = (int) transformedBounds[0];
    bounds[3] = (int) transformedBounds[1];
    bounds[5] = (int) transformedBounds[2];
    // put bounds in correct order
    int tmp;
    for (int i = 0; i < 6; i += 2)
      {
      if (bounds[i + 1] < bounds[i])
        {
        tmp = bounds[i];
        bounds[i] = bounds[i + 1];
        bounds[i + 1] = tmp;
        }
      }
    vtkDebugMacro(<< "Transformed bounds are: "
		<< bounds[0] << ", " << bounds[1] << ", "
                << bounds[2] << ", " << bounds[3] << ", "
		<< bounds[4] << ", " << bounds[5]);
    }
}

void vtkVolume16Reader::AdjustSpacingAndOrigin (int dimensions[3], float Spacing[3], float origin[3])
{
  for (int i = 0; i < 3; i++)
    {
    if (Spacing[i] < 0)
      {
      origin[i] = origin[i] + Spacing[i] * dimensions[i];
      Spacing[i] = -Spacing[i];
      }
    }
  vtkDebugMacro("Adjusted Spacing " << Spacing[0] << ", " << Spacing[1] << ", " << Spacing[2]);
  vtkDebugMacro("Adjusted origin " << origin[0] << ", " << origin[1] << ", " << origin[2]);
}

void vtkVolume16Reader::TransformSlice (unsigned short *slice, unsigned short *pixels, int k, int dimensions[3], int bounds[3])
{
  int iSize = this->DataDimensions[0];
  int jSize = this->DataDimensions[1];

  if (!this->Transform)
    {
    memcpy (pixels + iSize * jSize * k, slice, iSize * jSize * sizeof (unsigned short));
    }
  else
    {
    float transformedIjk[4], ijk[4];
    int i, j;
    int xyz[3];
    int index;
    int xSize = dimensions[0];
    int xySize = dimensions[0] * dimensions[1];

    // now move slice into pixels

    ijk[2] = k;
    ijk[3] = 1.0;
    for (j = 0; j < jSize; j++)
      {
      ijk[1] = j;
      for (i = 0; i < iSize; i++, slice++)
        {
        ijk[0] = i;
        this->Transform->MultiplyPoint (ijk, transformedIjk);
	xyz[0] = (int) ((float)transformedIjk[0] - bounds[0]);
	xyz[1] = (int) ((float)transformedIjk[1] - bounds[2]);
	xyz[2] = (int) ((float)transformedIjk[2] - bounds[4]);
        index = xyz[0] +
                xyz[1] * xSize +
                xyz[2] * xySize;
	*(pixels + index) = *slice;
        }
      }
    }
}

void vtkVolume16Reader::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkVolumeReader::PrintSelf(os,indent);

  os << indent << "HeaderSize: " << this->HeaderSize << "\n";
  os << indent << "SwapBytes: " << this->SwapBytes << "\n";
  os << indent << "Data Dimensions: (" << this->DataDimensions[0] << ", "
                                   << this->DataDimensions[1] << ")\n";
  os << indent << "Data Mask: " << this->DataMask << "\n";

  if ( this->Transform )
    {
    os << indent << "Transform:\n";
    this->Transform->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Transform: (None)\n";
    }
}
