/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPLOT3DReader.hh
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
// .NAME vtkPLOT3DReader - read PLOT3D data files
// .SECTION Description
// vtkPLOT3DReader is a reader object that reads PLOT3D formatted files and 
// generates a structured grid on output. PLOT3D is a computer graphics 
// program designed to visualize the grids and solutions of computational 
// fluid dynamics. Please see the "PLOT3D User's Manual" available from 
// NASA Ames Research Center, Moffett Field CA.
//
// PLOT3D files consist of a grid file (also known as XYZ file), an 
// optional solution file (also known as a Q file), and an optional function 
// file that contains user created data. The Q file contains solution 
// information as follows: the four parameters free stream mach number 
// (Fsmach), angle of attack (Alpha), Reynolds number (Re), and total 
// integration time (Time). In addition, the solution file contains 
// the flow density (scalar), flow momentum (vector), and flow energy (scalar).
//
// The reader can generate additional scalars and vectors (or "functions")
// from this information. To use vtkPLOT3DReader, you must specify the 
// particular function number for the scalar and vector you want to visualize.
// This implementation of the reader provides the following functions. The
// scalar functions are:
//    -1  - don't read or compute any scalars
//    100 - density
//    110 - pressure
//    120 - temperature
//    130 - enthalpy
//    140 - internal energy
//    144 - kinetic energy
//    153 - velocity magnitude
//    163 - stagnation energy
//    170 - entropy
//    184 - swirl.
//
// The vector functions are:
//    -1  - don't read or compute any vectors
//    200 - velocity
//    201 - vorticity
//    202 - momentum
//    210 - pressure gradient.
//
// (Other functions are described in the PLOT3D spec, but only those listed are
// implemented here.) Note that by default, this reader creates the density 
// scalar (100) and momentum vector (202) as output. (These are just read in
// from the solution file.) Please note that the validity of computation is
// a function of this class's gas constants (R, Gamma) and the equations used.
// They may not be suitable for your computational domain.
//
// The format of the function file is as follows. An integer indicating 
// number of grids, then an integer specifying number of functions per each 
// grid. This is followed by the (integer) dimensions of each grid in the 
// file. Finally, for each grid, and for each function, a float value per 
// each point in the current grid. Note: if both a function from the function
// file is specified, as well as a scalar from the solution file (or derived
// from the solution file), the function file takes precedence.

#ifndef __vtkPLOT3DReader_h
#define __vtkPLOT3DReader_h

#include <stdio.h>
#include "vtkStructuredGridSource.hh"
#include "vtkFloatScalars.hh"
#include "vtkFloatVectors.hh"

// file formats
#define VTK_WHOLE_SINGLE_GRID_NO_IBLANKING 0
#define VTK_WHOLE_MULTI_GRID_NO_IBLANKING 2

class vtkPLOT3DReader : public vtkStructuredGridSource 
{
public:
  vtkPLOT3DReader();
  ~vtkPLOT3DReader();
  char *GetClassName() {return "vtkPLOT3DReader";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the PLOT3D file format to use
  vtkSetClampMacro(FileFormat,int,0,7);
  vtkGetMacro(FileFormat,int);

  // Description:
  // Set/Get the PLOT3D geometry filename.
  vtkSetStringMacro(XYZFilename);
  vtkGetStringMacro(XYZFilename);

  // Description:
  // Set/Get the PLOT3D solution filename.
  vtkSetStringMacro(QFilename);
  vtkGetStringMacro(QFilename);

  // Description:
  // Set/Get the PLOT3D function filename.
  vtkSetStringMacro(FunctionFilename);
  vtkGetStringMacro(FunctionFilename);

  // Description:
  // Specify the grid to read.
  vtkSetMacro(GridNumber,int);
  vtkGetMacro(GridNumber,int);

  // Description:
  // Specify the scalar function to extract. If =-1, then no scalar 
  // function is extracted.
  vtkSetMacro(ScalarFunctionNumber,int);
  vtkGetMacro(ScalarFunctionNumber,int);

  // Description:
  // Specify the vector function to extract. If =-1, then no vector
  // function is extracted.
  vtkSetMacro(VectorFunctionNumber,int);
  vtkGetMacro(VectorFunctionNumber,int);

  // Description:
  // Specify which function to extract from the function file. If =-1, 
  // then no function is extracted.
  vtkSetMacro(FunctionFileFunctionNumber,int);
  vtkGetMacro(FunctionFileFunctionNumber,int);

  // these are read from PLOT3D file
  // Description:
  // Get the free-stream mach number.
  vtkGetMacro(Fsmach,float);

  // Description:
  // Get the angle of attack.
  vtkGetMacro(Alpha,float);

  // Description:
  // Get the Reynold's number.
  vtkGetMacro(Re,float);

  // Description:
  // Get the total integration time.
  vtkGetMacro(Time,float);

  // Description:
  // Set/Get the gas constant.
  vtkSetMacro(R,float);
  vtkGetMacro(R,float);

  // Description:
  // Set/Get the ratio of specific heats.
  vtkSetMacro(Gamma,float);
  vtkGetMacro(Gamma,float);

  // Description:
  // Set/Get the x-component of the free-stream velocity.
  vtkSetMacro(Uvinf,float);
  vtkGetMacro(Uvinf,float);

  // Description:
  // Set/Get the y-component of the free-stream velocity.
  vtkSetMacro(Vvinf,float);
  vtkGetMacro(Vvinf,float);

  // Description:
  // Set/Get the z-component of the free-stream velocity.
  vtkSetMacro(Wvinf,float);
  vtkGetMacro(Wvinf,float);

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
  void MapFunction(int fNumber,vtkPointData *outputPD);

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
  int ReadBinaryGrid(FILE *fp,vtkStructuredGrid *output);
  int ReadBinarySolution(FILE *fp, vtkStructuredGrid *output);
  vtkFloatPoints *Grid;
  vtkFloatScalars *Density;
  vtkFloatScalars *Energy;
  vtkFloatVectors *Momentum;

  // derived functions from data in PLOT3D files
  void ComputeDensity(vtkPointData *outputPD);
  void ComputePressure(vtkPointData *outputPD);
  void ComputeTemperature(vtkPointData *outputPD);
  void ComputeEnthalpy(vtkPointData *outputPD);
  void ComputeInternalEnergy(vtkPointData *outputPD);
  void ComputeKineticEnergy(vtkPointData *outputPD);
  void ComputeVelocityMagnitude(vtkPointData *outputPD);
  void ComputeStagnationEnergy(vtkPointData *outputPD);
  void ComputeEntropy(vtkPointData *outputPD);
  void ComputeSwirl(vtkPointData *outputPD);

  void ComputeVelocity(vtkPointData *outputPD);
  void ComputeVorticity(vtkPointData *outputPD);
  void ComputeMomentum(vtkPointData *outputPD);
  void ComputePressureGradient(vtkPointData *outputPD);
};

#endif


