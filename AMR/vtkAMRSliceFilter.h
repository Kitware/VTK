/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMRSliceFilter.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
// .NAME vtkAMRSliceFilter.h -- Creates slices from AMR datasets
//
// .SECTION Description
//  A concrete instance of vtkHierarchicalBoxDataSetAlgorithm which implements
//  functionality for extracting slices from AMR data. Unlike the conventional
//  slice filter, the output of this filter is a 2-D AMR dataset itself.

#ifndef VTKAMRSLICEFILTER_H_
#define VTKAMRSLICEFILTER_H_

#include "vtkHierarchicalBoxDataSetAlgorithm.h"

class vtkInformation;
class vtkInformationVector;
class vtkHierarchicalBoxDataSet;
class vtkMultiProcessController;
class vtkPlane;

class VTK_AMR_EXPORT vtkAMRSliceFilter :
  public vtkHierarchicalBoxDataSetAlgorithm
{
  public:
      static vtkAMRSliceFilter* New();
      vtkTypeMacro( vtkAMRSliceFilter, vtkHierarchicalBoxDataSetAlgorithm );
      void PrintSelf( std::ostream &os, vtkIndent indent );

      // Inline Gettters & Setters
      vtkSetMacro(OffSetFromOrigin,double);
      vtkGetMacro(OffSetFromOrigin,double);

      // Description:
      // Set/Get the Axis normal. There are only 3 acceptable values
      // 1-(X-Normal); 2-(Y-Normal); 3-(Z-Normal)
      vtkSetMacro(Normal,int);
      vtkGetMacro(Normal,int);

      // Description:
      // Set/Get a multiprocess controller for paralle processing.
      // By default this parameter is set to NULL by the constructor.
      vtkSetMacro( Controller, vtkMultiProcessController* );
      vtkGetMacro( Controller, vtkMultiProcessController* );

      // Standard Pipeline methods
      virtual int RequestData(
         vtkInformation*,vtkInformationVector**,vtkInformationVector*);
      virtual int FillInputPortInformation(int port, vtkInformation *info);
      virtual int FillOutputPortInformation(int port, vtkInformation *info);

  protected:
    vtkAMRSliceFilter();
    ~vtkAMRSliceFilter();

    // Description:
    // A utility function that checks if the input AMR data is 2-D.
    bool IsAMRData2D( vtkHierarchicalBoxDataSet *input );

    // Description:
    // Returns the axis-aligned cut plane.
    vtkPlane* GetCutPlane( vtkHierarchicalBoxDataSet *input );

    double origin[3];
    double OffSetFromOrigin;
    int    Normal; // 1=>X-Normal, 2=>Y-Normal, 3=>Z-Normal

    vtkMultiProcessController *Controller;

  private:
    vtkAMRSliceFilter( const vtkAMRSliceFilter& ); // Not implemented
    void operator=( const vtkAMRSliceFilter& ); // Not implemented
};

#endif /* VTKAMRSLICEFILTER_H_ */
