/*=========================================================================

  Program:   Visualization Library
  Module:    vlPolyW.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlPolyWriter - write vl polygonal data
// .SECTION Description
// vlPolyWriter is a source object that writes ASCII or binary 
// polygonal data files in vl format. See text for format details.
// .SECTION Caveats
// Binary files written on one system may not be readable on other systems.

#ifndef __vlPolyWriter_hh
#define __vlPolyWriter_hh

#include "vlDSW.hh"
#include "PolyF.hh"

class vlPolyWriter : public vlDataSetWriter, public vlPolyFilter
{
public:
  vlPolyWriter() {};
  ~vlPolyWriter() {};
  char *GetClassName() {return "vlPolyWriter";};
  void PrintSelf(ostream& os, vlIndent indent);

  // Multiple inheritance/Object interface
  void Modified();
  unsigned long int GetMTime();
  void DebugOn();
  void DebugOff();

protected:
  void WriteData();
  void Execute() {this->Write();};

};

#endif


