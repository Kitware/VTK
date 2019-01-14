/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOSPRayMaterialLibrary.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOSPRayMaterialLibrary
 * @brief   a collection of materials for vtk apps to draw from
 *
 * A singleton instance of this class manages a collection of materials.
 * The materials can be read in from disk or created programmatically.
 *
 * @sa vtkOSPRayMaterialHelpers
*/

#ifndef vtkOSPRayMaterialLibrary_h
#define vtkOSPRayMaterialLibrary_h

#include "vtkRenderingOSPRayModule.h" // For export macro
#include "vtkObject.h"

#include <initializer_list> //for initializer_list!
#include <set> //for set!
#include <vector> //for vector!

class vtkOSPRayMaterialLibraryInternals;
class vtkTexture;

class VTKRENDERINGOSPRAY_EXPORT vtkOSPRayMaterialLibrary : public vtkObject
{
public:
  static vtkOSPRayMaterialLibrary *New();
  vtkTypeMacro(vtkOSPRayMaterialLibrary,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Called to kick off events in all followers.
   */
  void Fire();

  /**
   * Reads the given file of materials and creates the in memory data
   * structures needed to display objects with them. Returns false only if
   * file could not be meaningfully interpretted.
   */
  bool ReadFile(const char*FileName);

  /**
   * Serialize contents to an in memory buffer.
   */
  const char *WriteBuffer();

  /**
   * DeSerialize contents from an in memory buffer as ReadFile does from a
   * file or set of files. Returns false only if buffer could not be
   * meaningfully interpretted.
   */
  bool ReadBuffer(const char*Buffer);

  /**
   * Returns the set of material nicknames.
   */
  std::set<std::string> GetMaterialNames();

  /**
   * Return an implementation name for the given material nickname.
   */
  std::string LookupImplName(const std::string& nickname);

  /**
   * Returns a unform variable, the name format is nickname:varname.
   */
  std::vector<double> GetDoubleShaderVariable(const std::string& nickname, const std::string& varname);

  /**
   * Returns a texture, the name format is nickname:varname.
   */
  vtkTexture* GetTexture(const std::string& nickname, const std::string& varname);

  /**
    * Add Material
    * Adds a new material nickname to the set of known materials.
    * If the name is a repeat, we replace the old one.
    **/
  void AddMaterial(const std::string& nickname, const std::string& implname);

  /**
    * Add Texture
    * Adds a new texture. Replaces any previous content.
    **/
  void AddTexture(const std::string& nickname, const std::string& texturename, vtkTexture* tex);

  /**
    * Add control variable
    * Adds a new control variable. Replaces any previous content.
    * @{
    **/
  void AddShaderVariable(const std::string& nickname,
                         const std::string& variablename,
                         int numVars, const double *x);
  void AddShaderVariable(const std::string& nickname,
                         const std::string& variablename,
                         const std::initializer_list<double>& data)
  {
    this->AddShaderVariable(nickname, variablename,
                            static_cast<int>(data.size()), data.begin());
  }
  /**@}*/


protected:
  vtkOSPRayMaterialLibrary();
  virtual ~vtkOSPRayMaterialLibrary();

  bool InternalParse(const char *name, bool IsFile);
  bool InternalParseJSON(const char *name, bool IsFile, std::istream *doc);
  bool InternalParseMTL(const char *name, bool IsFile, std::istream *doc);

private:
  vtkOSPRayMaterialLibrary(const vtkOSPRayMaterialLibrary&) = delete;
  void operator=(const vtkOSPRayMaterialLibrary&) = delete;

  vtkOSPRayMaterialLibraryInternals *Internal;
};

#endif
