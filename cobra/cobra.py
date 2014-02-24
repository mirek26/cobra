#!/usr/bin/python
import os
import sys
import tempfile
import subprocess
F = None

def VARIABLE(s):
  assert type(s) is list
  F.write("VARIABLE '{0}'\n".formate(s))

def VARIABLES(lst):
  assert type(lst) is list
  assert all(type(x) is str for x in lst)
  F.write("VARIABLES {0}\n".format(str(lst)))

def RESTRICTION(s):
  assert type(s) is str
  F.write("RESTRICTION {0}\n".format(s))

def ALPHABET(lst):
  assert type(lst) is list
  assert all(type(x) is str for x in lst)
  F.write("ALPHABET {0}\n".format(lst))

def MAPPING(name, lst):
  assert type(name) is str
  assert type(lst) is list
  assert all(type(x) is str for x in lst)
  F.write("MAPPING '{0}' {1}\n".format(name, str(lst)))

def EXPERIMENT(name, num, *rest):
  F.write("\nEXPERIMENT '{0}' {1} {2}\n".format(name, num, rest))

def OUTCOME(name, formula):
  assert type(name) is str
  assert type(formula) is str
  F.write("  OUTCOME '{0}' {1}\n".format(repr(name), formula))

if __name__ == '__main__':
  fout = tempfile.NamedTemporaryFile()
  F = fout.file
  execfile(sys.argv[1])
  F.close()
  print fout.name
  os.system("cat "+fout.name)
  print "="*80
  subprocess.call(['./cobra', fout.name])
