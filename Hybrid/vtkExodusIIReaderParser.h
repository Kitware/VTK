/*=========================================================================

  Program:   ParaView
  Module:    vtkExodusIIReaderParser.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkExodusIIReaderParser - internal parser used by vtkExodusIIReader.
// .SECTION Description
// vtkExodusIIReaderParser is an internal XML parser used by vtkExodusIIReader.
// This is not for public use.

#ifndef __vtkExodusIIReaderParser_h
#define __vtkExodusIIReaderParser_h

#include "vtkXMLParser.h"
#include "vtkSmartPointer.h"

#include <vtksys/ios/sstream>
#include <vtkstd/map>
#include <vtkstd/vector>
#include <vtkstd/set>
#include <vtkstd/string>

class vtkMutableDirectedGraph;
class vtkStringArray;
class vtkUnsignedCharArray;

class VTK_HYBRID_EXPORT vtkExodusIIReaderParser : public vtkXMLParser
{
public:
  static vtkExodusIIReaderParser* New();
  vtkTypeMacro(vtkExodusIIReaderParser, vtkXMLParser);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns the SIL. 
  // This is valid only after Go().
  vtkGetObjectMacro(SIL, vtkMutableDirectedGraph);

  // Description:
  // Trigger parsing of the XML file.
  void Go(const char* filename);

  // Returns if the parser has some information about the block with given "id".
  // This is valid only after Go().
  bool HasInformationAboutBlock(int id)
    {
    return (this->BlockID_To_VertexID.find(id) != this->BlockID_To_VertexID.end());
    }

  // Description:
  // Given a block "id" return the name as determined from the xml.
  // This is valid only after Go().
  vtkstd::string GetBlockName(int id);

  // Description:
  // Fills up the blockIdsSet with the block ids referred to by the XML.
  // This is valid only after Go().
  void GetBlockIds(vtkstd::set<int>& blockIdsSet)
    {
    vtkstd::map<int, vtkIdType>::iterator iter;
    for (iter = this->BlockID_To_VertexID.begin();
      iter != this->BlockID_To_VertexID.end();
      ++iter)
      {
      blockIdsSet.insert(iter->first);
      }
    }
//BTX
protected:
  vtkExodusIIReaderParser();
  ~vtkExodusIIReaderParser();

  virtual void StartElement( const char* tagName, const char** attrs);
  virtual void EndElement(const char* tagName);
  void FinishedParsing();

  const char* GetValue(const char* attr,const char** attrs)
    {
    int i;
    for (i=0;attrs[i];i+=2)
      {
      const char* name=strrchr(attrs[i],':');
      if (!name)
        {
        name=attrs[i];
        }
      else
        {
        name++;
        }
      if (strcmp(attr,name)==0)
        {
        return attrs[i+1];
        }
      }
    return NULL;
    }

  // Convenience methods to add vertices/edges to the SIL.
  vtkIdType AddVertexToSIL(const char* name);
  vtkIdType AddChildEdgeToSIL(vtkIdType src, vtkIdType dst);
  vtkIdType AddCrossEdgeToSIL(vtkIdType src, vtkIdType dst);

  // Description:
  // Returns the vertex id for the "part" with given 
  // part_number_instance_string formed as
  // "{part-number} Instance: {part-instance}"
  vtkIdType GetPartVertex(const char* part_number_instance_string);

  // For each of the blocks, this maps the "id" attribute in the XML to the
  // vertex id for the block in the SIL.
  vtkstd::map<int, vtkIdType> BlockID_To_VertexID;


  // Maps block "id"s to material names.
  vtkstd::map<int, vtkstd::string> BlockID_To_MaterialName;

  // Maps material name to vertex id.
  // This will be build only if <material-list> is present in the XML.
  vtkstd::map<vtkstd::string, vtkIdType> MaterialName_To_VertexID;

  vtkstd::map<vtkIdType, vtkstd::string> PartVertexID_To_Descriptions;

  // These save the values read from <material-specification /> element present
  // withint the <part /> elements.
  // key: part vertex id
  // value: material name = (desp + spec)
  vtkstd::map<vtkIdType, vtkstd::string> MaterialSpecifications;

  // Maps the "{part-number} Instance: {part-instance}" key for the vertex id
  // for the part vertex in the Assemblies hierarchy.
  vtkstd::map<vtkstd::string, vtkIdType> Part_To_VertexID;

  // Maps a block-id to the "{part-number} Instance: {part-instance}" string.
  vtkstd::map<int, vtkstd::string> BlockID_To_Part;
  
  vtkMutableDirectedGraph* SIL;
  vtkSmartPointer<vtkStringArray> NamesArray;
  vtkSmartPointer<vtkUnsignedCharArray> CrossEdgesArray;

  vtkstd::string BlockPartNumberString;

  vtkIdType RootVertex;
  vtkIdType BlocksVertex;
  vtkIdType AssembliesVertex;
  vtkIdType MaterialsVertex;
  vtkstd::vector<vtkIdType> CurrentVertex;

  bool InBlocks;
  bool InMaterialAssignments;

private:
  vtkExodusIIReaderParser(const vtkExodusIIReaderParser&); // Not implemented
  void operator=(const vtkExodusIIReaderParser&); // Not implemented
//ETX
};

#endif

