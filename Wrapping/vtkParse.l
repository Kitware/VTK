%option yylineno
%{
/* We do not care of interactive mode */
#define YY_NEVER_INTERACTIVE 1
#define YY_NO_UNPUT 1

#ifdef _WIN32
#pragma warning ( disable : 4127 )
#pragma warning ( disable : 4131 )
#pragma warning ( disable : 4244 )
#pragma warning ( disable : 4251 )
#pragma warning ( disable : 4267 )
#pragma warning ( disable : 4305 )
#pragma warning ( disable : 4309 )
#pragma warning ( disable : 4706 )
#pragma warning ( disable : 4786 )
#endif

%}


%%

"/*" { int c1 = 0, c2 = input();
       for (;;)
	 {
         if (c2 == 0) 
           {
           fprintf(yyout,"Cannot find closing comment.\n");
           break;
           }
         if (c1 == '*' && c2 == '/') break;
         c1 = c2; c2 = input();
         }
     };

^[\t ]*"//BTX".*$ { 
       int c1 = 0, c2 = 0, c3 = 0, c4 = 0, c5 = input();
       for (;;)
	 {
         if (c5 == 0) 
           {
           fprintf(yyout,"Cannot find matching //ETX.\n");
           break;
           }
         if (c1 == '/' && c2 == '/' && c3 == 'E' && c4 == 'T' && c5 == 'X') break;
         c1 = c2; c2 = c3; c3 = c4; c4 = c5; c5 = input();
         }
       for (;;)
         {
         if (c5 == 0) break;
         if (c5 == '\n') break;
         c5 = input();
         } 
     };

^[\t ]*"// .NAME".* { 
     int pos = 1;
     while (yytext[pos-1] != 'M' || yytext[pos] != 'E')
       {
       pos++; 
       }
     yylval.str = (char *)malloc(yyleng + 1 - pos - 1);
     memcpy(yylval.str,yytext+ pos + 1,yyleng - pos - 1);
     yylval.str[yyleng - pos - 1] = '\0';
     data.NameComment = vtkstrdup(yylval.str);
     };

^[\t ]*"// .SECTION Description".* { 
     CommentState = 1;
     data.Description = (char*)malloc(100000);
     data.Description[0] = '\0';
     };

^[\t ]*"// .SECTION See Also".* { 
     CommentState = 3;
     data.SeeAlso = (char*)malloc(100000);
     data.SeeAlso[0] = '\0';
     };

^[\t ]*"// .SECTION see also".* { 
     CommentState = 3;
     data.SeeAlso = (char*)malloc(100000);
     data.SeeAlso[0] = '\0';
     };

^[\t ]*"// .SECTION Caveats".* { 
     CommentState = 4;
     data.Caveats = (char*)malloc(100000);
     data.Caveats[0] = '\0';
     };

^[\t ]*"// Description:".* { 
     CommentState = 2;
     HaveComment = 1;
     CommentText[0] = '\0';
     };

^[\t ]*[\r\n]  { 
     HaveComment = 0; 
     CommentState = 0; 
     };

^[\t ]*"//".*   { 
     int pos = 1;
     while (yytext[pos-1] != '/' || yytext[pos] != '/') pos++; 
     yylval.str = (char *)malloc(yyleng + 1 - pos - 1 + 1);
     memcpy(yylval.str,yytext+ pos + 1,yyleng - pos - 1);
     yylval.str[yyleng - pos - 1] = '\n';
     yylval.str[yyleng - pos] = '\0';
     switch (CommentState)
       {
       case 1: strcat(data.Description,yylval.str); break;
       case 2: strcat(CommentText,yylval.str); break;
       case 3: strcat(data.SeeAlso,yylval.str); break;
       case 4: strcat(data.Caveats,yylval.str); break;
       }
     };

"//".* ;

"VTK_"[A-Za-z0-9_]*"EXPORT" ;

"vtkNotUsed"[ ]*"("[^)]*")" ;

"["[\t\n\r\ ]*[1-9][0-9]*[\t\n\r ]*"]"  {
   sscanf(yytext+1,"%d",&yylval.integer); return(ARRAY_NUM);}


"class "[\t\n\r ]*"vtk"[a-zA-Z0-9_]*[\t\n\r ]*";"  return(CLASS_REF);

"void"[\t\n\r ]*"("[\t\n\r ]*"*"[a-zA-Z0-9_]*[\t\n\r ]*")"[\t\n\r ]*"("[\t\n\r ]*"void"[\t\n\r ]*"*"[\n\t ]*")" return(VAR_FUNCTION);

"short"[\t\n\r ]*"int" return(SHORT);
"long"[\t\n\r ]*"int"  return(LONG);
"int"[\t\n\r ]*"short" return(SHORT);
"int"[\t\n\r ]*"long"  return(LONG);

"class"		return(CLASS);
"public"  	return(PUBLIC);
"private"  	return(PRIVATE);
"protected" 	return(PROTECTED);
"int" 	        return(INT);
"float" 	return(FLOAT);
"short" 	return(SHORT);
"long"  	return(LONG);
"double" 	return(DOUBLE);
"void" 	        return(VOID);
"char" 	        return(CHAR);
"virtual"       return(VIRTUAL);
"const"         return(CONST);
"operator"      return(OPERATOR);
"unsigned"      return(UNSIGNED);
"friend"        return(FRIEND);
"static"        return(STATIC);

"vtkFloatingPointType" return(DOUBLE);
"vtkIdType"            return(LONG);
"vtkSetMacro"          return(SetMacro);
"vtkGetMacro"          return(GetMacro);
"vtkSetStringMacro"    return(SetStringMacro);
"vtkGetStringMacro"    return(GetStringMacro);
"vtkSetClampMacro"     return(SetClampMacro);
"vtkSetObjectMacro"    return(SetObjectMacro);
"vtkSetReferenceCountedObjectMacro" return(SetReferenceCountedObjectMacro);
"vtkGetObjectMacro"    return(GetObjectMacro);
"vtkBooleanMacro"      return(BooleanMacro);
"vtkSetVector2Macro"   return(SetVector2Macro);
"vtkSetVector3Macro"   return(SetVector3Macro);
"vtkSetVector4Macro"   return(SetVector4Macro);
"vtkSetVector6Macro"   return(SetVector6Macro);
"vtkGetVector2Macro"   return(GetVector2Macro);
"vtkGetVector3Macro"   return(GetVector3Macro);
"vtkGetVector4Macro"   return(GetVector4Macro);
"vtkGetVector6Macro"   return(GetVector6Macro);
"vtkSetVectorMacro"    return(SetVectorMacro);
"vtkGetVectorMacro"    return(GetVectorMacro);
"vtkViewportCoordinateMacro"     return(ViewportCoordinateMacro);
"vtkWorldCoordinateMacro" return(WorldCoordinateMacro);
"vtkTypeMacro"         return(TypeMacro);
"vtkTypeRevisionMacro" return(TypeMacro);

[0-9]+		{ sscanf(yytext,"%d",&yylval.integer); return(NUM);}

\"[^\"]+\"	{
		yylval.str =  vtkstrdup(yytext + 1);
		yylval.str[strlen(yytext)-2] = '\0';
		return(STRING);
		}

^"#"[^\n]*$     ;


"vtk"[0-9a-zA-Z_]*  { yylval.str = (char *)malloc(yyleng + 1);
                     memcpy(yylval.str,yytext,yyleng);
                     yylval.str[yyleng] = '\0';
                     return(VTK_ID);
                    }

[a-zA-Z_][0-9a-zA-Z_]*  { yylval.str = (char *)malloc(yyleng + 1);
                          memcpy(yylval.str,yytext,yyleng);
                          yylval.str[yyleng] = '\0';
                          return(ID);
                        }

[\t\n\r ] ;


"["	return(yytext[0]);
"]"	return(yytext[0]);


[~{}():;*=,&.\-] {	return(yytext[0]); }

.		return(OTHER);
%%

