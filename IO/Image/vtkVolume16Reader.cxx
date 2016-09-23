/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolume16Reader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkVolume16Reader.h"

#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTransform.h"
#include "vtkUnsignedShortArray.h"

vtkStandardNewMacro(vtkVolume16Reader);

vtkCxxSetObjectMacro(vtkVolume16Reader,Transform,vtkTransform);

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
vtkVolume16Reader::~vtkVolume16Reader()
{
  this->SetTransform(NULL);
}

//----------------------------------------------------------------------------
void vtkVolume16Reader::SetDataByteOrderToBigEndian()
{
#ifndef VTK_WORDS_BIGENDIAN
  this->SwapBytesOn();
#else
  this->SwapBytesOff();
#endif
}

//----------------------------------------------------------------------------
void vtkVolume16Reader::SetDataByteOrderToLittleEndian()
{
#ifdef VTK_WORDS_BIGENDIAN
  this->SwapBytesOn();
#else
  this->SwapBytesOff();
#endif
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
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


//----------------------------------------------------------------------------
int vtkVolume16Reader::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  int dim[3];

  this->ComputeTransformedDimensions(dim);

  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
               0, dim[0]-1, 0, dim[1]-1, 0, dim[2]-1);

  vtkDataObject::SetPointDataActiveScalarInfo(outInfo, VTK_UNSIGNED_SHORT, 1);
  outInfo->Set(vtkDataObject::SPACING(), this->DataSpacing, 3);
  outInfo->Set(vtkDataObject::ORIGIN(), this->DataOrigin, 3);
  // spacing and origin ?

  return 1;
}

//----------------------------------------------------------------------------
int vtkVolume16Reader::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  int first, last;
  int *dim;
  int dimensions[3];
  double Spacing[3];
  double origin[3];

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkDataObject* output_do = outInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkImageData *output = this->AllocateOutputData(output_do,
                                                  outInfo);
  vtkUnsignedShortArray *newScalars =
    vtkArrayDownCast<vtkUnsignedShortArray>(output->GetPointData()->GetScalars());

  // Validate instance variables
  if (this->FilePrefix == NULL)
  {
    vtkErrorMacro(<< "FilePrefix is NULL");
    return 1;
  }

  if (this->HeaderSize < 0)
  {
    vtkErrorMacro(<< "HeaderSize " << this->HeaderSize << " must be >= 0");
    return 1;
  }

  dim = this->DataDimensions;

  if (dim[0] <= 0 || dim[1] <= 0)
  {
    vtkErrorMacro(<< "x, y dimensions " << dim[0] << ", " << dim[1]
                  << "must be greater than 0.");
    return 1;
  }

  if ( (this->ImageRange[1]-this->ImageRange[0]) <= 0 )
  {
    this->ReadImage(this->ImageRange[0], newScalars);
  }
  else
  {
    first = this->ImageRange[0];
    last = this->ImageRange[1];
    this->ReadVolume(first, last, newScalars);
  }

  // calculate dimensions of output from data dimensions and transform
  this->ComputeTransformedDimensions (dimensions);
  output->SetDimensions(dimensions);

  // calculate spacing of output from data spacing and transform
  this->ComputeTransformedSpacing(Spacing);

  // calculate origin of output from data origin and transform
  this->ComputeTransformedOrigin(origin);

  // adjust spacing and origin if spacing is negative
  this->AdjustSpacingAndOrigin(dimensions, Spacing, origin);

  output->SetSpacing(Spacing);
  output->SetOrigin(origin);

  return 1;
}

//----------------------------------------------------------------------------
vtkImageData *vtkVolume16Reader::GetImage(int ImageNumber)
{
  vtkUnsignedShortArray *newScalars;
  int *dim;
  int dimensions[3];
  vtkImageData *result;

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

  result = vtkImageData::New();
  newScalars = vtkUnsignedShortArray::New();
  this->ReadImage(ImageNumber, newScalars);
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

//----------------------------------------------------------------------------
// Read a slice of volume data.
void vtkVolume16Reader::ReadImage(int sliceNumber,
                                  vtkUnsignedShortArray *scalars)
{
  unsigned short *pixels;
  FILE *fp;
  int numPts;
  char filename[VTK_MAXPATH];

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
    return;
  }

  numPts = this->DataDimensions[0] * this->DataDimensions[1];

  // get a pointer to the data
  pixels = scalars->WritePointer(0, numPts);

  // read the image data
  this->Read16BitImage (fp, pixels, this->DataDimensions[0],
                        this->DataDimensions[1], this->HeaderSize,
                        this->SwapBytes);

  // close the file
  fclose (fp);
}

//----------------------------------------------------------------------------
// Read a volume of data.
void vtkVolume16Reader::ReadVolume(int first, int last,
                                   vtkUnsignedShortArray *scalars)
{
  unsigned short *pixels;
  unsigned short *slice;
  FILE *fp;
  int numPts;
  int fileNumber;
  int status;
  int numberSlices = last - first + 1;
  char filename[VTK_MAXPATH];
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
      delete [] slice;
      return;
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
}

//----------------------------------------------------------------------------
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
  shortPtr += xsize*(ysize - 1);
  for (int j=0; j<ysize; j++, shortPtr -= xsize)
  {
    if ( ! fread(shortPtr,sizeof (unsigned short),xsize,fp) )
    {
      vtkErrorMacro(<<"Error reading raw pgm data!");
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

//----------------------------------------------------------------------------
void vtkVolume16Reader::ComputeTransformedSpacing (double spacing[3])
{
  if (!this->Transform)
  {
    memcpy (spacing, this->DataSpacing, 3 * sizeof (double));
  }
  else
  {
    double transformedSpacing[4];
    memcpy (transformedSpacing, this->DataSpacing, 3 * sizeof (double));
    transformedSpacing[3] = 1.0;
    this->Transform->MultiplyPoint (transformedSpacing, transformedSpacing);

    for (int i = 0; i < 3; i++)
    {
      spacing[i] = transformedSpacing[i];
    }
    vtkDebugMacro("Transformed Spacing " <<
      spacing[0] << ", " << spacing[1] << ", " << spacing[2]);
  }
}

//----------------------------------------------------------------------------
void vtkVolume16Reader::ComputeTransformedOrigin (double origin[3])
{
  if (!this->Transform)
  {
    memcpy (origin, this->DataOrigin, 3 * sizeof (double));
  }
  else
  {
    double transformedOrigin[4];
    memcpy (transformedOrigin, this->DataOrigin, 3 * sizeof (double));
    transformedOrigin[3] = 1.0;
    this->Transform->MultiplyPoint (transformedOrigin, transformedOrigin);

    for (int i = 0; i < 3; i++)
    {
      origin[i] = transformedOrigin[i];
    }
    vtkDebugMacro("Transformed Origin " <<
      origin[0] << ", " << origin[1] << ", " << origin[2]);
  }
}

//----------------------------------------------------------------------------
void vtkVolume16Reader::ComputeTransformedDimensions (int dimensions[3])
{
  double transformedDimensions[4];
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

//----------------------------------------------------------------------------
void vtkVolume16Reader::ComputeTransformedBounds (int bounds[6])
{
  double transformedBounds[4];

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

//----------------------------------------------------------------------------
void vtkVolume16Reader::AdjustSpacingAndOrigin (int dimensions[3], double spacing[3],
  double origin[3])
{
  for (int i = 0; i < 3; i++)
  {
    if (spacing[i] < 0)
    {
      origin[i] = origin[i] + spacing[i] * dimensions[i];
      spacing[i] = -spacing[i];
    }
  }
  vtkDebugMacro("Adjusted Spacing " << spacing[0] << ", " << spacing[1] << ", " << spacing[2]);
  vtkDebugMacro("Adjusted origin " << origin[0] << ", " << origin[1] << ", " << origin[2]);
}

//----------------------------------------------------------------------------
void vtkVolume16Reader::TransformSlice (unsigned short *slice, unsigned short *pixels, int k, int dimensions[3], int bounds[6])
{
  int iSize = this->DataDimensions[0];
  int jSize = this->DataDimensions[1];

  if (!this->Transform)
  {
    memcpy (pixels + iSize * jSize * k, slice, iSize * jSize * sizeof (unsigned short));
  }
  else
  {
    double transformedIjk[4], ijk[4];
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
        xyz[0] = (int) ((double)transformedIjk[0] - bounds[0]);
        xyz[1] = (int) ((double)transformedIjk[1] - bounds[2]);
        xyz[2] = (int) ((double)transformedIjk[2] - bounds[4]);
        index = xyz[0] +
                xyz[1] * xSize +
                xyz[2] * xySize;
        *(pixels + index) = *slice;
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkVolume16Reader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

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
