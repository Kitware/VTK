/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperOctreeSampleFunction.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkHyperOctreeSampleFunction - sample an implicit function over an
// hyperoctree
// .SECTION Description
// vtkHyperOctreeSampleFunction is a source object that evaluates an implicit
// function to drive the subdivision process. The user can specify
// the threshold over which a subdivision occurs, the maximum and minimum
// level of subdivisions and the dimension of the hyperoctree.
//
// .SECTION See Also
// vtkSampleFunction

#ifndef vtkHyperOctreeSampleFunction_h
#define vtkHyperOctreeSampleFunction_h

#include "vtkFiltersHyperTreeModule.h" // For export macro
#include "vtkHyperOctreeAlgorithm.h"

class vtkImplicitFunction;

class VTKFILTERSHYPERTREE_EXPORT vtkHyperOctreeSampleFunction : public vtkHyperOctreeAlgorithm
{
public:
  vtkTypeMacro(vtkHyperOctreeSampleFunction,vtkHyperOctreeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkHyperOctreeSampleFunction *New();

  // Description:
  // Return the maximum number of levels of the hyperoctree.
  // \post positive_result: result>=1
  int GetLevels();

  // Description:
  // Set the maximum number of levels of the hyperoctree. If
  // GetMinLevels()>=levels, GetMinLevels() is changed to levels-1.
  // \pre positive_levels: levels>=1
  // \post is_set: this->GetLevels()==levels
  // \post min_is_valid: this->GetMinLevels()<this->GetLevels()
  void SetLevels(int levels);

  // Description:
  // Return the minimal number of levels of systematic subdivision.
  // \post positive_result: result>=0
  int GetMinLevels();

  // Description:
  // Set the minimal number of levels of systematic subdivision.
  // \pre positive_minLevels: minLevels>=0
  // \post is_set: this->GetMinLevels()==minLevels
  void SetMinLevels(int minLevels);

  // Description:
  // Return the threshold over which a subdivision is required.
  // \post positive_result: result>0
  double GetThreshold();

  // Description:
  // Set the threshold over which a subdivision is required.
  // \pre positive_threshold: threshold>=0
  // \post is_set: this->GetThreshold()==threshold
  void SetThreshold(double threshold);

  // Description:
  // Return the dimension of the tree (1D:binary tree(2 children), 2D:quadtree(4 children),
  // 3D:octree (8 children))
  // \post valid_result: result>=1 && result<=3
  int GetDimension();

   // Set the dimension of the tree with `dim'. See GetDimension() for details.
  // \pre valid_dim: dim>=1 && dim<=3
  // \post dimension_is_set: GetDimension()==dim
  void SetDimension(int dim);

  // Description:
  // Set the size on each axis.
  vtkSetVector3Macro(Size,double);

  // Description:
  // Return the size on each axis.
  vtkGetVector3Macro(Size,double);

  // Description:
  // Set the origin (position of corner (0,0,0) of the root.
  vtkSetVector3Macro(Origin,double);
  // Return the origin (position of corner (0,0,0) ) of the root.
  vtkGetVector3Macro(Origin,double);

  // Description:
  // Return the length along the x-axis.
  // \post positive_result: result>0
  double GetWidth();

  // Description:
  // Set the length along the x-axis.
  // \pre positive_width: width>0
  // \post width_is_set: GetWidth()==width
  void SetWidth(double width);

  // Description:
  // Return the length along the y-axis.
  // Relevant only if GetDimension()>=2
  // \post positive_result: result>0
  double GetHeight();

  // Description:
  // Set the length along the y-axis.
  // Relevant only if GetDimension()>=2
  // \pre positive_height: height>0
  // \post height_is_set: GetHeight()==height
  void SetHeight(double height);

  // Description:
  // Return the length along the z-axis.
  // Relevant only if GetDimension()>=3
  // \post positive_result: result>0
  double GetDepth();

  // Description:
  // Return the length along the z-axis.
  // Relevant only if GetDimension()>=3
  // \pre positive_depth: depth>0
  // \post depth_is_set: GetDepth()==depth
  void SetDepth(double depth);

  // Description:
  // Specify the implicit function to use to generate data.
  virtual void SetImplicitFunction(vtkImplicitFunction*);
  vtkGetObjectMacro(ImplicitFunction,vtkImplicitFunction);

  // Description:
  // Set what type of scalar data this source should generate.
  vtkSetMacro(OutputScalarType,int);
  vtkGetMacro(OutputScalarType,int);
  void SetOutputScalarTypeToDouble()
    {this->SetOutputScalarType(VTK_DOUBLE);}
  void SetOutputScalarTypeToFloat()
    {this->SetOutputScalarType(VTK_FLOAT);}
  void SetOutputScalarTypeToLong()
    {this->SetOutputScalarType(VTK_LONG);}
  void SetOutputScalarTypeToUnsignedLong()
    {this->SetOutputScalarType(VTK_UNSIGNED_LONG);};
  void SetOutputScalarTypeToInt()
    {this->SetOutputScalarType(VTK_INT);}
  void SetOutputScalarTypeToUnsignedInt()
    {this->SetOutputScalarType(VTK_UNSIGNED_INT);}
  void SetOutputScalarTypeToShort()
    {this->SetOutputScalarType(VTK_SHORT);}
  void SetOutputScalarTypeToUnsignedShort()
    {this->SetOutputScalarType(VTK_UNSIGNED_SHORT);}
  void SetOutputScalarTypeToChar()
    {this->SetOutputScalarType(VTK_CHAR);}
  void SetOutputScalarTypeToUnsignedChar()
    {this->SetOutputScalarType(VTK_UNSIGNED_CHAR);}

  // Description:
  // Return the MTime also considering the implicit function.
  unsigned long GetMTime();

protected:
  // Description:
  // Default constructor.
  // Set dimension to 3, width, height and depth to 1, levels to 5, minLevels
  // to 1, implicitFunction to 0, OutputScalarType to VTK_DOUBLE,
  // Threshold is 0.1.
  vtkHyperOctreeSampleFunction();
  ~vtkHyperOctreeSampleFunction();


  int RequestInformation (vtkInformation * vtkNotUsed(request),
                          vtkInformationVector ** vtkNotUsed( inputVector ),
                          vtkInformationVector *outputVector);

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  void Subdivide(vtkHyperOctreeCursor *cursor,
                 int level,
                 vtkHyperOctree *output);

  int Dimension;
  double Size[3]; // size on each axis
  double Origin[3]; // position of corner (0,0,0) of the root.
  int Levels;
  int MinLevels;

  int OutputScalarType;
  vtkImplicitFunction *ImplicitFunction;
  double Threshold;

private:
  vtkHyperOctreeSampleFunction(const vtkHyperOctreeSampleFunction&);  // Not implemented.
  void operator=(const vtkHyperOctreeSampleFunction&);  // Not implemented.
};

#endif
