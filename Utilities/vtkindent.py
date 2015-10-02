#!/usr/bin/env python
"""
This script takes "whitesmith" indented source files as input,
and re-indents the braces according to the "allman" style.

Written by David Gobbi on Sep 30, 2015.
"""

import sys
import re

def reindent(filename):
    """Reindent a file from whitesmith style to allman style"""

    # This first part of this function clears all strings and comments
    # where non-grammatical braces might be hiding.

    keychar = re.compile(r"""[/"']""")
    c_comment = re.compile(r"\/\*(\*(?!\/)|[^*])*\*\/")
    c_comment_start = re.compile(r"\/\*(\*(?!\/)|[^*])*$")
    c_comment_end = re.compile(r"^(\*(?!\/)|[^*])*\*\/")
    cpp_comment = re.compile(r"\/\/.*")
    string_literal = re.compile(r'"([^\\"]|\\.)*"')
    string_literal_start = re.compile(r'"([^\\"]|\\.)*\\$')
    string_literal_end = re.compile(r'^([^\\"]|\\.)*"')
    char_literal = re.compile(r"'([^\\']|\\.)*'")
    char_literal_start = re.compile(r"'([^\\']|\\.)*\\$")
    char_literal_end = re.compile(r"^([^\\']|\\.)*'")

    f = open(filename)
    lines = f.readlines()
    f.close()
    n = len(lines)
    newlines = []

    cont = None

    for i in range(n):
        line = lines[i].rstrip()
        if cont is not None:
            match = cont.match(line)
            if match:
                line = line[match.end():]
                cont = None
            else:
                if cont is c_comment_end:
                    newlines.append("")
                    continue
                else:
                    newlines.append('\\')
                    continue

        pos = 0
        while True:
            match = keychar.search(line, pos)
            if match is None:
                break
            pos = match.start()
            end = match.end()
            match = c_comment.match(line, pos)
            if match:
                line = line[0:pos] + " " + line[match.end():]
                pos += 1
                continue
            match = c_comment_start.match(line, pos)
            if match:
                if line[-1] == '\\':
                    line = line[0:pos] + ' \\'
                else:
                    line = line[0:pos]
                cont = c_comment_end
                break
            match = cpp_comment.match(line, pos)
            if match:
                if line[-1] == '\\':
                    line = line[0:pos] + ' \\'
                else:
                    line = line[0:pos]
                break
            match = string_literal.match(line, pos)
            if match:
                line = line[0:pos] + "\"\"" + line[match.end():]
                pos += 2
                continue
            match = string_literal_start.match(line, pos)
            if match:
                line = line[0:pos] + "\"\"\\"
                cont = string_literal_end
                break
            match = char_literal.match(line, pos)
            if match:
                line = line[0:pos] + "\' \'" + line[match.end():]
                pos += 3
                continue
            match = char_literal_start.match(line, pos)
            if match:
                line = line[0:pos] + "\' \'\\"
                cont = char_literal_end
                break
            pos += 1

        newlines.append(line.rstrip())

    # Use a stack to keep track of braces and, whenever a closing brace is
    # found, properly indent it and its opening brace.
    # For #if directives, check whether there are mismatched braces within
    # the conditional block, and if so, print a warning and reset the stack
    # to the depth that it had at the start of the block.

    # stack holds tuples (delim, row, col, newcol)
    stack = []
    lastdepth = 0

    # save the stack for conditional compilation blocks
    dstack = []

    directive = re.compile(r" *# *(..)")
    label = re.compile(r""" *(case  *)?(' '|""|[A-Za-z0-9_]| *:: *)+ *:$""")
    delims = re.compile(r"[{}()\[\]]")
    spaces = re.compile(r" *")
    cplusplus = re.compile(r" *# *ifdef  *__cplusplus")

    lastpos = 0
    newpos = 0
    continuation = False
    new_context = True
    in_else = False
    in_define = False
    in_assign = False
    leaving_define = False
    save_stack = None

    for i in range(n):
        line = newlines[i]
        pos = 0

        # restore stack when leaving #define
        if leaving_define:
            stack, lastpos, newpos, continuation = save_stack
            save_stack = None
            in_define = False
            leaving_define = False

        # handle #if conditionals
        is_directive = False
        in_else = False
        match = directive.match(line)
        if match:
            is_directive = True
            if match.groups()[0] == 'if':
                dstack.append((list(stack), line))
            elif match.groups()[0] in ('en', 'el'):
                oldstack, dline = dstack.pop()
                if len(stack) > len(oldstack) and not cplusplus.match(dline):
                    sys.stderr.write(filename + ":" + str(i) + ": ")
                    sys.stderr.write("mismatched delimiter in \"" +
                                     dline + "\" block\n")
                if match.groups()[0] == 'el':
                    in_else = True
                    stack = oldstack
                    dstack.append((list(stack), line))
            elif match.groups()[0] == 'de':
                in_define = True
                leaving_define = False
                save_stack = (stack, lastpos, newpos, continuation)
                stack = []
                new_context = True

        # remove backslash at end of line, if present
        if len(line) > 0 and line[-1] == '\\':
            line = line[0:-1].rstrip()
        elif in_define:
            leaving_define = True

        if not is_directive and len(line) > 0 and not continuation:
            # what is the indentation of the current line?
            match = spaces.match(line)
            newpos = match.end()

        # all statements end with ':', ';', '{', or '}'
        if len(line) > 0:
            if (new_context or line[-1] in ('{', '}', ';') or
                (label.match(line) and not continuation)):
                continuation = False
                new_context = False
            elif not is_directive:
                continuation = True

        # search for braces
        while True:
            match = delims.search(line, pos)
            if match is None:
                break
            pos = match.start()
            delim = line[pos]
            if delim in ('(', '['):
                # save delim, row, col, and current indentation
                stack.append((delim, i, pos, newpos))
            elif delim == '{':
                if in_assign:
                    # do not adjust braces for initializer lists
                    stack.append((delim, i, pos, pos))
                elif ((in_else or in_define) and spaces.sub("", line) == "{"):
                    # for opening braces that might have no match
                    indent = " "*lastpos
                    lines[i] = spaces.sub(indent, lines[i], count=1)
                    stack.append((delim, i, lastpos, lastpos))
                else:
                    # save delim, row, col, and previous indentation
                    stack.append((delim, i, pos, lastpos))
                newpos = pos + 2
                lastpos = newpos
            else:
                # found a ')', ']', or '}' delimiter, so pop its partner
                try:
                    ldelim, j, k, newpos = stack.pop()
                except IndexError:
                    ldelim = ""
                if ldelim != {'}':'{', ')':'(', ']':'['}[delim]:
                    sys.stderr.write(filename + ":" + str(i) + ": ")
                    sys.stderr.write("mismatched \'" + delim + "\'\n")
                # adjust the indentation of matching '{', '}'
                if (ldelim == '{' and delim == '}' and
                    spaces.sub("", lines[i][0:pos]) == "" and
                    spaces.sub("", lines[j][0:k]) == ""):
                    indent = " "*newpos
                    lines[i] = spaces.sub(indent, lines[i], count=1)
                    lines[j] = spaces.sub(indent, lines[j], count=1)
            pos += 1

        # check for " = " and #define assignments for the sake of
        # the { inializer list } that might be on the following line
        if len(line) > 0:
            if (line[-1] == '=' or
                (is_directive and in_define and not leaving_define)):
                in_assign = True
            elif not is_directive:
                in_assign = False

        lastpos = newpos

    if len(dstack) != 0:
        sys.stderr.write(filename + ": ")
        sys.stderr.write("mismatched #if conditional.\n")

    if len(stack) != 0:
        sys.stderr.write(filename + ":" + str(stack[0][1]) + ": ")
        sys.stderr.write("no match for " + stack[0][0] +
                         " before end of file.\n")

    ofile = open(filename, 'w')
    ofile.writelines(lines)
    ofile.close()


if __name__ == "__main__":

    for filename in sys.argv[1:]:
         reindent(filename)
