/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMandelbrotSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageMandelbrotSource - Mandelbrot image.
// .SECTION Description
// vtkImageMandelbrotSource creates an unsigned char image of the Mandelbrot
// set.  The values in the image are the number of iterations it takes for
// the magnitude of the value to get over 2.  The equation repeated is
// z = z^2 + C (z and C are complex).  Initial value of z is zero, and the
// real value of C is mapped onto the x axis, and the imaginary value of C
// is mapped onto the Y Axis.  I was thinking of extending this source
// to generate Julia Sets (initial value of Z varies).  This would be 4
// possible parameters to vary, but there are no more 4d images :(
// The third dimension (z axis) is the imaginary value of the initial value.

#ifndef vtkImageMandelbrotSource_h
#define vtkImageMandelbrotSource_h

#include "vtkImagingSourcesModule.h" // For export macro
#include "vtkImageAlgorithm.h"

class VTKIMAGINGSOURCES_EXPORT vtkImageMandelbrotSource : public vtkImageAlgorithm
{
public:
  static vtkImageMandelbrotSource *New();
  vtkTypeMacro(vtkImageMandelbrotSource,vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the extent of the whole output Volume.
  void SetWholeExtent(int extent[6]);
  void SetWholeExtent(int minX, int maxX, int minY, int maxY,
                            int minZ, int maxZ);
  vtkGetVector6Macro(WholeExtent,int);

  // Description:
  // This flag determines whether the Size or spacing of
  // a data set remain constant (when extent is changed).
  // By default, size remains constant.
  vtkSetMacro(ConstantSize, int);
  vtkGetMacro(ConstantSize, int);
  vtkBooleanMacro(ConstantSize, int);

  // Description:
  // Set the projection from  the 4D space (4 parameters / 2 imaginary numbers)
  // to the axes of the 3D Volume.
  // 0=C_Real, 1=C_Imaginary, 2=X_Real, 4=X_Imaginary
  void SetProjectionAxes(int x, int y, int z);
  void SetProjectionAxes(int a[3]) {this->SetProjectionAxes(a[0],a[1],a[2]);}
  vtkGetVector3Macro(ProjectionAxes, int);

  // Description:
  // Imaginary and real value for C (constant in equation)
  // and X (initial value).
  vtkSetVector4Macro(OriginCX, double);
  //void SetOriginCX(double cReal, double cImag, double xReal, double xImag);
  vtkGetVector4Macro(OriginCX, double);

  // Description:
  // Imaginary and real value for C (constant in equation)
  // and X (initial value).
  vtkSetVector4Macro(SampleCX, double);
  //void SetOriginCX(double cReal, double cImag, double xReal, double xImag);
  vtkGetVector4Macro(SampleCX, double);

  // Description:
  // Just a different way of setting the sample.
  // This sets the size of the 4D volume.
  // SampleCX is computed from size and extent.
  // Size is ignored when a dimension i 0 (collapsed).
  void SetSizeCX(double cReal, double cImag, double xReal, double xImag);
  double *GetSizeCX();
  void GetSizeCX(double s[4]);

  // Description:
  // The maximum number of cycles run to see if the value goes over 2
  vtkSetClampMacro(MaximumNumberOfIterations, unsigned short,
                   static_cast<unsigned short>(1),
                   static_cast<unsigned short>(5000));
  vtkGetMacro(MaximumNumberOfIterations, unsigned short);

  // Description:
  // Convienence for Viewer.  Pan 3D volume relative to spacing.
  // Zoom constant factor.
  void Zoom(double factor);
  void Pan(double x, double y, double z);

  // Description:
  // Convienence for Viewer.  Copy the OriginCX and the SpacingCX.
  // What about other parameters ???
  void CopyOriginAndSample(vtkImageMandelbrotSource *source);

  // Description:
  // Set/Get a subsample rate.
  vtkSetClampMacro(SubsampleRate, int, 1, VTK_INT_MAX);
  vtkGetMacro(SubsampleRate, int);

protected:
  vtkImageMandelbrotSource();
  ~vtkImageMandelbrotSource();

  int ProjectionAxes[3];

  // WholeExtent in 3 space (after projection).
  int WholeExtent[6];

  // Complex constant/initial-value at origin.
  double OriginCX[4];
  // Initial complex value at origin.
  double SampleCX[4];
  unsigned short MaximumNumberOfIterations;

  // A temporary vector that is computed as needed.
  // It is used to return a vector.
  double SizeCX[4];

  // A flag for keeping size constant (vs. keeping the spacing).
  int ConstantSize;

  int SubsampleRate;

  // see vtkAlgorithm for details
  virtual int RequestData(vtkInformation *request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);

  virtual int RequestInformation (vtkInformation *,
                                  vtkInformationVector**,
                                  vtkInformationVector *);
  double EvaluateSet(double p[4]);
private:
  vtkImageMandelbrotSource(const vtkImageMandelbrotSource&);  // Not implemented.
  void operator=(const vtkImageMandelbrotSource&);  // Not implemented.
};


#endif


