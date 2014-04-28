#
# FAKE-COIN problem model for COBRA
#

N = 12
srange = lambda s, n: [s + str(i) for i in range(1, n+1)]

VARIABLES(["y"] + srange("x", N))
RESTRICTION("Exactly-1(%s)" % ",".join(srange("x", N)))

ALPHABET(srange("", N))
MAPPING("X", ["x"+str(i) for i in range(1, N + 1)])

# Helper function for generating strings that represent range of parameters
# For example, params(2,5) = "X$2, X$3, X$4, X$5"
params = lambda n0, n1: "|".join("X$" + str(i) for i in range(n0, n1+1))

for m in range(1, N//2 + 1):
  # weighing m coins agains m coins
  EXPERIMENT("weighing" + str(m), 2*m)
  PARAMS_DISTINCT(range(1, 2*m + 1))
  PARAMS_SORTED(range(1, m + 1))
  PARAMS_SORTED(range(m+1, 2*m + 1))
  PARAMS_SORTED([1, m + 1])

  # left side is lighter
  OUTCOME("lighter", "((%s) & !y) | ((%s) & y)" % (params(1, m), params(m+1, 2*m)))
  OUTCOME("heavier", "((%s) & y) | ((%s) & !y)" % (params(1, m), params(m+1, 2*m)))

  # both sides weight the same
  OUTCOME("same", "!(%s)" % params(1, 2*m))