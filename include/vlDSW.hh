/*=========================================================================

  Program:   Visualization Library
  Module:    vlDSW.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlDataSetWriter - write any type of vl dataset to file
// .SECTION Description
// vlDataSetWriter is an abstract class for mapper objects that write their 
// data to disk (or into a communications port).

#ifndef __vlDataSetWriter_hh
#define __vlDataSetWriter_hh

#include "vlDataW.hh"

class vlDataSetWriter : public vlDataWriter
{
public:
  vlDataSetWriter() {};
  ~vlDataSetWriter() {};
  char *GetClassName() {return "vlDataSetWriter";};
  void PrintSelf(ostream& os, vlIndent indent);

  void SetInput(vlDataSet *input);
  void SetInput(vlDataSet &input) {this->SetInput(&input);};
  vlDataSet *GetInput() {return (vlDataSet *)this->Input;};

protected:
  void WriteData();

};

#endif


