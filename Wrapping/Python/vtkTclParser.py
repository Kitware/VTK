
import re

#based on HTMLParser.
token = "[+\-]?[\$0-9A-Za-z_\.{}]+"
hexnumber = "0x[0-9abcdefABCDEF]+"
number = "[+-]?[0-9]+(?:(?:\.[0-9]+)?(?:e[+-][0-9]+)?)?"
number2 = "[+-]?\.[0-9]+(?:e[+-][0-9]+)?"
word_separator = "[ \t]"
command_separator = "[\n;]"

word = "%s*(%s|%s|%s|%s)"% (word_separator, hexnumber, number2, number, token)
reWord = re.compile("^%s" % word )
reNewWord = re.compile("^(?:%s|%s)*%s" % (command_separator, word_separator, word))
reCondition = re.compile("%s*([!<>=]+)" % word_separator)
reOperation = re.compile("%s*([|&<>=!+\-/%%*()]+)" % word_separator)

reStackBegin = re.compile("%s*(?<!\\\)\[" % word_separator)
reStackEnd = re.compile("%s*(?<!\\\)\]" % word_separator)

reNewStackBegin = re.compile("^(?:%s|%s)*(?<!\\\)\[" % (word_separator, command_separator))
reNewStackEnd = re.compile("^(?:%s|%s)*(?<!\\\)\]" % (word_separator, command_separator))

reString = re.compile("%s*(?<!\\\)\"" % word_separator)

reComment = re.compile("%s*(#[^\n]*)" % word_separator)
reNewComment = re.compile("(?:%s|%s)*(#[^\n]*)" % (word_separator, command_separator))

reVariable = re.compile("\$[0-9A-Za-z_]+")
reFormattingNewLine = re.compile("[\\\][\n]")

reIgnore1 = re.compile("%s*package%s[^\n]*" % (command_separator, word_separator))
reIgnore2 = re.compile("(%s|%s)*iren AddObserver UserEvent {wm deiconify .vtkInteract}" % (command_separator, word_separator))
reIgnore3 = re.compile("(%s|%s)*wm%s[^\n]*" % (command_separator, word_separator, word_separator))
reIgnore4 = re.compile("(%s|%s)*update" % (command_separator,word_separator))

reBlockBegin = re.compile("(?:%s|%s)*\{" % (word_separator, command_separator))
reBlockEnd = re.compile("(?:%s|%s)*\}" % (word_separator, command_separator))

reStringText = re.compile("(?<!\\\)[^\"\]\[\$]+")
class vtkTclParser:
  def __init__(self):
    self.InString = ""
    self.CommandStack = []
    self.InProcessString = False
    pass

  def success(self):
    if len(self.InString.strip()) == 0:
      return True
    return False

  def reset(self):
    self.InProcessString = False
    self.InString = ""
    self.CommandStack = []
    pass

  def close(self):
    pass
    
  def Error(self, str):
    print str

  def feed(self, str):
    str = re.sub(reFormattingNewLine," ",str)
    self.InString += str
    cmd = self.ProcessCommand()
    while cmd:
      self.handle_command(cmd)
      cmd = self.ProcessCommand()
    return 

  def ProcessCommand(self):
    cmd = self.GetFirstWordInNewCommand()
    if not cmd:
      return None
    arguments = []
    arg = self.GetNextWordInCommand()
    while arg != None:
      arguments.append(arg)
      arg = self.GetNextWordInCommand()
    translated = self.translate_command(cmd, arguments)
    return translated

  def ProcessString(self):
    if self.InProcessString:
      return None
    self.InProcessString = True
    result = "\""
    arg = self.GetNextWordInCommand(True)
  
    while arg:
      if (arg[0] == "["):
        arg = arg[1:len(arg)-1]
        result += "\" + str(" + arg + ") + \""
      else:
        m = reVariable.search(arg)
        while m:
          result += arg[:m.start(0)] + "\" + "
          result += arg[m.start(0)+1:m.end(0)] + " + \""
          arg = arg[m.end():]
          m = reVariable.search(arg)
        result += arg
      arg = self.GetNextWordInCommand(True)
    result += "\""
    self.InProcessString = False
    return result

    
  def translate_command(self, command, arguments):
    pass

  def translate_operator(self, op):
    pass


  def handle_command(self, translated_cmd):
    pass

  def translate_token(self, token):
    pass

  def Ignore(self):
    match = reIgnore1.match(self.InString)
    if not match:
      match = reIgnore2.match(self.InString)
    if not match:
      match = reIgnore3.match(self.InString)
    if not match:
      match = reIgnore4.match(self.InString)
    if match:
      self.InString = self.InString[match.end(0):]
      return True
    return False
        
      
  def GetNextWordInCommand(self, processing_string= False):
    if processing_string:
      match = reStringText.match(self.InString)
      if match:
        self.InString = self.InString[match.end(0):]
        return match.group(0)
    match = reBlockBegin.match(self.InString)
    if match:
      return self.GetBlock()

    match = reWord.match(self.InString)
    if match:
      word = match.group(1)
      self.InString = self.InString[match.end(0):]
      word = self.translate_token(word)
      if processing_string:
        word = "[" + word + "]"
      return word
    match = reStackBegin.match(self.InString)
    if match:
      self.InString = self.InString[match.end(0):]
      if not processing_string:
        return self.ProcessCommand()
      if processing_string:
        return "[" + self.ProcessCommand() + "]"
    match = reStackEnd.match(self.InString)
    if match:
      self.InString = self.InString[match.end(0):]
      return None
    match = reString.match(self.InString)
    if match:
      self.InString = self.InString[match.end(0):]
      if not self.InProcessString:
        return self.ProcessString()
      else:
        return None
    match = reOperation.match(self.InString)
    if match:
      self.InString = self.InString[match.end(0):]
      return self.translate_operator(match.group(1))
    return None

  def GetFirstWordInNewCommand(self):
    while self.Ignore():
      pass
    match = reNewWord.match(self.InString)
    if match:
      word = match.group(1)
      self.InString = self.InString[match.end(0):]
      word = self.translate_token(word)
      return word
    match = reNewStackBegin.match(self.InString)
    if match:
      self.InString = self.InString[match.end(0):]
      return self.ProcessCommand()
    match = reNewStackEnd.match(self.InString)
    if match:
      self.InString = self.InString[match.end(0):]
      return None
    match = reNewComment.match(self.InString)
    if match:
      self.InString = self.InString[match.end(0):]
      return match.group(1)
    match = reString.match(self.InString)
    if match:
      self.InString = self.InString[match.end(0):]
      if not self.InProcessString:
        return self.ProcessString()
      else:
        return None
    return None

  def GetBlock(self):
    brace_count = 0
    start_index = 0
    end_index = len(self.InString)
    for i in range(0, len(self.InString)):
      if not self.InString[i] in ["{","}"]:
        continue
      if self.InString[i] == "}":
        brace_count -= 1
        if brace_count == 0:
          block = self.InString[start_index:i]
          self.InString = self.InString[i+1:]
          return block
        continue
      if self.InString[i] == "{":
        brace_count += 1
        if brace_count== 1:
          start_index = i+1
        continue
    self.Error("Unterminated block!")
    return None

      
      
      
    
    
    
  
