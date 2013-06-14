# pocet kouli
N = 14
INFTY = 1000

from collections import namedtuple

# sada kouli - unk (nic nevim), stdl (normal nebo lehci), stdh (normal nebo tezsi), std (normal)
Balls = namedtuple("B", "unk stdl stdh std l h")
Experiment = namedtuple("E", "left right")
Data = namedtuple("D", "wc exp")

def generate_quadruple(sum):
	return [(a, b, c, sum - a - b - c) 
			for a in range(sum + 1) 
			for b in range(sum + 1 - a)
			for c in range(sum + 1 - a - b)]

def balls(unk, stdl = 0, stdh = 0, std = None, l = 0, h = 0):
	return Balls(unk, stdl, stdh, std if std != None else N - unk - stdl - stdh, l, h)

def printballs(balls):
	return "? "*balls.unk + "- "*balls.stdl + "+ "*balls.stdh + "o "*balls.std

def free(state):
	return state.unk*2 + state.stdl + state.stdh

def probs(state, experiment):
	total = state.unk*2 + state.stdl + state.stdh
	left = experiment.left.unk + experiment.left.stdl + experiment.right.unk + experiment.right.stdh
	right = experiment.left.unk + experiment.left.stdh + experiment.right.unk + experiment.right.stdl
	return ((total-left-right) / total, left/total, right/total)

E = [Experiment(balls(*left), balls(*right)) 
		for x in range(1, N//2+1) 
		for left in generate_quadruple(x)
		for right in generate_quadruple(x) 
		if left <= right and (left[3] == 0 or right[3] == 0)]

def next(state):
	for e in E:
		if feasible(state, e):
			results = [perform(state, e, r) for r in ['=', '<', '>']]
			yield ((e, max(free(s) for s in results), results))

def feasible(state, experiment):
	return (state.unk >= experiment.left.unk + experiment.right.unk and
		   state.stdl >= experiment.left.stdl + experiment.right.stdl and
		   state.stdh >= experiment.left.stdh + experiment.right.stdh and
		   state.std >= experiment.left.std + experiment.right.std);

def perform(state, experiment, result):
	unk, stdl, stdh, std, l, h = state
	if result == '=':
		unk -= experiment.left.unk + experiment.right.unk
		stdl -= experiment.left.stdl + experiment.right.stdl
		stdh -= experiment.left.stdh + experiment.right.stdh
	else:
		lighter, heavier = ((experiment.left, experiment.right)
							if result == '<' else 
							(experiment.right, experiment.left))
		stdl = lighter.unk + lighter.stdl
		stdh = heavier.unk + heavier.stdh
		# no unknown
		unk = 0
	
	std = N - unk - stdl - stdh - l - h
	if (unk == 0 and stdl == 1 and stdh == 0): stdl, l = 0, 1
	if (unk == 0 and stdl == 0 and stdh == 1): stdh, h = 0, 1
	return balls(unk, stdl, stdh, std, l, h)

cache = {}
def analyze(state):
	wcpred = [0] + [1]*3 + [2]*6 + [3]*15
	if state not in cache: 
		cache[state] = Data(INFTY, INFTY)
		wc, exp = INFTY, INFTY
		if (state.l == 1 or state.h == 1 or state.std == N):
			wc, exp = 0, 0.0
		else:
			for e, p, r in next(state):
				a = [analyze(x) for x in r]
				wc = min(wc, 1 + max(x.wc for x in a))
				exp = min(exp, 1 + sum(x*p for x,p in zip([x.exp for x in a], probs(state, e))))
		cache[state] = Data(wc, exp)
		# if (wcpred[free(state)] != wc ):
		# 	print("!!!%s"%str(state))
		# else:
		# 	print(".")
	return cache[state]

def info(state):
	def printr(c, s, pr, wc, exp):
		if pr > 0:
			print("    '%c' [P=%.2f] -> %s: free=%i wc=%i exp=%.2f"%(c, pr, str(tuple(s)), free(s), wc, exp))
	# about the state
	d = analyze(state)
	print("%s free=%i wc=%i exp=%.2f"%(str(tuple(state)), free(state), d.wc, d.exp))
	# possibilities for the next experiment
	for e, p, r in sorted(next(state), key = lambda x:x[1]):
		print("\n" + printballs(e.left) + "  vs  " + printballs(e.right))
		a = [analyze(x) for x in r]
		pr = probs(state, e)
		fr = max(free(x) for x in r)
		wc = max(x.wc for x in a)
		exp = sum(x*p for x,p in zip([x.exp for x in a], pr))
		print("  free=%i wc=%i exp=%.2f"%(fr, wc, exp))
		printr('=', r[0], pr[0], a[0].wc, a[0].exp)
		printr('<', r[1], pr[1], a[1].wc, a[1].exp)
		printr('>', r[2], pr[2], a[2].wc, a[2].exp)

s = balls(12)
e = Experiment(balls(4,0,0,0), balls(4,0,0,0))