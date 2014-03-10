#
# BAGS OF COINS problem model for COBRA
#

N = 10 # number of coins
M = 5  # maximal number of coins on the weight

combination = [0]*M

def generate_combinations(n):
  global combinations
  combinations = []
  generate_helper(n, 1)
  return combinations

def generate_helper(n, i):
  if i > M:
    if n == 0: combinations.append(combination[:])
    return
  for j in range(n//i + 1):
    combination[i - 1] = j
    generate_helper(n - j*i, i + 1)

# x1 tells whether the first bag contains fake coins
VARIABLES(["x%i"%i for i in range(N)] + ["f"])
RESTRICTION("!f")
ALPHABET([str(x) for x in range(N)] + ['-'])
MAPPING("X", ["x%i"%i for i in range(N)] + ["f"])

m = [M//i for i in range(1, M + 1)]
EXPERIMENT("weighting %s"%str(m), sum(m))

s = 1
param_groups = []
for i in range(M):
  param_groups.append(",".join("X$" + str(i) for i in range(s, s + m[i])))
  if m[i] > 1:
    PARAMS_SORTED(range(s, s + m[i]))
  s += m[i]

for i in range(M + 1):
  formula = []
  for c in generate_combinations(i):
    formula.append(" & ".join("Exactly-%i(%s)"%(c[j], param_groups[j]) for j in range(M)))
  OUTCOME("%i fakes"%i, " |\n ".join(formula))
