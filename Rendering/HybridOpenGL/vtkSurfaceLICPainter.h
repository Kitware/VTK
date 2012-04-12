/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSurfaceLICPainter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSurfaceLICPainter - painter that performs LIC on the surface of
//  arbitrary geometry.
//
// .SECTION Description
//  vtkSurfaceLICPainter painter performs LIC on the surface of arbitrary
//  geometry. Point vectors are used as the vector field for generating the LIC.
//  The implementation is based on "Image Space Based Visualization on Unsteady
//  Flow on Surfaces" by Laramee, Jobard and Hauser appeared in proceedings of
//  IEEE Visualization '03, pages 131-138.

#ifndef __vtkSurfaceLICPainter_h
#define __vtkSurfaceLICPainter_h

#include "vtkRenderingHybridOpenGLModule.h" // For export macro
#include "vtkPainter.h"

class vtkRenderWindow;

class VTKRENDERINGHYBRIDOPENGL_EXPORT vtkSurfaceLICPainter : public vtkPainter
{
public:
  static vtkSurfaceLICPainter* New();
  vtkTypeMacro(vtkSurfaceLICPainter, vtkPainter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Release any graphics resources that are being consumed by this mapper.
  // The parameter window could be used to determine which graphic
  // resources to release. In this case, releases the display lists.
  virtual void ReleaseGraphicsResources(vtkWindow *);

  // Description:
  // Get the output data object from this painter.
  // Overridden to pass the input points (or cells) vectors as the tcoords to
  // the deletage painters. This is required by the internal GLSL shader
  // programs used for generating LIC.
  virtual vtkDataObject* GetOutput();

  // Description:
  // Enable/Disable this painter.
  vtkSetMacro(Enable, int);
  vtkGetMacro(Enable, int);
  vtkBooleanMacro(Enable, int);

  // Description:
  // Set the vectors to used for applying LIC. By default point vectors are
  // used. Arguments are same as those passed to
  // vtkAlgorithm::SetInputArrayToProcess except the first 3 arguments i.e. idx,
  // port, connection.
  void SetInputArrayToProcess(int fieldAssociation, const char *name);
  void SetInputArrayToProcess(int fieldAssociation, int fieldAttributeType);

  // Description:
  // Enable/Disable enhanced LIC that improves image quality by increasing
  // inter-streamline contrast while suppressing artifacts. Enhanced LIC
  // performs two passes of LIC, with a 3x3 Laplacian high-pass filter in
  // between that processes the output of pass #1 LIC and forwards the result
  // as the input 'noise' to pass #2 LIC. This flag is automatically turned
  // off during user interaction.
  vtkSetMacro( EnhancedLIC, int );
  vtkGetMacro( EnhancedLIC, int );
  vtkBooleanMacro( EnhancedLIC, int );

  // Description:
  // Get/Set the number of integration steps in each direction.
  vtkSetMacro(NumberOfSteps, int);
  vtkGetMacro(NumberOfSteps, int);

  // Description:
  // Get/Set the step size (in pixels).
  vtkSetMacro(StepSize, double);
  vtkGetMacro(StepSize, double);

  // Description:
  // Control the contribution of the LIC in the final output image.
  // 0.0 produces same result as disabling LIC altogether, while 1.0 implies
  // show LIC result alone.
  vtkSetClampMacro(LICIntensity, double, 0.0, 1.0);
  vtkGetMacro(LICIntensity, double);

  // Description:
  // Check if PrepareForRendering passes.
  int GetRenderingPreparationSuccess()
      { return this->RenderingPreparationSuccess; }

  // Description:
  // Check if the LIC process runs properly.
  int GetLICSuccess() { return this->LICSuccess; }

  // Description:
  // Returns true is the rendering context supports extensions needed by this
  // painter.
  static bool IsSupported(vtkRenderWindow*);
//BTX
protected:
  vtkSurfaceLICPainter();
  ~vtkSurfaceLICPainter();

  // Description:
  // Computes data bounds.
  void GetBounds(vtkDataObject* data, double bounds[6]);

  // Description:
  // Take part in garbage collection.
  virtual void ReportReferences(vtkGarbageCollector *collector);

  // Description:
  // Some subclasses may need to do some preprocessing
  // before the actual rendering can be done eg. build efficient
  // representation for the data etc. This should be done here.
  // This method get called after the ProcessInformation()
  // but before RenderInternal().
  virtual void PrepareForRendering(vtkRenderer*, vtkActor*);

  // Description:
  // Performs the actual rendering. Subclasses may override this method.
  // default implementation merely call a Render on the DelegatePainter,
  // if any. When RenderInternal() is called, it is assured that the
  // DelegatePainter is in sync with this painter i.e. UpdateDelegatePainter()
  // has been called.
  virtual void RenderInternal(vtkRenderer* renderer, vtkActor* actor,
                              unsigned long typeflags, bool forceCompileOnly);

  // Description:
  // Prepares output data. Returns true if vectors are available.
  bool PrepareOutput();
  bool FixTCoords(vtkDataSet* ds);

  // Description:
  // Returns true when rendering LIC is possible.
  bool CanRenderLIC(vtkRenderer*, vtkActor*);

  // Unit is a pixel length.
  int    NumberOfSteps;
  double StepSize;

  int    Enable;
  int    EnhancedLIC;
  int    RenderingPreparationSuccess;
  int    LICSuccess;
  double LICIntensity;

private:
  vtkSurfaceLICPainter(const vtkSurfaceLICPainter&); // Not implemented.
  void operator=(const vtkSurfaceLICPainter&); // Not implemented.

  vtkDataObject* Output;
  class vtkInternals;
  vtkInternals* Internals;
//ETX
};

#endif
