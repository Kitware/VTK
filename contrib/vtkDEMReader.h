/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDEMReader.h
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
// .NAME vtkDEMReader - read a ditigal elevation model (DEM) file
// .SECTION Description
// vtkDEMReader reads digital elevation files and creates image data.
// Digital elevation files are produced by the
// <A HREF="http://www.usgs.org">US Geological Survey</A>. 
// A complete description of the DEM file is located at the USGS site.
// The reader reads the entire dem file and create a vtkImageData that
// contains a single scalar component that is the elevation in meters.
// The spacing is also expressed in meters. A number of get methods
// provide acees to fields on the header.
#ifndef __vtkDEMReader_h
#define __vtkDEMReader_h

#include <stdio.h>
#include "vtkImageSource.h"
class VTK_EXPORT vtkDEMReader : public vtkImageSource
{
public:
  vtkDEMReader();
  ~vtkDEMReader();
  static vtkDEMReader *New() {return new vtkDEMReader;};
  const char *GetClassName() {return "vtkDEMReader";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify file name of Digital Elevation Model (DEM) file
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // An ASCII description of the map
  vtkGetStringMacro(MapLabel);

  // Description:
  // Code 1=DEM-1, 2=DEM_2, ...
  vtkGetMacro(DEMLevel,int);

  // Description:
  // Code 1=regular, 2=random, reserved for future use
  vtkGetMacro(ElevationPattern,  int);

  // Description:
  // Ground planimetric reference system
  vtkGetMacro(GroundSystem,  int);

  // Description:
  // Zone in ground planimetric reference system
  vtkGetMacro(GroundZone,  int);

  // Description:
  // Map Projection parameters. All are zero.
  vtkGetVectorMacro(ProjectionParameters,float,15);

  // Description:
  // Defining unit of measure for ground planimetric coordinates throughout
  // the file. 0 = radians, 1 = feet, 2 = meters, 3 = arc-seconds.
  vtkGetMacro(PlaneUnitOfMeasure,  int);

  // Description:
  // Defining unit of measure for elevation coordinates throughout
  // the file. 1 = feet, 2 = meters
  vtkGetMacro(ElevationUnitOfMeasure,  int);

  // Description:
  // Number of sides in the polygon which defines the coverage of
  // the DEM file. Set to 4.
  vtkGetMacro(PolygonSize,  int);

  // Description:
  // Minimum and maximum elevation for the DEM. The units in the file
  // are in ElevationUnitOfMeasure. This class converts them to meters.
  vtkGetVectorMacro(ElevationBounds,float,2);

  // Description:
  // Counterclockwise angle (in radians) from the primary axis of the planimetric
  // reference to the primary axis of the DEM local reference system.
  // IGNORED BY THIS IMPLEMENTATION.
  vtkGetMacro(LocalRotation,  float);

  // Description:
  // Accuracy code for elevations. 0=unknown accuracy
  vtkGetMacro(AccuracyCode,  int);

  // Description:
  // DEM spatial resolution for x,y,z. Values are expressed in units of resolution.
  // Since elevations are read as integers, this permits fractional elevations.
  vtkGetVectorMacro(SpatialResolution,float,3);

  // Description:
  // The number of rows and columns in the DEM.
  vtkGetVectorMacro(ProfileDimension,int,2);

  // Description:
  // Reads the DEM Type A record to compute the extent, origin and
  // spacing of the image data. The number of scalar components is set
  // to 1 and the output scalar type is VTK_FLOAT. Since this class needs
  // to read the whole file, this method also sets the UpdateExtent.
  void UpdateImageInformation();

protected:
  vtkTimeStamp ReadHeaderTime;
  int NumberOfColumns;
  int NumberOfRows;
  int WholeExtent[6];
  char *FileName;
  char MapLabel[145];
  int DEMLevel;
  int ElevationPattern;
  int GroundSystem;
  int GroundZone;
  float ProjectionParameters[15];
  int PlaneUnitOfMeasure;
  int ElevationUnitOfMeasure;
  int PolygonSize;
  float GroundCoords[4][2];
  float ElevationBounds[2];
  float LocalRotation;
  int AccuracyCode;
  float SpatialResolution[3];
  int ProfileDimension[2];
  int ProfileSeekOffset;
  void ComputeExtentOriginAndSpacing (int extent[6], float origin[6], float spacing[6]);
  int ReadTypeARecord ();
  int ReadProfiles (vtkImageData *data);
  void Execute(vtkImageData *outData);
};

#endif

