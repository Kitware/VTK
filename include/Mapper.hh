/*=========================================================================

  Program:   Visualization Library
  Module:    Mapper.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlMapper - abstract class specifies interface to map data to graphics primitives
// .SECTION Description
// vlMapper is an abstract class to specify interface between data and 
// graphics primitives. Subclasses of vlMapper map data through a 
// lookuptable and control the creation of rendering primitives that
// interface to the graphics library. The mapping can be controlled by 
// supplying a lookup table and specifying a scalar range to map data
// through.

#ifndef __vlMapper_hh
#define __vlMapper_hh

#include "Object.hh"
#include "GeomPrim.hh"
#include "Lut.hh"
#include "DataSet.hh"

class vlRenderer;

class vlMapper : public vlObject 
{
public:
  vlMapper();
  ~vlMapper();
  char *GetClassName() {return "vlMapper";};
  void PrintSelf(ostream& os, vlIndent indent);
  void operator=(const vlMapper& m);

  unsigned long int GetMTime();

  void SetStartRender(void (*f)(void *), void *arg);
  void SetEndRender(void (*f)(void *), void *arg);
  void SetStartRenderArgDelete(void (*f)(void *));
  void SetEndRenderArgDelete(void (*f)(void *));

  // Description:
  // Method initiates the mapping process. Generally sent by the actor 
  // as each frame is rendered.
  virtual void Render(vlRenderer *) = 0;

  void SetLookupTable(vlLookupTable *lut);
  void SetLookupTable(vlLookupTable& lut) {this->SetLookupTable(&lut);};
  vlLookupTable *GetLookupTable();

  // Description:
  // Create default lookup table. Generally used to create one when none
  // is available.
  virtual void CreateDefaultLookupTable();

  // Description:
  // Turn on/off flag to control whether scalar data is used to color objects.
  vlSetMacro(ScalarsVisible,int);
  vlGetMacro(ScalarsVisible,int);
  vlBooleanMacro(ScalarsVisible,int);

  // Description:
  // Specify range in terms of (smin,smax) through which to map scalars
  // into lookup table.
  vlSetVector2Macro(ScalarRange,float);
  vlGetVectorMacro(ScalarRange,float,2);

  // Description:
  // Return bounding box of data in terms of (xmin,xmax, ymin,ymax, zmin,zmax).
  // Used in the rendering process to automatically create a camera in the 
  // proper initial configuration.
  virtual float *GetBounds() = 0;

  float *GetCenter();
  virtual vlDataSet *GetInput() {return this->Input;};

protected:
  vlDataSet *Input;

  void (*StartRender)(void *);
  void (*StartRenderArgDelete)(void *);
  void *StartRenderArg;
  void (*EndRender)(void *);
  void (*EndRenderArgDelete)(void *);
  void *EndRenderArg;
  vlLookupTable *LookupTable;
  int ScalarsVisible;
  vlTimeStamp BuildTime;
  float ScalarRange[2];
  int SelfCreatedLookupTable;

};

#endif


