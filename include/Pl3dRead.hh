/*=========================================================================

  Program:   Visualization Library
  Module:    Pl3dRead.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Description:
---------------------------------------------------------------------------
This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
//
// Read PLOT3D data files
//
#ifndef __vlPLOT3DReader_h
#define __vlPLOT3DReader_h

#include <stdio.h>
#include "SGridSrc.hh"
#include "FScalars.hh"
#include "FVectors.hh"

// file formats
#define WHOLE_SINGLE_GRID_NO_IBLANKING 0
#define WHOLE_SINGLE_GRID_WITH_IBLANKING 1
#define WHOLE_MULTI_GRID_NO_IBLANKING 2
#define WHOLE_MULTI_GRID_WITH_IBLANKING 3
#define PLANES_SINGLE_GRID_NO_IBLANKING 4
#define PLANES_SINGLE_GRID_WITH_IBLANKING 5
#define PLANES_MULTI_GRID_NO_IBLANKING 6
#define PLANES_MULTI_GRID_WITH_IBLANKING 7

// file types
#define BINARY 0
#define ASCII 1

class vlPLOT3DReader : public vlStructuredGridSource 
{
public:
  vlPLOT3DReader();
  ~vlPLOT3DReader();
  char *GetClassName() {return "vlPLOT3DReader";};
  void PrintSelf(ostream& os, vlIndent indent);

  vlSetStringMacro(XYZFilename);
  vlGetStringMacro(XYZFilename);

  vlSetStringMacro(QFilename);
  vlGetStringMacro(QFilename);

  vlSetStringMacro(FunctionFilename);
  vlGetStringMacro(FunctionFilename);

  vlSetClampMacro(FileFormat,int,0,7);
  vlGetMacro(FileFormat,int);

  vlSetClampMacro(GridNumber,int,0,LARGE_INTEGER);
  vlGetMacro(GridNumber,int);

  vlSetMacro(AutoRelease,int);
  vlGetMacro(AutoRelease,int);
  vlBooleanMacro(AutoRelease,int);

  // these are read from PLOT3D file
  vlGetMacro(Fsmach,float);
  vlGetMacro(Alpha,float);
  vlGetMacro(Re,float);
  vlGetMacro(Time,float);

  vlSetMacro(R,float);
  vlGetMacro(R,float);

  vlSetMacro(Gamma,float);
  vlGetMacro(Gamma,float);

  vlSetMacro(Uvinf,float);
  vlGetMacro(Uvinf,float);

  vlSetMacro(Vvinf,float);
  vlGetMacro(Vvinf,float);

  vlSetMacro(Wvinf,float);
  vlGetMacro(Wvinf,float);

  // derived functions from data in PLOT3D files
  void ComputeVelocity();
  void ComputeTemperature();
  void ComputePressure();

protected:
  void Execute();
  //Keep track of time change due to computing derived function.
  vlTimeStamp DerivedTime;

  //supplied in PLOT3D file
  float Fsmach;
  float Alpha;
  float Re;
  float Time;

  //parameters used in computing derived functions
  float R; 
  float Gamma;
  float Uvinf;
  float Vvinf;
  float Wvinf;
  
  //data read from file (grid always carried in Points field)
  vlFloatScalars *Density;
  vlFloatVectors *Momentum;
  vlFloatScalars *Energy;
  void ReleaseSupportingData();

  char *XYZFilename;
  char *QFilename;
  char *FunctionFilename;
  int GridNumber; //for multi-grid files, the one we're interested in
  int FileFormat; //various PLOT3D formats
  int FileType; // BINARY=0, ASCII=1
  int GetFileType(FILE *fp);
  int AutoRelease; //controls whether data is automatically freed after use

  //methods to read data
  void ReadGrid();
  void ReadDensity();
  void ReadMomentum();
  void ReadEnergy();
  void ReadFunctionFile();
};

#endif


