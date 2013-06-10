#ifndef __vtkScalarBarActorInternal_h
#define __vtkScalarBarActorInternal_h
// VTK-HeaderTest-Exclude: vtkScalarBarActorInternal.h

#include "vtkColor.h" // for AnnotationColors, LabelColorMap, and tuples
#include "vtkFreeTypeUtilities.h" // for bounding box calculations
#include "vtkSmartPointer.h" // for "smart vectors"
#include "vtkStdString.h" // for LabelMap

#include <map>
#include <vector>

class vtkActor2D;
class vtkCellArray;
class vtkTextActor;
class vtkPoints;
class vtkPolyData;
class vtkPolyDataMapper2D;
class vtkUnsignedCharArray;

/// A vector of smart pointers.
template<class T>
class vtkSmartVector : public std::vector<vtkSmartPointer<T> >
{
public:
  /**\brief Convert to an array of "dumb" pointers for functions
    *  that need a contiguous array pointer as input.
    */
  T** PointerArray()
    {
    // NB: This is relatively evil. But much cheaper than copying the array.
    // It assumes the compiler won't pad the class.
    return reinterpret_cast<T**>(&((*this)[0]));
    }
};

/// A structure to represent pixel coordinates for text or swatch bounds.
struct vtkScalarBarBox
{
  /// The position of the box in viewport (pixel) coordinates.
  vtkTuple<int,2> Posn;

  /**\brief Size of the box, stored as (thickness, length) not (width, height).
    *
    * Thickness is a measure of the box size perpendicular to the long axis of the scalar bar.
    * When the scalar bar orientation is horizontal, thickness measures height.
    * Length is a measure of the box size parallel to the long axis of the scalar bar.
    * When the scalar bar orientation is horizontal, length measures width.
    */
  vtkTuple<int,2> Size;
};

/// Internal state for the scalar bar actor shared with subclasses.
class vtkScalarBarActorInternal
{
public:
  vtkScalarBarActorInternal()
    {
    this->Viewport = 0;
    this->SwatchColors = 0;
    this->SwatchPts = 0;
    this->Polys = 0;
    this->AnnotationBoxes = 0;
    this->AnnotationBoxesMapper = 0;
    this->AnnotationBoxesActor = 0;
    this->AnnotationLeaders = 0;
    this->AnnotationLeadersMapper = 0;
    this->AnnotationLeadersActor = 0;
    this->NanSwatch = 0;
    this->NanSwatchMapper = 0;
    this->NanSwatchActor = 0;
    }

  // Define types for smart vectors containing various base classes.
  typedef vtkSmartVector<vtkTextActor> ActorVec;
  typedef vtkSmartVector<vtkTextActor> MathActorVec;

  // Other vector container types.
  typedef std::vector<double> DoubleVec;
  typedef std::vector<vtkColor3ub> ColorVec;

  /**\brief Cache of dimensions fixed during geometry assembly.
    *
    * Only valid within methods invoked by vtkScalarBarActor::RebuildLayout().
    */
  //@{
  vtkViewport* Viewport;
  /// Resolution of the viewport; used to render text at the appropriate detail.
  float DPI;
  /// The thickness and length of the (square) NaN swatch.
  double NanSwatchSize;
  /// Space in pixels between swatches when in indexed lookup mode.
  double SwatchPad;
  /// Number of annotated values (at least lut->GetNumberOfAnnotatedValues(), but maybe more)
  int NumNotes;
  /// Number of color swatches to draw for either the continuous or categorical scalar bar, not including a NaN swatch.
  int NumColors;
  /// Either NumColors or NumColors + 1, depending on whether the NaN swatch is to be drawn.
  int NumSwatches;
  /// Permutation of (0, 1) that transforms thickness,length into width,height.
  int TL[2]; // VERTICAL => TL={0,1}, HORIZONTAL => TL={1,0}, Size[TL[0]] == width, Size[TL[1]] == height
  /// Point coordinates for the scalar bar actor
  vtkPoints* SwatchPts;
  /// Cells representing color swatches (for the scalar bar actor)
  vtkCellArray* Polys;
  /// Colors of swatches in \a Polys
  vtkUnsignedCharArray* SwatchColors;
  /// The bounding box of the entire scalar bar frame.
  vtkScalarBarBox Frame;
  /// The bounding box of the scalar bar (excluding NaN swatch)
  vtkScalarBarBox ScalarBarBox;
  /// The bounding box of the NaN swatch
  vtkScalarBarBox NanBox;
  /// The bounding box of tick mark anchor points (tick labels are not fully contained)
  vtkScalarBarBox TickBox;
  /// The bounding box of the scalar bar title text.
  vtkScalarBarBox TitleBox;
  /// Map from viewport coordinates to label text of each annotation.
  std::map<double,vtkStdString> Labels;
  /// Map from viewport coordinates to the leader line color of each annotation.
  std::map<double,vtkColor3ub> LabelColors;
  //@}

  /// Cache of classes holding geometry assembled and ready for rendering.
  //@{
  ActorVec TextActors;
  vtkPolyData*         AnnotationBoxes;
  vtkPolyDataMapper2D* AnnotationBoxesMapper;
  vtkActor2D*          AnnotationBoxesActor;
  vtkPolyData*         AnnotationLeaders;
  vtkPolyDataMapper2D* AnnotationLeadersMapper;
  vtkActor2D*          AnnotationLeadersActor;
  ActorVec             AnnotationLabels;
  std::vector<double>  AnnotationAnchors;
  std::vector<vtkColor3ub> AnnotationColors;
  vtkPolyData*         NanSwatch;
  vtkPolyDataMapper2D* NanSwatchMapper;
  vtkActor2D*          NanSwatchActor;
  //@}
};

#endif // __vtkScalarBarActorInternal_h
