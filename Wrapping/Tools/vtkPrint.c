#include <stdio.h>
#include "vtkParse.h"

/* print the parsed structures */
void vtkParseOutput(FILE *fp, FileInfo *data)
{
  int i;

  fprintf(fp,"Class Name: %s\n",data->ClassName);
  fprintf(fp,"Name Comment: %s\n",data->NameComment ? data->NameComment : "None");
  fprintf(fp,"Description: %s\n",data->Description ?data->Description : "None");
  fprintf(fp,"Caveats: %s\n",data->Caveats ? data->Caveats : "None");
  fprintf(fp,"See Also: %s\n",data->SeeAlso ?data->SeeAlso : "None");
  fprintf(fp,"Has Delete: %s\n",data->HasDelete ? "Yes" : "No");
  fprintf(fp,"Abstract: %s\n",data->HasDelete ? "Yes" : "No");
  fprintf(fp,"Number Of Super Classes: %i\n",data->NumberOfSuperClasses);
  for (i = 0; i < data->NumberOfSuperClasses; i++)
  {
    fprintf(fp,"  %s\n",data->SuperClasses[i]);
  }

  fprintf(fp,"Number Of Functions: %i\n",data->NumberOfFunctions);
  for (i = 0; i < data->NumberOfFunctions; i++)
  {
    fprintf(fp,"  Function Name: %s\n",data->Functions[i].Name ?
            data->Functions[i].Name : "None");
    fprintf(fp,"  Function Signature: %s\n",data->Functions[i].Signature ?
            data->Functions[i].Signature : "None");
    fprintf(fp,"    Number Of Arguments: %i\n",
            data->Functions[i].NumberOfArguments);
    fprintf(fp,"    Array Failure: %s\n",
            data->Functions[i].ArrayFailure ? "Yes" : "No");
    fprintf(fp,"    Pure Virtual: %s\n",
            data->Functions[i].IsPureVirtual ? "Yes" : "No");
    fprintf(fp,"    Public: %s\n",
            data->Functions[i].IsPublic ? "Yes" : "No");
    fprintf(fp,"    Operator: %s\n",
            data->Functions[i].IsOperator ? "Yes" : "No");
    fprintf(fp,"    Have Hint: %s\n",
            data->Functions[i].HaveHint ? "Yes" : "No");
    fprintf(fp,"    Hint Size: %i\n",
            data->Functions[i].HintSize);
    fprintf(fp,"    Return Type: %i\n",
            data->Functions[i].ReturnType);
    fprintf(fp,"    ReturnClass: %s\n",
            data->Functions[i].ReturnClass ?
            data->Functions[i].ReturnClass : "None");
    fprintf(fp,"    Comment: %s\n",
            data->Functions[i].Comment ?
            data->Functions[i].Comment : "None");
  }
}
