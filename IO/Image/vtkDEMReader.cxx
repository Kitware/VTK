/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDEMReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDEMReader.h"

#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkPointData.h"

vtkStandardNewMacro(vtkDEMReader);

#define VTK_SW  0
#define VTK_NW  1
#define VTK_NE  2
#define VTK_SE  3
#define VTK_METERS_PER_FEET .305
#define VTK_METERS_PER_ARC_SECOND 23.111

void ConvertDNotationToENotation (char *line);

vtkDEMReader::vtkDEMReader()
{
  int i, j;
  this->NumberOfColumns = 0;
  this->NumberOfRows = 0;
  for (i = 0; i < 6; i++)
    {
    this->WholeExtent[i] = 0;
    }
  this->FileName = NULL;
  for (i = 0; i < 145; i++)
    {
    this->MapLabel[i] = '\0';
    }
  this->DEMLevel = 0;
  this->ElevationPattern = 0;
  this->GroundSystem = 0;
  this->ProfileSeekOffset = 0;
  this->GroundZone = 0;
  for (i = 0; i < 15; i++)
    {
    this->ProjectionParameters[i] = 0;
    }
  this->PlaneUnitOfMeasure = 0;
  this->ElevationUnitOfMeasure = 0;
  this->PolygonSize = 0;
  for (i = 0; i < 2; i++)
    {
    this->ElevationBounds[i] = 0;
    this->ProfileDimension[i] = 0;
    for (j = 0; j < 4; j++)
      {
      this->GroundCoords[j][i] = 0;
      }
    }
  this->LocalRotation = 0;
  this->AccuracyCode = 0;
  for (i = 0; i < 3; i++)
    {
    this->SpatialResolution[i] = 0;
    }
  this->ElevationReference = REFERENCE_ELEVATION_BOUNDS;

  this->SetNumberOfInputPorts(0);
}

vtkDEMReader::~vtkDEMReader()
{
  if (this->FileName)
    {
    delete [] this->FileName;
    }
}

//----------------------------------------------------------------------------
int vtkDEMReader::RequestInformation (
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector ** vtkNotUsed( inputVector ),
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  double spacing[3], origin[3];
  int extent[6];

  if (!this->FileName)
    {
    vtkErrorMacro(<< "A FileName must be specified.");
    return 0;
    }

  // read the header of the file to determine dimensions, origin and spacing
  this->ReadTypeARecord ();

  // compute the extent based on the header information
  this->ComputeExtentOriginAndSpacing (extent, origin, spacing);

  // fill in the pertinent stuff from the header
  outInfo->Set(vtkDataObject::ORIGIN(),origin,3);
  outInfo->Set(vtkDataObject::SPACING(),spacing,3);

  vtkImageData::SetNumberOfScalarComponents(1, outInfo);
  vtkImageData::SetScalarType(VTK_FLOAT, outInfo);

  // whole dem must be read
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),extent,6);

  return 1;
}

//----------------------------------------------------------------------------
// Convert to Imaging API
int vtkDEMReader::RequestData(
  vtkInformation* vtkNotUsed( request ),
  vtkInformationVector** vtkNotUsed( inputVector ),
  vtkInformationVector* outputVector)
{
  // get the data object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkImageData *output = vtkImageData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  output->SetExtent(
    outInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));
  output->AllocateScalars(outInfo);

  if (!this->FileName)
    {
    vtkErrorMacro(<< "A FileName must be specified.");
    return 0;
    }

  if (output->GetScalarType() != VTK_FLOAT)
    {
    vtkErrorMacro("Execute: This source only outputs floats.");
    return 1;
    }

//
// Read header
//
  if (this->ReadTypeARecord () == 0)
    {
    //
    // Read Profiles
    //
    this->ReadProfiles (output);
    }

  // Name the scalars.
  output->GetPointData()->GetScalars()->SetName("Elevation");

  return 1;
}

int vtkDEMReader::ReadTypeARecord ()
{
  char record[1025];
  float elevationConversion;
  FILE *fp;

  if (this->GetMTime () < this->ReadHeaderTime)
    {
    return 0;
    }

  if (!this->FileName)
    {
    vtkErrorMacro(<< "A FileName must be specified.");
    return -1;
    }

  if ((fp = fopen(this->FileName, "rb")) == NULL)
    {
    vtkErrorMacro(<< "File " << this->FileName << " not found");
    return -1;
    }

  vtkDebugMacro (<< "reading DEM header: type A record");

  //
  // read the record. it is always 1024 characters long
  //
  int result = fscanf(fp, "%512c", record);
  if (result != 1)
    {
    vtkErrorMacro("For the file " << this->FileName
                  << " fscanf expected 1 items but got " << result);
    return -1;
    }
  result = fscanf(fp, "%512c", record+512);
  if (result != 1)
    {
    vtkErrorMacro("For the file " << this->FileName
                  << " fscanf expected 1 items but got " << result);
    return -1;
    }
  record[1024] = '\0';

  //
  // convert any D+ or D- to E+ or E-. c++ and c i/o cannot read D+/-
  //
  ConvertDNotationToENotation (record);

  char *current = record;

  this->MapLabel[144] = '\0';
  sscanf(current, "%144c", this->MapLabel);
  current += 144;

  sscanf(current, "%6d%6d%6d%6d",
                 &this->DEMLevel,
                 &this->ElevationPattern,
                 &this->GroundSystem,
                 &this->GroundZone);
  current += 24;
  sscanf(current, "%24g%24g%24g%24g%24g%24g%24g%24g%24g%24g%24g%24g%24g%24g%24g",
   &this->ProjectionParameters[0],
   &this->ProjectionParameters[1],
   &this->ProjectionParameters[2],
   &this->ProjectionParameters[3],
   &this->ProjectionParameters[4],
   &this->ProjectionParameters[5],
   &this->ProjectionParameters[6],
   &this->ProjectionParameters[7],
   &this->ProjectionParameters[8],
   &this->ProjectionParameters[9],
   &this->ProjectionParameters[10],
   &this->ProjectionParameters[11],
   &this->ProjectionParameters[12],
   &this->ProjectionParameters[13],
   &this->ProjectionParameters[14]);
  current += 360;
  sscanf(current, "%6d%6d%6d",
   &this->PlaneUnitOfMeasure,
   &this->ElevationUnitOfMeasure,
   &this->PolygonSize);
  current += 18;
  sscanf(current, "%24g%24g%24g%24g%24g%24g%24g%24g",
   &this->GroundCoords[0][0], &this->GroundCoords[0][1],
   &this->GroundCoords[1][0], &this->GroundCoords[1][1],
   &this->GroundCoords[2][0], &this->GroundCoords[2][1],
   &this->GroundCoords[3][0], &this->GroundCoords[3][1]);
  current += 192;
  sscanf(current, "%24g%24g",
   &this->ElevationBounds[0], &this->ElevationBounds[1]);
  elevationConversion = 1.0;
  if (this->ElevationUnitOfMeasure == 1) // feet
    {
    elevationConversion = VTK_METERS_PER_FEET;
    }
  else if (this->ElevationUnitOfMeasure == 3) // arc-seconds
    {
    elevationConversion = VTK_METERS_PER_ARC_SECOND;
    }
  this->ElevationBounds[0] *= elevationConversion;
  this->ElevationBounds[1] *= elevationConversion;
  current += 48;
  sscanf(current, "%24g",
   &this->LocalRotation);
  current += 24;
  sscanf(current, "%6d",
   &this->AccuracyCode);
  current += 6;
  char buf[13];
  buf[12] = 0;
  strncpy(buf, current, 12);
  sscanf(buf, "%12g", &this->SpatialResolution[0]);
  strncpy(buf, current+12, 12);
  sscanf(buf, "%12g", &this->SpatialResolution[1]);
  strncpy(buf, current+24, 12);
  sscanf(buf, "%12g", &this->SpatialResolution[2]);
  current += 36;
  sscanf(current, "%6d%6d",
   &this->ProfileDimension[0],
   &this->ProfileDimension[1]);

  this->ProfileSeekOffset = ftell (fp);

  this->ReadHeaderTime.Modified();
  fclose (fp);

  return 0;
}

void vtkDEMReader::ComputeExtentOriginAndSpacing (int extent[6],
                                                  double origin[3],
                                                  double spacing[3])
{
  float eastMost, westMost, northMost, southMost;
  float planeConversion;

  //
  // compute number of samples
  //
  eastMost = this->GroundCoords[VTK_NE][0];
  if (eastMost <  this->GroundCoords[VTK_SE][0])
    {
    eastMost = this->GroundCoords[VTK_SE][0];
    }
  westMost = this->GroundCoords[VTK_NW][0];
  if (westMost >  this->GroundCoords[VTK_SW][0])
    {
    westMost = this->GroundCoords[VTK_SW][0];
    }
  northMost = this->GroundCoords[VTK_NE][1];
  if (northMost <  this->GroundCoords[VTK_NW][1])
    {
    northMost = this->GroundCoords[VTK_NW][1];
    }
  southMost = this->GroundCoords[VTK_SW][1];
  if (southMost >  this->GroundCoords[VTK_SE][1])
    {
    southMost = this->GroundCoords[VTK_SE][1];
    }

  //
  // compute the number of rows and columns
  //
  this->NumberOfColumns = (int) ((eastMost - westMost) / this->SpatialResolution[0] + 1.0);
  this->NumberOfRows = (int) ((northMost - southMost) / this->SpatialResolution[1] + 1.0);

  //
  // convert to extent
  //
  extent[0] = 0; extent[1] = this->NumberOfColumns - 1;
  extent[2] = 0; extent[3] = this->NumberOfRows - 1;
  extent[4] = 0; extent[5] = 0;;

  //
  // compute the spacing in meters
  //
  planeConversion = 1.0;
  if (this->PlaneUnitOfMeasure == 1) // feet
    {
    planeConversion = VTK_METERS_PER_FEET;
    }
  else if (this->PlaneUnitOfMeasure == 3) // arc-seconds
    {
    planeConversion = VTK_METERS_PER_ARC_SECOND;
    }

  //
  // get the origin from the ground coordinates
  //
  origin[0] = this->GroundCoords[VTK_SW][0];
  origin[1] = this->GroundCoords[VTK_SW][1];
  if ( this->ElevationReference == REFERENCE_ELEVATION_BOUNDS )
    {
    origin[2] = this->ElevationBounds[0];
    }
  else //REFERENCE_SEA_LEVEL
    {
    origin[2] = 0.0;
    }

  spacing[0] = this->SpatialResolution[0] * planeConversion;
  spacing[1] = this->SpatialResolution[1] * planeConversion;
  spacing[2] = 1.0;
}

int vtkDEMReader::ReadProfiles (vtkImageData *data)
{
  char record[145];
  float *outPtr, *ptr;
  float elevationExtrema[2];
  float localElevation;
  float planCoords[2];
  float units = this->SpatialResolution[2];
  float elevationConversion;
  float lowPoint;
  int column, row;
  int columnCount;
  int elevation;
  int lastRow;
  int numberOfColumns;
  int profileId[2], profileSize[2];
  int rowId, columnId;
  int updateInterval;
  int status = 0;
  int result;
  FILE *fp;

  if (!this->FileName)
    {
    vtkErrorMacro(<< "A FileName must be specified.");
    return -1;
    }

  if ((fp = fopen(this->FileName, "rb")) == NULL)
    {
    vtkErrorMacro(<< "File " << this->FileName << " not found");
    return -1;
    }

  vtkDebugMacro (<< "reading profiles");

  // elevation will always be stored in meters
  elevationConversion = 1.0;
  if (this->ElevationUnitOfMeasure == 1) // feet
    {
    elevationConversion = VTK_METERS_PER_FEET;
    }
  else if (this->ElevationUnitOfMeasure == 3) // arc-seconds
    {
    elevationConversion = VTK_METERS_PER_ARC_SECOND;
    }

  units *= elevationConversion;
  // seek to start of profiles
  fseek (fp, this->ProfileSeekOffset, SEEK_SET);
  record[120] = '\0';

  // initialize output to the lowest elevation
  lowPoint = this->ElevationBounds[0];
  ptr = outPtr = (float *) data->GetScalarPointer();
  for (int i = 0; i < this->NumberOfColumns * this->NumberOfRows; i++)
    {
    *ptr++ = lowPoint;
    }
  numberOfColumns = this->NumberOfColumns;
  updateInterval = numberOfColumns / 100;
  columnCount = this->ProfileDimension[1];
  for (column = 0; column < columnCount; column++)
    {
    //
    // read four int's
    //
    status = fscanf (fp, "%6d%6d%6d%6d",
                   &profileId[0],       /* 1 */
                   &profileId[1],       /* 1 */
                   &profileSize[0],     /* 2 */
                   &profileSize[1]);    /* 2 */
    if (status == EOF)
      {
      break;
      }
    //
    // read the doubles as strings so we can convert floating point format
    //
    result = fscanf(fp, "%120c", record);
    if (result != 1)
      {
      vtkErrorMacro("For the file " << this->FileName
                    << " fscanf expected 1 items but got " << result);
      return -1;
    }
    //
    // convert any D+ or D- to E+ or E-
    //
    ConvertDNotationToENotation (record);
    sscanf(record, "%24g%24g%24g%24g%24g",
                   &planCoords[0],      /* 3 */
                   &planCoords[1],      /* 3 */
                   &localElevation,     /* 4 */
                   &elevationExtrema[0],        /* 5 */
                   &elevationExtrema[1]);       /* 5 */
    rowId = profileId[0] - 1;
    columnId = profileId[1] - 1;
    lastRow = rowId + profileSize[0] - 1;
    // report progress at the start of each column
    if (column % updateInterval == 0)
      {
      this->UpdateProgress ((float) column / ((float) columnCount - 1));
      if (this->GetAbortExecute())
        {
        break;
        }
      }
    // read a column
    for (row = rowId; row <= lastRow; row++)
      {
      result = fscanf(fp, "%6d", &elevation);
      if (result != 1)
        {
        vtkErrorMacro("For the file " << this->FileName
                      << " fscanf expected 1 items but got " << result);
        return -1;
        }
      *(outPtr + columnId + row * numberOfColumns) = elevation * units;
      }
    }
  fclose (fp);

  return status;
}

// Description: Converts Fortran D notation to C++ e notation
void ConvertDNotationToENotation (char *line)
{
  char *ptr = line;

  // first convert D+ to E+
  while (*ptr && (ptr = strstr (ptr, "D+")))
    {
    *ptr = 'e'; ptr++;
    *ptr = '+'; ptr++;
    }

  // first now D- to E-
  ptr = line;
  while (*ptr && (ptr = strstr (ptr, "D-")))
    {
    *ptr = 'e'; ptr++;
    *ptr = '-'; ptr++;
    }
}


// Return the elevation reference.
const char *vtkDEMReader::GetElevationReferenceAsString(void)
{
  if ( this->ElevationReference == REFERENCE_SEA_LEVEL )
    {
    return "Sea Level";
    }
  else // REFERENCE_ELEVATION_BOUNDS
    {
    return "Elevation Bounds";
    }
}


void vtkDEMReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "File Name: "
     << (this->FileName ? this->FileName : "(none)") << "\n";
  if (this->FileName)
    {
    this->UpdateInformation ();
    os << indent << "MapLabel: " << this->MapLabel << "\n";
    os << indent << "DEMLevel: " << this->DEMLevel << "\n";
    os << indent << "ElevationPattern: " << this->ElevationPattern
       << (this->ElevationPattern == 1 ? " (regular)" : " (random)") << "\n";
    os << indent << "GroundSystem: " << this->GroundSystem;
    if (this->GroundSystem == 0)
      {
      os << " (Geographic)\n";
      }
    else if (this->GroundSystem == 1)
      {
      os << " (UTM)\n";
      }
    else if (this->GroundSystem == 2)
      {
      os << " (State plane)\n";
      }
    else
      {
      os << " (unknown)\n";
      }
    os << indent << "GroundZone: " << this->GroundZone << "\n";
    os << indent << "ElevationRefernce: " << this->GetElevationReferenceAsString() << "\n";
    os << indent << "ProjectionParameters: all zero" << "\n"; // this->ProjectionParameters
    os << indent << "PlaneUnitOfMeasure: " << this->PlaneUnitOfMeasure;
    if (this->PlaneUnitOfMeasure == 0)
      {
      os << indent << " (radians)\n";
      }
    else if (this->PlaneUnitOfMeasure == 1)
      {
      os << indent << " (feet)\n";
      }
    else if (this->PlaneUnitOfMeasure == 2)
      {
      os << indent << " (meters)\n";
      }
    else if (this->PlaneUnitOfMeasure == 3)
      {
      os << indent << " (arc-seconds)\n";
      }
    else
      {
      os << indent << " (unknown)\n";
      }

    os << indent << "ElevationUnitOfMeasure: " << this->ElevationUnitOfMeasure;
    if (this->ElevationUnitOfMeasure == 1)
      {
      os << indent << " (feet)\n";
      }
    else if (this->ElevationUnitOfMeasure == 2)
      {
      os << indent << " (meters)\n";
      }
    else
      {
      os << indent << " (unknown)\n";
      }
    os << indent << "PolygonSize: " << this->PolygonSize << "\n";
    os << indent << "GroundCoordinates: \n";
    os << indent << "        " << this->GroundCoords[0][0] << ", " << this->GroundCoords[0][1] << "\n";
    os << indent << "        " << this->GroundCoords[1][0] << ", " << this->GroundCoords[1][1] << "\n";
    os << indent << "        " << this->GroundCoords[2][0] << ", " << this->GroundCoords[2][1] << "\n";
    os << indent << "        " << this->GroundCoords[3][0] << ", " << this->GroundCoords[3][1] << "\n";

    os << indent << "ElevationBounds: " << this->ElevationBounds[0] << ", "
                                        << this->ElevationBounds[1]
                                        << " (meters)\n";
    os << indent << "LocalRotation: " << this->LocalRotation << "\n";
    os << indent << "AccuracyCode: " << this->AccuracyCode << "\n";
    os << indent << "SpatialResolution: " << this->SpatialResolution[0] << ", "
                                          << this->SpatialResolution[1];
    if (this->PlaneUnitOfMeasure == 0)
      {
      os << indent << "(radians)";
      }
    else if (this->PlaneUnitOfMeasure == 1)
      {
      os << indent << "(feet)";
      }
    else if (this->PlaneUnitOfMeasure == 2)
      {
      os << indent << "(meters)";
      }
    else if (this->PlaneUnitOfMeasure == 3)
      {
      os << indent << "(arc-seconds)";
      }
    else
      {
      os << indent << " (unknown)\n";
      }

    os << indent << this->SpatialResolution[2];
    if (this->ElevationUnitOfMeasure == 1)
      {
      os << indent << "(feet)\n";
      }
    else if (this->ElevationUnitOfMeasure == 2)
      {
      os << indent << "(meters)\n";
      }
    else
      {
      os << indent << "(unknown)\n";
      }
    os << indent << "ProfileDimension: " << this->ProfileDimension[0] << ", "
                                         << this->ProfileDimension[1] << "\n";
  }
}
