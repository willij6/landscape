#!/usr/bin/python3

# This program generates the elevation data
# which is fed into the viewing program.

# ALGORITHM DESCRIPTION
# First, a maze is randomly generated.
# The walls of the maze become the rivers, and
# the open passages of the maze become the divides.
# Now, try to assign a height to each point in the maze,
# subject to the constraints that
# 1. Rivers flow downhill
# 2. Slope never exceeds some constant, say, 1.0
# 3. The perimeter of the map (where all the rivers empty) has height 0
#
# Among _all_ such ways of assigning heights, there is one that
# simultaneously maximizes all the heights.  We take that joint maximum
#
# This is because all the constraints are of the form h_1 <= h_2 + c
#
# To actually find this maximum, we propagate constraints.
# (Maybe this is called "relaxation"?)
# We start out knowing that all the points on the perimeter are
# at height at most 0.  If there's a constraint h_1 <= h_2 + c,
# then the moment we learn that h_2 <= b_2, we also know h_1 <= b_2 + c.
#
# So each upper bound gives rise to upper bounds on the neighboring
# height variables.  Our goal is to propagate these upper bounds until
# everything has run out of steam.
#
# To do this, we maintain a table listing the best known upper bound for
# each location on the map, and a priority queue of facts that need to
# be processed.  Each fact is an upper bound on a specific location.
#
# To process a fact from the queue, we check whether it is already known
# in the main table and if so, we do nothing.
# Otherwise, we update the main table, and use the constraints between
# adjacent cells to create four new facts, which we throw into the
# priority queue.
#
# Assuming some positivity on the signs in the constraints, this is
# secretly Dijsktra's algorithm, which motivates the use of a priority
# queue.

# In the event that the constraints are inconsistent, the algorithm
# never terminates! TODO: fix this.

# WHAT EXACTLY THIS PROGRAM DOES
# It generates a file `heights.txt` and a file `drainage.txt`
# Each is an unformatted list of newline separated numbers,
# which are meant to be interpreted as a two-dimensional array.
# heights.txt contains the actual height, while drainage.txt
# contains data on how much water flows through each point.
# The viewer uses this to determine how to color the map.
# TODO: directly send color data

import random
import math
from queue import PriorityQueue

# The drainage network is randomly generated
# but the heights are generated deterministically
# from the drainage network.
# Fixing this seed right here allows one to
# fiddle with the algorithm for determining heights,
# to see what kind of changes are made, without
# drastically altering the overall geography
random.seed(17)

# n determines the size of the map:
# The created map is (2*n+1) times (2*n+1)
n = 64
# viewer.c wants a 127x127 map, so n had better be 64.
# TODO: fix this

# changing verbose to True will make the program output
# ASCII depictions of what's going on, which is mainly useful
# when n is significantly smaller
verbose = False # changing this to True makes the program display


# STEP 1: Load the slope constraints into memory
# there are two lists:
# * lower_slopes -- lower bounds on slopes
# * upper_slopes -- upper bounds on slopes
# Of course nothing will work if lower_slopes > upper_slopes
# Even letting lower_slopes > 0 happen tends to cause problems.
#
# The point of these _lists_ is that we'd like to let
# the slope constraints vary by how much imaginary erosion
# is around, which is determined by the total area being drained
# by whatever point we're looking at.

slope_file = open('slopes','r')
lower_slopes = []
upper_slopes = []
for line in slope_file:
    line = line[:-1] # lose trailing newline
    (lower,upper) = line.split(' ')
    lower_slopes.append(int(lower)) # TODO: floats instead?
    upper_slopes.append(int(upper))
slope_file.close()
n_slopes = len(lower_slopes)
max_slope = max(upper_slopes)



# STEP 2: Generate the random maze

# Using the stupid algorithm (that happens to generate
# uniform random mazes)
# Choose a random starting point
# Do a random walk
# Every time you first reach a vertex, add the edge you used
# to generate it

# We'll store the set of open passages in the maze
# in this set
open_locs = set()
# (Later, the open passages will become the non-rivers)

# random starting point
i = random.randint(0,n-1)*2+1
j = random.randint(0,n-1)*2+1
open_locs.add((i,j))

# do a random walk
# the variable 'found' counts the vertices we've reached.
# (we can stop once we've reached all of them)
found = 1 # so far, we've visited one vertex: the starting point
while(found < n*n):
    choice = random.randint(0,3)
    (dx,dy) = [[1,0],[0,1],[0,-1],[-1,0]][choice]
    ii = i + dx
    jj = j + dy
    iii = ii + dx
    jjj = jj + dy
    # (ii,jj) is the "edge", (iii,jjj) is the "vertex"

    # check if we fell off the edge of the map
    if(iii < 0 or iii >= 2*n+1):
        continue
    if(jjj < 0 or jjj >= 2*n+1):
        continue
    # if this is the first time we've been here, change stuff
    if((iii,jjj) not in open_locs):
        open_locs.add((ii,jj))
        open_locs.add((iii,jjj))
        found += 1
    # move to the next location in our walk    
    (i,j) = (iii,jjj)



# now, invert open_locs,
# clearing the walls and filling in the spaces
for i in range(0,2*n+1):
    for j in range(0,2*n+1):
        if (i,j) in open_locs:
            open_locs.remove((i,j))
        else:
            open_locs.add((i,j))
# now open_locs is a set of where all the rivers should be






# STEP 3: Determine Drainage
# Before we can process all the height constraints,
# we need to determine which was is downstream, i.e.,
# which way all the rivers flow
#
# At the same time, we can calculate the total area drained
# by each point, which helps determine the coloring for the map,
# as well as determining the upper and lower bounds on the slopes

parents = {} # parents[p] = q if q is directly downstream from p
             # parents[p] is not defined if p isn't a river
             # parents[p] = p if p is in the ocean

# drainage_total[p] = (the area drained by p) if p is a river else 0
drainage_total = {}

for i in range(2*n+1):
    for j in range(2*n+1):
        if((i,j) not in open_locs):
            drainage_total[(i,j)] = 0

            
ocean = set()
for i in range(2*n+1):
    ocean.add((i,0))
    ocean.add((0,i))
    ocean.add((i,2*n))
    ocean.add((2*n,i))
             
for p in ocean:
    parents[p] = p



# # the recursive way
# def handle(p,depth):
#     # print((" " * depth) + "Handling " + str(p))
#     drainage_total[p] = 1 # itself
#     for d in [(1,0),(0,1),(-1,0),(0,-1)]:
#         nbr = (d[0]+p[0],d[1]+p[1])
#         if (nbr == parents[p] or nbr not in open_locs
#             or nbr in ocean):
#             continue
#         parents[nbr] = p
#         drainage_total[p] += handle(nbr,depth+1)
#     return drainage_total[p]

# for p in ocean:
#     handle(p,0)

    
# the nonrecursive way

tasks = []
def process_tasks():
    while len(tasks) > 0:
        action = tasks.pop()
        action()


# get_action((i,j)) returns a callable object that
#  [does the first half of the processing,
#   and pushes the remaining actions that need to be done
#   onto the tasks list]
def get_action(p):
    def action():
        nbrs = []
        for d in [(1,0),(0,1),(-1,0),(0,-1)]:
            nbr = (d[0]+p[0],d[1]+p[1])
            if(nbr == parents[p] or nbr not in open_locs
               or nbr in ocean):
                continue
            nbrs.append(nbr)

        # follow_up takes care of the remaining things
        def follow_up():
            drainage_total[p] = 1+sum([drainage_total[n] for n in nbrs])
        tasks.append(follow_up)                     
        for n in nbrs:
            parents[n] = p
            tasks.append(get_action(n))
    return action

for p in ocean:
    tasks.append(get_action(p))
process_tasks()





    


# STEP 4: calculate the heights
    
# ultimately, heights[p] should be the elevation of location p
# During the computation, it will always contain upper bounds on the heights,
# i.e., it will only decrease
heights = {}

for i in range(2*n+1):
    for j in range(2*n+1):
        heights[(i,j)] = 1000000 # TODO

# whenever we know that location p has height at most val,
# this imposes some constraints on the neighbors as well.
# update_list is a list of pairs (val,p) that need to be processed
update_list = PriorityQueue()

# process the fact that heights[p] should be <= val
def flag(p,val):
    (i,j) = p
    if val < heights[(i,j)]:
        update_list.put((val,(i,j)))
        heights[(i,j)] = val
    # else, don't add it to update_list, because
    # this isn't news

    
# we know initially that the perimeter of the map should be 0
for p in ocean:
    flag(p,0) # this populates update_list


while not update_list.empty():
    (val,loc) = update_list.get()
    if(val > heights[loc]):
        continue # this instruction was superseded
    for d in [[1,0],[0,1],[-1,0],[0,-1]]:
        (dx,dy) = d
        nbr = (loc[0]+dx,loc[1]+dy)
        # check for out of bounds
        if(nbr[0] < 0 or nbr[0] > 2*n or nbr[1] < 0 or nbr[1] > 2*n):
            continue
        # TODO: encapsulate the algorithm for finding the maximal
        # solution to some constraints, so that it's not mixed
        # up with the actual constraints
        if(loc not in open_locs and nbr not in open_locs):
            flag(nbr,heights[loc]+max_slope)
        else:
            # downstream neighbor
            if(loc in open_locs and parents[loc] == nbr
               or loc not in open_locs):
                capacity = drainage_total[nbr]
                capacity = math.log(capacity+1)/math.log((2*n+1)**2+2)
                capacity = int(capacity*n_slopes)
                flag(nbr, heights[loc] - lower_slopes[capacity])
            else:
                #upstream nbr
                capacity = drainage_total[loc]
                capacity = math.log(capacity+1)/math.log((2*n+1)**2+2)
                capacity = int(capacity*n_slopes)
                flag(nbr, heights[loc] + upper_slopes[capacity])

# STEP 5: output the heights

ht = open('heights.txt','w')
for i in range(2*n+1):
    for j in range(2*n+1):
        data = heights[(i,j)]
        ht.write(str(data) + "\n")

dr = open('drainage.txt','w')


for i in range(2*n+1):
    for j in range(2*n+1):
        data = int(math.sqrt(drainage_total[(i,j)]))
        dr.write(str(data) + "\n")


dr.close()            


# STEP 6: if we're in verbose mode, display some stuff

# debugging function which graphically outputs open_locs
def disp():
    for i in range(2*n+1):
        line = ""
        for j in range(2*n+1):
            if((i,j) in open_locs):
                line += "~"
            else:
                line += "#"
        print(line)

def disp_parents():
    for i in range(2*n+1):
        line = ""
        for j in range(2*n+1):
            line += get_parent_char((i,j))
        print(line)


def get_parent_char(p):
    if(p not in parents):
        return '#'
    par = parents[p]
    dx = par[0] - p[0]
    dy = par[1] - p[1]
    if((dx,dy) == (1,0)):
        return 'v'
    elif((dx,dy) == (0,1)):
        return '>'
    elif((dx,dy) == (0,-1)):
        return '<'
    elif((dx,dy) == (-1,0)):
        return '^'
    elif((dx,dy) == (0,0)):
        return 'o'
    else:
        return '?'

def disp_heights():
    for i in range(2*n+1):
        line = ""
        for j in range(2*n+1):
            line += str(heights[(i,j)])
        print( line)
        


    
if(verbose):
    disp()
    disp_parents()            
    disp_heights()

