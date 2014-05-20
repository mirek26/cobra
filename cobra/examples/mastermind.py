#
# MASTERMIND model for COBRA
#

from itertools import permutations, combinations

N = 4  # number of pegs
C = ['A', 'B', 'C', 'D', 'E', 'F'] # peg colors

# x1_A tells whether the color of the first peg is A
VARIABLES(["x%i%s"%(n,c) for n in range(1, N + 1) for c in C])
ALPHABET(C)

for n in range(1, N+1):
  CONSTRAINT("Exactly-1(%s)" % (",".join("x%i%s"%(n,c) for c in C)))
  MAPPING("F%i"%n, ["x%i%s"%(n,c) for c in C])

EXPERIMENT("guess", N)
for num_total in range(N + 1): # total number of pegs
  for num_blacks in range(num_total + 1): # number of black pegs
    num_whites = num_total - num_blacks
    formula = []
    # select B
    for blacks in combinations(range(1, N+1), num_blacks):
      remaining = set(range(1, N+1)) - set(blacks)
      # select W
      for whites in combinations(remaining, num_whites):
        for whitesfor_c in combinations(remaining, num_whites):
          for whitesfor in permutations(whitesfor_c):
            if any(whites[i] == whitesfor[i] for i in range(num_whites)):
              continue
            formula.append(set())
            for b in blacks:
              formula[-1].add("F%i$%i"%(b, b))
            for i in range(num_whites):
              formula[-1].add("F%i$%i"%(whitesfor[i], whites[i]))
              formula[-1].add("!F%i$%i"%(whites[i], whites[i]))
              formula[-1].add("!F%i$%i"%(whitesfor[i], whitesfor[i]))
            for p in remaining - set(whites):
              for q in remaining - set(whitesfor):
                formula[-1].add("!F%i$%i"%(q, p))

    if len(formula) > 0:
      # this outcome can be the last one only if num_blacks == N
      OUTCOME("%i + %i"%(num_blacks, num_whites),
               " | ".join(" & ".join(clause) for clause in formula),
               num_blacks == N)
