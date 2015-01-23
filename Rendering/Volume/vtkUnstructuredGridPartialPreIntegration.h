/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridPartialPreIntegration.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

// .NAME vtkUnstructuredGridPartialPreIntegration - performs piecewise linear ray integration.
//
// .SECTION Description
//
// vtkUnstructuredGridPartialPreIntegration performs piecewise linear ray
// integration.  This will give the same results as
// vtkUnstructuredGridLinearRayIntegration (with potentially a error due to
// table lookup quantization), but should be notably faster.  The algorithm
// used is given by Moreland and Angel, "A Fast High Accuracy Volume
// Renderer for Unstructured Data."
//
// This class is thread safe only after the first instance is created.
//

#ifndef vtkUnstructuredGridPartialPreIntegration_h
#define vtkUnstructuredGridPartialPreIntegration_h

#include "vtkRenderingVolumeModule.h" // For export macro
#include "vtkUnstructuredGridVolumeRayIntegrator.h"
#include "vtkMath.h" // For all the inline methods

class vtkPartialPreIntegrationTransferFunction;
class vtkVolumeProperty;

class VTKRENDERINGVOLUME_EXPORT vtkUnstructuredGridPartialPreIntegration : public vtkUnstructuredGridVolumeRayIntegrator
{
public:
  vtkTypeMacro(vtkUnstructuredGridPartialPreIntegration,
                       vtkUnstructuredGridVolumeRayIntegrator);
  static vtkUnstructuredGridPartialPreIntegration *New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  virtual void Initialize(vtkVolume *volume, vtkDataArray *scalars);

  virtual void Integrate(vtkDoubleArray *intersectionLengths,
                         vtkDataArray *nearIntersections,
                         vtkDataArray *farIntersections,
                         float color[4]);

  // Description:
  // Integrates a single ray segment.  \c color is blended with the result
  // (with \c color in front).  The result is written back into \c color.
  static void IntegrateRay(double length,
                           double intensity_front, double attenuation_front,
                           double intensity_back, double attenuation_back,
                           float color[4]);
  static void IntegrateRay(double length,
                           const double color_front[3],
                           double attenuation_front,
                           const double color_back[3],
                           double attenuation_back,
                           float color[4]);

  // Description:
  // Looks up Psi (as defined by Moreland and Angel, "A Fast High Accuracy
  // Volume Renderer for Unstructured Data") in a table.  The table must be
  // created first, which happens on the first instantiation of this class
  // or when BuildPsiTable is first called.
  static float Psi(float taufD, float taubD);
  static float *GetPsiTable(int &size);
  static void BuildPsiTable();

protected:
  vtkUnstructuredGridPartialPreIntegration();
  ~vtkUnstructuredGridPartialPreIntegration();

  vtkVolumeProperty *Property;

  vtkPartialPreIntegrationTransferFunction *TransferFunctions;
  vtkTimeStamp TransferFunctionsModified;
  int NumIndependentComponents;

//BTX
  enum {PSI_TABLE_SIZE = 512};
//ETX
  static float PsiTable[PSI_TABLE_SIZE*PSI_TABLE_SIZE];
  static int PsiTableBuilt;

private:
  vtkUnstructuredGridPartialPreIntegration(const vtkUnstructuredGridPartialPreIntegration&);  // Not implemented.
  void operator=(const vtkUnstructuredGridPartialPreIntegration&);  // Not implemented.
};

inline float vtkUnstructuredGridPartialPreIntegration::Psi(float taufD,
                                                           float taubD)
{
  float gammaf = taufD/(taufD+1);
  float gammab = taubD/(taubD+1);
  int gammafi = vtkMath::Floor(gammaf*PSI_TABLE_SIZE);
  int gammabi = vtkMath::Floor(gammab*PSI_TABLE_SIZE);
  return PsiTable[gammafi*PSI_TABLE_SIZE + gammabi];
}

inline float *vtkUnstructuredGridPartialPreIntegration::GetPsiTable(int &size)
{
  size = PSI_TABLE_SIZE;
  return PsiTable;
}

inline void vtkUnstructuredGridPartialPreIntegration::IntegrateRay(
                                                       double length,
                                                       double intensity_front,
                                                       double attenuation_front,
                                                       double intensity_back,
                                                       double attenuation_back,
                                                       float color[4])
{
  float taufD = length*attenuation_front;
  float taubD = length*attenuation_back;
  float Psi = vtkUnstructuredGridPartialPreIntegration::Psi(taufD, taubD);
  float zeta = static_cast<float>(exp(-0.5*(taufD+taubD)));
  float alpha = 1-zeta;

  float newintensity = (1-color[3])*(  intensity_front*(1-Psi)
                                     + intensity_back*(Psi-zeta) );
  // Is setting the RGB values the same the right thing to do?
  color[0] += newintensity;
  color[1] += newintensity;
  color[2] += newintensity;
  color[3] += (1-color[3])*alpha;
}

inline void vtkUnstructuredGridPartialPreIntegration::IntegrateRay(
                                                    double length,
                                                    const double color_front[3],
                                                    double attenuation_front,
                                                    const double color_back[3],
                                                    double attenuation_back,
                                                    float color[4])
{
  float taufD = length*attenuation_front;
  float taubD = length*attenuation_back;
  float Psi = vtkUnstructuredGridPartialPreIntegration::Psi(taufD, taubD);
  float zeta = static_cast<float>(exp(-0.5*(taufD+taubD)));
  float alpha = 1-zeta;

  color[0] += (1-color[3])*(color_front[0]*(1-Psi) + color_back[0]*(Psi-zeta));
  color[1] += (1-color[3])*(color_front[1]*(1-Psi) + color_back[1]*(Psi-zeta));
  color[2] += (1-color[3])*(color_front[2]*(1-Psi) + color_back[2]*(Psi-zeta));
  color[3] += (1-color[3])*alpha;
}

#endif //vtkUnstructuredGridPartialPreIntegration_h
