/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParseJavaBeans.c

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <stdio.h>
#include <string.h>
#include "vtkParse.h"
#include "vtkParseMain.h"
#include "vtkParseHierarchy.h"

HierarchyInfo *hierarchyInfo = NULL;
int numberOfWrappedFunctions = 0;
FunctionInfo *wrappedFunctions[1000];
extern FunctionInfo *currentFunction;

void output_temp(FILE *fp,int i)
{
  unsigned int aType =
    (currentFunction->ArgTypes[i] & VTK_PARSE_UNQUALIFIED_TYPE);

  /* ignore void */
  if (aType == VTK_PARSE_VOID)
  {
    return;
  }

  if (currentFunction->ArgTypes[i] == VTK_PARSE_FUNCTION)
  {
    fprintf(fp,"Object id0, String id1");
    return;
  }

  if ((aType == VTK_PARSE_CHAR_PTR) ||
      (aType == VTK_PARSE_STRING) ||
      (aType == VTK_PARSE_STRING_REF))
  {
    fprintf(fp,"String ");
  }
  else
  {
    switch ((aType & VTK_PARSE_BASE_TYPE) & ~VTK_PARSE_UNSIGNED)
    {
      case VTK_PARSE_FLOAT:   fprintf(fp,"double "); break;
      case VTK_PARSE_DOUBLE:   fprintf(fp,"double "); break;
      case VTK_PARSE_INT:   fprintf(fp,"int "); break;
      case VTK_PARSE_SHORT:   fprintf(fp,"int "); break;
      case VTK_PARSE_LONG:   fprintf(fp,"int "); break;
      case VTK_PARSE_ID_TYPE:   fprintf(fp,"int "); break;
      case VTK_PARSE_LONG_LONG:   fprintf(fp,"int "); break;
      case VTK_PARSE___INT64:   fprintf(fp,"int "); break;
      case VTK_PARSE_VOID:     fprintf(fp,"void "); break;
      case VTK_PARSE_SIGNED_CHAR:   fprintf(fp,"char "); break;
      case VTK_PARSE_CHAR:     fprintf(fp,"char "); break;
      case VTK_PARSE_OBJECT:     fprintf(fp,"%s ",currentFunction->ArgClasses[i]); break;
      case VTK_PARSE_UNKNOWN: return;
    }
  }

  fprintf(fp,"id%i",i);
  if (((aType & VTK_PARSE_INDIRECT) == VTK_PARSE_POINTER) &&
      (aType != VTK_PARSE_CHAR_PTR) &&
      (aType != VTK_PARSE_OBJECT_PTR))
  {
    fprintf(fp,"[]");
  }
}

void return_result(FILE *fp)
{
  switch (currentFunction->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE)
  {
    case VTK_PARSE_FLOAT:
      fprintf(fp,"double ");
      break;
    case VTK_PARSE_VOID:
      fprintf(fp,"void ");
      break;
    case VTK_PARSE_CHAR:
      fprintf(fp,"char ");
      break;
    case VTK_PARSE_DOUBLE:
      fprintf(fp,"double ");
      break;
    case VTK_PARSE_INT:
    case VTK_PARSE_SHORT:
    case VTK_PARSE_LONG:
    case VTK_PARSE_ID_TYPE:
    case VTK_PARSE_LONG_LONG:
    case VTK_PARSE___INT64:
    case VTK_PARSE_UNSIGNED_CHAR:
    case VTK_PARSE_UNSIGNED_INT:
    case VTK_PARSE_UNSIGNED_SHORT:
    case VTK_PARSE_UNSIGNED_LONG:
    case VTK_PARSE_UNSIGNED_ID_TYPE:
    case VTK_PARSE_UNSIGNED_LONG_LONG:
    case VTK_PARSE_UNSIGNED___INT64:
      fprintf(fp,"int ");
      break;
    case VTK_PARSE_CHAR_PTR:
    case VTK_PARSE_STRING:
    case VTK_PARSE_STRING_REF:
      fprintf(fp,"String ");
      break;
    case VTK_PARSE_OBJECT_PTR:
      fprintf(fp,"%s ",currentFunction->ReturnClass);
      break;

      /* handle functions returning vectors */
      /* this is done by looking them up in a hint file */
    case VTK_PARSE_FLOAT_PTR:
    case VTK_PARSE_DOUBLE_PTR:
      fprintf(fp,"double[] ");
      break;
    case VTK_PARSE_INT_PTR:
    case VTK_PARSE_SHORT_PTR:
    case VTK_PARSE_LONG_PTR:
    case VTK_PARSE_ID_TYPE_PTR:
    case VTK_PARSE_LONG_LONG_PTR:
    case VTK_PARSE___INT64_PTR:
    case VTK_PARSE_SIGNED_CHAR_PTR:
    case VTK_PARSE_UNSIGNED_CHAR_PTR:
    case VTK_PARSE_UNSIGNED_INT_PTR:
    case VTK_PARSE_UNSIGNED_SHORT_PTR:
    case VTK_PARSE_UNSIGNED_LONG_PTR:
    case VTK_PARSE_UNSIGNED_ID_TYPE_PTR:
    case VTK_PARSE_UNSIGNED_LONG_LONG_PTR:
    case VTK_PARSE_UNSIGNED___INT64_PTR:
      fprintf(fp,"int[]  "); break;
  }
}

/* Check to see if two types will map to the same Java type,
 * return 1 if type1 should take precedence,
 * return 2 if type2 should take precedence,
 * return 0 if the types do not map to the same type */
static int CheckMatch(
  unsigned int type1, unsigned int type2, const char *c1, const char *c2)
{
  static unsigned int floatTypes[] = {
    VTK_PARSE_DOUBLE, VTK_PARSE_FLOAT, 0 };

  static unsigned int intTypes[] = {
    VTK_PARSE_UNSIGNED_LONG_LONG, VTK_PARSE_UNSIGNED___INT64,
    VTK_PARSE_LONG_LONG, VTK_PARSE___INT64, VTK_PARSE_ID_TYPE,
    VTK_PARSE_UNSIGNED_LONG, VTK_PARSE_LONG,
    VTK_PARSE_UNSIGNED_INT, VTK_PARSE_INT,
    VTK_PARSE_UNSIGNED_SHORT, VTK_PARSE_SHORT,
    VTK_PARSE_UNSIGNED_CHAR, VTK_PARSE_SIGNED_CHAR, 0 };

  static unsigned int stringTypes[] = {
    VTK_PARSE_CHAR_PTR, VTK_PARSE_STRING_REF, VTK_PARSE_STRING, 0 };

  static unsigned int *numericTypes[] = { floatTypes, intTypes, 0 };

  int i, j;
  int hit1, hit2;

  if ((type1 & VTK_PARSE_UNQUALIFIED_TYPE) ==
      (type2 & VTK_PARSE_UNQUALIFIED_TYPE))
  {
    if ((type1 & VTK_PARSE_BASE_TYPE) == VTK_PARSE_OBJECT)
    {
      if (strcmp(c1, c2) == 0)
      {
        return 1;
      }
      return 0;
    }
    else
    {
      return 1;
    }
  }

  for (i = 0; numericTypes[i]; i++)
  {
    hit1 = 0;
    hit2 = 0;
    for (j = 0; numericTypes[i][j]; j++)
    {
      if ((type1 & VTK_PARSE_BASE_TYPE) == numericTypes[i][j])
      {
        hit1 = j+1;
      }
      if ((type2 & VTK_PARSE_BASE_TYPE) == numericTypes[i][j])
      {
        hit2 = j+1;
      }
    }
    if (hit1 && hit2 &&
        (type1 & VTK_PARSE_INDIRECT) == (type2 & VTK_PARSE_INDIRECT))
    {
      if (hit1 < hit2)
      {
        return 1;
      }
      else
      {
        return 2;
      }
    }
  }

  hit1 = 0;
  hit2 = 0;
  for (j = 0; stringTypes[j]; j++)
  {
    if ((type1 & VTK_PARSE_UNQUALIFIED_TYPE) == stringTypes[j])
    {
      hit1 = j+1;
    }
    if ((type2 & VTK_PARSE_UNQUALIFIED_TYPE) == stringTypes[j])
    {
      hit2 = j+1;
    }
  }
  if (hit1 && hit2)
  {
    if (hit1 < hit2)
    {
      return 1;
    }
    else
    {
      return 2;
    }
  }

  return 0;
}

/* have we done one of these yet */
int DoneOne()
{
  int i,j;
  int match;
  FunctionInfo *fi;

  for (i = 0; i < numberOfWrappedFunctions; i++)
  {
    fi = wrappedFunctions[i];

    if ((!strcmp(fi->Name,currentFunction->Name))
        &&(fi->NumberOfArguments == currentFunction->NumberOfArguments))
    {
      match = 1;
      for (j = 0; j < fi->NumberOfArguments; j++)
      {
        if (!CheckMatch(currentFunction->ArgTypes[j], fi->ArgTypes[j],
                        currentFunction->ArgClasses[j],fi->ArgClasses[j]))
        {
          match = 0;
        }
      }
      if (!CheckMatch(currentFunction->ReturnType, fi->ReturnType,
                      currentFunction->ReturnClass, fi->ReturnClass))
      {
        match = 0;
      }
      if (match) return 1;
    }
  }
  return 0;
}

static int isClassWrapped(const char *classname)
{
  HierarchyEntry *entry;

  if (hierarchyInfo)
  {
    entry = vtkParseHierarchy_FindEntry(hierarchyInfo, classname);

    if (entry == 0 ||
        vtkParseHierarchy_GetProperty(entry, "WRAP_EXCLUDE") ||
        !vtkParseHierarchy_IsTypeOf(hierarchyInfo, entry, "vtkObjectBase"))
    {
      return 0;
    }
  }

  return 1;
}

int checkFunctionSignature(ClassInfo *data)
{
  static unsigned int supported_types[] = {
    VTK_PARSE_VOID, VTK_PARSE_BOOL, VTK_PARSE_FLOAT, VTK_PARSE_DOUBLE,
    VTK_PARSE_CHAR, VTK_PARSE_UNSIGNED_CHAR, VTK_PARSE_SIGNED_CHAR,
    VTK_PARSE_INT, VTK_PARSE_UNSIGNED_INT,
    VTK_PARSE_SHORT, VTK_PARSE_UNSIGNED_SHORT,
    VTK_PARSE_LONG, VTK_PARSE_UNSIGNED_LONG,
    VTK_PARSE_ID_TYPE, VTK_PARSE_UNSIGNED_ID_TYPE,
    VTK_PARSE_LONG_LONG, VTK_PARSE_UNSIGNED_LONG_LONG,
    VTK_PARSE___INT64, VTK_PARSE_UNSIGNED___INT64,
    VTK_PARSE_OBJECT, VTK_PARSE_STRING,
    0
  };

  int i, j;
  int args_ok = 1;
  unsigned int rType =
    (currentFunction->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE);
  unsigned int aType = 0;
  unsigned int baseType = 0;

  /* some functions will not get wrapped no matter what else */
  if (currentFunction->IsOperator ||
      currentFunction->ArrayFailure ||
      !currentFunction->IsPublic ||
      !currentFunction->Name)
  {
    return 0;
  }

  /* NewInstance and SafeDownCast can not be wrapped because it is a
     (non-virtual) method which returns a pointer of the same type as
     the current pointer. Since all methods are virtual in Java, this
     looks like polymorphic return type.  */
  if (!strcmp("NewInstance",currentFunction->Name))
  {
    return 0;
  }

  if (!strcmp("SafeDownCast",currentFunction->Name))
  {
    return 0;
  }

  /* The GetInput() in vtkMapper cannot be overriden with a
   * different return type, Java doesn't allow this */
  if (strcmp(data->Name, "vtkMapper") == 0 &&
      strcmp(currentFunction->Name, "GetInput") == 0)
  {
    return 0;
  }

  /* function pointer arguments for callbacks */
  if (currentFunction->NumberOfArguments == 2 &&
      currentFunction->ArgTypes[0] == VTK_PARSE_FUNCTION &&
      currentFunction->ArgTypes[1] == VTK_PARSE_VOID_PTR &&
      rType == VTK_PARSE_VOID)
  {
    return 1;
  }

  /* check to see if we can handle the args */
  for (i = 0; i < currentFunction->NumberOfArguments; i++)
  {
    aType = (currentFunction->ArgTypes[i] & VTK_PARSE_UNQUALIFIED_TYPE);
    baseType = (aType & VTK_PARSE_BASE_TYPE);

    for (j = 0; supported_types[j] != 0; j++)
    {
      if (baseType == supported_types[j]) { break; }
    }
    if (supported_types[j] == 0)
    {
      args_ok = 0;
    }

    if (baseType == VTK_PARSE_OBJECT)
    {
      if ((aType & VTK_PARSE_INDIRECT) != VTK_PARSE_POINTER)
      {
        args_ok = 0;
      }
      else if (!isClassWrapped(currentFunction->ArgClasses[i]))
      {
        args_ok = 0;
      }
    }

    if (aType == VTK_PARSE_OBJECT) args_ok = 0;
    if (((aType & VTK_PARSE_INDIRECT) != VTK_PARSE_POINTER) &&
        ((aType & VTK_PARSE_INDIRECT) != 0) &&
        (aType != VTK_PARSE_STRING_REF)) args_ok = 0;
    if (aType == VTK_PARSE_STRING_PTR) args_ok = 0;
    if (aType == VTK_PARSE_UNSIGNED_CHAR_PTR) args_ok = 0;
    if (aType == VTK_PARSE_UNSIGNED_INT_PTR) args_ok = 0;
    if (aType == VTK_PARSE_UNSIGNED_SHORT_PTR) args_ok = 0;
    if (aType == VTK_PARSE_UNSIGNED_LONG_PTR) args_ok = 0;
    if (aType == VTK_PARSE_UNSIGNED_ID_TYPE_PTR) args_ok = 0;
    if (aType == VTK_PARSE_UNSIGNED_LONG_LONG_PTR) args_ok = 0;
    if (aType == VTK_PARSE_UNSIGNED___INT64_PTR) args_ok = 0;
  }

  baseType = (rType & VTK_PARSE_BASE_TYPE);

  for (j = 0; supported_types[j] != 0; j++)
  {
    if (baseType == supported_types[j]) { break; }
  }
  if (supported_types[j] == 0)
  {
    args_ok = 0;
  }

  if (baseType == VTK_PARSE_OBJECT)
  {
    if ((rType & VTK_PARSE_INDIRECT) != VTK_PARSE_POINTER)
    {
      args_ok = 0;
    }
    else if (!isClassWrapped(currentFunction->ReturnClass))
    {
      args_ok = 0;
    }
  }

  if (((rType & VTK_PARSE_INDIRECT) != VTK_PARSE_POINTER) &&
      ((rType & VTK_PARSE_INDIRECT) != 0) &&
      (rType != VTK_PARSE_STRING_REF)) args_ok = 0;
  if (rType == VTK_PARSE_STRING_PTR) args_ok = 0;

  /* eliminate unsigned char * and unsigned short * */
  if (rType == VTK_PARSE_UNSIGNED_INT_PTR) args_ok = 0;
  if (rType == VTK_PARSE_UNSIGNED_SHORT_PTR) args_ok = 0;
  if (rType == VTK_PARSE_UNSIGNED_LONG_PTR) args_ok = 0;
  if (rType == VTK_PARSE_UNSIGNED_ID_TYPE_PTR) args_ok = 0;
  if (rType == VTK_PARSE_UNSIGNED_LONG_LONG_PTR) args_ok = 0;
  if (rType == VTK_PARSE_UNSIGNED___INT64_PTR) args_ok = 0;

  /* make sure we have all the info we need for array arguments in */
  for (i = 0; i < currentFunction->NumberOfArguments; i++)
  {
    aType = (currentFunction->ArgTypes[i] & VTK_PARSE_UNQUALIFIED_TYPE);

    if (((aType & VTK_PARSE_INDIRECT) == VTK_PARSE_POINTER)&&
        (currentFunction->ArgCounts[i] <= 0)&&
        (aType != VTK_PARSE_OBJECT_PTR)&&
        (aType != VTK_PARSE_CHAR_PTR)) args_ok = 0;
  }

  /* if we need a return type hint make sure we have one */
  switch (rType)
  {
    case VTK_PARSE_FLOAT_PTR:
    case VTK_PARSE_VOID_PTR:
    case VTK_PARSE_DOUBLE_PTR:
    case VTK_PARSE_INT_PTR:
    case VTK_PARSE_SHORT_PTR:
    case VTK_PARSE_LONG_PTR:
    case VTK_PARSE_ID_TYPE_PTR:
    case VTK_PARSE_LONG_LONG_PTR:
    case VTK_PARSE___INT64_PTR:
    case VTK_PARSE_SIGNED_CHAR_PTR:
    case VTK_PARSE_BOOL_PTR:
    case VTK_PARSE_UNSIGNED_CHAR_PTR:
      args_ok = currentFunction->HaveHint;
      break;
  }

  /* make sure there isn't a Java-specific override */
  if (!strcmp("vtkObject",data->Name))
  {
    /* remove the original vtkCommand observer methods */
    if (!strcmp(currentFunction->Name,"AddObserver") ||
        !strcmp(currentFunction->Name,"GetCommand") ||
        (!strcmp(currentFunction->Name,"RemoveObserver") &&
         (currentFunction->ArgTypes[0] != VTK_PARSE_UNSIGNED_LONG)) ||
        ((!strcmp(currentFunction->Name,"RemoveObservers") ||
          !strcmp(currentFunction->Name,"HasObserver")) &&
         (((currentFunction->ArgTypes[0] != VTK_PARSE_UNSIGNED_LONG) &&
           (currentFunction->ArgTypes[0] !=
            (VTK_PARSE_CHAR_PTR|VTK_PARSE_CONST))) ||
          (currentFunction->NumberOfArguments > 1))) ||
        (!strcmp(currentFunction->Name,"RemoveAllObservers") &&
         (currentFunction->NumberOfArguments > 0)))
    {
      args_ok = 0;
    }
  }
  else if (!strcmp("vtkObjectBase",data->Name))
  {
    /* remove the special vtkObjectBase methods */
    if (!strcmp(currentFunction->Name,"Print"))
    {
      args_ok = 0;
    }
  }

  /* make sure it isn't a Delete or New function */
  if (!strcmp("Delete",currentFunction->Name) ||
      !strcmp("New",currentFunction->Name))
  {
    args_ok = 0;
  }

  return args_ok;
}

void outputFunction(FILE *fp, ClassInfo *data)
{
  unsigned int rType =
    (currentFunction->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE);
  unsigned int aType = 0;
  int i;
  /* beans */
  char *beanfunc;

  /* make the first letter lowercase for set get methods */
  beanfunc = strdup(currentFunction->Name);
  if (isupper(beanfunc[0])) beanfunc[0] = beanfunc[0] + 32;

  args_ok = checkFunctionSignature(data);

  if (currentFunction->IsPublic && args_ok &&
      strcmp(data->Name,currentFunction->Name) &&
      strcmp(data->Name, currentFunction->Name + 1))
  {
    /* make sure we haven't already done one of these */
    if (!DoneOne())
    {
      fprintf(fp,"\n  private native ");
      return_result(fp);
      fprintf(fp,"%s_%i(",currentFunction->Name,numberOfWrappedFunctions);

      for (i = 0; i < currentFunction->NumberOfArguments; i++)
      {
        if (i)
        {
          fprintf(fp,",");
        }
        output_temp(fp,i);

        /* ignore args after function pointer */
        if (currentFunction->ArgTypes[i] == VTK_PARSE_FUNCTION)
        {
          break;
        }
      }
      fprintf(fp,");\n");
      fprintf(fp,"  public ");
      return_result(fp);
      fprintf(fp,"%s(",beanfunc);

      for (i = 0; i < currentFunction->NumberOfArguments; i++)
      {
        if (i)
        {
          fprintf(fp,",");
        }
        output_temp(fp,i);

        /* ignore args after function pointer */
        if (currentFunction->ArgTypes[i] == VTK_PARSE_FUNCTION)
        {
          break;
        }
      }
      /* if not void then need return otherwise none */
      if (rType == VTK_PARSE_VOID)
      {
        fprintf(fp,")\n    { %s_%i(",currentFunction->Name,
                numberOfWrappedFunctions);
      }
      else
      {
        fprintf(fp,")\n    { return %s_%i(",currentFunction->Name,
                numberOfWrappedFunctions);
      }
      for (i = 0; i < currentFunction->NumberOfArguments; i++)
      {
        if (i)
        {
          fprintf(fp,",");
        }
        fprintf(fp,"id%i",i);

        /* ignore args after function pointer */
        if (currentFunction->ArgTypes[i] == VTK_PARSE_FUNCTION)
        {
          break;
        }
      }
      if ((currentFunction->NumberOfArguments == 1) &&
          (currentFunction->ArgTypes[0] == VTK_PARSE_FUNCTION)) fprintf(fp,",id1");

      /* stick in secret beanie code for set methods */
      if (rType == VTK_PARSE_VOID)
      {
        aType = (currentFunction->ArgTypes[0] & VTK_PARSE_UNQUALIFIED_TYPE);

        /* only care about set methods and On/Off methods */
        if (!strncmp(beanfunc,"set",3) &&
            currentFunction->NumberOfArguments == 1 &&
            (((aType & VTK_PARSE_INDIRECT) == 0 &&
              (aType & VTK_PARSE_UNSIGNED) == 0)||
             aType == VTK_PARSE_CHAR_PTR ||
             (aType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_OBJECT))
        {
          char prop[256];

          strncpy(prop,beanfunc+3,strlen(beanfunc)-3);
          prop[strlen(beanfunc)-3] = '\0';
          if (isupper(prop[0])) prop[0] = prop[0] + 32;
          fprintf(fp,");\n      changes.firePropertyChange(\"%s\",null,",prop);

          /* handle basic types */
          if ((aType == VTK_PARSE_CHAR_PTR) ||
              (aType == VTK_PARSE_STRING) ||
              (aType == VTK_PARSE_STRING_REF))
          {
            fprintf(fp," id0");
          }
          else
          {
            switch ((aType & VTK_PARSE_BASE_TYPE) & ~VTK_PARSE_UNSIGNED)
            {
              case VTK_PARSE_FLOAT:
              case VTK_PARSE_DOUBLE:   fprintf(fp," new Double(id0)"); break;
              case VTK_PARSE_INT:
              case VTK_PARSE_SHORT:
              case VTK_PARSE_LONG:   fprintf(fp," new Integer(id0)"); break;
              case VTK_PARSE_OBJECT:   fprintf(fp," id0"); break;
              case VTK_PARSE_CHAR:   /* not implemented yet */
              default:  fprintf(fp," null");
            }
          }
        }
        /* not a set method is it an On/Off method ? */
        else
        {
          if (!strncmp(beanfunc + strlen(beanfunc) - 2, "On",2))
          {
            /* OK we think this is a Boolean method so need to fire a change */
            char prop[256];
            strncpy(prop,beanfunc,strlen(beanfunc)-2);
            prop[strlen(beanfunc)-2] = '\0';
            fprintf(fp,");\n      changes.firePropertyChange(\"%s\",null,new Integer(1)",
                    prop);
          }
          if (!strncmp(beanfunc + strlen(beanfunc) - 3, "Off",3))
          {
            /* OK we think this is a Boolean method so need to fire a change */
            char prop[256];
            strncpy(prop,beanfunc,strlen(beanfunc)-3);
            prop[strlen(beanfunc)-3] = '\0';
            fprintf(fp,");\n      changes.firePropertyChange(\"%s\",null,new Integer(0)",
                    prop);
          }
        }
      }
      fprintf(fp,"); }\n");

      wrappedFunctions[numberOfWrappedFunctions] = currentFunction;
      numberOfWrappedFunctions++;
    }
  }
  free(beanfunc);
}

/* print the parsed structures */
void vtkParseOutput(FILE *fp, FileInfo *file_info)
{
  OptionInfo *options;
  ClassInfo *data;
  int i;

  if ((data = file_info->MainClass) == NULL)
  {
    return;
  }

  /* get the command-line options */
  options = vtkParse_GetCommandLineOptions();

  /* get the hierarchy info for accurate typing */
  if (options->HierarchyFileNames)
  {
    hierarchyInfo = vtkParseHierarchy_ReadFiles(
      options->NumberOfHierarchyFileNames, options->HierarchyFileNames);
  }

  fprintf(fp,"// java wrapper for %s object\n//\n",data->Name);
  fprintf(fp,"\npackage vtk;\n");

  /* beans */
  if (!data->NumberOfSuperClasses)
  {
    fprintf(fp,"import java.beans.*;\n");
  }

if (strcmp("vtkObject",data->Name))
{
    fprintf(fp,"import vtk.*;\n");
}
  fprintf(fp,"\npublic class %s",data->Name);
  if (strcmp("vtkObject",data->Name))
  {
    if (data->NumberOfSuperClasses)
      fprintf(fp," extends %s",data->SuperClasses[0]);
  }
  fprintf(fp,"\n{\n");

  fprintf(fp,"  public %s getThis%s() { return this;}\n\n",
          data->Name, data->Name+3);

  /* insert function handling code here */
  for (i = 0; i < data->NumberOfFunctions; i++)
  {
    currentFunction = data->Functions[i];
    outputFunction(fp, data);
  }

if (!data->NumberOfSuperClasses)
{
    fprintf(fp,"\n  public %s() { this.VTKInit();};\n",data->Name);
    fprintf(fp,"  protected int vtkId = 0;\n");

    /* beans */
    fprintf(fp,"  public void addPropertyChangeListener(PropertyChangeListener l)\n  {\n");
    fprintf(fp,"    changes.addPropertyChangeListener(l);\n  }\n");
    fprintf(fp,"  public void removePropertyChangeListener(PropertyChangeListener l)\n  {\n");
    fprintf(fp,"    changes.removePropertyChangeListener(l);\n  }\n");
    fprintf(fp,"  protected PropertyChangeSupport changes = new PropertyChangeSupport(this);\n\n");

    /* if we are a base class and have a delete method */
    if (data->HasDelete)
    {
      fprintf(fp,"\n  public native void VTKDelete();\n");
      fprintf(fp,"  protected void finalize() { this.VTKDelete();};\n");
    }
}
  if ((!data->IsAbstract)&&
      strcmp(data->Name,"vtkDataWriter") &&
      strcmp(data->Name,"vtkPointSet") &&
      strcmp(data->Name,"vtkDataSetSource")
      )
  {
    fprintf(fp,"  public native void   VTKInit();\n");
  }
  if (!strcmp("vtkObject",data->Name))
  {
    fprintf(fp,"  public native String Print();\n");
  }
  fprintf(fp,"}\n");
}
