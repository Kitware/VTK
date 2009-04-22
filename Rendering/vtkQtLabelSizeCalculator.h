#ifndef __vtkQtLabelSizeCalculator_h
#define __vtkQtLabelSizeCalculator_h

#include "vtkPassInputTypeAlgorithm.h"
//#include "QVTKWin32Header.h"

class vtkIntArray;
class vtkStringArray;
class vtkTextProperty;

// .NAME vtkQtLabelSizeCalculator
// .SECTION Description
// This filter takes an input dataset, an array to process
// (which must be a string array), and a text property.
// It creates a new output array (named "LabelSize" by default) with
// 4 components per tuple that contain the width, height, horizontal
// offset, and descender height (in that order) of each string in
// the array based on placing the labels on a Qimage.
//
// Use the inherited SelectInputArrayToProcess to indicate a string array.
// In no input array is specified, the first of the following that
// is a string array is used: point scalars, cell scalars, field scalars.

class VTK_RENDERING_EXPORT vtkQtLabelSizeCalculator : public vtkPassInputTypeAlgorithm
{
public:
  static vtkQtLabelSizeCalculator* New();
  virtual void PrintSelf( ostream& os, vtkIndent indent );
  vtkTypeRevisionMacro(vtkQtLabelSizeCalculator,vtkPassInputTypeAlgorithm);

  // Description:
  // Get/Set the font used compute label sizes.
  // This defaults to "Arial" at 12 points.
  virtual void SetFontProperty( vtkTextProperty* fontProp );
  vtkGetObjectMacro(FontProperty,vtkTextProperty);

  // Description:
  // The name of the output array containing text label sizes
  // This defaults to "LabelSize"
  vtkSetStringMacro(LabelSizeArrayName);
  vtkGetStringMacro(LabelSizeArrayName);

protected:
  vtkQtLabelSizeCalculator();
  virtual ~vtkQtLabelSizeCalculator();

  virtual int FillInputPortInformation( int port, vtkInformation* info );
  virtual int RequestData(
    vtkInformation* request,
    vtkInformationVector** inInfo,
    vtkInformationVector* outInfo );

  virtual vtkIntArray* LabelSizesForArray( vtkAbstractArray* labels );

  vtkTextProperty* FontProperty;
  char* LabelSizeArrayName;

private:
  vtkQtLabelSizeCalculator( const vtkQtLabelSizeCalculator& ); // Not implemented.
  void operator = ( const vtkQtLabelSizeCalculator& ); // Not implemented.
};

#endif // __vtkQtLabelSizeCalculator_h
