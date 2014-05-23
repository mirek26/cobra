#
# MASTERMIND model for COBRA
#

from itertools import permutations, combinations

N = 3  # number of pegs
C = ['A', 'B', 'C', 'D', 'E', 'F','G','H','I','J','K','L'] # peg colors

# x1_A tells whether the color of the first peg is A
VARIABLES(["x%i%s"%(n,c) for n in range(1, N + 1) for c in C])
ALPHABET(C)

for n in range(1, N+1):
  CONSTRAINT("Exactly-1(%s)" % (",".join("x%i%s"%(n,c) for c in C)))
  MAPPING("F%i"%n, ["x%i%s"%(n,c) for c in C])

EXPERIMENT("guess", N)
for num_blacks in range(N + 1): # number of black markers
  OUTCOME("%i blacks"%num_blacks ,"Exactly-%i(%s)"%(
           num_blacks,
           ", ".join("F%i$%i"%(i,i) for i in range(1, N + 1))))
