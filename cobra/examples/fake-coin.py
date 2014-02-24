#
# FAKE-COIN problem model for COBRA
#

N = 12
srange = lambda s, n: [s + str(i) for i in range(1, n+1)]

VARIABLES(["y"] + srange("x", N))
RESTRICTION("Exactly-1(%s)" % ",".join(srange("x", N)))
ALPHABET(srange("", N))

MAPPING("X", ["x"+str(i) for i in range(N)])

# Helper function for generating strings that represent range of parameters
# For example, params(2,5) = "X$2, X$3, X$4, X$5"
params = lambda s, n0, n1: ",".join(s + str(i) for i in range(n0, n1+1))

for m in range(1, N//2 + 1):
  # weighting m coins agains m coins
  EXPERIMENT("weighting" + str(m),
              2*m,
              "distinct(%s)" % params("", 1, 2*m + 1),
              "sorted(%s)" % params("", 1, m + 1),
              "sorted(%s)" % params("", m+1, 2*m + 1),
              "sorted([1, %i])" % (m + 1))

  # left side is lighter
  OUTCOME("lighter", "(Or(%s) & !y) | (Or(%s) & y)" % (params("X$", 1, m), params("X$", m+1, 2*m)))
  OUTCOME("heavier", "(Or(%s) & y) | (Or(%s) & !y)" % (params("X$", 1, m), params("X$", m+1, 2*m)))

  # both sides weight the same
  OUTCOME("same", "!Or(%s)" % params("X$", 1, 2*m))