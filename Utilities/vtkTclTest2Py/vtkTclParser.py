"""This parses the tcl script and calls virtual methods to translate the code
to target language."""


import re

# Some module variables used internally.
token = "[+\-]?[\$0-9A-Za-z_\.{}]+"
hexnumber = "0x[0-9abcdefABCDEF]+"
number = "[+-]?[0-9]+(?:(?:\.[0-9]+)?(?:e[+-][0-9]+)?)?"
number2 = "[+-]?\.[0-9]+(?:e[+-][0-9]+)?"
word_separator = "[ \t]"
command_separator = "[\n;]"
word = "%s*(%s|%s|%s|%s)"% (word_separator, hexnumber, number2, number, token)
reWord = re.compile("^%s" % word )
reNewWord = re.compile("^(?:%s|%s)*%s" % (command_separator, word_separator,
    word))
reCondition = re.compile("%s*([!<>=]+)" % word_separator)
reOperation = re.compile("%s*([|&<>=!+\-/%%*()]+)" % word_separator)
reStackBegin = re.compile("%s*(?<!\\\)\[" % word_separator)
reStackEnd = re.compile("%s*(?<!\\\)\]" % word_separator)
reNewStackBegin = re.compile("^(?:%s|%s)*(?<!\\\)\[" % (word_separator,
    command_separator))
reNewStackEnd = re.compile("^(?:%s|%s)*(?<!\\\)\]" % (word_separator,
    command_separator))
reString = re.compile("%s*(?<!\\\)\"" % word_separator)
reComment = re.compile("%s*(#[^\n]*)" % word_separator)
reNewComment = re.compile("(?:%s|%s)*(#[^\n]*)" % (word_separator,
    command_separator))
reVariable = re.compile("\$[0-9A-Za-z_]+")
reFormattingNewLine = re.compile("[\\\][\n]")
reIgnore1 = re.compile("%s*package%s[^\n]*" % (command_separator,
    word_separator))
reIgnore2 = re.compile(\
    "(%s|%s)*iren AddObserver UserEvent {wm deiconify .vtkInteract}" \
    % (command_separator, word_separator))
reIgnore3 = re.compile("(%s|%s)*wm%s[^\n]*" % (command_separator,
    word_separator, word_separator))
reIgnore4 = re.compile("(%s|%s)*update" % (command_separator,word_separator))
reBlockBegin = re.compile("(?:%s|%s)*\{" % (word_separator, command_separator))
reBlockEnd = re.compile("(?:%s|%s)*\}" % (word_separator, command_separator))
reStringText = re.compile("(?<!\\\)[^\"\]\[\$]+")


class vtkTclParser:

    def __init__(self):
        self._instring = ""
        self._command_stack = []
        self._in_process_string = False
        pass

    def success(self):
        """Indicates if the most recent feed was translated in its entirity"""
        if len(self._instring.strip()) == 0:
            return True
        return False

    def reset(self):
        """resets internal state. All translated text gets discarded"""
        self._in_process_string = False
        self._instring = ""
        self._command_stack = []
        pass

    def _error(self, str):
        """prints error"""
        print str

    def feed(self, str):
        """translates the argument"""
        str = re.sub(reFormattingNewLine," ",str)
        self._instring += str
        cmd = self._process_command()
        while cmd:
            self.handle_command(cmd)
            cmd = self._process_command()
        return

    def _process_command(self):
        """"Internal method: Processes a single line in tcl script"""
        cmd = self._get_first_word_in_new_command()
        if not cmd:
            return None
        arguments = []
        arg = self._get_next_word_in_command()
        while arg != None:
            arguments.append(arg)
            arg = self._get_next_word_in_command()
        translated = self.translate_command(cmd, arguments)
        return translated

    def _process_string(self):
        """Internal method: process an string enclosed in quotes"""
        if self._in_process_string:
            return None
        self._in_process_string = True
        result = "\""
        arg = self._get_next_word_in_command(True)
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
            arg = self._get_next_word_in_command(True)
        result += "\""
        self._in_process_string = False
        return result

    def translate_command(self, command, arguments):
        """called to translate a command"""
        pass

    def translate_operator(self, op):
        """called to translate an operator"""
        pass


    def handle_command(self, translated_cmd):
        """called to handle a translated command"""
        pass

    def translate_token(self, token):
        """called to translate a token"""
        pass

    def _ignore(self):
        """Internal method: checks for ignored command"""
        match = reIgnore1.match(self._instring)
        if not match:
          match = reIgnore2.match(self._instring)
        if not match:
          match = reIgnore3.match(self._instring)
        if not match:
          match = reIgnore4.match(self._instring)
        if match:
          self._instring = self._instring[match.end(0):]
          return True
        return False

    def _get_next_word_in_command(self, processing_string= False):
        """Internal method: returns next token in current command, if any"""
        if processing_string:
          match = reStringText.match(self._instring)
          if match:
            self._instring = self._instring[match.end(0):]
            return match.group(0)
        match = reBlockBegin.match(self._instring)
        if match:
          #return "{" + self._get_block() + "}"
          return self._get_block()

        match = reWord.match(self._instring)
        if match:
          word = match.group(1)
          self._instring = self._instring[match.end(0):]
          word = self.translate_token(word)
          if processing_string:
            word = "[" + word + "]"
          return word
        match = reStackBegin.match(self._instring)
        if match:
          self._instring = self._instring[match.end(0):]
          if not processing_string:
            return self._process_command()
          if processing_string:
            return "[" + self._process_command() + "]"
        match = reStackEnd.match(self._instring)
        if match:
          self._instring = self._instring[match.end(0):]
          return None
        match = reString.match(self._instring)
        if match:
          self._instring = self._instring[match.end(0):]
          if not self._in_process_string:
            return self._process_string()
          else:
            return None
        match = reOperation.match(self._instring)
        if match:
          self._instring = self._instring[match.end(0):]
          return self.translate_operator(match.group(1))
        return None

    def _get_first_word_in_new_command(self):
        """Internal method: returns start token in a command, if any"""
        while self._ignore():
          pass
        match = reNewWord.match(self._instring)
        if match:
          word = match.group(1)
          self._instring = self._instring[match.end(0):]
          word = self.translate_token(word)
          return word
        match = reNewStackBegin.match(self._instring)
        if match:
          self._instring = self._instring[match.end(0):]
          return self._process_command()
        match = reNewStackEnd.match(self._instring)
        if match:
          self._instring = self._instring[match.end(0):]
          return None
        match = reNewComment.match(self._instring)
        if match:
          self._instring = self._instring[match.end(0):]
          return match.group(1)
        match = reString.match(self._instring)
        if match:
          self._instring = self._instring[match.end(0):]
          if not self._in_process_string:
            return self._process_string()
          else:
            return None
        return None

    def _get_block(self):
        """Internal method: returns everything between { and }"""
        brace_count = 0
        start_index = 0
        end_index = len(self._instring)
        for i in range(0, len(self._instring)):
          if not self._instring[i] in ["{","}"]:
            continue
          if self._instring[i] == "}":
            brace_count -= 1
            if brace_count == 0:
              block = self._instring[start_index:i]
              self._instring = self._instring[i+1:]
              return block
            continue
          if self._instring[i] == "{":
            brace_count += 1
            if brace_count== 1:
              start_index = i+1
            continue
        self._error("Unterminated block!")
        return None

