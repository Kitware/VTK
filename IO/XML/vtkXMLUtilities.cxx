/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLUtilities.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLUtilities.h"

#include "vtkObjectFactory.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLDataParser.h"

#include <vtksys/ios/sstream>

#if !defined(_WIN32) || defined(__CYGWIN__)
# include <unistd.h> /* unlink */
#else
# include <io.h> /* unlink */
#endif

#include <vector>

typedef std::vector<vtkXMLDataElement*> vtkXMLUtilitiesDataElementContainer;

vtkStandardNewMacro(vtkXMLUtilities);

#define  VTK_XML_UTILITIES_FACTORED_POOL_NAME "FactoredPool"
#define  VTK_XML_UTILITIES_FACTORED_NAME      "Factored"
#define  VTK_XML_UTILITIES_FACTORED_REF_NAME  "FactoredRef"

//----------------------------------------------------------------------------
inline int vtkXMLUtilitiesEncodeEntities(unsigned char c, ostream &output)
{
  switch (c)
    {
    case '&':
      output << "&amp;";
      return 1;

    case '"':
      output << "&quot;";
      return 1;

    case '\'':
      output << "&apos;";
      return 1;

    case '<':
      output << "&lt;";
      return 1;

    case '>':
      output << "&gt;";
      return 1;
    }

  return 0;
}

//----------------------------------------------------------------------------
void vtkXMLUtilities::EncodeString(const char *input, int input_encoding,
                                   ostream &output, int output_encoding,
                                   int special_entities)
{
  // No string

  if (!input)
    {
    return;
    }

  // If either the input or output encoding is not specified,
  // or they are the same, dump as is (if no entites had to be converted)

  int no_input_encoding = (input_encoding <= VTK_ENCODING_NONE ||
                           input_encoding >= VTK_ENCODING_UNKNOWN);

  int no_output_encoding = (output_encoding <= VTK_ENCODING_NONE ||
                            output_encoding >= VTK_ENCODING_UNKNOWN);

  if (!special_entities &&
      (no_input_encoding || no_output_encoding ||
       input_encoding == output_encoding))
    {
    output << input;
    return;
    }

  // Convert

  const unsigned char *str = (const unsigned char*)input;

  // If either the input or output encoding is not specified, just process
  // the entities

  if (no_input_encoding || no_output_encoding)
    {
    while (*str)
      {
      if (!vtkXMLUtilitiesEncodeEntities(*str, output))
        {
        output << *str;
        }
      str++;
      }
    return;
    }

  // To VTK_UTF_8...

  if (output_encoding == VTK_ENCODING_UTF_8)
    {
    int from_iso_8859 = (input_encoding >= VTK_ENCODING_ISO_8859_1 &&
                         input_encoding <= VTK_ENCODING_ISO_8859_16);

    // From ISO-8859 or US-ASCII

    if (input_encoding == VTK_ENCODING_US_ASCII || from_iso_8859)
      {
      while (*str)
        {
        if (!special_entities || !vtkXMLUtilitiesEncodeEntities(*str, output))
          {
          if (*str > 0x7F)
            {
#if 0
            // This should be the right implementation, but it seems that
            // it just does not work for Expat. Brad and I should dig into
            // that later, but it seems weird. In the meantime, just
            // output the hex representation.

            output << "&#x"
                   << hex << (0xC0 | (*str >> 6))
                   << hex << (0x80 | (*str & 0x3F))
                   << ';';
#else
            output << "&#x" << hex << (int)(*str) << ';';
#endif
            }
          else if (*str < 30)
            {
            output << "&#x" << hex << (int)(*str) << ';';
            }
          else
            {
            output << *str;
            }
          }
        str++;
        }
      }

    // From VTK_ENCODING_UTF_8 (i.e. just encode the entities)
    // To be completed (need the whole &#x)

    else if (input_encoding == VTK_ENCODING_UTF_8)
      {
      while (*str)
        {
        if (!vtkXMLUtilitiesEncodeEntities(*str, output))
          {
          output << *str;
          }
        str++;
        }
      }

    // Unsupported input encoding

    else
      {
      vtkGenericWarningMacro(
        << "Input encoding not supported (" << input_encoding << ")");
      }
    }

  // From VTK_ENCODING_UTF_8...

  else if (input_encoding == VTK_ENCODING_UTF_8)
    {
    int to_iso_8859 = (output_encoding >= VTK_ENCODING_ISO_8859_1 &&
                       output_encoding <=VTK_ENCODING_ISO_8859_16);

    // To US-ASCII or ISO 8859

    if (output_encoding == VTK_ENCODING_US_ASCII || to_iso_8859)
      {
      while (*str)
        {
        if (!special_entities || !vtkXMLUtilitiesEncodeEntities(*str, output))
          {
          // Multi-byte 2-chars converted into one char

          if (*str > 0x7F)
            {
            output << (unsigned char)((*str << 6) | (str[1] & 0x3F));
            str++;
            }
          else
            {
            output << *str;
            }
          }
        str++;
        }
      }

    // Unsupported output encoding

    else
      {
      vtkGenericWarningMacro(
        << "Output encoding not supported (" << input_encoding << ")");
      }
    }
}

//----------------------------------------------------------------------------
void vtkXMLUtilities::CollateAttributes(vtkXMLDataElement *elem,
                                        ostream &os,
                                        const char *sep)
{
  if (!elem)
    {
    return;
    }

  int i, nb = elem->GetNumberOfAttributes();
  for (i = 0; i < nb; i++)
    {
    const char *name = elem->GetAttributeName(i);
    if (name)
      {
      const char *value = elem->GetAttribute(name);
      if (value)
        {
        if (i)
          {
          os << (sep ? sep : " ");
          }
        os << name << "=\"";
        vtkXMLUtilities::EncodeString(
          value, elem->GetAttributeEncoding(), os, VTK_ENCODING_UTF_8, 1);
        os << '\"';
        }
      }
    }

  return;
}

//----------------------------------------------------------------------------
void vtkXMLUtilities::FlattenElement(vtkXMLDataElement *elem,
                                     ostream &os,
                                     vtkIndent *indent,
                                     int indent_attributes)
{
  if (!elem)
    {
    return;
    }

  unsigned long pos = os.tellp();

  // Name

  if (indent)
    {
    os << *indent;
    }

  os << '<' << elem->GetName();

  // Attributes

  if (elem->GetNumberOfAttributes())
    {
    os << ' ';
    if (indent && indent_attributes)
      {
      unsigned long len = (unsigned long)os.tellp() - pos;
      if (os.fail())
        {
        return;
        }
      char *sep = new char [1 + len + 1];
      sep[0] = '\n';
      memset(sep + 1, ' ', len);
      sep[len + 1] = '\0';
      vtkXMLUtilities::CollateAttributes(elem, os, sep);
      delete [] sep;
      }
    else
      {
      vtkXMLUtilities::CollateAttributes(elem, os);
      }
    }

  const char *cdata = elem->GetCharacterData();
  int nb_nested = elem->GetNumberOfNestedElements();
  int need_close_tag = (nb_nested || cdata);

  if (!need_close_tag)
    {
    os << "/>";
    }
  else
    {
    os << '>';
    }

  // cdata

  if (cdata)
    {
    vtkXMLUtilities::EncodeString(
      cdata, elem->GetAttributeEncoding(), os, VTK_ENCODING_UTF_8, 1);
    }

  // Nested elements

  if (nb_nested)
    {
    if (indent)
      {
      os << '\n';
      }
    for (int i = 0; i < nb_nested; i++)
      {
      if (indent)
        {
        vtkIndent next_indent = indent->GetNextIndent();
        vtkXMLUtilities::FlattenElement(elem->GetNestedElement(i),
                                        os, &next_indent);
        }
      else
        {
        vtkXMLUtilities::FlattenElement(elem->GetNestedElement(i), os);
        }
      }
    if (indent)
      {
      os << *indent;
      }
    }

  // Close

  if (need_close_tag)
    {
    os << "</" << elem->GetName() << '>';
    }

  if (indent)
    {
    os << '\n';
    }
}

//----------------------------------------------------------------------------
int vtkXMLUtilities::WriteElementToFile(vtkXMLDataElement *elem,
                                        const char *filename,
                                        vtkIndent *indent)
{
  if (!elem || !filename)
    {
    return 0;
    }

  ofstream os(filename, ios::out);
  vtkXMLUtilities::FlattenElement(elem, os, indent);

  os.flush();
  if (os.fail())
    {
    os.close();
    unlink(filename);
    return 0;
    }
  return 1;
}

//----------------------------------------------------------------------------
vtkXMLDataElement*
vtkXMLUtilities::ReadElementFromStream(istream &is, int encoding)
{
  vtkXMLDataElement *res = NULL;
  vtkXMLDataParser* xml_parser = vtkXMLDataParser::New();
  xml_parser->SetAttributesEncoding(encoding);

  xml_parser->SetStream(&is);
  if (xml_parser->Parse())
    {
    res = xml_parser->GetRootElement();
    // Bump up the ref count since we are going to delete the parser
    // which actually owns the element
    res->SetReferenceCount(res->GetReferenceCount() + 1);
    vtkXMLUtilities::UnFactorElements(res);
    }

  xml_parser->Delete();
  return res;
}

//----------------------------------------------------------------------------
vtkXMLDataElement*
vtkXMLUtilities::ReadElementFromString(const char *str, int encoding)
{
  if (!str)
    {
    return 0;
    }

  vtksys_ios::stringstream strstr;
  strstr << str;
  vtkXMLDataElement *res =
    vtkXMLUtilities::ReadElementFromStream(strstr, encoding);

  return res;
}

//----------------------------------------------------------------------------
vtkXMLDataElement*
vtkXMLUtilities::ReadElementFromFile(const char *filename, int encoding)
{
  if (!filename)
    {
    return NULL;
    }

  ifstream is(filename);
  return vtkXMLUtilities::ReadElementFromStream(is, encoding);
}

//----------------------------------------------------------------------------
void vtkXMLUtilities::ReadElementFromAttributeArray(
        vtkXMLDataElement *element,
        const char** atts,
        int encoding)
{
  if(atts)
    {
    // If the target encoding is VTK_ENCODING_NONE or VTK_ENCODING_UNKNOWN,
    // then keep the internal/default encoding, otherwise encode each
    // attribute using that new format

    if (encoding != VTK_ENCODING_NONE && encoding != VTK_ENCODING_UNKNOWN)
      {
      element->SetAttributeEncoding(encoding);
      }

    // Process each attributes returned by Expat in UTF-8 encoding, and
    // convert them to our encoding

    for (int i = 0; atts[i] && atts[i + 1]; i += 2)
      {
      if (element->GetAttributeEncoding() == VTK_ENCODING_UTF_8)
        {
        element->SetAttribute(atts[i], atts[i + 1]);
        }
      else
        {
        vtksys_ios::ostringstream str;
        vtkXMLUtilities::EncodeString(
          atts[i+1], VTK_ENCODING_UTF_8, str, element->GetAttributeEncoding(), 0);
        str << ends;
        element->SetAttribute(atts[i], str.str().c_str());
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkXMLUtilitiesFindSimilarElementsInternal(
  vtkXMLDataElement *elem,
  vtkXMLDataElement *tree,
  vtkXMLUtilitiesDataElementContainer *results)
{
  if (!elem || !tree || !results || elem == tree)
    {
    return;
    }

  // If the element is equal to the current tree, append it to the
  // results, otherwise check the sub-trees

  if (elem->IsEqualTo(tree))
    {
    results->push_back(tree);
    }
  else
    {
    for (int i = 0; i < tree->GetNumberOfNestedElements(); i++)
      {
      vtkXMLUtilitiesFindSimilarElementsInternal(
        elem, tree->GetNestedElement(i), results);
      }
    }
}

//----------------------------------------------------------------------------
int vtkXMLUtilities::FindSimilarElements(vtkXMLDataElement *elem,
                                         vtkXMLDataElement *tree,
                                         vtkXMLDataElement ***results)
{
  if (!elem || ! tree)
    {
    return 0;
    }

  // Create a data element container, and find all similar elements

  vtkXMLUtilitiesDataElementContainer *container =
    new vtkXMLUtilitiesDataElementContainer;

  vtkXMLUtilitiesFindSimilarElementsInternal(elem, tree, container);

  // If nothing was found, exit now

  int size = (int)container->size();
  if (size)
    {
    // Allocate an array of element and copy the contents of the container
    // to this flat structure

    *results = new vtkXMLDataElement* [size];

    size = 0;
    for (vtkXMLUtilitiesDataElementContainer::const_iterator
           it = container->begin(); it != container->end(); ++it)
      {
      if (*it)
        {
        (*results)[size++] = *it;
        }
      }
    }

  delete container;

  return size;
}

//----------------------------------------------------------------------------
void vtkXMLUtilities::FactorElements(vtkXMLDataElement *tree)
{
  if (!tree)
    {
    return;
    }

  // Create the factored pool, and add it to the tree so that it can
  // factor itself too

  vtkXMLDataElement *pool = vtkXMLDataElement::New();
  pool->SetName(VTK_XML_UTILITIES_FACTORED_POOL_NAME);
  pool->SetAttributeEncoding(tree->GetAttributeEncoding());
  tree->AddNestedElement(pool);

  // Factor the tree, as long as some factorization has occurred
  // (multiple pass might be needed because larger trees are factored
  // first)

  while (vtkXMLUtilities::FactorElementsInternal(tree, tree, pool)) {};

  // Nothing factored, remove the useless pool

  if (!pool->GetNumberOfNestedElements())
    {
    tree->RemoveNestedElement(pool);
    }

  pool->Delete();
}

//----------------------------------------------------------------------------
int vtkXMLUtilities::FactorElementsInternal(vtkXMLDataElement *tree,
                                            vtkXMLDataElement *root,
                                            vtkXMLDataElement *pool)
{
  if (!tree || !root || !pool)
    {
    return 0;
    }

  // Do not bother factoring something already factored

  if (tree->GetName() &&
      !strcmp(tree->GetName(), VTK_XML_UTILITIES_FACTORED_REF_NAME))
    {
    return 0;
    }

  // Try to find all trees similar to the current tree

  vtkXMLDataElement **similar_trees;
  int nb_of_similar_trees = vtkXMLUtilities::FindSimilarElements(
    tree, root, &similar_trees);

  // None was found, try to factor the sub-trees

  if (!nb_of_similar_trees)
    {
    int res = 0;
    for (int i = 0; i < tree->GetNumberOfNestedElements(); i++)
      {
      res += vtkXMLUtilities::FactorElementsInternal(
        tree->GetNestedElement(i), root, pool);
      }
    return res ? 1 : 0;
    }

  // Otherwise replace those trees with factored refs

  char buffer[5];
  sprintf(buffer, "%02d_", pool->GetNumberOfNestedElements());

  vtksys_ios::ostringstream id;
  id << buffer << tree->GetName();

  vtkXMLDataElement *factored = vtkXMLDataElement::New();
  factored->SetName(VTK_XML_UTILITIES_FACTORED_NAME);
  factored->SetAttributeEncoding(pool->GetAttributeEncoding());
  factored->SetAttribute("Id", id.str().c_str());
  pool->AddNestedElement(factored);
  factored->Delete();

  vtkXMLDataElement *tree_copy = vtkXMLDataElement::New();
  tree_copy->DeepCopy(tree);
  factored->AddNestedElement(tree_copy);
  tree_copy->Delete();

  for (int i = 0; i < nb_of_similar_trees; i++)
    {
    similar_trees[i]->RemoveAllAttributes();
    similar_trees[i]->RemoveAllNestedElements();
    similar_trees[i]->SetCharacterData(NULL, 0);
    similar_trees[i]->SetName(VTK_XML_UTILITIES_FACTORED_REF_NAME);
    similar_trees[i]->SetAttribute("Id", id.str().c_str());
    }

  tree->RemoveAllAttributes();
  tree->RemoveAllNestedElements();
  tree->SetCharacterData(NULL, 0);
  tree->SetName(VTK_XML_UTILITIES_FACTORED_REF_NAME);
  tree->SetAttribute("Id", id.str().c_str());

  delete [] similar_trees;

  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLUtilities::UnFactorElements(vtkXMLDataElement *tree)
{
  if (!tree)
    {
    return;
    }

  // Search for the factored pool, if not found, we are done

  vtkXMLDataElement *pool = tree->FindNestedElementWithName(
    VTK_XML_UTILITIES_FACTORED_POOL_NAME);
  if (!pool)
    {
    return;
    }

  // Remove the pool from the tree, because it makes no sense
  // unfactoring it too

  pool->Register(tree);
  tree->RemoveNestedElement(pool);

  // Unfactor the tree

  vtkXMLUtilities::UnFactorElementsInternal(tree, pool);

  // Remove the useless empty pool

  pool->UnRegister(tree);
}

//----------------------------------------------------------------------------
int vtkXMLUtilities::UnFactorElementsInternal(vtkXMLDataElement *tree,
                                              vtkXMLDataElement *pool)
{
  if (!tree || !pool)
    {
    return 0;
    }

  int res = 0;

  // We found a factor, replace it with the corresponding sub-tree

  if (tree->GetName() &&
      !strcmp(tree->GetName(), VTK_XML_UTILITIES_FACTORED_REF_NAME))
    {
    vtkXMLDataElement *original_tree =
      pool->FindNestedElementWithNameAndAttribute(
        VTK_XML_UTILITIES_FACTORED_NAME, "Id", tree->GetAttribute("Id"));
    if (original_tree && original_tree->GetNumberOfNestedElements())
      {
      tree->DeepCopy(original_tree->GetNestedElement(0));
      res++;
      }
    }

  // Now try to unfactor the sub-trees

  for (int i = 0; i < tree->GetNumberOfNestedElements(); i++)
    {
    res += vtkXMLUtilities::UnFactorElementsInternal(
      tree->GetNestedElement(i), pool);
    }

  return res ? 1 : 0;
}
