#
# CODE777 game model for COBRA
#

N = 3 # number of cards
A = ('a','b') # attributes
V = [str(x) for x in range(3)]  # values

from itertools import product, compress

ALPHABET(V)

for n in range(N):
  for a in range(len(A)):
    VARIABLES(["c%i%s%s"%(n, A[a], v) for v in V])
    if n != 0:
      # add a constraint that card n is lexicographically smaller than n-1
      alleq = " & ".join("(c%i%s%s<->c%i%s%s)"%(n,A[i],v,n-1,A[i],v)
                         for i in range(a) for v in V)
      conj = []
      for v in range(len(V)):
        prev = "|".join("c%i%s%s"%(n,A[a],V[w]) for w in range(v)) # one of previous values
        if prev: prev = prev + " | "
        conj.append("(%s(c%i%s%s->c%i%s%s))"%(prev,n-1,A[a],V[v],n,A[a],V[v]));
      greater = " & ".join(conj)
      CONSTRAINT("(%s) -> (%s)"%(alleq, greater) if alleq else greater)

    CONSTRAINT("Exactly-1(%s)"%",".join("c%i%s%s"%(n,A[a],v) for v in V))
    MAPPING("X%i%s"%(n, A[a]), ["c%i%s%s"%(n, A[a], v) for v in V])


for selected in product([0,1], repeat=2*len(A)):
  if not (any(selected[:len(A)]) and any(selected[len(A):])):
    continue
  attrs1 = list(compress(A, selected[:len(A)]))
  attrs2 = list(compress(A, selected[len(A):]))
  EXPERIMENT("compare %s against %s"%(",".join(attrs1), ",".join(attrs2)), len(attrs1) + len(attrs2))

  cards1 = [" & ".join(
                  "X%i%s$%i"%(n, attrs1[i], i + 1) for i in range(len(attrs1)))
            for n in range(N)]
  cards2 = [" & ".join(
                  "X%i%s$%i"%(n, attrs2[i], len(attrs1)+i+1) for i in range(len(attrs2)))
            for n in range(N)]
  formula = []
  for i in range(1, N):
    formula.append("AtLeast-%i(%s) & "%(i, ",".join(cards1)) +
                   "AtMost-%i(%s)"%(i-1, ",".join(cards2)))
  outcome = " | ".join(formula)

  OUTCOME(">", outcome)
  OUTCOME("<=", "!(%s)"%outcome)

