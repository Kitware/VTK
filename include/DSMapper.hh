/*=========================================================================

  Program:   Visualization Library
  Module:    DSMapper.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
//
// DataSetMapper takes DataSet as input
//
#ifndef __vlDataSetMapper_h
#define __vlDataSetMapper_h

#include "Mapper.hh"
#include "DataSet.hh"
#include "Renderer.hh"

class vlDataSetMapper : public vlMapper 
{
public:
  vlDataSetMapper();
  ~vlDataSetMapper();
  char *GetClassName() {return "vlDataSetMapper";};
  void PrintSelf(ostream& os, vlIndent indent);
  void Render(vlRenderer *ren);
  float *GetBounds();
  virtual void SetInput(vlDataSet *in);
  virtual vlDataSet* GetInput();

protected:
  int CreateMapper();
  vlDataSet *Input;
  vlMapper *Mapper;
};

#endif


