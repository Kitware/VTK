/*=========================================================================

  Program:   Visualization Library
  Module:    Pl3dRead.cc
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
#include <ctype.h>
#include "Pl3dRead.hh"
#include "ByteSwap.hh"

vlPLOT3DReader::vlPLOT3DReader()
{
  this->Fsmach = 0.0;
  this->Alpha = 0.0;
  this->Re = 0.0;
  this->Time = 0.0;

  this->R = 1.0;
  this->Gamma = 1.4;
  this->Uvinf = 0.0;
  this->Vvinf = 0.0;
  this->Wvinf = 0.0;

  this->Density = NULL;
  this->Momentum = NULL;
  this->Energy = NULL;

  this->XYZFilename = NULL;
  this->QFilename = NULL;
  this->FunctionFilename = NULL;

  this->FileFormat = WHOLE_SINGLE_GRID_NO_IBLANKING;
  this->FileType = BINARY;
  this->GridNumber = 0;
  this->AutoRelease = 1;
}

vlPLOT3DReader::~vlPLOT3DReader()
{
  this->ReleaseSupportingData();

  if ( this->XYZFilename ) delete [] this->XYZFilename;
  if ( this->QFilename ) delete [] this->QFilename;
  if ( this->FunctionFilename ) delete [] this->FunctionFilename;
}

void vlPLOT3DReader::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlPLOT3DReader::GetClassName()))
    {
    vlStructuredGridSource::PrintSelf(os,indent);

    os << indent << "XYZ Filename: " << this->XYZFilename << "\n";
    os << indent << "Q Filename: " << this->QFilename << "\n";
    os << indent << "Function Filename: " << this->FunctionFilename << "\n";
    }
}

void vlPLOT3DReader::Execute()
{
  FILE *xyzFp;
  vlFloatPoints *newPts;
  vlCellArray *newPolys;
//
// Initialize
//
  this->Initialize();

  if ((xyzFp = fopen(this->XYZFilename, "r")) == NULL)
    {
    vlErrorMacro(<< "File: " << this->XYZFilename << " not found");
    return;
    }
}

void vlPLOT3DReader::ReadGrid()
{
}

void vlPLOT3DReader::ReadDensity()
{
}

void vlPLOT3DReader::ReadMomentum()
{
}

void vlPLOT3DReader::ReadEnergy()
{
}

void vlPLOT3DReader::ReadFunctionFile()
{
}

// 
// The following are "derived functions"; that is, functions that are 
// computed from information provided in the PLOT3D files.
//
void vlPLOT3DReader::ReleaseSupportingData()
{
  if ( this->Density )
    {
    this->Density->UnRegister(this);
    this->Density = NULL;
    }

  if ( this->Momentum )
    {
    this->Momentum->UnRegister(this);
    this->Momentum = NULL;
    }

  if ( this->Energy )
    {
    this->Energy->UnRegister(this);
    this->Energy = NULL;
    }
}

void vlPLOT3DReader::ComputeVelocity()
{
  // make sure data is available
  if ( ! this->Density ) this->ReadDensity();
  if ( ! this->Momentum ) this->ReadMomentum();
  if ( ! this->Energy ) this->ReadEnergy();

  // Compute velocity


  // clean up after ourselves
  if ( this->AutoRelease ) this->ReleaseSupportingData();

  this->Modified();
  this->DerivedTime.Modified();
}

void vlPLOT3DReader::ComputeTemperature()
{
  this->Modified();
  this->DerivedTime.Modified();
}

void vlPLOT3DReader::ComputePressure()
{
  this->Modified();
  this->DerivedTime.Modified();
}

