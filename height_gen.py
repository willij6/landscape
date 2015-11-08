#!/usr/bin/python3
import random
import math
from queue import PriorityQueue

random.seed(17)

n = 64
verbose = False

classic = False

open_locs = set()


# load the slope bounds into memory
slope_file = open('slopes','r')
lower_slopes = [] # if these are positive, problems can arise
upper_slopes = []
for line in slope_file:
    line = line[:-1] # lose trailing newline
    (lower,upper) = line.split(' ')
    lower_slopes.append(int(lower))
    upper_slopes.append(int(upper))
slope_file.close()
n_slopes = len(lower_slopes)
max_slope = max(upper_slopes)

# i = n*3//4*2+1
# j = n//2*2+1
# starti = i
# startj = j
# # j = random.randint(0,n-1)*2+1
# to_process = [((i,j),(i,j))]
# while len(to_process) > 0:
#     (p,gate) = to_process.pop(random.randint(0,len(to_process)-1))
#     if(p in open_locs):
#         continue
#     open_locs.add(gate)
#     open_locs.add(p)
#     temp = []
#     for choice in range(4):
#         (dx,dy) = [[1,0],[0,1],[0,-1],[-1,0]][choice]
#         g2 = (p[0]+dx,p[1]+dy)
#         p2 = (p[0]+2*dx,p[1]+2*dy)
#         if(p2[0] < 0 or p2[0] >= 2*n+1):
#             continue
#         if(p2[1] < 0 or p2[1] >= 2*n+1):
#             continue
#         to_process.append((p2,g2))
#     #     temp.append((p2,g2))
#     # random.shuffle(temp)
#     # to_process.extend(temp)
        

i = random.randint(0,n-1)*2+1
j = random.randint(0,n-1)*2+1

open_locs.add((i,j))
        

# random odd numbers in inclusive range 1 to 2n-1
found = 1
while(found < n*n):
    choice = random.randint(0,3)
    (dx,dy) = [[1,0],[0,1],[0,-1],[-1,0]][choice]
    ii = i + dx
    jj = j + dy
    iii = ii + dx
    jjj = jj + dy
    if(iii < 0 or iii >= 2*n+1):
        continue
    if(jjj < 0 or jjj >= 2*n+1):
        continue
    if((iii,jjj) in open_locs):
        (i,j) = (iii,jjj)
        continue
    open_locs.add((ii,jj))
    open_locs.add((iii,jjj))
    found += 1


for i in range(0,2*n+1):
    for j in range(0,2*n+1):
        if (i,j) in open_locs:
            open_locs.remove((i,j))
        else:
            open_locs.add((i,j))


        
def disp():
    for i in range(2*n+1):
        line = ""
        for j in range(2*n+1):
            if((i,j) in open_locs):
                line += "~"
            else:
                line += "#"
        print(line)
            
parents = {}
inroads = {}
revlist = PriorityQueue()

drainage_total = {}
for i in range(2*n+1):
    for j in range(2*n+1):
        if((i,j) not in open_locs):
            drainage_total[(i,j)] = 0




    
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
            
        
def set_auto(p):
    parents[p] = p
    if(p not in inroads):
        inroads[p] = 0
        revlist.put((-inroads[p],p))

for i in range(2*n+1):
    set_auto((i,0))
    set_auto((0,i))
    set_auto((i,2*n))
    set_auto((2*n,i))

perimeter = set(parents)
    
queue = set()
def maybe_queue(p):
    if(p in open_locs):
        queue.add(p)
        
for i in range(1,2*n):
    maybe_queue((i,1))
    maybe_queue((i,2*n-1))
    maybe_queue((1,i))
    maybe_queue((2*n-1,i))

while(len(queue) > 0):
    (i,j) = queue.pop()
    for choice in range(4):
        (dx,dy) = [[1,0],[0,1],[-1,0],[0,-1]][choice]
        ii = i + dx
        jj = j + dy
        if (ii,jj) in parents:
            parents[(i,j)] = (ii,jj)
            inroads[(i,j)] = 1 + inroads[(ii,jj)]
            revlist.put((-inroads[(i,j)],(i,j)))
        else:
            maybe_queue((ii,jj))



while(not revlist.empty()):
    (junk,p) = revlist.get()
    total = 1
    for choice in range(4):
        (dx,dy) = [[1,0],[0,1],[-1,0],[0,-1]][choice]
        nbhr = (p[0]+dx,p[1]+dy)
        if(nbhr in drainage_total):
            if(nbhr not in inroads or inroads[nbhr] > inroads[p]):
                total += drainage_total[nbhr]
    drainage_total[p] = total



heights = {}

for i in range(2*n+1):
    for j in range(2*n+1):
        heights[(i,j)] = 1000000 # TODO

update_list = PriorityQueue()
def flag(p,val):
    (i,j) = p
    if val < heights[(i,j)]:
        update_list.put((val,(i,j)))
        heights[(i,j)] = val

for p in perimeter:
    flag(p,0)

while not update_list.empty():
    (val,loc) = update_list.get()
    if(val > heights[loc]):
        continue # this instruction was superseded
    for d in [[1,0],[0,1],[-1,0],[0,-1]]:
        (dx,dy) = d
        nbhr = (loc[0]+dx,loc[1]+dy)
        if(nbhr[0] < 0 or nbhr[0] > 2*n or nbhr[1] < 0 or nbhr[1] > 2*n):
            continue

        

        if(loc not in open_locs and nbhr not in open_locs):
            flag(nbhr,heights[loc]+max_slope)
        else:
            # downstream neighbor
            if(loc in open_locs and parents[loc] == nbhr
               or loc not in open_locs):
                capacity = drainage_total[nbhr]
                capacity = math.log(capacity+1)/math.log((2*n+1)**2+2)
                capacity = int(capacity*n_slopes)
                flag(nbhr, heights[loc] - lower_slopes[capacity])
            else:
                #upstream nbhr
                capacity = drainage_total[loc]
                capacity = math.log(capacity+1)/math.log((2*n+1)**2+2)
                capacity = int(capacity*n_slopes)
                flag(nbhr, heights[loc] + upper_slopes[capacity])



        



        
def disp_heights():
    for i in range(2*n+1):
        line = ""
        for j in range(2*n+1):
            line += str(heights[(i,j)])
        print( line)
        



        
def finesse_heights():
    for i in range(2*n+1):
        for j in range(2*n+1):
            heights[(i,j)] *= 2
            if (i,j) not in open_locs:
                heights[(i,j)] += 1
                
# finesse_heights()
# heights[(starti,startj)] += 100

def output_heights():
    for i in range(2*n+1):
        for j in range(2*n+1):
            print(heights[(i,j)])

if(verbose):
    disp()
    disp_parents()            
    disp_heights()

if(classic):
    ht = open('classic.txt','w')
else:
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
