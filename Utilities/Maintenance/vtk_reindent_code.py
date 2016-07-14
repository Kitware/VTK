#!/usr/bin/env python
"""
Usage: python vtk_reindent_code.py [--test] <file1> [<file2> ...]

This script takes old-style "Whitesmiths" indented VTK source files as
input, and re-indents the braces according to the new VTK style.
Only the brace indentation is modified.

If called with the --test option, then it will print an error message
for each file that it would modify, but it will not actually modify the
files.

Written by David Gobbi on Sep 30, 2015.
"""

import sys
import os
import re

def reindent(filename, dry_run=False):
    """Reindent a file from Whitesmiths style to Allman style"""

    # The first part of this function clears all strings and comments
    # where non-grammatical braces might be hiding.  These changes will
    # not be saved back to the file, they just simplify the parsing.

    # look for ', ", /*, and //
    keychar = re.compile(r"""[/"']""")
    # comments of the form /* */
    c_comment = re.compile(r"\/\*(\*(?!\/)|[^*])*\*\/")
    c_comment_start = re.compile(r"\/\*(\*(?!\/)|[^*])*$")
    c_comment_end = re.compile(r"^(\*(?!\/)|[^*])*\*\/")
    # comments of the form //
    cpp_comment = re.compile(r"\/\/.*")
    # string literals ""
    string_literal = re.compile(r'"([^\\"]|\\.)*"')
    string_literal_start = re.compile(r'"([^\\"]|\\.)*\\$')
    string_literal_end = re.compile(r'^([^\\"]|\\.)*"')
    # character literals ''
    char_literal = re.compile(r"'([^\\']|\\.)*'")
    char_literal_start = re.compile(r"'([^\\']|\\.)*\\$")
    char_literal_end = re.compile(r"^([^\\']|\\.)*'")

    # read the file
    try:
        f = open(filename)
        lines = f.readlines()
        f.close()
    except:
        sys.stderr.write(filename + ": ")
        sys.stderr.write(str(sys.exc_info()[1]) + "\n")
        sys.exit(1)

    # convert strings to "", char constants to '', and remove comments
    n = len(lines) # 'lines' is the input
    newlines = []  # 'newlines' is the output

    cont = None    # set if e.g. we found /* and we are looking for */

    for i in range(n):
        line = lines[i].rstrip()

        if cont is not None:
            # look for closing ' or " or */
            match = cont.match(line)
            if match:
                # found closing ' or " or */
                line = line[match.end():]
                cont = None
            else:
                # this whole line is in the middle of a string or comment
                if cont is c_comment_end:
                    # still looking for */, clear the whole line
                    newlines.append("")
                    continue
                else:
                    # still looking for ' or ", set line to backslash
                    newlines.append('\\')
                    continue

        # start at column 0 and search for ', ", /*, or //
        pos = 0
        while True:
            match = keychar.search(line, pos)
            if match is None:
                break
            pos = match.start()
            end = match.end()
            # was the match /* ... */ ?
            match = c_comment.match(line, pos)
            if match:
                line = line[0:pos] + " " + line[match.end():]
                pos += 1
                continue
            # does the line have /* ... without the */ ?
            match = c_comment_start.match(line, pos)
            if match:
                if line[-1] == '\\':
                    line = line[0:pos] + ' \\'
                else:
                    line = line[0:pos]
                cont = c_comment_end
                break
            # does the line have // ?
            match = cpp_comment.match(line, pos)
            if match:
                if line[-1] == '\\':
                    line = line[0:pos] + ' \\'
                else:
                    line = line[0:pos]
                break
            # did we find "..." ?
            match = string_literal.match(line, pos)
            if match:
                line = line[0:pos] + "\"\"" + line[match.end():]
                pos += 2
                continue
            # did we find "... without the final " ?
            match = string_literal_start.match(line, pos)
            if match:
                line = line[0:pos] + "\"\"\\"
                cont = string_literal_end
                break
            # did we find '...' ?
            match = char_literal.match(line, pos)
            if match:
                line = line[0:pos] + "\' \'" + line[match.end():]
                pos += 3
                continue
            # did we find '... without the final ' ?
            match = char_literal_start.match(line, pos)
            if match:
                line = line[0:pos] + "\' \'\\"
                cont = char_literal_end
                break
            # if we got to here, we found / that wasn't /* or //
            pos += 1

        # strip any trailing whitespace!
        newlines.append(line.rstrip())

    # The second part of this function looks for braces in the simplified
    # code that we wrote to "newlines" after removing the contents of all
    # string literals, character literals, and comments.

    # Whenever we encounter an opening brace, we push its position onto a
    # stack.  Whenever we encounter the matching closing brace, we indent
    # the braces as a pair.

    # For #if directives, we check whether there are mismatched braces
    # within the conditional block, and if so, we print a warning and reset
    # the stack to the depth that it had at the start of the block.

    # For #define directives, we save the stack and then restart counting
    # braces until the end of the #define.  Then we restore the stack.

    # all changes go through this function
    lines_changed = {} # keeps track of each line that was changed
    def changeline(i, newtext, lines_changed=lines_changed):
         if newtext != lines[i]:
              lines[i] = newtext
              lines_changed[i] = newtext

    # we push a tuple (delim, row, col, newcol) onto this stack whenever
    # we find a {, (, or [ delimiter, this keeps track of where we found
    # the delimeter and what column we want to move it to
    stack = []
    lastdepth = 0

    # this is a superstack that allows us to save the entire stack when we
    # enter into an #if conditional block
    dstack = []

    # these are syntactic elements we need to look for
    directive = re.compile(r"\s*#\s*(..)")
    label = re.compile(r"""(case(?!\w)([^:]|::)+|\w+\s*(::\s*)*\s*:(?!:))""")
    cflow = re.compile(r"(if|else|for|do|while|switch)(?!\w)")
    delims = re.compile(r"[{}()\[\];]")
    spaces = re.compile(r"\s*")
    other = re.compile(r"(\w+|[^{}()\[\];\w\s]+)\s*")
    cplusplus = re.compile(r"\s*#\s*ifdef\s+__cplusplus")

    indentation = 0        # current indentation column
    continuation = False   # true if line continues an unfinished statement
    new_context = True     # also set when we enter a #define statement
    in_else = False        # set if in an #else
    in_define = False      # set if in #define
    in_assign = False      # set to deal with "= {" or #define x {"
    leaving_define = False # set if at the end of a #define
    save_stack = None      # save stack when entering a #define

    for i in range(n):
        line = newlines[i]

        # restore stack when leaving #define
        if leaving_define:
            stack, indentation, continuation = save_stack
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
                dstack.append((list(stack), indentation, continuation,
                               line))
            elif match.groups()[0] in ('en', 'el'):
                oldstack, oldindent, oldcont, dline = dstack.pop()
                if len(stack) > len(oldstack) and not cplusplus.match(dline):
                    sys.stderr.write(filename + ":" + str(i) + ": ")
                    sys.stderr.write("mismatched delimiter in \"" +
                                     dline + "\" block\n")
                if match.groups()[0] == 'el':
                    in_else = True
                    indentation = oldindent
                    continuation = oldcont
                    stack = oldstack
                    dstack.append((list(stack), indentation, continuation,
                                  line))
            elif match.groups()[0] == 'de':
                in_define = True
                leaving_define = False
                save_stack = (stack, indentation, continuation)
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
            if not line[match.end()] == '{':
                indentation = match.end()
                continuation = True

        # new_context marks beginning of a file or a macro
        if new_context:
            continuation = False
            indentation = 0
            new_context = False

        # skip initial whitespace
        if is_directive:
            pos = directive.match(line).end()
        else:
            pos = spaces.match(line).end()

        # check for a label e.g. case
        match = label.match(line, pos)
        if match:
            base = True
            for item in stack:
                if item[0] != '{':
                    base = False
            if base:
                word = re.match(r"\w*", match.group())
                if word in ("case", "default"):
                    indentation = pos
                continuation = False
                # check for multiple labels on the same line
                while match:
                    pos = spaces.match(line, match.end()).end()
                    match = label.match(line, pos)

        # parse the line
        while pos != len(line):
            # check for if, else, for, while, do, switch
            match = cflow.match(line, pos)
            if match:
                # if we are at the beginning of the line
                if spaces.match(line).end() == pos:
                    indentation = pos
                pos = spaces.match(line, match.end()).end()
                continue

            # check for a delimiter {} () [] or ;
            match = delims.match(line, pos)
            if not match:
                # check for any other identifiers, operators
                match = other.match(line, pos)
                if match:
                    pos = match.end()
                    continue
                else:
                    break

            # found a delimiter
            delim = line[pos]

            if delim in ('(', '['):
                # save delim, row, col, and current indentation
                stack.append((delim, i, pos, indentation))
            elif delim == '{':
                if in_assign or line[0:pos-1].rstrip()[-1:] == "=":
                    # do not adjust braces for initializer lists
                    stack.append((delim, i, -1, indentation))
                elif ((in_else or in_define) and spaces.sub("", line) == "{"):
                    # for opening braces that might have no match
                    indent = " "*indentation
                    changeline(i, spaces.sub(indent, lines[i], count=1))
                    stack.append((delim, i, pos, indentation))
                else:
                    # save delim, row, col, and previous indentation
                    stack.append((delim, i, pos, indentation))
                if spaces.sub("", newlines[i][0:pos]) == "":
                    indentation += 2
                continuation = False
            elif delim == ';':
                # ';' marks end of statement unless inside for (;;)
                if len(stack) == 0 or stack[-1][0] == '{':
                    continuation = False
            else:
                # found a ')', ']', or '}' delimiter, so pop its partner
                try:
                    ldelim, j, k, indentation = stack.pop()
                    in_assign = (k < 0)
                except IndexError:
                    ldelim = ""
                if ldelim != {'}':'{', ')':'(', ']':'['}[delim]:
                    sys.stderr.write(filename + ":" + str(i) + ": ")
                    sys.stderr.write("mismatched \'" + delim + "\'\n")
                # adjust the indentation of matching '{', '}'
                if (ldelim == '{' and delim == '}' and not in_assign and
                      spaces.sub("", line[0:pos]) == ""):
                    if spaces.sub("", newlines[j][0:k]) == "":
                        indent = " "*indentation
                        changeline(j, spaces.sub(indent, lines[j], count=1))
                        changeline(i, spaces.sub(indent, lines[i], count=1))
                    elif i != j:
                        indent = " "*indentation
                        changeline(i, spaces.sub(indent, lines[i], count=1))
                if delim == '}':
                    continuation = False

            # eat whitespace and continue
            pos = spaces.match(line, match.end()).end()

        # check for " = " and #define assignments for the sake of
        # the { inializer list } that might be on the following line
        if len(line) > 0:
            if (line[-1] == '=' or
                (is_directive and in_define and not leaving_define)):
                in_assign = True
            elif not is_directive:
                in_assign = False

    if len(dstack) != 0:
        sys.stderr.write(filename + ": ")
        sys.stderr.write("mismatched #if conditional.\n")

    if len(stack) != 0:
        sys.stderr.write(filename + ":" + str(stack[0][1]) + ": ")
        sys.stderr.write("no match for " + stack[0][0] +
                         " before end of file.\n")

    if lines_changed:
        # remove any trailing whitespace
        trailing = re.compile(r" *$")
        for i in range(n):
            lines[i] = trailing.sub("", lines[i])
        while n > 0 and lines[n-1].rstrip() == "":
            n -= 1
        if dry_run:
            errcount = len(lines_changed)
            line_numbers = list(lines_changed.keys())
            line_numbers.sort()
            line_numbers = [str(l + 1) for l in line_numbers[0:10] ]
            if errcount > len(line_numbers):
                line_numbers.append("...")
            sys.stderr.write("Warning: " + filename +
                             ": incorrect brace indentation on " +
                             str(errcount) +
                             (" lines: ", "line: ")[errcount == 1] +
                             ", ".join(line_numbers) + "\n")
        else:
            # rewrite the file
            ofile = open(filename, 'w')
            ofile.writelines(lines)
            ofile.close()
        return True

    return False


if __name__ == "__main__":

    # ignore generated files
    ignorefiles = ["lex.yy.c", "vtkParse.tab.c"]

    files = []
    opt_ignore = False # ignore all further options
    opt_test = False # the --test option

    for arg in sys.argv[1:]:
        if arg[0:1] == '-' and not opt_ignore:
            if arg == '--':
                opt_ignore = True
            elif arg == '--test':
                opt_test = True
            else:
                sys.stderr.write("%s: unrecognized option %s\n" %
                                 (os.path.split(sys.argv[0])[-1], arg))
                sys.exit(1)
        elif os.path.split(arg)[-1] not in ignorefiles:
            files.append(arg)

    # if --test was set, whenever a file needs modification, we set
    # "failed" and continue checking the rest of the files
    failed = False

    for filename in files:
        # repeat until no further changes occur
        while reindent(filename, dry_run=opt_test):
            if opt_test:
                failed = True
                break

    if failed:
        sys.exit(1)
