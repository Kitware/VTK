# This is the translator that converts Tcl test to python.
# Not all Tcl test are translatable. 
# To ensure that a test can be translated :
# 1) do not use Tcl arrays
# 2) do not use string substitution except in variable names
#    eg. obj${i} GetOutput is okay
#        obj12 GetOutputAs${i} is not okay.
# 3) do not use expr within expr. As such it is typically superflous.
# 4) event handler procedures in Python take 2 arguments, hence,
#    define the Tcl event handlers with 2 default arguments.
# 5) define procedures before using them or setting them on VTK objects
#    as callbacks.
# 6) do not use puts etc.
# 7) always quote strings such as filenames, arguments etc.


import sys
import re
for i in range(0, len(sys.argv)):
    if sys.argv[i] == '-A' and i < len(sys.argv)-1:
        sys.path = sys.path + [sys.argv[i+1]]

import vtkTclParser
import vtkpython

reVariable = re.compile("^([+\-])?\$([^\$\{\}]+)$")
reCompoundVariable = re.compile("\$(?:([^\$\}\{]+)|\{([^\$\}]+)\})")

class vtkTclToPyConvertor(vtkTclParser.vtkTclParser):
  def __init__(self):
    vtkTclParser.vtkTclParser.__init__(self)
    self.VTKObjects = []
    self.OutputFP = None
    self.Output = ""
    self.Indent = ""
    self.ProcList = []

  def printHeader(self):
    self.handle_command("""#!/usr/bin/env python""")
    pass
  
  def printFooter(self):
    self.handle_command("# --- end of script --")
    pass
    
  def reset(self):
    self.Output = ""
    self.Indent = ""
    self.VTKObjects = []
    vtkTclParser.vtkTclParser.reset(self)
    self.ProcList = []

  def GetBlockParser(self):
    p = vtkTclToPyConvertor()
    p.Indent = self.Indent + "  "
    p.ProcList = self.ProcList[:]
    return p

  def translate_operator(self, op):
    if op == "&&":
      return "and"
    if op == "||":
      return "or"
    return op

  def translate_token(self, token):
    """called to transate every token."""
    if token.find("$") == -1:
      return token
    match =  reVariable.match(token)
    if match:
      result = ""
      if match.group(1) != None:
        result += match.group(1)
      result += match.group(2)
      return result
    result = "locals()[get_variable_name(\""
    match =  reCompoundVariable.search(token)
    while match:
      result += token[:match.start(0)] + "\", "
      if match.group(1) != None:
        result += match.group(1)
      else:
        result += match.group(2)
      result += ", \""
      token = token[match.end(0):]
      match =  reCompoundVariable.search(token)
    result += token +"\")]"
    return result
    
  def translate_command(self, command, arguments):
    #self.Error("to translate_command %s %s" % (command, `arguments`))
    translated_cmd = None
   
    if len(command) > 0 and command[0] == "#":
      translated_cmd = command
    elif len(command) > 0 and command[0] == "\"":
      translated_cmd = command
    elif command == "global" and len(arguments) >= 1:
      translated_cmd = "global %s" % arguments[0]
      for arg in arguments[1:]:
        translated_cmd += ", %s" % arg
    elif command == "eval" and len(arguments) > 0:
      translated_cmd = self.translate_command(arguments[0], arguments[1:])
      pass
    elif command == "catch" and len(arguments) == 1:
      p = self.GetBlockParser() 
      p.Indent = ""
      p.feed(arguments[0])
      translated_cmd = "catch.catch(globals(),\"\"\"%s\"\"\")" % p.Output
    elif command == "expr":
      translated_cmd = "expr.expr(globals(), locals(),["
      i = False
      for arg in arguments:
        if i:
          translated_cmd += ","
        translated_cmd  += "\"%s\"" % arg
        i = True
      translated_cmd += "])"
    elif command == "lindex" and len(arguments) == 2:
      translated_cmd = "lindex(%s,%s)" % (arguments[0], arguments[1]) 
    elif command == "append" and len(arguments) >= 2:
      translated_cmd = "%s += %s" % (arguments[0], arguments[1])
      for arg in arguments[2:]:
        translated_cmd += " + %s" % arg
    elif command == "proc" and len(arguments) == 3:
      translated_cmd = "def %s (" % arguments[0]
      proc_args = arguments[1].split()
      i = False
      pair = 0
      for pa in proc_args:
        if pa.strip() == "{":
          pair = 1
          continue
        elif pa.strip() == "}":
          pair = 0
          continue
        if i and pair != 2:
          translated_cmd += ","
        if pair == 2:
          translated_cmd += "="
        if pair == 1:
          pair = 2
        i = True
        translated_cmd += pa
      translated_cmd +="):\n"
      p = self.GetBlockParser()
      p.feed(arguments[2])
      translated_cmd += p.Output
      self.ProcList.append(arguments[0])
    elif command == "set" and len(arguments) == 2:
      #translate a set command.
      translated_cmd = "%s = %s" % (arguments[0], arguments[1]) 
    elif command == "foreach" and len(arguments) == 3:
      p = self.GetBlockParser()
      p.feed(arguments[0])
      arguments[0] = p.Output.strip()
      p = self.GetBlockParser()
      p.feed(arguments[1])
      arguments[1] = p.Output.strip()
      p = self.GetBlockParser()
      p.Indent = self.Indent + "  "
      p.feed(arguments[2])
      translated_cmd = "for %s in %s.split():\n" % (arguments[0], arguments[1])
      translated_cmd += p.Output
      translated_cmd += "\n" + self.Indent + "  pass"
    elif command == "for" and len(arguments) == 4:
      p = self.GetBlockParser()
      p.feed(arguments[0])
      translated_cmd = p.Output.strip() + "\n"
      p = self.GetBlockParser()
      p.feed(arguments[1])
      translated_cmd += self.Indent + "while " + p.Output.strip() + ":\n"
      p = self.GetBlockParser()
      p.Indent = self.Indent + "  "
      p.feed(arguments[3])
      translated_cmd += p.Output
      p = self.GetBlockParser()
      p.Indent = self.Indent + "  "
      p.feed(arguments[2])
      translated_cmd += p.Output
    elif command == "while" and len(arguments) == 2:
      p = self.GetBlockParser()
      p.feed(arguments[0])
      translated_cmd = "while %s:\n" % p.Output.strip()
      p = self.GetBlockParser()
      p.Indent = self.Indent + "  "
      p.feed(arguments[1])
      translated_cmd += p.Output
      translated_cmd += "\n" + self.Indent + "  pass"
    elif command in ["if", "elseif"] and len(arguments) >= 2:
      p = self.GetBlockParser()
      p.Indent = self.Indent
      p.feed(arguments[0])
      if command == "if":
        translated_cmd  = "if (%s):\n" % p.Output.strip()
      else:
        translated_cmd  = "elif (%s):\n" % p.Output.strip()
      p = self.GetBlockParser()
      p.Indent  = self.Indent + "  "
      p.feed(arguments[1])
      translated_cmd += p.Output
      if len(arguments) > 2:
        translated_cmd += self.Indent + \
          self.translate_command(arguments[2], arguments[3:])
      translated_cmd += "\n" + self.Indent + "  pass"
    elif command=="else" and len(arguments)==1:
      translated_cmd ="else:\n"
      p = self.GetBlockParser()
      p.Indent = self.Indent + "  "
      p.feed(arguments[0])
      translated_cmd += p.Output
      translated_cmd += "\n" + self.Indent + "  pass"
    elif command == "return" and len(arguments) == 0:
      translated_cmd = "return" 
    elif command == "return" and len(arguments) == 1:
      translated_cmd = "return %s" % arguments[0]
    elif command == "open" and len(arguments) >= 1:
      translated_cmd = "open(%s" % arguments[0]
      for arg in arguments[1:]:
        translated_cmd += ", %s" % arg
      translated_cmd +=")"
    elif command == "close" and len(arguments) == 1:
      translated_cmd = "%s.close()" % arguments[0]
    elif command == "gets" and len(arguments) == 1:
      translated_cmd = "gets(%s)" % arguments[0]
    elif command == "gets" and len(arguments) == 2:
      translated_cmd = "gets(%s," % arguments[0]
      if len(arguments[1]) == 0 or arguments[1][0] not in ["\"","'"]:
        translated_cmd += "\"%s\"" % arguments[1]
      else:
        translated_cmd += "%s" % arguments[1]
      translated_cmd +=", globals())" 
    elif command == "incr" and len(arguments) == 1:
      translated_cmd = "%s = %s + 1" % (arguments[0], arguments[0])
    elif command == "incr" and len(arguments) == 2:
      translated_cmd = "%s = %s + %s" % (arguments[0], arguments[0], arguments[1])
    elif command == "info" and len(arguments) > 0:
      translated_cmd = "info.%s(globals(), locals(), "% arguments[0]
      i = False
      for arg in arguments[1:]:
        if i:
          translated_cmd +=","
        i = True
        translated_cmd += " %s" % arg
      translated_cmd +=")"
    elif command == "tcl_platform" and len(arguments) >= 3:
      new_cmd = "tcl_platform(\"" + arguments[1] + "\")"
      if len(arguments) > 3:
        return self.translate_command(new_cmd, arguments[3:])
      return translated_cmd
    elif command in vtkpython.__dict__.keys():
      #translate a VTK object create command.
      if len(arguments) == 1:
        self.VTKObjects.append(arguments[0])
        translated_cmd = "%s = vtkpython.%s()" % (arguments[0], command)
      else:
        self.Error("Invalid command: %s %s" % (command, `arguments`))
    elif command == "BuildBackdrop" and len(arguments) == 7:
      translated_cmd = "[base, back, left] = BuildBackdrop(%s" % arguments[0]
      for arg in arguments[1:]:
        translated_cmd += ", %s" % arg
      translated_cmd += ")"
    elif command in ["case1", "case2", "case3", "case4", "case5", "case6", "case7", 
      "case8", "case9", "case10", "case11", "case12", "case13", "case14"] and \
        len(arguments) == 3:
      translated_cmd = "%s(%s, %s, %s, caseLabel)" % (command, arguments[0], arguments[1], arguments[2])
    elif len(arguments) >= 2 and arguments[0][0] in ["<",">","=","!"]:
      translated_cmd = command 
      for arg in arguments:
        translated_cmd+=  " " + arg
    elif command == "file" and len(arguments) > 0:
      translated_cmd = "file.%s(" % arguments[0]
      i = False
      for arg in arguments[1:]:
        if i:
          translated_cmd +=", "
        i = True
        if arg.strip() == "-force":
          translated_cmd += "\"%s\"" % arg
        else:
          translated_cmd += "%s" % arg
      translated_cmd += ")"
    elif len(arguments) > 0 and arguments[0] == "UnRegister":
      translated_cmd = "%s.UnRegister(" % command
      if len(arguments) < 2 or len(arguments[1].strip()) == 0:
        translated_cmd += "None"
      else:
        translated_cmd += "%s" % arguments[1]
      translated_cmd += ")"
      pass
    elif command in self.ProcList:
      translated_cmd = "%s(" % command
      i = False
      for arg in arguments:
        if i:
          translated_cmd +=","
        i = True
        translated_cmd += arg
      translated_cmd +=")"
    elif command == "source":
      translated_cmd = "#skipping source"
      pass
    elif len(arguments) > 0:
      translated_cmd = ""
      if arguments[0].strip() == "Delete":
        translated_cmd = "#"
      if arguments[0].strip() == "Print" :
        translated_cmd = "#"
      #translate a  VTK object command invocation.
      translated_cmd += "%s.%s(" % (command, arguments[0])
      if len(arguments) > 9:
        translated_cmd += "["
      for i in range(1,len(arguments)):
        if i > 1:
          translated_cmd += ","
        if arguments[i] != "" :
          translated_cmd += arguments[i]
        else:
          translated_cmd += "None"

      if len(arguments) > 9:
        translated_cmd += "]"
      translated_cmd += ")"
    elif command and len(arguments) == 0:
      return command
    return translated_cmd
    pass

  def handle_command(self, translated_cmd):
    translated_cmd = self.Indent + translated_cmd + "\n"
    if self.OutputFP:
      self.OutputFP.write(translated_cmd)
    self.Output += translated_cmd
    pass

if __name__ == "__main__":
  input_file = None
  output_file = None
  
  for i in range(0, len(sys.argv)):
    if sys.argv[i] == "-i" and i < len(sys.argv)-1:
      input_file = sys.argv[i+1]
    if sys.argv[i] == "-o" and i < len(sys.argv)-1:
      output_file = sys.argv[i+1]
  if not input_file or not output_file:
    print "Usage: %s -i <input tcl test> <output tcl test>" % sys.argv[0]
    sys.exit(1)
  data = ""
  try:
    fp = file(input_file, "r")
    data = fp.read()
    fp.close()
  except:
    print "Failed to read file %s" % input_file
    sys.exit(1)
  try:
    ofp = file(output_file, "w")
  except:
    print "Failed to write file %s" % output_file
    sys.exit(1)
  p = vtkTclToPyConvertor()
  p.OutputFP = ofp
  p.printHeader()
  p.feed(data)
  p.printFooter()
  ofp.close()
  if p.success():
    sys.exit(0)
  print "Conversion failed!"
  sys.exit(1)
  
    
