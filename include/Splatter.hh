/*=========================================================================

  Program:   Visualization Library
  Module:    Splatter.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlGaussianSplatter - splat points with Gaussian distribution
// .SECTION Description
// vlGaussianSplatter is a filter that injects input points into a structured 
// points dataset. As each point is injected, it "splats" or distributes 
// values to neighboring voxels in the structured points dataset. Data is
// distributed using a Gaussian distribution function. The distribution
// function is modified using scalar values (expands distribution) or normals
// (creates ellipsoidal distribution rather than spherical).

#ifndef __vlGaussianSplatter_h
#define __vlGaussianSplatter_h

#include "DS2SPtsF.hh"

class vlGaussianSplatter : public vlDataSetToStructuredPointsFilter 
{
public:
  vlGaussianSplatter();
  ~vlGaussianSplatter() {};
  char *GetClassName() {return "vlGaussianSplatter";};
  void PrintSelf(ostream& os, vlIndent indent);

  void ComputeModelBounds();

  void SetSampleDimensions(int i, int j, int k);
  void SetSampleDimensions(int dim[3]);
  vlGetVectorMacro(SampleDimensions,int);

  // Description:
  // Specify the radius of propagation of the splat. This value is expressed
  // as percentage  of the sampling structured point set. Smaller numbers 
  // greatly reduce execution time.
  vlSetClampMacro(Radius,float,0.0,1.0);
  vlGetMacro(Radius,float);

  // Description:
  // Multiply Gaussian splat distribution by this value.
  vlSetClampMacro(ScaleFactor,float,0.0,LARGE_FLOAT);
  vlGetMacro(ScaleFactor,float);

  // Description:
  // Specify sharpness of decay of splat
  vlSetMacro(ExponentFactor,float);
  vlGetMacro(ExponentFactor,float);

  // Description:
  // Control the shape of elliptical splatting. Eccentricity is the ratio
  // of the major axis (aligned along normal) to the minor (axes) aligned
  // along other two axes.
  vlSetClampMacro(Eccentricity,float,0.001,LARGE_FLOAT);
  vlGetMacro(Eccentricity,float);

  void SetModelBounds(float *bounds);
  void SetModelBounds(float xmin, float xmax, float ymin, float ymax, float zmin, float zmax);
  vlGetVectorMacro(ModelBounds,float);

  // Description:
  // Turn on/off the generation of elliptical splats.
  vlSetMacro(NormalWarping,int);
  vlGetMacro(NormalWarping,int);
  vlBooleanMacro(NormalWarping,int);

  // Description:
  // Turn on/off the scaling of splats by scalar value.
  vlSetMacro(ScalarWarping,int);
  vlGetMacro(ScalarWarping,int);
  vlBooleanMacro(ScalarWarping,int);

  // Description:
  // Turn on/off the capping of the outside parts of the structured point
  // set by setting to a specified cap value.
  vlSetMacro(Capping,int);
  vlGetMacro(Capping,int);
  vlBooleanMacro(Capping,int);
  
  // Description:
  // Specify the cap value to use.
  vlSetMacro(CapValue,float);
  vlGetMacro(CapValue,float);

protected:
  void Execute();
  void Cap(vlFloatScalars *s);

  int SampleDimensions[3]; // dimensions of volume to splat into
  float Radius; // maximum distance splat propagates (as fraction 0->1)
  float ExponentFactor; // scale exponent of gaussian function
  float ModelBounds[6]; // bounding box of splatting dimensions
  int NormalWarping; // on/off warping of splat via normal
  float Eccentricity;// elliptic distortion due to normals
  int ScalarWarping; // on/off warping of splat via scalar
  float ScaleFactor; // splat size influenced by scale factor
  int Capping; // Cap side of volume to close surfaces
  float CapValue; // value to use for capping

  // recursive propagation of splat
  void SplitIJK(int i, int idir, int j, int jdir, int k, int kdir);
  void SplitIJ(int i, int idir, int j, int jdir, int k);
  void SplitIK(int i, int idir, int j, int k, int kdir);
  void SplitJK(int i, int j, int jdir, int k, int kdir);
  void SplitI(int i, int idir, int j, int k);
  void SplitJ(int i, int j, int jdir, int k);
  void SplitK(int i, int j, int k, int kdir);

  float Gaussian(float x[3]);  
  float EccentricGaussian(float x[3]);  
  float ScalarSampling(float s) {return this->ScaleFactor * s;};
  float PositionSampling(float s) {return this->ScaleFactor;};
  void SetScalar(int idx, float dist2);

};

#endif


