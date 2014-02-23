#
# CODE777 game model for COBRA
#

N = 3 # number of cards
A = ('a','b') # attributes
V = [str(x) for x in range(7)]  # values

from itertools import product, compress

@alphabet(V)

for n in range(N):
  for a in A:
    @variables(["c%i_%s_%s"%(n, a, v) for v in V])
    @restriction("Exactly-1(%s)"%",".join("c%i_%s_%s"%(n,a,v) for v in V))
    @mapping("X%i_%s"%(n, a), ["c%i_%s_%s"%(n, a, v) for v in V])

for selected in product([0,1], repeat=2*len(A)):
  if not (any(selected[:len(A)]) and any(selected[len(A):])):
    continue
  attrs1 = list(compress(A, selected[:len(A)]))
  attrs2 = list(compress(A, selected[len(A):]))
  @experiment("compare %s against %s"%(",".join(attrs1), ",".join(attrs2)))
  @param(".^%i"%(len(attrs1) + len(attrs2)))

  cards1 = [" & ".join(
                  "X%i_%s($%i)"%(n, attrs1[i], i) for i in range(len(attrs1)))
            for n in range(N)]
  cards2 = [" & ".join(
                  "X%i_%s($%i)"%(n, attrs2[i], len(attrs1)+i) for i in range(len(attrs2)))
            for n in range(N)]
  formula = []
  for i in range(1, N):
    formula.append("AtLeast-%i(%s) & "%(i, ",".join(cards1)) +
                   "AtMost-%i(%s)"%(i-1, ",".join(cards2)))
  outcome = " | ".join(formula)

  @outcome(">", outcome)
  @outcome("<=", "!(%s)"%outcome)

