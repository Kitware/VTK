/*=========================================================================

  Program:   Visualization Library
  Module:    StrPts.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlStructuredPoints - topologically and geometrically regular array of data
// .SECTION Description
// vlStructuredPoints is a data object that is a concrete implementation of
// vlDataSet. vlStructuredPoints represents a geometric structure that is 
// a topological and geometrical regular array of points. Examples include
// volumes (voxel data) and pixmaps. 

#ifndef __vlStructuredPoints_h
#define __vlStructuredPoints_h

#include "StrData.hh"

class vlStructuredPoints : public vlStructuredData {
public:
  vlStructuredPoints();
  vlStructuredPoints(const vlStructuredPoints& v);
  ~vlStructuredPoints();
  char *GetClassName() {return "vlStructuredPoints";};
  char *GetDataType() {return "vlStructuredPoints";};
  void PrintSelf(ostream& os, vlIndent indent);

  // dataset interface
  vlDataSet *MakeObject() {return new vlStructuredPoints(*this);};
  float *GetPoint(int ptId);
  vlCell *GetCell(int cellId);
  void Initialize();
  int FindCell(float x[3], vlCell *cell, float tol2, int& subId, float pcoords[3]);
  int GetCellType(int cellId);

  // Description:
  // Set the aspect ratio of the cubical cells that compose the structured
  // point set.
  vlSetVector3Macro(AspectRatio,float);
  vlGetVectorMacro(AspectRatio,float,3);

  // Description:
  // Set the origin of the data. The origin plus aspect ratio determine the
  // position in space of the structured points.
  vlSetVector3Macro(Origin,float);
  vlGetVectorMacro(Origin,float,3);

protected:
  float Origin[3];
  float AspectRatio[3];
};

#endif
