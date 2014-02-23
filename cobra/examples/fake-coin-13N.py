#
# FAKE-COIN problem model for COBRA
# 13 coins + one is known to be real (x_0)

N = 14
srange = lambda s, n: [s + str(i) for i in range(1, n+1)]

@variables(["y"] + srange("x", N))
@restriction("!x_0 & Exactly-1(%s)" % ",".join(srange("x", N)))
@alphabet(srange("", N))

@mapping("X", ["x"+str(i) for i in range(N)])

# Helper function for generating strings that represent range of parameters
# For example, params(2,5) = "X$2, X$3, X$4, X$5"
params = lambda n0, n1: ",".join("X$" + str(i) for i in range(n0, n1+1))

for m in range(1, N//2 + 1):
  # weighting m coins agains m coins
  @experiment("weighting" + str(m))
  @params("{1..12}^2 $1<$2")

  # left side is lighter
  @outcome("(Or(%s) & !y) | (Or(%s) & y)" % (params(1, m), params(m+1, 2*m)))
  @outcome("(Or(%s) & y) | (Or(%s) & !y)" % (params(1, m), params(m+1, 2*m)))

  # both sides weight the same
  @outcome("!Or(%s)" % params(1, 2*m))