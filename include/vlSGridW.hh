/*=========================================================================

  Program:   Visualization Library
  Module:    vlSGridW.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlStructuredGridWriter - write vl structured grid data file
// .SECTION Description
// vlStructuredGridWriter is a source object that writes ASCII or binary 
// structured grid data files in vl format. See text for format details.
// .SECTION Caveats
// Binary files written on one system may not be readable on other systems.

#ifndef __vlStructuredGridWriter_hh
#define __vlStructuredGridWriter_hh

#include "vlDataW.hh"
#include "SGrid.hh"

class vlStructuredGridWriter : public vlDataWriter
{
public:
  vlStructuredGridWriter() {};
  ~vlStructuredGridWriter() {};
  char *GetClassName() {return "vlStructuredGridWriter";};
  void PrintSelf(ostream& os, vlIndent indent);

  void SetInput(vlStructuredGrid *input);
  void SetInput(vlStructuredGrid &input) {this->SetInput(&input);};
  vlStructuredGrid *GetInput() {return (vlStructuredGrid *)this->Input;};
                               
protected:
  void WriteData();

};

#endif


