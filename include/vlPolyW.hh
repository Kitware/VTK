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

#include "vlDataW.hh"
#include "PolyData.hh"

class vlPolyWriter : public vlDataWriter
{
public:
  vlPolyWriter() {};
  ~vlPolyWriter() {};
  char *GetClassName() {return "vlPolyWriter";};
  void PrintSelf(ostream& os, vlIndent indent);

  void SetInput(vlPolyData *input);
  void SetInput(vlPolyData &input) {this->SetInput(&input);};
  vlPolyData *GetInput() {return (vlPolyData *)this->Input;};
                               
protected:
  void WriteData();

};

#endif


