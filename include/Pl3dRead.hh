/*=========================================================================

  Program:   Visualization Library
  Module:    Pl3dRead.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlPLOT3DReader - read PLOT3D data files
// .SECTION Description
// vlPLOT3D is a reader object that reads PLOT3D formatted files and generates
// a structured grid on output. PLOT3D is a computer graphics program designed
// to visualizae the grids and solutions of computational fluid dynamics.
// Please see the "PLOT3D User's Manual" available from NASA Ames Research 
// Center, Moffett Field CA.

#ifndef __vlPLOT3DReader_h
#define __vlPLOT3DReader_h

#include <stdio.h>
#include "SGridSrc.hh"
#include "FScalars.hh"
#include "FVectors.hh"

// file formats
#define WHOLE_SINGLE_GRID_NO_IBLANKING 0
#define WHOLE_MULTI_GRID_NO_IBLANKING 2

class vlPLOT3DReader : public vlStructuredGridSource 
{
public:
  vlPLOT3DReader();
  ~vlPLOT3DReader();
  char *GetClassName() {return "vlPLOT3DReader";};
  void PrintSelf(ostream& os, vlIndent indent);

  // Description:
  // Specify the PLOT3D file format to use
  vlSetClampMacro(FileFormat,int,0,7);
  vlGetMacro(FileFormat,int);

  // Description:
  // Set/Get the PLOT3D geometry filename.
  vlSetStringMacro(XYZFilename);
  vlGetStringMacro(XYZFilename);

  // Description:
  // Set/Get the PLOT3D solution filename.
  vlSetStringMacro(QFilename);
  vlGetStringMacro(QFilename);

  // Description:
  // Set/Get the PLOT3D function filename.
  vlSetStringMacro(FunctionFilename);
  vlGetStringMacro(FunctionFilename);


  // Description:
  // Specify the grid to read.
  vlSetMacro(GridNumber,int);
  vlGetMacro(GridNumber,int);

  // Description:
  // Specify the scalar function to extract. If =-1, then no scalar 
  // function is extracted.
  vlSetMacro(ScalarFunctionNumber,int);
  vlGetMacro(ScalarFunctionNumber,int);

  // Description:
  // Specify the vector function to extract. If =-1, then no vector
  // function is extracted.
  vlSetMacro(VectorFunctionNumber,int);
  vlGetMacro(VectorFunctionNumber,int);

  // Description:
  // Specify which function to extract from the function file. If =-1, 
  // then no function is extracted.
  vlSetMacro(FunctionFileFunctionNumber,int);
  vlGetMacro(FunctionFileFunctionNumber,int);


  // these are read from PLOT3D file
  // Description:
  // Get the free-stream mach number.
  vlGetMacro(Fsmach,float);

  // Description:
  // Get the angle of attack.
  vlGetMacro(Alpha,float);

  // Description:
  // Get the Reynold's number.
  vlGetMacro(Re,float);

  // Description:
  // Get the total integration time.
  vlGetMacro(Time,float);

  // Description:
  // Set/Get the gas constant.
  vlSetMacro(R,float);
  vlGetMacro(R,float);

  // Description:
  // Set/Get the ratio of specific heats.
  vlSetMacro(Gamma,float);
  vlGetMacro(Gamma,float);

  // Description:
  // Set/Get the x-component of the free-stream velocity.
  vlSetMacro(Uvinf,float);
  vlGetMacro(Uvinf,float);

  // Description:
  // Set/Get the y-component of the free-stream velocity.
  vlSetMacro(Vvinf,float);
  vlGetMacro(Vvinf,float);

  // Description:
  // Set/Get the z-component of the free-stream velocity.
  vlSetMacro(Wvinf,float);
  vlGetMacro(Wvinf,float);

protected:
  void Execute();
  int GetFileType(FILE *fp);

  //plot3d filenames
  int FileFormat; //various PLOT3D formats
  char *XYZFilename;
  char *QFilename;
  char *FunctionFilename;

  //flags describing data to be read
  int GridNumber; //for multi-grid files, the one we're interested in
  int ScalarFunctionNumber;
  int VectorFunctionNumber;
  int FunctionFileFunctionNumber;
  void MapFunction(int fNumber);

  //temporary variables used during read
  float *TempStorage;
  int NumPts;
  int NumGrids;

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
  
  //methods to read data
  int ReadASCIIGrid(FILE *fp);
  int ReadASCIISolution(FILE *fp);
  int ReadASCIIFunctionFile(FILE *fp);
  int ReadBinaryGrid(FILE *fp);
  int ReadBinarySolution(FILE *fp);
  int ReadBinaryFunctionFile(FILE *fp);
  vlFloatPoints *Grid;
  vlFloatScalars *Density;
  vlFloatScalars *Energy;
  vlFloatVectors *Momentum;

  // derived functions from data in PLOT3D files
  void ComputeDensity();
  void ComputePressure();
  void ComputeTemperature();
  void ComputeEnthalpy();
  void ComputeInternalEnergy();
  void ComputeKineticEnergy();
  void ComputeVelocityMagnitude();
  void ComputeStagnationEnergy();
  void ComputeEntropy();
  void ComputeSwirl();

  void ComputeVelocity();
  void ComputeVorticity();
  void ComputeMomentum();
  void ComputePressureGradient();
};

#endif


