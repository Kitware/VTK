#!/usr/bin/env python
#
# tcl2py.py - try to Pythonize a vtk tcl script
#
# This code is based on tcl2py.py from VTK 3.2 which was written by
# Randy Heiland.
#
# Changes:
#
#  (21/12/2002) -- Prabhu Ramachandran
#  Made to work with re instead of regex.  Superficial cleanup of the
#  code, fixed a few issues, using new VTK package structure, added
#  support for VTK_DATA_ROOT and set keyword, primitive support for
#  expr, made code easier to run with usage message etc.  Seems to
#  generate decent output now.  Simple Tcl scripts work out of the
#  box!  Complex ones need work.  For a truly general solution the
#  code has to be rewritten.
#

import sys
import string
import re


def scanner(name,function):
    file = open(name,'r')
    while 1:
        line = file.readline()
        if not line: break
        function(line)
    file.close()


def processLine(line, output, keepTcl=1):
    line = string.strip(line) + "\n"
    if line[0] == '#':
        output.write(line)
    elif line[:15] == 'package require':
        if keepTcl == 1:
            output.write('# ' + line)
    elif string.find(line, "deiconify") > -1:
        output.write('# ' + line)        
    else:
        if keepTcl == 1:
            output.write('#' + line)

        if string.find(line, 'expr') > -1:
            match = re.search("\[\s*expr ([^\]]+)\]", line).groups()[0]
            match = re.sub('\s+', '', match)
            line = re.sub("\[\s*expr ([^\]]+)\]", match, line)
        line = re.sub('\[','',line)
        line = re.sub('\]','',line)
        line = re.sub('ren1','ren',line)
        line = re.sub(';','\n',line)
        line = re.sub('\$','',line)
        line = re.sub('{',' ',line)
        line = re.sub('}',' ',line)

        n = len(line)

        keys = string.split(line)

        # handle set keyword.
        inSet = 0
        if len(keys) and keys[0] == 'set':
            output.write(keys[1] + ' = ')
            keys = keys[2:]
            inSet = 1

        keysLength = len(keys)
            
        inaModule = 0
        inaForLoop = 0
        if keysLength == 0:
            output.write(line)
        # Catch some tcl-specific keywords and comment out
        elif keys[0] == 'proc':
            inaModule = 0
            output.write( 'def ' + keys[1] + '(')
            if len(keys) > 2:
                output.write(keys[2])
                for i in range(3,len(keys)):
                    output.write(', ' + keys[i])
            output.write('):\n')
        elif keys[0] == 'catch':
            output.write( '#' + line)
        elif keys[0] == 'source':
            output.write( '#' + line)
            if re.search("colors.tcl",line) > -1:
                output.write("from colors import *")
        elif keys[0] == 'wm':
            output.write( '#' + line)
        elif keysLength > 1 and keys[1] == 'SetUserMethod':
            if keepTcl == 1:
                output.write( '#' + line)
        elif keys[0] == 'for' and keys[1]=='set':
            inaForLoop = 1
            #print '...Handling for loop'
            output.write( "for " + keys[2] + " in range(" + keys[3] +", ")
            upper = keys[6]
            if keys[5] == "<=":
                output.write( upper + "+1):\n" )
            else:
                output.write( upper + "):\n" )

        # Detect vtk class instance; Pythonize it.
        elif line[:3] == 'vtk':
            output.write( keys[1] + ' = vtk.' + keys[0] + '()\n' )
        else:
            lparen = 0
            finishedFlag = 0
            # for i in range(len(keys)-1):
            for i in range(len(keys)):
                ls = len(keys[i])
                if keys[i] == 'eval':
                    continue
                # continuation mark
                elif keys[i] == '\\':
                    output.write( "  \\\n")
                elif keys[i][-8:] == 'FileName':
                    if keys[i+1][0:1] == '"':
                        f_name = re.sub('"VTK_DATA_ROOT/', \
                                        'VTK_DATA_ROOT + "/', keys[i+1])
                        output.write( keys[i] + "(" + f_name + ")")
                    else:
                        f_name = re.sub('VTK_DATA_ROOT/', \
                                        'VTK_DATA_ROOT + "/', keys[i+1])
                        if f_name[:13] == 'VTK_DATA_ROOT':
                            output.write(keys[i] + "(" + f_name + "\")")
                        else:
                            output.write( keys[i] + "(\"" + keys[i+1] + "\")")
                    finishedFlag = 1
                    break
                elif keys[i] == 'SetColor':
                    #print '...doing SetColor'
                    #print keys
                    if not re.search('[-\d.]', keys[i+1][0:1]):
                        #print '...got a named color'
                        color = keys[i+1][0:]
                        #print 'color = ' + color
                        output.write( "SetColor(" + color+"[0]," + \
                        color+"[1]," + color+"[2])" )
                    else:
                        output.write( "SetColor("+keys[i+1]+", "+keys[i+2]+", "+keys[i+3]+")")
                    finishedFlag = 1
                    break
                elif keys[i][:3]=='Set' or keys[i][:3]=='Add' or keys[i][:6]=='Insert':
                    output.write( keys[i] + '(' )
                    lparen = 1
                elif i < len(keys)-1 and \
                                re.search('[-\d.]', keys[i+1][0:1]) and \
                not re.search('[-\d.]', keys[i][ls-1:ls]):
                    output.write( keys[i] + '(' )
                    lparen = 1
                elif keys[i][:3] == 'Get':
                    output.write( keys[i] + '()' )
                    if i < len(keys)-1:
                        output.write( '.' )
                else:
                    if i < len(keys)-1:
                        npos = re.search("[-\d.]", keys[i][0:1])
                        if npos > -1 or keys[i][0:3] == 'VTK':
                            output.write( keys[i] + ', ' )
                        else:
                            output.write( keys[i] + '.' )
                    else:
                        if inaModule == 1:
                            output.write( '\t' )
                        output.write( keys[i] )

            if finishedFlag == 0:
                if keys[-1][:3] != 'Get' and \
                   (not  re.search("[-+\d.]", keys[-1])):
                    if lparen == 0:
                        output.write( '(' )
                        lparen = 1
            else:
                output.write( '\n' )

            # Terminating right paren.
            #output.write( ')\n' )

            if lparen == 1:
                output.write( ')\n' )
            if inSet == 1:
                output.write( '\n' )


def usage():
    msg = """Usage:\n  tcl2py.py script.tcl [keep_tcl_code]\n
This script attempts to convert a VTK-Tcl script to Python.
The converted code is printed on the standard output.

Options:

  keep_tcl_code -- If 1 it keeps the original tcl code
                   in a commented line.  If zero it does not print
                   the translated Tcl line.  Defaults to 1.

Example:

 $ ./tcl2py.py example.tcl 0 > example.py
"""
    print msg


def main():
    global keepTcl
    if len(sys.argv) < 2:
        usage()
        sys.exit(1)

    keepTcl = 1
    if len(sys.argv) > 2:
        try:
            keepTcl = int(sys.argv[2])
        except ValueError:
            usage()
            print "Second argument must be 0 or 1."
            sys.exit(1)

    name = sys.argv[1]
    output = sys.stdout
    if name[-4:] == '.tcl':
        input = open(name, 'r')
        #output = open(name[:-4] + '.py', 'w')
    else:
        input = open(name + '.tcl', 'r')
        #output = open(name + '.py', 'w')

    # Standard stuff; import useful things and handle VTK_DATA_ROOT.
    output.write('#!/usr/bin/env python\n')
    output.write('\nimport vtk\n')
    output.write('from vtk.util.misc import vtkGetDataRoot\n')
    output.write('VTK_DATA_ROOT = vtkGetDataRoot()\n\n')

    for line in input.readlines():
        processLine(line, output, keepTcl)
    input.close()

    output.write('iren.Initialize()\nrenWin.Render()\niren.Start()\n')
    output.close()


if __name__ == "__main__":
    main()
    
