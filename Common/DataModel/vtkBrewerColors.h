#ifndef __vtkBrewerColors_h
#define __vtkBrewerColors_h

#include "vtkCommonDataModelModule.h" // For export macro

#include "vtkColor.h" // For arguments
#include "vtkObject.h"
#include "vtkStdString.h" // For arguments

class vtkColorSchemeInternal;

class vtkLookupTable;

/**\brief Cynthia Brewer's color schemes.
 *
 * Thanks to
 * Cynthia A. Brewer (Dept. of Geography, Pennsylvania State University) and to
 * OVIS (http://ovis.ca.sandia.gov/) for providing this class.
 */
class VTKCOMMONDATAMODEL_EXPORT vtkBrewerColors : public vtkObject
{
public:
  static vtkBrewerColors* New();
  vtkTypeMacro(vtkBrewerColors,vtkObject);
  virtual void PrintSelf( ostream& os, vtkIndent indent );

  // Description:
  // Get the number of color schemes available for use.
  vtkIdType GetNumberOfSchemes();

  // Description:
  // Return the name of a color scheme given an integer between 0 and GetNumberOfSchemes()-1.
  const char* GetScheme( vtkIdType index );

  // Description:
  // Set/get the color scheme that should be used.
  virtual void SetCurrentScheme( const char* schemeName );
  vtkStdString GetCurrentScheme() { return this->CurrentScheme; }

  // Description:
  // Create a new color scheme with the given name.
  // Returns true on success or false on error.
  // An error will occur when a null pointer is passed or when a scheme of the same name already exists.
  //
  // Upon success, the current scheme is set to the newly-created scheme (which has no colors in it,
  // so you must call AddColor to insert some).
  virtual bool CreateScheme( const char* schemeName );

  // Description:
  // Get the number of colors available in the current color scheme.
  int GetNumberOfColors();

  // Description:
  // Get the color at the specified index in the current scheme.
  // If the index is out of range then black will be returned.
  vtkColor4ub GetColor( int index ) const;

  // Description:
  // Get the color at the specified index. If the index is out of range then
  // the call wraps around, i.e. uses the mod operator.
  vtkColor4ub GetColorRepeating( int index ) const;

  // Description:
  // Set the color at the specified index.
  // Does nothing if the index is out of range.
  void SetColor( int index, const vtkColor4ub& color );

  // Description:
  // Adds the color to the end of the colors in the current scheme.
  void AddColor( const vtkColor4ub& color );

  // Description:
  // Inserts the color at the specified index in the current color scheme.
  void InsertColor( int index, const vtkColor4ub& color );

  // Description:
  // Removes the color at the specified index in the list.
  void RemoveColor( int index );

  // Description:
  // Clears the list of colors.
  void ClearColors();

  // Description:
  // Turn this scheme into a deep copy of the supplied scheme.
  void DeepCopy( vtkBrewerColors* other );

  // Description:
  // Create a vtkLookupTable object from the current color scheme (or populate the non-NULL table specifed).
  vtkLookupTable* CreateLookupTable( vtkLookupTable* lut );

protected:
  vtkBrewerColors();
  virtual ~vtkBrewerColors();

  vtkColorSchemeInternal* Storage;
  vtkStdString CurrentScheme;
  vtkColor4ub* CurrentColorCache;
  vtkIdType CurrentSize;

private:
  void operator = ( const vtkBrewerColors& ); // Not implemented.
  vtkBrewerColors( const vtkBrewerColors& ); // Not implemented.
};

#endif // __vtkBrewerColors_h
