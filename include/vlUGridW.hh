/*=========================================================================

  Program:   Visualization Library
  Module:    vlUGridW.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlUnstructuredGridWriter - write vl unstructured grid data file
// .SECTION Description
// vlUnstructuredGridWriter is a source object that writes ASCII or binary 
// unstructured grid data files in vl format. See text for format details.
// .SECTION Caveats
// Binary files written on one system may not be readable on other systems.

#ifndef __vlUnstructuredGridWriter_hh
#define __vlUnstructuredGridWriter_hh

#include "vlDataW.hh"
#include "UGridF.hh"

class vlUnstructuredGridWriter : public vlDataWriter, public vlUnstructuredGridFilter
{
public:
  vlUnstructuredGridWriter() {};
  ~vlUnstructuredGridWriter() {};
  char *GetClassName() {return "vlUnstructuredGridWriter";};

protected:
  void WriteData();
  void Execute() {this->Write();};

};

#endif


