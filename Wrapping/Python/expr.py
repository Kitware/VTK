import math
operators = ";**;~;*;/;%;+;-;<<;>>;&;^;|;<;<=;>;>=;!=;==;(;"

def expr(caller_globals_dict, caller_localvars_dict, arguments): 
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
          eval_str +=  arg 
        else:  
          if arg.strip() == "double":
            arg = "float" #tcl double is python float
          eval_str +=  " " + arg 
      continue
    eval_str += " " + `arg`
  eval_str += "\"\"\")"
  #print "eval_str : %s" % eval_str
  return eval(eval_str, caller_globals_dict, caller_localvars_dict)
    

# following is old, crappy expr code! Here only if I later 
# regret calling it crappy :).
  

def expr1(caller_localvars_dict, arguments):
  arguments = in_order_2_post_order(arguments)
  #print `arguments`
  my_locals = locals()
  for key in caller_localvars_dict.keys():
    if my_locals.has_key(key):
        continue
    my_locals[key] = caller_localvars_dict[key]
  stack = []
  for arg in arguments:
    #print "arg : %s" % `arg`
    if operators.find(";" + arg + ";") == -1:
      stack.append(arg)
      continue
    length = len(stack)
    if length == 0:
      raise "Invalid operation stack"
    op2 = stack[length-1]
    if caller_localvars_dict.has_key(op2):
      op2 = caller_localvars_dict[op2]
    if type(op2) == type("str"):
      op2 = eval(op2)
    op1 = None
    stack = stack[:length-1]
    #print `stack`
    if length >= 2:
      op1 = stack[length-2]
      if caller_localvars_dict.has_key(op1):
        op1 = caller_localvars_dict[op1]
      if type(op1) == type("str"):
        op1 = eval(op1)
      stack = stack[:length-2]
      #print `stack`
    eval_str = ""
    #print "operands : %s %s" % (`op1`, `op2`)
    if op1 == None:
      #single operand. no type conversion possible
      eval_str += " %s%s " % (arg, op2)
    elif type(op2) == type("string") and type(op1) != type("string"):
      eval_str += " %s %s type(eval(\"%s\"))(%s) " % (op1, arg, op1, `op2`)
    elif type(op2) != type("string") and type(op1) == type("string"):
      eval_str += " type(eval(\"%s\"))(eval(\"%s\")) %s %s " % (op2, `op1`, arg, op2)
    else:
      eval_str += " %s %s %s " % (op1, arg, op2)
    #print "eval_str %s" % eval_str
    val = eval(eval_str)
    stack.append(val)
  return stack[0] 

def in_order_2_post_order(arguments):
  index = 0
  stack = []
  opstack = []
  for arg in arguments:
    if arg == "(":
      opstack.append(arg)
    elif arg == ")":
      for i in range(len(opstack)-1,-1,-1):
        top = opstack[i]
        opstack = opstack[:i]
        if top == "(":
          break
        stack.append(top)
    else :
      op = ";" + arg + ";"
      index = operators.find(op)
      if index == -1:
        stack.append(arg)
        continue
      #arg is an operator. check the priority with top of stack.
      while True:
        if len(opstack)==0:
          break 
        top = opstack[len(opstack)-1]
        if operators.find(";" + top + ";") >= index:
          break
        stack.append(top)
        opstack = opstack[:len(opstack) -1]
      opstack.append(arg)
  return stack + opstack
