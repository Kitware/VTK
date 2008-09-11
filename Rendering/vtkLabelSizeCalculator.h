#ifndef __vtkLabelSizeCalculator_h
#define __vtkLabelSizeCalculator_h

#include "vtkPassInputTypeAlgorithm.h"

class vtkIntArray;
class vtkFreeTypeUtilities;
class vtkStringArray;
class vtkTextProperty;

// .NAME vtkLabelSizeCalculator
// .SECTION Description
// This filter takes an input dataset, an array to process
// (which must be a string array), and a text property.
// It creates a new output array (named "LabelSize" by default) with
// 4 components per tuple that contain the width, height, horizontal
// offset, and descender height (in that order) of each string in
// the array.
//
// Use the inherited SelectInputArrayToProcess to indicate a string array.
// In no input array is specified, the first of the following that
// is a string array is used: point scalars, cell scalars, field scalars.

class VTK_RENDERING_EXPORT vtkLabelSizeCalculator : public vtkPassInputTypeAlgorithm
{
public:
  static vtkLabelSizeCalculator* New();
  virtual void PrintSelf( ostream& os, vtkIndent indent );
  vtkTypeRevisionMacro(vtkLabelSizeCalculator,vtkPassInputTypeAlgorithm);

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
  vtkLabelSizeCalculator();
  virtual ~vtkLabelSizeCalculator();

  virtual int FillInputPortInformation( int port, vtkInformation* info );
  virtual int RequestData(
    vtkInformation* request,
    vtkInformationVector** inInfo,
    vtkInformationVector* outInfo );

  virtual vtkIntArray* LabelSizesForArray( vtkAbstractArray* labels );

  virtual void SetFontUtil( vtkFreeTypeUtilities* fontProp );
  vtkGetObjectMacro(FontUtil,vtkFreeTypeUtilities);

  vtkTextProperty* FontProperty;
  vtkFreeTypeUtilities* FontUtil;
  char* LabelSizeArrayName;

private:
  vtkLabelSizeCalculator( const vtkLabelSizeCalculator& ); // Not implemented.
  void operator = ( const vtkLabelSizeCalculator& ); // Not implemented.
};

#endif // __vtkLabelSizeCalculator_h
