%p 5000
%a 2500
%%

"/*" { int c1 = 0, c2 = input();
       for (;;)
	 {
         if (c2 == EOF) break;
         if (c1 == '*' && c2 == '/') break;
         c1 = c2; c2 = input();
         }
     };

"//BTX".*$ 
     { int c1 = 0, c2 = 0, c3 = 0, c4 = 0, c5 = input();
       for (;;)
	 {
         if (c5 == EOF) break;
         if (c1 == '/' && c2 == '/' && c3 == 'E' && c4 == 'T' && c5 == 'X') break;
         c1 = c2; c2 = c3; c3 = c4; c4 = c5; c5 = input();
         }
       for (;;)
         {
         if (c5 == EOF) break;
         if (c5 == '\n') break;
         c5 = input();
         } 
     };


"//".*$           ;

"vtkNotUsed"[ ]*"("[^)]*")" ;

"["[\t\n\ ]*[1-9][0-9]*[\t\n ]*"]"  
  { sscanf(yytext+1,"%d",&yylval.integer); return(ARRAY_NUM);}


"class "[\t\n ]*"vtk"[a-zA-Z0-9_]*[\t\n ]*";"  return(CLASS_REF);

"void"[\t\n ]*"("[\t\n ]*"*"[a-zA-Z0-9_]*[\t\n ]*")"[\t\n ]*"("[\t\n ]*"void"[\t\n ]*"*"[\n\t ]*")" return(VAR_FUNCTION);

"short"[\t\n ]*"int" return(SHORT);
"long"[\t\n ]*"int"  return(LONG);
"int"[\t\n ]*"short" return(SHORT);
"int"[\t\n ]*"long"  return(LONG);

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

"vtkSetMacro"          return(SetMacro);
"vtkGetMacro"          return(GetMacro);
"vtkSetStringMacro"    return(SetStringMacro);
"vtkGetStringMacro"    return(GetStringMacro);
"vtkSetClampMacro"     return(SetClampMacro);
"vtkSetObjectMacro"    return(SetObjectMacro);
"vtkSetRefCountedObjectMacro" return(SetRefCountedObjectMacro);
"vtkGetObjectMacro"    return(GetObjectMacro);
"vtkBooleanMacro"      return(BooleanMacro);
"vtkSetVector2Macro"   return(SetVector2Macro);
"vtkSetVector3Macro"   return(SetVector3Macro);
"vtkSetVector4Macro"   return(SetVector4Macro);
"vtkSetVectorMacro"    return(SetVectorMacro);
"vtkGetVectorMacro"    return(GetVectorMacro);
"vtkImageSetMacro"     return(ImageSetMacro);
"vtkImageSetExtentMacro" return(ImageSetExtentMacro);

[0-9]+		{ sscanf(yytext,"%d",&yylval.integer); return(NUM);}

\"[^\"]+\"	{
		yylval.str =  strdup(yytext + 1);
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

[\t\n ]+ ;

"["	return(yytext[0]);
"]"	return(yytext[0]);


[~{}():;*=,&.---]	return(yytext[0]);

.		return(OTHER);
%%
