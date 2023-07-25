// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
* @class vtkContinuousScatterplot
* @brief Given a 3D domain space represented by an
* unstructured grid composed of tetrahedral cells with bivariate fields, this filter
* tessellates each cell in the domain to polyhedral fragments by intersecting the
* projection of the cell into 2-D range space against two sets of cutting planes, one set
* is defined along the first field, the second set is defined along the second field. The
* volume of these subdivided polyhedral fragments can be computed and aggregated over
* cells to depict the density distribution of the data projection in the bivariate range
* space.
*
* @section vtkContinuousScatterplot-introduction Introduction
* Given a bivariate field (f1,f2) defined on an unstructured grid which is
* composed of tetrahedral cells, we can initially subdivide each cell based on its
* projection in the range into a number of fragments along the first field f1, we refer
* to these polyhedral fragments as Frag(f1) = {frag(f1)_1, frag(f1)_2, ... , frag(f1)_n},
* where frag(f1)_n refers to the nth fragment along the first field subdivision. Each
* fragment has a range value and the value difference between the neighbouring fragments
* is represented as fragment width fw_f1, which is uniformly distributed across the
* range.
* Based on the structure of Frag(f1), for each of its cell "frag(f1)_n", we
* can further subdivide this cell based on the second field f2 using fragment width
* fw_f2. The tessellation along the second field results in an even finer fragment
* collection which we refer to as Frag(f1,f2) = {frag(f1,f2)_1, frag(f1,f2)_2, ... ,
* frag(f1,f2)_m}. We can observe that Frag(f1,f2) is a finer tessellation of the domain
* than Frag(f1) and will be used to compute the density distribution in the bivariate
* range space. The algorithm for fragment computation is similar to the first stage of
* the work in [0].
* Each fragment "s" in Frag(f1,f2) has range values (f1(s), f2(s)) in the bivariate
* fields. These values can be further mapped to a 2-D bin with a resolution rexX * resY.
* The mapped bin index (binIndexX, binIndexY) of the fragment can be computed by linear
* interpolation on its range values :
*           binIndexX = (int) resX * (f1(s) - f1_min) / (f1_max - f1_min)
*           binIndexY = (int) resY * (f2(s) - f2_min) / (f2_max - f2_min),
*        where (f1_min, f1_max) is the range in first field.
* Once we know which bin a fragment coincides, the density value in each bin equals to
* the total geometric volume of the fragments in this bin. This volume distribution
* over the bins will be exported as a point data array in the output data structure.
* If we map this 2-D bin to a 2-D image with each bin corresponding to a pixel and
* bin density to pixel transparency, then the image can be displayed as a continuous
* scatterplot.

* @section vtkContinuousScatterplot-algorithm Algorithm
* The algorithm of this filter can be described as:
*   Require: R.1 The domain space is an unstructured grid data set composed of
*                tetrahedral cells;
*            R.2 The range space contains two scalar fields, say f1 and f2.
*
*   The most important step is to compute the fragments. The implementation processes
*   the input grid one cell at a time, explicitly computing the intersection of the cell
*   with the cutting planes defined by the fragment boundaries in each scalar field.
*   In order to subdivide the cell, we need to define a list of cutting planes in each
*   field. The interval between neighbouring cutting planes is related to the output 2-D
*   bin resolution (resX, resY) and can be computed as :
*                     fw_f1 = (f1_max - f1_min) / resX
*                     fw_f2 = (f2_max - f2_min) / resY,
*                 where (f1_max,f1_min) is the scalar range of first field.
*
*      1. For each tetrahedron T in the input grid:
*
*        1.1 Subdivide the cell T based on the first field f1, we will obtain a list
*            of fragments: Frag(f1) = {frag(f1)_1, frag(f1)_2, ... , frag(f1)_n}. The
*            steps for subdivision can be described as:
*
*            1.1.1 For each cutting plane s with respect to the first field f1,
*                  its field value f1(s) = f1_min + n * fw_f1, where n refers to the n-th
*                  cutting plane:
*
*              1.1.2. Traverse each edge e starting from point a to b in the cell, we
*                     will maintain three data classes, namely fragmentFace,
*                     residualFace and cutSet:
*                     A. fragmentFace contains vertices in the current fragment.
*                     B. cutSet contains vertices whose range values equal to f1(s).
*                        This set contains the current cutting plane.
*                     C. residualFace contains the rest of the vertices in the cell.
*                     In order to classify edge vertices into these classes, the
*                     following case table is used for each vertex "a" :
*                       case 0 :          f1(a)------ f1(s) ------f1(b)
*                              condition: f1(a) < f1(s) , f1(b) > f1(s)
*                              class:     p(s,e), a -> fragmentFace
*                                         p(s,e) -> cutSet
*                                         p(s,e) -> residualFace
*
*                       case 1 :          f1(b)------ f1(s) ------f1(a)
*                              condition: f1(a) > f1(s) , f1(b) < f1(s)
*                              class:     p(s,e) -> fragmentFace
*                                         p(s,e) -> cutSet
*                                         a -> residualFace
*
*                       case 2 :    f1(s),f1(a)-------------------f1(b)
*                              condition: f1(s) == f1(a), f1(s) <= f1(b)
*                              class:     a -> fragmentFace
*                                         a -> residualFace
*                                         a -> cutSet
*
*                       case 3 :          f1(a)-------------------f1(b), f1(s)
*                              condition: f1(s) > f1(a), f1(s) == f1(b)
*                              class:     a -> fragmentFace
*
*                       case 4 :    f1(s),f1(b)-------------------f1(a)
*                              condition: f1(s) < f1(a), f1(s) == f1(b)
*                              class:     a -> residualFace
*                       Remark: 1. we use "->" to indicate "belongs to" relation.
*                               2. p(s,e) refers to the interpolated point of range value
*                                  f1(s) on the edge e.
*
*             1.1.3. After we have traversed every edge in a cell for the cutting plane
*                    s, three classes for storing fragment, cutting plane and residual
*                    faces are updated. The faces of the current fragment frag(f1)
*                    are the union of all elements in fragmentFace and cutSet.
*
*    1.2 Take the output of step 1.1, traverse each fragment in Frag(f1), define a list
*        of cutting planes with respect to field f2, further subdivide the fragments in
*        Frag(f1) following steps from 1.1.2 to 1.1.3. The output of this step will be
*        the fragment collection Frag(f1,f2). Each fragment in Frag(f1,f2) can be further
*        mapped to a 2-D bin based on its range values. The density value in each bin
*        equals to the total geometric volume of the fragments in this bin. This volume
*        distribution over the bins will be exported as a point data array in the output
*        data structure.
*
* @section vtkContinuousScatterplot-filter-design VTK Filter Design
* The input and output ports of the filter:
*      Input port : the input data set should be a vtkUnstructuredGrid, with each of its
*                   cell defined as a tetrahedron. At least two scalar fields are
*                   associated with the data. The user needs to specify the name of the
*                   two scalar arrays beforehand.
*      Output port: the output data set is a 2D image stored as a vtkImageData.
*                   The resolution of the output image can be set by the user.
*                   The volume distribution of fragments in each pixel or bin
*                   is stored in an point data array named "volume" in the output
*                   vtkImageData.
*
* @section vtkContinuousScatterplot-how-to-use How To Use This Filter
* Suppose we have a tetrahedral mesh stored in a vtkUnstructuredGrid, we call this
* data set "inputData". This data set has two scalar arrays whose names are "f1"
* and "f2" respectively. We would like the resolution of output image set to (resX,resY).
* Given these input, this filter can be called as follows in c++ sample code:
*
*     vtkSmartPointer<vtkContinuousScatterplot> csp =
*                            vtkSmartPointer<vtkContinuousScatterplot>::New();
*     csp->SetInputData(inputData);
*     csp->SetField1("f1",resX);
*     csp->SetField2("f2",resY);
*     csp->Update();
*
* Then the output, "csp->GetOutput()", will be a vtkImageData containing a scalar
* array whose name is "volume". This array contains the volume distribution of the
* fragments.
*
* [0] H.Carr and D.Duke, Joint contour nets: Topological analysis of multivariate data.
*     IEEE Transactions on Visualization and Computer Graphics, volume 20,
*     issue 08, pages 1100-1113, 2014
*/

#ifndef vtkContinuousScatterplot_h
#define vtkContinuousScatterplot_h

#include "vtkImageAlgorithm.h"
#include "vtkInfovisCoreModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKINFOVISCORE_EXPORT vtkContinuousScatterplot : public vtkImageAlgorithm
{
public:
  static vtkContinuousScatterplot* New();
  vtkTypeMacro(vtkContinuousScatterplot, vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Get the tolerance used when comparing floating point numbers for equality.
   */
  vtkGetMacro(Epsilon, double);

  /**
   * Set the tolerance used when comparing floating point numbers for equality.
   */
  vtkSetMacro(Epsilon, double);

  /**
   * Specify the name of the first field to be used in subdividing the dataset.
   * Specify the resolution along x axis of the output image.
   */
  void SetField1(const char* fieldName, vtkIdType ResX);

  /**
   * Specify the name of the second field to be used in subdividing the dataset.
   * Specify the resolution along y axis of the output image.
   */
  void SetField2(const char* fieldName, vtkIdType ResY);

protected:
  vtkContinuousScatterplot();

  // Configure input port to accept only vtkUnstructuredGrid.
  int FillInputPortInformation(int port, vtkInformation* info) override;

  // Configure out port to be a vtkImageData data set.
  int FillOutputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  // Set the tolerance used when comparing floating numbers for equality.
  double Epsilon;

  // Names of the scalar fields to be used in the filter.
  const char* Fields[2];

  // Resolution of the output image.
  vtkIdType ResX, ResY;

private:
  vtkContinuousScatterplot(const vtkContinuousScatterplot&) = delete;
  void operator=(const vtkContinuousScatterplot&) = delete;
};
VTK_ABI_NAMESPACE_END
#endif
