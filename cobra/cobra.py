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
  F.write("VARIABLES {0}\n".format(",".join(lst)))

def RESTRICTION(s):
  assert type(s) is str
  F.write("RESTRICTION {0}\n".format(s))

def ALPHABET(lst):
  assert type(lst) is list
  assert all(type(x) is str for x in lst)
  F.write("ALPHABET {0}\n".format(str(lst)[1:-1]))

def PARAMS_SORTED(lst):
  assert all(type(x) is int for x in lst)
  F.write("  PARAMS_SORTED {0}\n".format(",".join(str(x) for x in lst)))

def PARAMS_DISTINCT(lst):
  assert all(type(x) is int for x in lst)
  F.write("  PARAMS_DISTINCT {0}\n".format(",".join(str(x) for x in lst)))

def MAPPING(name, lst):
  assert type(name) is str
  assert type(lst) is list
  assert all(type(x) is str for x in lst)
  F.write("MAPPING '{0}' {1}\n".format(name, ",".join(lst)))

def EXPERIMENT(name, num):
  F.write("\nEXPERIMENT '{0}' {1}\n".format(name, num))

def OUTCOME(name, formula):
  assert type(name) is str
  assert type(formula) is str
  F.write("  OUTCOME '{0}' {1}\n".format(name, formula))

if __name__ == '__main__':
  fout = tempfile.NamedTemporaryFile()
  F = fout.file
  execfile(sys.argv[1])
  F.close()
  print fout.name
  os.system("cat "+fout.name)
  print "="*80
  subprocess.call(['./cobra', fout.name])
