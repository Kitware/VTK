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
import string

for i in range(0, len(sys.argv)):
    if sys.argv[i] == '-A' and i < len(sys.argv)-1:
        sys.path = [sys.argv[i+1]] + sys.path

import vtkTclParser

reVariable = re.compile("^([+\-])?\$([^\$\{\}]+)$")
reCompoundVariable = re.compile("\$(?:([^\$\}\{]+)|\{([^\$\}]+)\})")

class vtkTclToPyConvertor(vtkTclParser.vtkTclParser):

    def __init__(self):
        vtkTclParser.vtkTclParser.__init__(self)
        self.output = ""
        self.indent = ""
        self._procedure_list = []
        self.class_list = []
        self.name_space = "vtk"

    def print_header(self, prefix_content=""):
        self.handle_command("""#!/usr/bin/env python
%s""" % (prefix_content))
        pass

    def print_footer(self):
        self.handle_command("# --- end of script --")
        pass

    def reset(self):
        self.output = ""
        self.indent = ""
        vtkTclParser.vtkTclParser.reset(self)
        self._procedure_list = []

    def _get_block_parser(self):
        p = vtkTclToPyConvertor()
        p.class_list = self.class_list
        p.name_space = self.name_space
        p.indent = self.indent + "    "
        p._procedure_list = self._procedure_list[:]
        return p

    def translate_block(self, block):
        p = self._get_block_parser()
        p.indent += "    "
        block = block.strip()
        if block[0] == "{":
            block = block[1:]
        if block[-1] == "}":
            block = block[:-1]
        p.feed(block)
        return p.output

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
        match =    reVariable.match(token)
        if match:
            result = ""
            if match.group(1) != None:
                result += match.group(1)
            result += match.group(2)
            return result
        result = "locals()[get_variable_name(\""
        match =    reCompoundVariable.search(token)
        while match:
            result += token[:match.start(0)] + "\", "
            if match.group(1) != None:
                result += match.group(1)
            else:
                result += match.group(2)
            result += ", \""
            token = token[match.end(0):]
            match =    reCompoundVariable.search(token)
        result += token +"\")]"
        return result

    def translate_command(self, command, arguments):
        #self._error("to translate_command %s %s" % (command, `arguments`))
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
            translated_cmd = "catch.catch(globals(),\"\"\"%s\"\"\")" % \
                              self.translate_block(arguments[0]).strip()
        elif command == "expr":
            translated_cmd = "expr.expr(globals(), locals(),["
            i = False
            for arg in arguments:
                if i:
                    translated_cmd += ","
                translated_cmd    += "\"%s\"" % arg
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
            # We add 2 default arguments to any procedure. This is necessary
            # since Tcl event handlers don't take any arguments while python
            # event handlers need 2 arguments.
            # Added 2 default arguments to ensure that such handler don't raise
            # errors. For all other procedures, adding two unused default arguments
            # makes no harm.
            proc_args.append("__vtk__temp0=0")
            proc_args.append("__vtk__temp1=0")
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
            p = self._get_block_parser()
            p.feed(arguments[2])
            translated_cmd += p.output
            self._procedure_list.append(arguments[0])
        elif command == "set" and len(arguments) == 2:
            #translate a set command.
            translated_cmd = "%s = %s" % (arguments[0], arguments[1])
        elif command == "foreach" and len(arguments) == 3:
            p = self._get_block_parser()
            p.feed(arguments[0])
            arguments[0] = p.output.strip()
            p = self._get_block_parser()
            p.feed(arguments[1])
            arguments[1] = p.output.strip()
            p = self._get_block_parser()
            p.indent = self.indent + "    "
            p.feed(arguments[2])
            translated_cmd = "for %s in %s.split():\n" % (arguments[0], arguments[1])
            translated_cmd += p.output
            translated_cmd += "\n" + self.indent + "    pass"
        elif command == "for" and len(arguments) == 4:
            p = self._get_block_parser()
            p.feed(arguments[0])
            translated_cmd = p.output.strip() + "\n"
            p = self._get_block_parser()
            p.feed(arguments[1])
            translated_cmd += self.indent + "while " + p.output.strip() + ":\n"
            p = self._get_block_parser()
            p.indent = self.indent + "    "
            p.feed(arguments[3])
            translated_cmd += p.output
            p = self._get_block_parser()
            p.indent = self.indent + "    "
            p.feed(arguments[2])
            translated_cmd += p.output
        elif command == "while" and len(arguments) == 2:
            p = self._get_block_parser()
            p.feed(arguments[0])
            translated_cmd = "while %s:\n" % p.output.strip()
            p = self._get_block_parser()
            p.indent = self.indent + "    "
            p.feed(arguments[1])
            translated_cmd += p.output
            translated_cmd += "\n" + self.indent + "    pass"
        elif command in ["if", "elseif"] and len(arguments) >= 2:
            p = self._get_block_parser()
            p.indent = self.indent
            p.feed(arguments[0])
            if command == "if":
                translated_cmd    = "if (%s):\n" % p.output.strip()
            else:
                translated_cmd    = "elif (%s):\n" % p.output.strip()
            p = self._get_block_parser()
            p.indent    = self.indent + "    "
            p.feed(arguments[1])
            translated_cmd += p.output
            if len(arguments) > 2:
                translated_cmd += self.indent + \
                    self.translate_command(arguments[2], arguments[3:])
            translated_cmd += self.indent + "    pass"
        elif command=="else" and len(arguments)==1:
            translated_cmd = "    pass\n"
            translated_cmd += self.indent + "else:\n"
            p = self._get_block_parser()
            p.indent = self.indent + "    "
            p.feed(arguments[0])
            translated_cmd += p.output
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
        elif command in self.class_list:
            #translate a VTK object create command.
            if len(arguments) == 1:
                translated_cmd = "%s = %s.%s()" % (arguments[0], self.name_space, command)
            else:
                self._error("Invalid command: %s %s" % (command, `arguments`))
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
                translated_cmd+=    " " + arg
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
            translated_cmd += ") # not needed in python"
            pass
        elif command in self._procedure_list:
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
        elif len(arguments) == 1 and arguments[0].strip() == "Delete":
            translated_cmd = "del %s" % command
        elif len(arguments) > 0:
            translated_cmd = ""
            if arguments[0].strip() == "Print" :
                translated_cmd = "#"
            #translate a    VTK object command invocation.
            translated_cmd += "%s.%s(" % (command, arguments[0])
            if len(arguments) > 9: #typically, methods taking more than 9 args in VTK take them as lists.
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
        translated_cmd = self.indent + translated_cmd + "\n"
        self.output += translated_cmd
        pass

if __name__ == "__main__":
    input_file = None
    output_file = None
    class_file = None
    prefix_file = None
    convert_file_list_file = None
    namespace = "vtk"
    touch_file = None
    kit_files_dir = ""
    for i in range(0, len(sys.argv)):
        if sys.argv[i] == "-i" and i < len(sys.argv)-1:
            input_file = sys.argv[i+1]
        if sys.argv[i] == "-o" and i < len(sys.argv)-1:
            output_file = sys.argv[i+1]
        if sys.argv[i] == "-f" and i < len(sys.argv)-1:
            class_file = sys.argv[i+1]
        if sys.argv[i] == "-n" and i < len(sys.argv)-1:
            namespace = sys.argv[i+1]
        if sys.argv[i] == "-p" and i < len(sys.argv)-1:
            prefix_file = sys.argv[i+1]
        if sys.argv[i] == "-l" and i < len(sys.argv)-1:
            convert_file_list_file = sys.argv[i+1]
        if sys.argv[i] == "-t" and i < len(sys.argv)-1:
            touch_file = sys.argv[i+1]
        if sys.argv[i] == "-k" and i < len(sys.argv)-1:
            kit_files_dir = sys.argv[i+1]


    if (not input_file or not output_file) and not convert_file_list_file:
        print "Usage: %s  [-o <output tcl test> -i <input tcl test>]"\
                "[-f <class name list>] [-n <namespace>] [-p <prefix file>]"\
                "[-l <semi-colon separated list of tcl tests to convert>]"\
                "[-t <file to touch on conversion complete>]"\
                "[-k <vtk*Kit.cmake file path>] [-k ...] ..." % sys.argv[0]
        print "Got Args: %s" % `sys.argv`
        sys.exit(1)
    class_list = []
    if class_file:
        try:
            fp = file(class_file, "r")
            new_class_list = fp.readlines()
            for a in new_class_list:
                class_list.append(string.strip(a))
            fp.close()
        except:
            print "Failed to read class list file %s" % class_file
            sys.exit(1)
    else:
        try:
            import vtkClassList
            class_list = vtkClassList.get_vtk_classes()
            pass
        except:
            print "ERROR: Failed to load module vtkClassList."
            sys.exit(1)

    if not class_list:
        print "Cannot find list of classes. Please provide -f <file> option"
        sys.exit(1)

    prefix_content = ""
    try:
        fp = file(prefix_file, "r")
        prefix_content = fp.read()
        fp.close()
    except:
        pass
    convert_file_list = []
    output_file_list = []
    if convert_file_list_file:
        try:
            fp = file(convert_file_list_file, "r")
            filename_list = fp.read().split(";")
            fp.close()
            for i in range(0, len(filename_list), 2):
                convert_file_list.append(filename_list[i])
                output_file_list.append(filename_list[i+1])
        except:
            print "Failed to read list of file to translate %s" % convert_file_list_file
            print "%s" % sys.exc_info()[1]
            sys.exit(1)


    if input_file:
        convert_file_list.append(input_file)
    if output_file:
        output_file_list.append(output_file)

    for i in range(0,len(convert_file_list)):
        data = ""
        ip_filename = convert_file_list[i].strip()
        op_filename = output_file_list[i].strip()
        try:
            print "Converting %s" % ip_filename
            fp = file(ip_filename, "r")
            data = fp.read()
            fp.close()
        except:
            print "Failed to read input file %s" % ip_filename
            sys.exit(1)

        p = vtkTclToPyConvertor()
        p.class_list = class_list
        p.name_space = namespace
        p.print_header(prefix_content)
        p.feed(data)
        p.print_footer()
        if p.success():
            try:
                ofp = file(op_filename, "w")
                ofp.write(p.output)
                ofp.close()
            except:
                print "Failed to write output file %s" % op_filename
                sys.exit(1)
        else:
            print "Conversion failed!"
            sys.exit(1)
    if touch_file:
        try:
            ofp = file(touch_file, "w")
            ofp.write("Done\n")
            ofp.close()
        except:
            print "Failed to touch file %s" % touch_file
            sys.exit(1)

    sys.exit(0)
