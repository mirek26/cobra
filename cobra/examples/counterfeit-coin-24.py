#
# FAKE-COIN problem model (alternative) for COBRA
#

N = 12
srange = lambda s, n: [s + str(i) for i in range(1, n+1)]

VARIABLES(srange("y", N) + srange("x", N))
CONSTRAINT("Exactly-1(%s, %s)" % (",".join(srange("x", N)), ",".join(srange("y", N))))

ALPHABET(srange("", N))
MAPPING("X", ["x"+str(i) for i in range(1, N + 1)])
MAPPING("Y", ["y"+str(i) for i in range(1, N + 1)])

# Helper function for generating strings representing list
# For example, params("X", 2, 5) = "X$2, X$3, X$4, X$5"
params = lambda s, n0, n1: ",".join(s + "$" + str(i) for i in range(n0, n1+1))

for m in range(1, N//2 + 1):
  # weighting m coins agains m coins
  EXPERIMENT("weighing" + str(m), 2*m)
  PARAMS_DISTINCT(range(1, 2*m + 1))
  PARAMS_SORTED(range(1, m + 1))
  PARAMS_SORTED(range(m+1, 2*m + 1))
  PARAMS_SORTED([1, m + 1])

  # left side is lighter
  OUTCOME("lighter", "Or(%s) | Or(%s)" % (params("X", 1, m), params("Y", m+1, 2*m)))
  OUTCOME("heavier","Or(%s) | Or(%s)" % (params("Y", 1, m), params("X", m+1, 2*m)))

  # both sides weight the same
  OUTCOME("same","!Or(%s,%s)" % (params("X", 1, 2*m), params("Y", 1, 2*m)))