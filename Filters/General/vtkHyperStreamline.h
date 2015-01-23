/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperStreamline.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkHyperStreamline - generate hyperstreamline in arbitrary dataset
// .SECTION Description
// vtkHyperStreamline is a filter that integrates through a tensor field to
// generate a hyperstreamline. The integration is along the maximum eigenvector
// and the cross section of the hyperstreamline is defined by the two other
// eigenvectors. Thus the shape of the hyperstreamline is "tube-like", with
// the cross section being elliptical. Hyperstreamlines are used to visualize
// tensor fields.
//
// The starting point of a hyperstreamline can be defined in one of two ways.
// First, you may specify an initial position. This is a x-y-z global
// coordinate. The second option is to specify a starting location. This is
// cellId, subId, and  cell parametric coordinates.
//
// The integration of the hyperstreamline occurs through the major eigenvector
// field. IntegrationStepLength controls the step length within each cell
// (i.e., this is the fraction of the cell length). The length of the
// hyperstreamline is controlled by MaximumPropagationDistance. This parameter
// is the length of the hyperstreamline in units of distance. The tube itself
// is composed of many small sub-tubes - NumberOfSides controls the number of
// sides in the tube, and StepLength controls the length of the sub-tubes.
//
// Because hyperstreamlines are often created near regions of singularities, it
// is possible to control the scaling of the tube cross section by using a
// logarithmic scale. Use LogScalingOn to turn this capability on. The Radius
// value controls the initial radius of the tube.

// .SECTION See Also
// vtkTensorGlyph vtkStreamer

#ifndef vtkHyperStreamline_h
#define vtkHyperStreamline_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

#define VTK_INTEGRATE_FORWARD 0
#define VTK_INTEGRATE_BACKWARD 1
#define VTK_INTEGRATE_BOTH_DIRECTIONS 2

#define VTK_INTEGRATE_MAJOR_EIGENVECTOR 0
#define VTK_INTEGRATE_MEDIUM_EIGENVECTOR 1
#define VTK_INTEGRATE_MINOR_EIGENVECTOR 2


class vtkHyperArray;

class VTKFILTERSGENERAL_EXPORT vtkHyperStreamline : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkHyperStreamline,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object with initial starting position (0,0,0); integration
  // step length 0.2; step length 0.01; forward integration; terminal
  // eigenvalue 0.0; number of sides 6; radius 0.5; and logarithmic scaling
  // off.
  static vtkHyperStreamline *New();

  // Description:
  // Specify the start of the hyperstreamline in the cell coordinate system.
  // That is, cellId and subId (if composite cell), and parametric coordinates.
  void SetStartLocation(vtkIdType cellId, int subId, double pcoords[3]);

  // Description:
  // Specify the start of the hyperstreamline in the cell coordinate system.
  // That is, cellId and subId (if composite cell), and parametric coordinates.
  void SetStartLocation(vtkIdType cellId, int subId, double r, double s,
                        double t);

  // Description:
  // Get the starting location of the hyperstreamline in the cell coordinate
  // system. Returns the cell that the starting point is in.
  vtkIdType GetStartLocation(int& subId, double pcoords[3]);

  // Description:
  // Specify the start of the hyperstreamline in the global coordinate system.
  // Starting from position implies that a search must be performed to find
  // initial cell to start integration from.
  void SetStartPosition(double x[3]);

  // Description:
  // Specify the start of the hyperstreamline in the global coordinate system.
  // Starting from position implies that a search must be performed to find
  // initial cell to start integration from.
  void SetStartPosition(double x, double y, double z);

  // Description:
  // Get the start position of the hyperstreamline in global x-y-z coordinates.
  double *GetStartPosition();

  // Description:
  // Set / get the maximum length of the hyperstreamline expressed as absolute
  // distance (i.e., arc length) value.
  vtkSetClampMacro(MaximumPropagationDistance,double,0.0,VTK_DOUBLE_MAX);
  vtkGetMacro(MaximumPropagationDistance,double);

  // Description:
  // Set / get the eigenvector field through which to ingrate. It is
  // possible to integrate using the major, medium or minor
  // eigenvector field.  The major eigenvector is the eigenvector
  // whose corresponding eigenvalue is closest to positive infinity.
  // The minor eigenvector is the eigenvector whose corresponding
  // eigenvalue is closest to negative infinity.  The medium
  // eigenvector is the eigenvector whose corresponding eigenvalue is
  // between the major and minor eigenvalues.
  vtkSetClampMacro(IntegrationEigenvector,int,
                   VTK_INTEGRATE_MAJOR_EIGENVECTOR,
                   VTK_INTEGRATE_MINOR_EIGENVECTOR);
  vtkGetMacro(IntegrationEigenvector,int);
  void SetIntegrationEigenvectorToMajor()
    {this->SetIntegrationEigenvector(VTK_INTEGRATE_MAJOR_EIGENVECTOR);};
  void SetIntegrationEigenvectorToMedium()
    {this->SetIntegrationEigenvector(VTK_INTEGRATE_MEDIUM_EIGENVECTOR);};
  void SetIntegrationEigenvectorToMinor()
    {this->SetIntegrationEigenvector(VTK_INTEGRATE_MINOR_EIGENVECTOR);};

  // Description:
  // Use the major eigenvector field as the vector field through which
  // to integrate.  The major eigenvector is the eigenvector whose
  // corresponding eigenvalue is closest to positive infinity.
  void IntegrateMajorEigenvector()
    {this->SetIntegrationEigenvectorToMajor();};

  // Description:
  // Use the medium eigenvector field as the vector field through which
  // to integrate. The medium eigenvector is the eigenvector whose
  // corresponding eigenvalue is between the major and minor
  // eigenvalues.
  void IntegrateMediumEigenvector()
    {this->SetIntegrationEigenvectorToMedium();};

  // Description:
  // Use the minor eigenvector field as the vector field through which
  // to integrate. The minor eigenvector is the eigenvector whose
  // corresponding eigenvalue is closest to negative infinity.
  void IntegrateMinorEigenvector()
    {this->SetIntegrationEigenvectorToMinor();};

  // Description:
  // Set / get a nominal integration step size (expressed as a fraction of
  // the size of each cell).
  vtkSetClampMacro(IntegrationStepLength,double,0.001,0.5);
  vtkGetMacro(IntegrationStepLength,double);

  // Description:
  // Set / get the length of a tube segment composing the
  // hyperstreamline. The length is specified as a fraction of the
  // diagonal length of the input bounding box.
  vtkSetClampMacro(StepLength,double,0.000001,1.0);
  vtkGetMacro(StepLength,double);

  // Description:
  // Specify the direction in which to integrate the hyperstreamline.
  vtkSetClampMacro(IntegrationDirection,int,
                  VTK_INTEGRATE_FORWARD,VTK_INTEGRATE_BOTH_DIRECTIONS);
  vtkGetMacro(IntegrationDirection,int);
  void SetIntegrationDirectionToForward()
    {this->SetIntegrationDirection(VTK_INTEGRATE_FORWARD);};
  void SetIntegrationDirectionToBackward()
    {this->SetIntegrationDirection(VTK_INTEGRATE_BACKWARD);};
  void SetIntegrationDirectionToIntegrateBothDirections()
    {this->SetIntegrationDirection(VTK_INTEGRATE_BOTH_DIRECTIONS);};

  // Description:
  // Set/get terminal eigenvalue.  If major eigenvalue falls below this
  // value, hyperstreamline terminates propagation.
  vtkSetClampMacro(TerminalEigenvalue,double,0.0,VTK_DOUBLE_MAX);
  vtkGetMacro(TerminalEigenvalue,double);

  // Description:
  // Set / get the number of sides for the hyperstreamlines. At a minimum,
  // number of sides is 3.
  vtkSetClampMacro(NumberOfSides,int,3,VTK_INT_MAX);
  vtkGetMacro(NumberOfSides,int);

  // Description:
  // Set / get the initial tube radius. This is the maximum "elliptical"
  // radius at the beginning of the tube. Radius varies based on ratio of
  // eigenvalues.  Note that tube section is actually elliptical and may
  // become a point or line in cross section in some cases.
  vtkSetClampMacro(Radius,double,0.0001,VTK_DOUBLE_MAX);
  vtkGetMacro(Radius,double);

  // Description:
  // Turn on/off logarithmic scaling. If scaling is on, the log base 10
  // of the computed eigenvalues are used to scale the cross section radii.
  vtkSetMacro(LogScaling,int);
  vtkGetMacro(LogScaling,int);
  vtkBooleanMacro(LogScaling,int);

protected:
  vtkHyperStreamline();
  ~vtkHyperStreamline();

  // Integrate data
  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int BuildTube(vtkDataSet *input, vtkPolyData *output);

  virtual int FillInputPortInformation(int port, vtkInformation *info);

  // Flag indicates where streamlines start from (either position or location)
  int StartFrom;

  // Starting from cell location
  vtkIdType StartCell;
  int StartSubId;
  double StartPCoords[3];

  // starting from global x-y-z position
  double StartPosition[3];

  //array of hyperstreamlines
  vtkHyperArray *Streamers;
  int NumberOfStreamers;

  // length of hyperstreamline in absolute distance
  double MaximumPropagationDistance;

  // integration direction
  int IntegrationDirection;

  // the length (fraction of cell size) of integration steps
  double IntegrationStepLength;

  // the length of the tube segments composing the hyperstreamline
  double StepLength;

  // terminal propagation speed
  double TerminalEigenvalue;

  // number of sides of tube
  int NumberOfSides;

  // maximum radius of tube
  double Radius;

  // boolean controls whether scaling is clamped
  int LogScaling;

  // which eigenvector to use as integration vector field
  int IntegrationEigenvector;
private:
  vtkHyperStreamline(const vtkHyperStreamline&);  // Not implemented.
  void operator=(const vtkHyperStreamline&);  // Not implemented.
};

#endif
