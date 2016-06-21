/*******************************************************************/
/*                               XDMF                              */
/*                   eXtensible Data Model and Format              */
/*                                                                 */
/*  Id : Id  */
/*                                                                 */
/*  Author:                                                        */
/*     Jerry A. Clarke                                             */
/*     clarke@arl.army.mil                                         */
/*     US Army Research Laboratory                                 */
/*     Aberdeen Proving Ground, MD                                 */
/*                                                                 */
/*     Copyright @ 2002 US Army Research Laboratory                */
/*     All Rights Reserved                                         */
/*     See Copyright.txt or http://www.arl.hpc.mil/ice for details */
/*                                                                 */
/*     This software is distributed WITHOUT ANY WARRANTY; without  */
/*     even the implied warranty of MERCHANTABILITY or FITNESS     */
/*     FOR A PARTICULAR PURPOSE.  See the above copyright notice   */
/*     for more information.                                       */
/*                                                                 */
/*******************************************************************/
#ifndef vtkXdmfDataArray_h
#define vtkXdmfDataArray_h

#include "vtkIOXdmf2Module.h" // For export macro
#include "vtkObject.h"

class vtkDataArray;
namespace xdmf2
{
class XdmfArray;
}

class VTKIOXDMF2_EXPORT vtkXdmfDataArray : public vtkObject
{
public:
  static vtkXdmfDataArray *New();
  vtkTypeMacro(vtkXdmfDataArray,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkDataArray *FromArray( void );

  char *ToArray( void );

  vtkDataArray *FromXdmfArray( char *ArrayName = NULL, int CopyShape = 1,
   int rank = 1, int Components = 1 , int MakeCopy = 1);

  char *ToXdmfArray( vtkDataArray *DataArray = NULL, int CopyShape = 1 );

  void SetArray( char *TagName );

  char *GetArray( void );

  void SetVtkArray( vtkDataArray *array);

  vtkDataArray *GetVtkArray( void );

protected:
  vtkXdmfDataArray();

private:
  vtkDataArray  *vtkArray;
  xdmf2::XdmfArray  *Array;
  vtkXdmfDataArray(const vtkXdmfDataArray&) VTK_DELETE_FUNCTION;
  void operator=(const vtkXdmfDataArray&) VTK_DELETE_FUNCTION;
};

#endif /* vtkXdmfDataArray_h */
