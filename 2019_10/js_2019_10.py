import numpy as np
import copy

# Jane Street Puzzle. October 2019. 
# see https://www.janestreet.com/puzzles/
 
# Place a collection of right triangles into the grid below. 
# The triangles must have integer-length legs, and the legs must be along 
# grid lines.
 
# Each triangle must contain exactly one number. That number represents the 
# area of the triangle containing it. (Every number must eventually be 
# contained in exactly one triangle.) The entire square (1-by-1 cell) 
# containing the number must be inside the triangle.
 
# Triangles’ interiors may not overlap. (But triangles’ boundaries may 
# intersect, as seen in the example.)
 
# As your answer to this month’s puzzle, please send in the product of the 
# odd horizontal leg lengths.


#******************************************************************************
# Let's retype target-square table given in the puzzle.
# Below, empty squares are represented as 0s and they are non-target.
uint32 = np.uint32
int32  = np.int32 # More than 32 bits is not needed

tor: int32 = 17 # n of original table rows 
toc: int32 = 17 # n of original columns

tt = np.array([[  0,  0,  0,  0,  0,  0,  0,  0,  0,  8,  0,  0,  0,  2,  0,  0,  0, ], 
               [  0,  0,  0, 12,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  7,  0,  0, ], 
               [  4,  0,  0,  0,  0, 10,  0,  0,  0,  0,  0,  0,  3,  0,  0,  0,  0, ], 
               [  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  5, ], 
               [  0,  0,  0,  0,  0,  0,  0,  7,  0,  0,  0,  0,  0, 10,  0,  0,  0, ], 
               [  0,  0,  3,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, ], 
               [  0,  7,  0,  0,  0,  0,  0,  0,  0,  0,  0,  3,  0,  0,  0,  0,  3, ], 
               [  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, ], 
               [  0,  0,  0,  0,  0,  0,  0,  0, 20,  0,  0,  0,  0,  0,  0,  0,  0, ], 
               [  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, ], 
               [  4,  0,  0,  0,  0, 14,  0,  0,  0,  0,  0,  0,  0,  0,  0, 18,  0, ], 
               [  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8,  0,  0, ], 
               [  0,  0,  0,  9,  0,  0,  0,  0,  0, 11,  0,  0,  0,  0,  0,  0,  0, ],
               [  6,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, ], 
               [  0,  0,  0,  0,  3,  0,  0,  0,  0,  0,  0,  7,  0,  0,  0,  0,  6, ], 
               [  0,  0, 12,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  4,  0,  0,  0, ], 
               [  0,  0,  0,  2,  0,  0,  0, 18,  0,  0,  0,  0,  0,  0,  0,  0,  0, ]], dtype=int32)
 

#******************************************************************************
# When writing the code it will be convenient to use table coordinates wrapped
# in a single object rather than two single numbers.
# Table coordinates are defined as follows

class TCoord: # Table Coordinates
    def __init__(self, i: int32 = 0, j: int32 = 0) -> None: 
        self.first  = i
        self.second = j
    def __iadd__(self, o: "TCoord"):
        self.first  += o.first
        self.second += o.second
        return self
    def swap(self) -> None: self.first, self.second = self.second, self.first 

def in_table(loc: TCoord) -> bool: return loc.first >= 0 and loc.second >= 0 and loc.first < tor and loc.second < toc


#******************************************************************************
# For convenience, let's create a list of target square coordinates
ltsq: [TCoord] = [] # list of target squares; list of coordinates

def init_ltsq() -> None:
    global ltsq
    global tt
    for i in range(tor):
        for j in range(toc):
            if tt[i][j] != 0: 
                ltsq.append(TCoord(i,j))
            
#******************************************************************************
# To solve the problem we look at the grid of tt square corners
# Note, however, that triangle interiors cannot overlap and we will need
# some functionality to check it. This problem can be solved in various
# ways. Conceptually, the easiest way is to make the grid more  -
# dense enough to make any two overlaping triangles share an internal
# point, i.e. point that does not belong to the triangle's line segments.
# If we take into account all possilble triangle sizes (we will look
# at this issue later) our grid needs more than 7 times as many lines.
# For simplicity, a scale of 10 will serve the purpose.
# Note that the original target-square table is 17x17, and grid 18x18.

scale: int32 = 10
gr   : int32 = tor * scale + 1
gc   : int32 = toc * scale + 1

#******************************************************************************
# When writing the code it will be convenient to use grid coordinates
# wrapped in a single object rather than two single numbers.
# The code will be clearer if grid coordinates are represented by a 
# structure different from TCoord.

class GCoord:
    def __init__(self, i: int32 = 0, j: int32 = 0) -> None: 
        self.first  = i
        self.second = j
    def __iadd__(self, o: "GCoord"):
        self.first  += o.first
        self.second += o.second
        return self
    def swap(self) -> None: self.first, self.second = self.second, self.first 


def in_grid(loc: GCoord) -> bool: return loc.first >= 0 and loc.second >= 0 and loc.first <= gr and loc.second <= gc;

def t2g(loc: int or int32 or TCoord) -> int or GCoord:
    if isinstance(loc, int) or isinstance(loc, int32): return loc*scale
    if isinstance(loc, TCoord)                       : return GCoord(t2g(loc.first), t2g(loc.second))

#******************************************************************************
# A triangle must cover only its target square. Since we are looking at a 
# more dense grid, it is enough to check if the triangle we are placing on
# the grid shares any of its internal points with another target square.
# Let's create a grid table that marks internal points of all original
# tt target squares with their tt table coordinates.
# Note that tt square (0,0) is not a target, so we can use it as "empty".

gmsqi: np.array(TCoord) = np.empty((gr, gc), dtype=TCoord)

def init_gmsqi():
    global smsqi
    for i in range(gmsqi.shape[0]):
        for j in range(gmsqi.shape[1]):
            gmsqi[i][j] = TCoord()
    for sq in ltsq:
        r: int32 = sq.first
        c: int32 = sq.second
        for i in range(t2g(r)+1, t2g(r+1)):
            for j in range(t2g(c)+1, t2g(c+1)):
                gmsqi[i][j] = sq

#******************************************************************************
# A triangle has boundary grid points and internal/gut grid points.
# Since we are using a dense grid, all we need to pay attention to are
# internal points.
# Class gps is used for this purpose.
# '.' -- "empty" grid point 
# '*' -- internal

class Gps: # grid point status: empty or internal
    def __init__(self, o = None):
        self.s: np.chararray = np.chararray((gr,gc))
        if not isinstance(o, Gps):
            for i in range(gr):
                for j in range(gc): self.s[i][j] = b'.'
        else:
            for i in range(gr):
                for j in range(gc): self.s[i][j] = o.s[i][j]
    def is_empty     (self, loc: GCoord) -> bool: return self.s[loc.first][loc.second] == b'.'
    def is_internal  (self, loc: GCoord) -> bool: return self.s[loc.first][loc.second] == b'*'
    def mark_internal(self, loc: GCoord) -> None:        
        self.s[loc.first][loc.second] =  b'*'
    def is_covered  (self, tloc: GCoord) -> bool:
        i: int32 = t2g(tloc.first)  + 1
        j: int32 = t2g(tloc.second) + 1
        return self.s[i][j] == b'*'
    def print(self) -> None:
        print("Gps: ")
        for i in range(gr):
            for j in range(gc): print('', self.s[i][j], end='')
            print()
    
#******************************************************************************
# An attempt to solve the puzzle manually may involve cutting paper triangles of
# various sizes. Let's call them templates. A template square entirely contained
# in the template may cover the target square. It defines the template position. 
# A template can be flipped horizonally, vertically, or longer and shorter arms
# can be swapped. This leads to the notion of triangle configuration.

class Config: # triangle configuration
    s: bool  = False # flip asymmetry
    h: bool  = False # flip horizontally
    v: bool  = False # flip vertically
    p: int32 = 0     # use this position index -- a template has a list
    
#******************************************************************************
# A template defines its corners, boundary points, and positions.
# The square angle corner is located at (0,0). For position (0,0),
# to get coordinates of grid points covered by the template, add its corners,
# boundary, and internal coordinates to the grid coordinates of the target 
# square.
# transform() computes the relative location of each triangle point taking
# into account triangle configuration. 

class Tm:
    def __init__(self, r: int32, c: int32): # r, c == original table size
        self.v: [GCoord] = [] # vertices/corners 
        self.b: [GCoord] = [] # boundary but not corners
        self.g: [GCoord] = [] # internal "guts"
        self.p: [TCoord] = [] # target square positions - upper left corner
        self.v.append(GCoord(t2g(0),t2g(0))) # scaled coordinates
        self.v.append(GCoord(t2g(0),t2g(c)))
        self.v.append(GCoord(t2g(r),t2g(0)))
        
        # find t.b
        for j in range(1, t2g(c)): self.b.append(GCoord(0,j))
        for i in range(1, t2g(r)): self.b.append(GCoord(i,0))
        rf: int32 = self.gcd(r,c) # reduction factor
        rr: int32 = r/rf          # reduced r -- we want relateively prime pair
        rc: int32 = c/rf          # reduced c
        for k in range(1, t2g(rf)): self.b.append(GCoord(r-k*rr, 0+k*rc))
        
        # find t.g
        for i in range(1, t2g(r)): # skip original (0,c) and (r,0)
            j: int32 = 1
            while True:
                # only columns that satisfy floating point j < (t2g(r)-i)*(c/r)
                if j*rr >= (t2g(r)-i)*rc: break
                self.g.append(GCoord(i,j))
                j += 1
        
        # find t.p -- (i+1,j+1) must be inside or at boundary
        for i in range(r):
            j: int32 = 0
            while True:
                # only columns that satisfy floating point (j+1) <= (r-i-1)*(c/r)
                if (j+1)*rr > (r-i-1)*rc: break
                self.p.append(TCoord(i,j))
                j += 1
                
    def symmetrical(self) -> bool: return self.v[1].second == self.v[2].first
    def transform_tcoord(self, cnfg: Config, loc: TCoord) -> GCoord:
        res: TCoord = copy.copy(loc) # TODO: Make this a copy
        res.first  -= self.p[cnfg.p].first
        res.second -= self.p[cnfg.p].second
        if cnfg.s: res.swap()
        if cnfg.h: res.first  = -res.first  + 1
        if cnfg.v: res.second = -res.second + 1
        return t2g(res)
    def transform_gcoord(self, cnfg: Config, loc: GCoord) -> GCoord:
        res: GCoord = copy.copy(loc) # TODO: Make this a copy
        res.first  -= t2g(self.p[cnfg.p].first)
        res.second -= t2g(self.p[cnfg.p].second)
        if cnfg.s: res.swap()
        if cnfg.h: res.first  = -res.first  + t2g(1)
        if cnfg.v: res.second = -res.second + t2g(1)
        return res
    def transform(self, cnfg: Config, loc: TCoord or GCoord) -> GCoord:
        # cloc = copy.copy(loc)
        if   isinstance(loc, TCoord): return self.transform_tcoord(cnfg, loc)
        elif isinstance(loc, GCoord): return self.transform_gcoord(cnfg, loc)
        return None
    def gcd(self, a: int, b: int) -> int32:
        x: int32
        while b != 0:
            x = a%b
            a = b
            b = x
        return a
    

#******************************************************************************
# It is easy to find all temlate sizes needed for the targets in tt table
tm2_2x2  : Tm = Tm(2, 2);
tm3_2x3  : Tm = Tm(2, 3);
tm4_2x4  : Tm = Tm(2, 4);
tm5_2x5  : Tm = Tm(2, 5);
tm6_2x6  : Tm = Tm(2, 6);
tm6_4x3  : Tm = Tm(4, 3);
tm7_2x7  : Tm = Tm(2, 7);
tm8_2x8  : Tm = Tm(2, 8);
tm8_4x4  : Tm = Tm(4, 4);
tm9_2x9  : Tm = Tm(2, 9);
tm9_3x6  : Tm = Tm(3, 6);
tm10_2x10: Tm = Tm(2,10);
tm10_4x5 : Tm = Tm(4, 5);
tm11_2x11: Tm = Tm(2,11);
tm12_2x12: Tm = Tm(2,12);
tm12_4x6 : Tm = Tm(4, 6);
tm12_8x3 : Tm = Tm(8, 3);
tm14_2x14: Tm = Tm(2,14);
tm14_4x7 : Tm = Tm(4, 7);
tm18_2x18: Tm = Tm(2,18); # does not fit tt
tm18_4x9 : Tm = Tm(4, 9);
tm18_3x12: Tm = Tm(3,12);
tm18_6x6 : Tm = Tm(6, 6);
tm20_2x20: Tm = Tm(2,20); # does not fit tt
tm20_4x10: Tm = Tm(4,10);
tm20_8x5 : Tm = Tm(8, 5);
    
#******************************************************************************
# We will need to easily look up all templates by target number.
n2tm: [[Tm]] = [[]]

def init_n2tm() -> None:
    global n2tm
    n2tm = n2tm*21
    n2tm = [[] for i in range(21)]
    e = lambda i, t: n2tm[i].append(t)
    e( 2,tm2_2x2)
    e( 3,tm3_2x3)
    e( 4,tm4_2x4)
    e( 5,tm5_2x5)
    e( 6,tm6_2x6)  
    e( 6,tm6_4x3)
    e( 7,tm7_2x7)  
    e( 8,tm8_2x8)  
    e( 8,tm8_4x4)
    e( 9,tm9_2x9)
    e( 9,tm9_3x6)
    e(10,tm10_2x10)
    e(10,tm10_4x5)
    e(11,tm11_2x11)
    e(12,tm12_2x12)
    e(12,tm12_4x6)
    e(12,tm12_8x3)
    e(14,tm14_2x14)
    e(14,tm14_4x7)
    e(18,tm18_4x9)
    e(18,tm18_3x12)
    e(18,tm18_6x6)
    e(20,tm20_4x10)
    e(20,tm20_8x5)
    
#******************************************************************************
# We will need to print some info
def print_pair(p: GCoord or TCoord): print("(", p.first, ',', p.second, ')', end='')

#******************************************************************************
# Now the algorithmic part.
# fits()  checks if a template in a particular configuration can be used to 
#         cover a given target square.
# place() places a template in a particular configuration 
# hll()   find horizonal leg length
def fits(gps: Gps, tl: TCoord, tm: Tm, cnfg: Config) -> bool:
    # tl   -- target square location
    # tm   -- triangle template
    # cnfg -- triangle configuration
    # assert triangle area == template area / 2 --- wrong template?
    global tt
    global gmsqi
    assert 2*tt[tl.first][tl.second]*scale*scale == tm.v[1].second*tm.v[2].first
    # check if corners are in table, are they in another triangle
    def image(templloc: GCoord):
        al = tm.transform(cnfg, templloc)
        al += t2g(tl) # actual grid location
        return al
    v0 = image(tm.v[0])
    if not in_grid(v0) or gps.is_internal(v0): return 0
    vc = image(tm.v[1])
    if not in_grid(vc) or gps.is_internal(vc): return 0
    vr = image(tm.v[2])
    if not in_grid(vr) or gps.is_internal(vr): return 0

    # check if template guts overlap another triangle or target square
    for gl in tm.g:
        al: GCoord = image(gl)
        mk: TCoord = gmsqi[al.first][al.second]

        if (mk.first != 0 or mk.second != 0) and mk != tl: return False
        if gps.is_internal(al): return False
    return True
        
def place(gps: Gps, tl: TCoord, tm: Tm, cnfg: Config) -> None:
    # place triangle, i.e., mark internal points in gps. 
    # tl   -- target square location
    # tm   -- triangle template
    # cnfg -- triangle configuration
    for gl in tm.g:
        al: GCoord = tm.transform(cnfg, gl)
        al += t2g(tl)
        gps.mark_internal(al)
    
def print_troc(tl: TCoord, tm: Tm, cnfg: Config) -> None:
    # print triangle in original grid coordinates
    # tl   -- target square location
    # tm   -- triangle template
    # cnfg -- triangle configuration
    # assert triangle area == template area / 2 --- wrong template?
    assert 2*tt[tl.first][tl.second]*scale*scale == tm.v[1].second*tm.v[2].first
    for e in tm.v:
        al: GCoord  = tm.transform(cnfg, e)
        al         += t2g(tl) # actual grid location
        al.first   /= scale
        al.second  /= scale
        print("\t", end='')
        print_pair(al)
        
def hll(tl: TCoord, tm: Tm, cnfg: Config) -> int32:
    # find horizontal leg length -- just check the template
    # tl   -- target square location
    # tm   -- triangle template
    # cnfg -- triangle configuration
    # assert triangle area == template area / 2 --- wrong template?
    assert 2*tt[tl.first][tl.second]*scale*scale == tm.v[1].second*tm.v[2].first
    if not cnfg.s: return tm.v[1].second / scale
    else         : return tm.v[2].first  / scale
    
# ******************************************************************************
#  options() finds the toal number of all templates configurations that fit.
#  Let's define data structures that can be used to save options.

class Option():
    def __init__(self, t: Tm = None, cg: Config = Config()):
        self.tm  : Tm     = t 
        self.cnfg: Config = cg
        
topt: [[Option]] = [[]] # target options record

def init_topt() -> None: 
    global topt
    topt = [[] for i in range(len(ltsq))]

def options(gps: Gps, tl: TCoord) -> int32:
    # Finds options that fit and records them in topt, skips covered targets
    global topt
    global n2tm
    global tt
    tn: int = tt[tl.first][tl.second]
    if gps.is_covered(tl): 
        gps.is_covered(tl)
        return 0
    k: int32 = 0
    while ltsq[k] != tl: k += 1 # find the index of tl
    topt[k] = []
    res: int = 0
    for tm in n2tm[tt[tl.first][tl.second]]:
        cnfg: Config = Config()
        def count(res: int):
            for p in range(len(tm.p)):
                cnfg.p = p
                if fits(gps, tl, tm, cnfg): 
                    res += 1
                    topt[k].append(Option(tm,copy.copy(cnfg))) # TODO: maybe need copies
            return res
    
        res = count(res)                 # h==0, v==0
        cnfg.h = True ; res = count(res) # h==1, v==0
        cnfg.v = True ; res = count(res) # h==1, v==1
        cnfg.h = False; res = count(res) # h==0, v==1
        if tm.symmetrical(): continue
        cnfg.s = True ; res = count(res)# h==0, v==1 
        cnfg.v = False; res = count(res)# h==0, v==0
        cnfg.h = True ; res = count(res)# h==1, v==0
        cnfg.v = True ; res = count(res)# h==1, v==1
    return res

def all_options(gps: Gps) -> None:
    # find all options for targets that have not been covered
    global ltsq
    tot: int = 0
    for sq in ltsq: tot += options(gps, sq)

#******************************************************************************
# Solve()
    
def solve() -> None:
    global ltsq
    global topt
    stack : [Gps] = [Gps()] # For placement decisions
    picked: [int] = []      # keeps the sequence of targets covered so far
    
    def pick_target() -> int: # pick target by max size
        Top = stack[-1]
        res: int = -2
        max: int = -1
        for k in range(len(ltsq)):
            t = ltsq[k]
            if Top.is_covered(t): continue
            if len(topt[k]) == 0: return -1
            n = tt[t.first][t.second]
            if n > 0 and n >= max:
                max = n
                res = k
        return res
        
    def put_triangle(pick: int) -> None:
        Top = stack[-1]
        stack.append(Gps(Top))
        vo = topt[pick]
        opt = vo[-1]
        NewTop = stack[-1]
        place(NewTop, ltsq[pick], opt.tm, opt.cnfg)
    
    found: bool = False
    while True:
        skip: bool = False
        all_options(stack[-1])
        pick = pick_target()
        
        if pick < 0:
            if pick < -1:
                found = True
                print(" SOLVED !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! ")
                Top = stack[-1]
                sig: int = 1
                for pck in range(len(picked)):
                    vo = topt[pck]
                    opt = vo[-1]
                    print(ltsq[pck])
                    print_troc(ltsq[pck], opt.tm, opt.cnfg)
                    print("\t", hll(ltsq[pck], opt.tm, opt.cnfg))
                    tmp = hll(ltsq[pck], opt.tm, opt.cnfg)
                    if tmp%2 != 0: sig *= tmp
                print("Answer:", sig)
            
            while True:
                stack.pop()       # erase failed attempt
                pick = picked[-1] # get last target decision
                topt[pick].pop()  # eliminate unsuccessful option
                if len(topt[pick]) != 0: 
                    skip = True # more options for last pick
                    break
                if len(picked) != 0: picked.pop()
                if len(picked) == 0: 
                    if not found: print(" FINISHED !!!!!  NO SOLUTION  !!!!!!!!")
                    else        : print(" FINISHED !!!!!  NO MORE SOLUTIONS  !!!!!!!! ")
                    return
                    
        if not skip: picked.append(pick)
        put_triangle(pick)

def main() -> int32:
    init_ltsq()
    init_n2tm()
    init_gmsqi()
    init_topt()
    solve()
    return 0

if __name__ == "__main__":
    main()
