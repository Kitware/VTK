// This is a ragel file for generating a paser for MTL files
// Note that some MTL files are whitespace sensitive
// Mainly with unquoted string names with spaces
// ala
//
// newmtl material name with spaces and no quotes
//
// or
//
// map_Kd my texture file.png
//
%%{
  machine simple_lexer;

  default = ^0;
  lineend = ('\n' | '\r\n');
  greyspace = [\t\v\f ];
  comment = '#'[^\r\n]* . lineend;

  action appendString { recentString += *p;  }
  action storeString
  {
    Token tok;
    tok.StringValue = recentString;
    tok.Type = Token::String;
    tokens.push_back(tok);
    recentString.clear();
    currentNum.clear();
    recentSpace.clear();
  }
  string = ([^\t\v\f\r\n ]+ @appendString);

  action appendSpace { recentSpace += *p;  }
  action storeSpace
  {
    Token tok;
    tok.StringValue = recentSpace;
    tok.Type = Token::Space;
    tokens.push_back(tok);
    recentSpace.clear();
    currentNum.clear();
    recentString.clear();
  }
  spacestring = ([\t\v\f ]+ @appendSpace);

  action appendNum { currentNum += *p;  }
  action storeNum
  {
    currentNum += '\0';
    Token tok;
    tok.StringValue = currentNum;
    tok.NumberValue = std::atof(currentNum.c_str());
    tok.Type = Token::Number;
    tokens.push_back(tok);
    currentNum.clear();
    recentString.clear();
    recentSpace.clear();
  }
  number = ('+'|'-')? @appendNum [0-9]+ @appendNum ('.' @appendNum [0-9]+ @appendNum)?;

  main := |*
    comment => { currentNum.clear(); recentString.clear(); recentSpace.clear(); };


    number => storeNum;

    string => storeString;

    spacestring => storeSpace;

    lineend =>
    {
      Token tok;
      tok.Type = Token::LineEnd;
      tokens.push_back(tok);
      currentNum.clear(); recentString.clear(); recentSpace.clear();
    };

    default => { std::string value = "Error unknown text: "; value += std::string(ts, te-ts); cerr << value << "\n"; };

  *|;

}%%

%% write data;

int parseMTL(
  const char *start,
  std::vector<Token>  &tokens
)
{
  int act, cs, res = 0;
  const char *p = start;
  const char *pe = p + strlen(start) + 1;
  const char *eof = pe;
  const char *ts = nullptr;
  const char *te = nullptr;

  std::string recentString;
  std::string recentSpace;
  std::string currentNum;

  %% write init;
  %% write exec;

  return res;
}
