#
# Bags of gold model for COBRA
#

N = 10 # number of bags
M = 5  # maximal number of coins on the balance scale

# x_i tells whether bag i contains fake coins
VARIABLES(["x%i"%i for i in range(N)])
ALPHABET([str(x) for x in range(N)])
MAPPING("X", ["x%i"%i for i in range(N)])

types = []
def gen_exp(exp, num):
  next = num if len(exp) == 0 else min(num, exp[-1])
  for i in range(next, 0, -1):
    types.append(exp + [i])
    gen_exp(exp + [i], num - i)

def exp_str(exp):
  return "".join(chr(ord('A') + p) * n for p,n in enumerate(exp))

from itertools import product, compress

gen_exp([], M)
for t in types:
  EXPERIMENT(exp_str(t), len(t))
  PARAMS_DISTINCT(range(1, len(t) + 1))
  fakes = dict((i, []) for i in range(M + 1))
  for pattern in product([0,1], repeat=len(t)):
    fakes[sum(compress(t, pattern))].append(pattern)

  for i in range(M + 1):
    if len(fakes[i])==0: continue
    formula = []
    for pattern in fakes[i]:
      conj = [("" if pattern[param] else "!") + "X$" + str(param+1) for param in range(len(t))]
      formula.append(" & ".join(conj))
    OUTCOME("%i fakes"%i, " | ".join(formula))