#
# FAKE-COIN problem model (alternative) for COBRA
#

N = 12
srange = lambda s, n: [s + str(i) for i in range(1, n+1)]

@variables(srange("y", N) + srange("x", N))
@restriction("Exactly-1(%s, %s)" % (",".join(srange("x", N)), ",".join(srange("y", N))))
@alphabet(srange("", N))
@mapping("X", ["x"+str(i) for i in range(N)])
@mapping("Y", ["y"+str(i) for i in range(N)])

# Helper function for generating strings representing list
# For example, params(2,5) = "X$2, X$3, X$4, X$5"
params = lambda s, n0, n1: ",".join(s + "$" + str(i) for i in range(n0, n1+1))

for m in range(1, N//2 + 1):
  # weighting m coins agains m coins
  @experiment("weighting" + str(m))
  @params("{1..12}^2 $1<$2")

  # left side is lighter
  @outcome("lighter", "Or(%s) | Or(%s)" % (params("X", 1, m), params("Y", m+1, 2*m)))
  @outcome("heavier","Or(%s) | Or(%s)" % (params("Y", 1, m), params("X", m+1, 2*m)))

  # both sides weight the same
  @outcome("same","!Or(%s,%s)" % (params("X", 1, 2*m), params("Y", 1, 2*m)))