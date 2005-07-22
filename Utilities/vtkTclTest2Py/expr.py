"""Evalues a tcl expr expression."""
import math

def expr(caller_globals_dict, caller_localvars_dict, arguments): 
    """the arguments to expr in tcl are passed as individual arguments to this 
    method.
    eg [expr sin(20)*$a] becomes expr.expr("sin","(","20",")","*", a)"""
    eval_str = "eval(\"\"\"" 
    for arg in arguments:
        if arg in caller_localvars_dict.keys():
            #arg is a local variable. First, get its value
            arg = caller_localvars_dict[arg]
        if type(arg) == type("string"):
            #this will obtain value in the variable.
            if arg in dir(math):
                eval_str += "math.%s" % arg
            else:
                if arg == "(":
                    eval_str +=    arg 
                else:    
                    if arg.strip() == "double":
                        arg = "float" #tcl double is python float
                    eval_str +=    " " + arg 
            continue
        eval_str += " " + `arg`
    eval_str += "\"\"\")"
    #print "eval_str : %s" % eval_str
    return eval(eval_str, caller_globals_dict, caller_localvars_dict)

