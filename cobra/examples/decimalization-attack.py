#
# PIN CRACKING model for COBRA
#

N = 4  # number of pegs
C = ['A', 'B', 'C', 'D', 'E'] #, 'F', 'G', 'H', 'I', 'J'] # peg colors

# x1_A tells whether the color of the first peg is A
VARIABLES(["x%i%s"%(n,c) for n in range(1, N + 1) for c in C])
ALPHABET(C)

for n in range(1, N+1):
  CONSTRAINT("Exactly-1(%s)" % (",".join("x%i%s"%(n,c) for c in C)))
  MAPPING("F%i"%n, ["x%i%s"%(n,c) for c in C])

EXPERIMENT("guess", 2*N)
form = " & ".join("(F%i$%i | F%i$%i)"%(i, 2*i-1, i, 2*i) for i in range(1, N+1));
OUTCOME("pin ok", form)
OUTCOME("pin wrong", "!("+ form + ")")
