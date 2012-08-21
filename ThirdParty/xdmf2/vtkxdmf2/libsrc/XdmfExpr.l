%{
#undef input
#undef output
#undef unput
#include <XdmfObject.h>
#include <XdmfExpr.h>
#include <XdmfArray.h>
/* Geneated by bison */
#include "XdmfExprYacc.tab.h"
// #include <ice.h>
#include <math.h>
#define YY_NEVER_INTERACTIVE 1

#ifndef yylval
#define yylval dice_yylval
extern YYSTYPE dice_yylval;
#endif


#if defined( _WIN32 ) && !defined( __CYGWIN__ )
/* Handle Windows properly */
#  include <io.h>
#  define YY_NO_UNISTD_H
#  define isatty _isatty
#endif

static int
GetPointerFromString( char *_c, void **_ptr ){
XDMF_BIG_INT	_p;

_p = 0;
if ( strcmp( _c, "NULL" ) == 0 ){
	*_ptr = NULL;
	return( 0 );
	}

  /* Pointer values must start with leading underscore */
  if (*_c == '_') {
	*_ptr = TagNameToArray( _c );
	if( *_ptr == NULL ) {
		return( 1 );
	}
	return( 0 );
/* Old
      _c++;
      while (*_c) {
	  if ((*_c >= '0') && (*_c <= '9'))
	    _p = (_p << 4) + (*_c - '0');
	  else if ((*_c >= 'a') && (*_c <= 'f'))
	    _p = (_p << 4) + ((*_c - 'a') + 10);
	  else
	    break;
	  _c++;
      }
*/

   } else {
	*_ptr = NULL;
	return( 1 );
  }

*_ptr = *((void **) &_p);
return( 0 );
}

%}

%%
join { return JOIN; }
JOIN { return JOIN; }
Join { return JOIN; }
where { return WHERE; }
WHERE { return WHERE; }
Where { return WHERE; }
index { return INDEX; }
INDEX { return INDEX; }
Index { return INDEX; }
">>" { return GTGT; }
"<<" { return LTLT; }
"==" { return EQEQ; }
"<" { return LT; }
"<=" { return LE; }
">" { return GT; }
">=" { return GE; }
"!=" { return NE; }
([-+]?[0-9]+)	{
	yylval.IntegerValue = atoi(yytext);
	return tokINTEGER;
	}
([-+]?[0-9]+|([-+]?[0-9]*\.[0-9]+)([eE][-+]?[0-9]+)?) {
	yylval.DoubleValue = atof(yytext);
	return lFLOAT;
	}
_+[0-9a-zA-Z_]*"_"* {
	if( GetPointerFromString( yytext, &yylval.ArrayPointer ) ){
		printf("Bad Pointer Conversion\n");
		yylval.ArrayPointer = NULL;
	}
	return tokARRAY;
	}
[A-Za-z][A-Za-z0-9]* {
	/* Symbol */
	XdmfExprSymbol	*sp;

	sp = XdmfExprSymbolLookup( yytext );
	yylval.Symbol = sp;
	return( NAME );
	}
[ \t]	;
\n	return 0;
. 	return yytext[0];
%%

extern int	XdmfExprInput();
extern void	XdmfExprUnput( int c );
extern void	XdmfExprOutput( int c );

#define input() XdmfExprInput()
#define unput(c) XdmfExprUnput((c))
#define output(c) XdmfExprOutput( ( c ) )

void XdmfYYUnput(int c, char* buf_ptr )
{
  yyunput(c, buf_ptr);
}
