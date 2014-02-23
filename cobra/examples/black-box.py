#
# BLOCK-BOX model for COBRA
#

N = 8 # size of the grid
M = 4 # number of marbles

############### HELPER CODE for generation of possible paths ###################
# Constants
MARBLE = 'o'
UNKNOWN = '.'
NOTHING = '-'

all_starts = ([(1, i, 1, 0) for i in range(1, N+1)] +
          [(i, 1, 0, 1) for i in range(1, N+1)] +
          [(N, i, -1, 0) for i in range(1, N+1)] +
          [(i, N, 0, -1) for i in range(1, N+1)])
all_ends = [(x - dx, y - dy) for x, y, dx, dy in all_starts]

class CellState:
  def __init__(self, value):
    self.val = value

# Globals
start = None
ends = {}
state = ([[CellState(NOTHING) for j in range(N+2)]] +
         [[CellState(NOTHING)] + [CellState(UNKNOWN) for j in range(N)] +
          [CellState(NOTHING)] for i in range(N)] +
         [[CellState(NOTHING) for j in range(N+2)]])
marbles = 0

def test():
  x, y, dx, dy = start
  if state[y+dx][x+dy].val == MARBLE or state[y-dx][x-dy].val == MARBLE:
    return (x - dx, y - dy)
  while x >= 1 and x <= N and y >= 1 and y <= N:
    l, r = state[y+dy+dx][x+dx+dy].val, state[y+dy-dx][x+dx-dy].val
    if state[y][x].val == MARBLE:
      return (x,y)
    else:
      if r == MARBLE and l == MARBLE:
        dx, dy = -dx, -dy
      elif r == MARBLE:
        dx, dy = dy, dx
      elif l == MARBLE:
        dx, dy = -dy, -dx
    x, y = x + dx, y + dy
  return (x,y)

def path_end(x, y):
  xx, yy = test()
  r = "\n".join("".join(q.val for q in p) for p in state)
  if xx != x or y != yy:
    print "!"*80
    print r, x, y, xx, yy
  if ends.has_key((x,y)):
    ends[(x,y)].append(r)
  else:
    ends['hit'].append(r)

def recurse_down(x, y, l, r, newl, newr, newdx, newdy):
  global state, marbles
  if ((l.val == newl or l.val == UNKNOWN) and
      (r.val == newr or r.val == UNKNOWN)):
    oldl, oldr = l.val, r.val
    l.val, r.val = newl, newr
    newx, newy = x + newdx, y + newdy
    marbles_new = ((newl == MARBLE and oldl != MARBLE) +
                   (newr == MARBLE and oldr != MARBLE))
    marbles += marbles_new
    if marbles <= M:
      if newx >= 1 and newx <= N and newy >= 1 and newy <= N:
        generate_paths((newx, newy, newdx, newdy))
      else:
        path_end(newx, newy)
    marbles -= marbles_new
    l.val, r.val = oldl, oldr

def generate_paths(start):
  global state, marbles
  x, y, dx, dy = start
  c, l, r = state[y][x], state[y+dy+dx][x+dx+dy], state[y+dy-dx][x+dx-dy]
  if (c.val == MARBLE) or (c.val == UNKNOWN and marbles <= M):
    oldc = c.val
    c.val = MARBLE
    path_end(x, y) # hit
    c.val = oldc
  if c.val != MARBLE:
    oldc = c.val
    c.val = NOTHING
    recurse_down(x, y, l, r, MARBLE, MARBLE, -dx, -dy)  # reflection
    recurse_down(x, y, l, r, NOTHING, MARBLE, dy, dx)   # turn left
    recurse_down(x, y, l, r, MARBLE, NOTHING, -dy, -dx) # turn right
    recurse_down(x, y, l, r, NOTHING, NOTHING, dx, dy)  # go straight
    c.val = oldc

def start_from(s):
  global start, ends, state
  x, y, dx, dy = start = s
  ends = dict([e, []] for e in all_ends)
  ends['hit'] = []
  # special cases - immediate reflections
  state[y-dx][x-dy].val = MARBLE
  path_end(x - dx, y - dy)
  state[y+dx][x+dy].val = MARBLE
  path_end(x - dx, y - dy)
  state[y+dx][x+dy].val = NOTHING
  state[y-dx][x-dy].val = NOTHING
  # all other cases
  generate_paths(start)

start_from((1,4,1,0))

############################ GAME description ##################################
vars = ["x%i_%i"%(x, y) for x in range(N) for y in range(N)]
@variables(vars)
@restriction("Exactly-%i(%s)"%(M, ",".join(vars)))

for s in all_starts:
  @experiment("ray from %i, %i"%(s[0], s[1]))
  start_from(start)
  @outcome("Hit", ends['hit'])
  for e in all_ends:
    @outcome("Reflection to %i %i"%e, ends[e])
