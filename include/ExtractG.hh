/*=========================================================================

  Program:   Visualization Library
  Module:    ExtractG.hh
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
// .NAME vlExtractGeomtry - extract cells entirely within a apcecified sphere
// .SECTION Description
// vlExtractGeometry extracts from its input dataset all cells that are
// entirely within a specified sphere.

#ifndef __vlExtractGeometry_h
#define __vlExtractGeometry_h

#include "DS2UGrid.hh"

class vlExtractGeometry : public vlDataSetToUnstructuredGridFilter
{
public:
  vlExtractGeometry();
  ~vlExtractGeometry() {};
  char *GetClassName() {return "vlExtractGeometry";};
  void PrintSelf(ostream& os, vlIndent indent);

  vlSetClampMacro(Radius,float, 0.0, LARGE_FLOAT);
  vlGetMacro(Radius,float);

  vlSetVector3Macro(Center,float);
  vlGetVectorMacro(Center,float);

protected:
  // Usual data generation method
  void Execute();

  float Radius;
  float Center[3];
};

#endif


