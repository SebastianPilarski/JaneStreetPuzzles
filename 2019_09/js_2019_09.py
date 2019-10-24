# Jane Street Puzzle. September 2019. 
# see https://www.janestreet.com/puzzles/
 
# Fill each region with the digits 1 though N, where N is the number of
# cells in the given region. For every cell in the grid, if K denotes the
# number in that cell, then the nearest value of K (looking only horizontally
# or vertically) must be located exactly K cells away. (As in the example,s
# below.) Some of the cells have already been filled in.
  
# Once the grid is completed, take the largest “horizontally concatenated
# number” from each region and compute the sum of these values. Enter this sum
# as your answer. Good luck!

import numpy as np

#******************************************************************************
# We need to represent the table from the puzzle. In our version of region
# defining table each region gets its unique number from 0 to 19. An 8-bit
# integer is enough to represent numbers from 0 to 19.
# Also, note that no region has more than 8 squares, so an 8-bit int is more
# than enough to represent "1 through N" stated in the puzzle. 
# To solve the problem, it is conveninent to mark values/digits excluded ats
#  each square. 8-bit unsigned int is sufficient for this task.
#  Thus, we use the following "typedefs".

numb_t = np.int8
mark_t = np.uint8
int32  = np.int32 # More than 32 bits is not needed

#******************************************************************************
# We have a table of 9 x 9 squares
tr: int32 = 9 # table, number of rows
tc: int32 = 9 # table, number of columns

#******************************************************************************
# Our representation of regions -- directly from the puzzle table.
# Each table square is labled with its region id.
# Note that regions are non-overlaping and cover the table

sq2reg = np.array([[ 0,  1,  1,  1,  2,  3,  4,  5,  6 ],
                   [ 0,  7,  1,  2,  2,  2,  4,  6,  6 ],
                   [ 0,  1,  1,  1,  2,  2,  4,  6,  6 ],
                   [ 0,  0,  1,  8,  2,  9,  9,  6,  6 ],
                   [10,  0, 11,  8, 12,  9, 13, 14, 14 ],
                   [10, 10, 11, 11, 12,  9, 13, 14, 14 ],
                   [10, 15, 16, 16, 12, 17, 17, 14, 14 ],
                   [15, 15, 15, 16, 16, 16, 18, 19, 19 ],
                   [15, 16, 16, 16, 18, 18, 18, 18, 19 ] ], dtype=numb_t)

#******************************************************************************
# Region is a collection of squares (list of coordinate pairs)
regs: [[(int32, int32)]] # lists of regions, to be initialized

#******************************************************************************
# some squares have initially known values

known = np.array([[ 0,  0,  0,  0,  0,  0,  0,  0,  0 ],
                  [ 0,  0,  0,  0,  0,  0,  0,  1,  0 ],
                  [ 0,  3,  0,  0,  0,  0,  0,  0,  0 ],
                  [ 0,  0,  0,  0,  0,  0,  0,  0,  2 ],
                  [ 0,  0,  0,  0,  1,  0,  0,  0,  0 ],
                  [ 2,  0,  0,  0,  0,  0,  0,  0,  0 ],
                  [ 0,  0,  0,  0,  0,  0,  0,  4,  0 ],
                  [ 0,  2,  0,  0,  0,  0,  0,  0,  0 ],
                  [ 0,  0,  0,  0,  0,  0,  0,  0,  0 ] ], dtype=numb_t)

#******************************************************************************
# solution found "by hand"

knownH = np.array([[ 4,  7,  8,  1,  1,  1,  3,  1,  7 ],
                   [ 1,  1,  2,  7,  2,  6,  1,  1,  3 ],
                   [ 5,  3,  6,  4,  3,  5,  2,  4,  6 ],
                   [ 3,  2,  5,  2,  4,  2,  3,  5,  2 ],
                   [ 4,  6,  3,  1,  1,  4,  2,  6,  3 ],
                   [ 2,  1,  2,  1,  3,  1,  1,  1,  2 ],
                   [ 3,  1,  1,  5,  2,  1,  2,  4,  5 ],
                   [ 5,  2,  3,  2,  4,  6,  1,  1,  3 ],
                   [ 4,  3,  8,  7,  3,  4,  2,  5,  2 ] ], dtype=numb_t)

#******************************************************************************
# To solve the problem it is conveninent to keep track of known/assumed
# values and excluded values for each sqare. At this stage we may not know
# if all subsequent values are implied, or we will have to apply a
# "try-and-error" procedure. Thus, compuational state should not be
# defined as a static table -- we may need a stack of them.

class state:
    def __init__(self, a = None):
        self.n = np.empty((tr, tc), dtype=numb_t) # known or assumed values
        self.x = np.empty((tr, tc), dtype=mark_t) # marks excluded values
        
        if a is not state:
            # x - excludes values larger than reg size
            # n - according to known
            for i in range(tr):
                for j in range(tc):
                    self.set_n(i, j, known[i][j])
        else: 
            # If decision (trial & error) is needed - was not in this problem 
            np.copyto(self.n, a.n)
            np.copyto(self.x, a.x)
    
    def set_n(self, r: int32, c: int32, m: numb_t) -> None:
        assert m >= 0 and m <= len(regs[sq2reg[r][c]])
        self.n[r][c] = m
        if m == 0: self.x[r][c] = ~0 & ~((0x01 << len(regs[sq2reg[r][c]])) - 1)
        else     : self.x[r][c] = ~(0x01 << (m-1)) # exclude all but m
    
    def constrain(self, r: int32, c: int32) -> int32:
        # excludes the value of n[r][c] around n[r][c] (up,down,left,right)
        # excludes the value of n[r][c] in the region of (r,c)
        # returns -1 if it detects assignment conflicts, 1 otherwise 
        assert self.n[r][c] != 0 and self.n[r][c] <= len(regs[sq2reg[r][c]])
        m           = np.copy(self.n[r][c])
        bit: mark_t = 0x01 << (m-1)
        
        def set(i: int32, j: int32) -> bool:
            if self.n[i][j] != 0: return self.n[i][j] != m # false if same value too close
            self.x[i][j] |= bit
            return ~self.x[i][j] != 0 # false if all values excluded
            
        if m > 1:
            for i in range(r+1, min(tr, r+m)): 
                if not set(i, c): return -1
            for i in range(r-1, max(-1, r-m), -1):
                if not set(i, c): return -1
            for j in range(c+1, min(tr, c+m)):
                if not set(r,j): return -1
            for j in range(c-1, max(-1, c-m), -1):
                if not set(r,j): return -1
        
        for e in regs[sq2reg[r][c]]:
            i = e[0]
            j = e[1]
            if r != i or c != j:
                if not set(i,j): return -1
        
        return 1
        
        
    def x_choices(self, r: int32, c: int32) -> int32:
        # find how many choices we have for n[r][c] accoring to x[r][c]
        # return -1 if all choices are excluded
        cntr: int32 = 0
        if self.n[r][c] == 0:
            w  = np.copy(self.x[r][c])
            s  = np.copy(sq2reg[r][c])
            ss = len(regs[s])
            for k in range(ss): cntr += 0 if ((w >> k) & 0x01) != 0 else 1
            if cntr == 0: cntr = -1
        return cntr
        
    def z_choices(self, r: int32, c: int32) -> int32:
        # find what choices we have to satisfy min distance
        op: int32 = 0
        if self.n[r][c] != 0:
            m = np.copy(self.n[r][c])
            bit: mark_t = 0x01 << (m-1)
            if r-m >= 0: 
                i: int32  = r-m
                if self.n[i][c] == m: return 0
                if self.n[i][c] == 0 and (self.x[i][c] & bit) == 0: op |= 0x01
            if r+m < tr: 
                i: int32  = r+m
                if self.n[i][c] == m: return 0
                if self.n[i][c] == 0 and (self.x[i][c] & bit) == 0: op |= 0x02
            if c-m >= 0: 
                j: int32  = c-m
                if self.n[r][j] == m: return 0
                if self.n[r][j] == 0 and (self.x[r][j] & bit) == 0: op |= 0x04
            if c+m < tc: 
                j: int32  = c+m
                if self.n[r][j] == m: return 0
                if self.n[r][j] == 0 and (self.x[r][j] & bit) == 0: op |= 0x08
        return op
        
    def messageX(self, r: int32, c: int32, m: int32) -> None: print("X implied", m, " at (", r, ",", c, "}")
    def messageR(self, r: int32, c: int32, m: int32) -> None: print("R implied", m, " at (", r, ",", c, "}")
    def messageZ(self, r: int32, c: int32, m: int32) -> None: print("Z implied", m, " at (", r, ",", c, "}")
    def messageY(self, r: int32, c: int32, m: int32) -> None: print("X excluded", m, " at (", r, ",", c, "}") 
            
    def x_imply(self, r: int32, c: int32) -> int32:
        # find a new implication due to exclusion of all but one value
        # return : 0 = no implication
        #          1 = new implication, no conflicts from constrain()
        #         -1 = new implication, a  conflict  from constrain()
        choices: int32 = self.x_choices(r,c)
        if choices < 0: return -1 # already a conflict?
        if choices < 1: return  0 # already chosen
        if choices > 1: return  0 # too many, no implication
        
        w  = np.copy(self.x[r][c])
        s  = np.copy(sq2reg[r][c])
        ss = len(regs[s])
            
        k:int32 = 0
        while k < ss:
            if (w >> k) & 0x01 == 0:
                k += 1
                break
            k += 1
        self.set_n(r, c, k)
        assert k > 0 and k <= ss
        self.messageX(r, c, k)
        return self.constrain(r,c)
        
    def r_imply(self, r: int32, c: int32) -> int32:
        # find a new implication due to exclusion in the rest of the region
        # return : 0 = no implication
        #          1 = new implication, no conflicts from constrain()
        #         -1 = new implication, a  conflict  from constrain()
        if self.n[r][c] != 0: return 0
        w = ~np.copy(self.x[r][c])
        s = np.copy(sq2reg[r][c])
        ss = len(regs[s])
        y: mark_t = w # finds values possible to set only at (r,c)
        for e in regs[s]:
            i = e[0]
            j = e[1]
            if i == r and j == c: continue
            y &= self.x[i][j] # eliminate bits not excluded elsewhere
        if y == 0: return 0 # no unique values at (r,c)
        m   : int32 = 0
        cntr: int32 = 0
        for k in range(0, ss):
            if (y >> k) & 0x01 != 0: 
                cntr += 1
                m     = k+1
        if cntr > 1: return -1
        self.messageR(r, c, m)
        self.set_n(r, c, m)
        return 1
        
    def z_imply(self, r: int32, c: int32) -> int32:
        # find a new implication due to unique min distance satifiability
        # return : 0 = no implication
        #          1 = new implication, no conflicts from constrain()
        #         -1 = new implication, a  conflict  from constrain()
        m : int32 = np.copy(self.n[r][c])
        op: int32 = self.z_choices(r,c)
        if op == 0x01: 
            self.messageZ(r-m, c, m)
            self.set_n(r-m, c, m)
            return self.constrain(r-m, c)
        if op == 0x02: 
            self.messageZ(r+m, c, m)
            self.set_n(r+m, c, m)
            return self.constrain(r+m, c)
        if op == 0x04: 
            self.messageZ(r, c-m, m)
            self.set_n(r, c-m, m)
            return self.constrain(r, c-m)
        if op == 0x08: 
            self.messageZ(r, c+m, m)
            self.set_n(r, c+m, m)
            return self.constrain(r, c+m)
        return 0
        
    def z_constrain(self, r: int32, c: int32, m: int32) -> int32:
        # find a new constrain due to min distance unsatisfiability
        # return : 0 = no new constrain
        #          1 = new constrain, no conflicts from x[r][c]
        #         -1 = new constrain, a  conflict  from x[r][c]
        if self.n[r][c] != 0: return 0
        bit: mark_t = mark_t(0x01 << (m-1))
        if self.x[r][c] & bit != 0: return 0 # if already excluded
        
        if r-m >= 0: 
            i: int32 = r-m
            if sq2reg[r][c] != sq2reg[i][c] and self.x[i][c] & bit == 0: return 0
        if r+m < tr: 
            i: int32 = r+m
            if sq2reg[r][c] != sq2reg[i][c] and self.x[i][c] & bit == 0: return 0
        if c-m >= 0: 
            j: int32 = c-m
            if sq2reg[r][c] != sq2reg[r][j] and self.x[r][j] & bit == 0: return 0
        if c+m < tc: 
            j: int32 = c+m
            if sq2reg[r][c] != sq2reg[r][j] and self.x[r][j] & bit == 0: return 0
        self.messageY(r,c,m)
        self.x[r][c] |= bit
        return -1 if ~self.x[r][c] == 0 else 1
        
    def x_imply_all(self) -> int32:
        # apply all possible x implications
        cntr: int32  = 0
        ok  : bool = True #  no-conflict
        for i in range(tr):
            if not ok: break
            for j in range(tc):
                if not ok: break
                res   = self.x_imply(i,j)
                ok   &= res >= 0
                cntr += res ==1
        return cntr if ok else -1
        
    def r_imply_all(self) -> int32:
        # apply all possible r implications
        cntr: int32  = 0
        ok  : bool = True  # no-conflict
        for i in range(tr):
            if not ok: break
            for j in range(tc):
                if not ok: break
                res   = self.r_imply(i,j)
                ok   &= res >= 0
                cntr += res == 1
        return cntr if ok else -1
        
    def z_imply_all(self) -> int32:
        # apply all possible z implications
        cntr: int32  = 0
        ok  : bool = True # no-conflict
        for i in range(tr):
            if not ok: break
            for j in range(tc):
                if not ok: break
                res   = self.z_imply(i,j)
                ok   &= res >= 0
                cntr += res == 1
        return cntr if ok else -1
        
    def z_constrain_all(self) -> int32:
        # apply all possible z constraints
        cntr: int32 = 0
        ok  : bool  = True
        for i in range(tr):
            if not ok: break
            for j in range(tc):
                if not ok: break
                for m in range(1, len(regs[sq2reg[i][j]])+1):
                    res   = self.z_constrain(i,j, m)
                    ok   &= res >= 0
                    cntr += res ==1
        return cntr if ok else -1
            
    def imply_all(self) -> int32:
        # find all possible implications and constraints
        cntr : int32 = 0 # total number of implications
        delta: int32 = 0 # change in cntr;
        res  : int32 = 0 # partial result
        while True:
            delta = 0
            res   = self.x_imply_all()
            if res < 0: return -1
            delta += res
                
            res   = self.r_imply_all()
            if res < 0: return -1
            delta += res
                
            res   = self.z_imply_all()
            if res < 0: return -1
            delta += res
                
            res = self.z_constrain_all()
            if res < 0: return -1
            cntr += delta
                
            if not (delta > 0 or res > 1): break
        print("result:", cntr)
        return cntr
            
    def print(self) -> None:
        # print known/assumed values, show region boundaries
        for i in range(tr):
            if i == 0: print("------------------------------------")
            for j in range(tc):
                if j == 0 or regs[sq2reg[i][j-1]] != regs[sq2reg[i][j]]: print("| ", end='')
                else: print("  ", end='')
                print(abs(self.n[i][j]), end=' ') 
            print()
            if i+1 == tr: print("------------------------------------")
            else:
                for j in range(tc): 
                    if regs[sq2reg[i+1][j]] != regs[sq2reg[i][j]]: print(" ---", end='')  
                    else: print("    ", end='')
            print()
            
    def printx(self) -> None:
        # print excluded values
        for i in range(tr):
            for j in range(tc):
                for k in range(8):
                    b: int32 = 0x01 & (self.x[i][j] >> k)
                    print(b, end='')
                print("  ", end='')
            print()


def init_regs() -> None:
    global regs
    regs = [[] for i in range(20)]
    for i in range(tr):
        for j in range(tc):
            regs[sq2reg[i][j]].append((i,j))
                
def sum(st: state) -> int32:
    sig: int32 = 0
    for sh in regs:
        max: int32 = 0
        for e in sh:
            s: int32 = 0
            i: int32 = e[0]
            j: int32 = e[1]
            while j > 0 and sq2reg[i, j-1] == sq2reg[i,j]: j = j-1
            while True:
                s *= 10
                s += st.n[i][j]
                j += 1
                if not (j < tc and sq2reg[i][j-1] == sq2reg[i][j]): break
            s = abs(s)
            if s > max: max = s
        print("max: ", max)
        sig += max
    print("SIG: ", sig)

def main() -> int32:
    init_regs()
    first: state = state()
    first.print()
    
    ok: bool = True
    for i in range(tr):
        if not ok: break
        for j in range(tc):
            if not ok: break
            if first.n[i][j] > 0: ok &= first.constrain(i,j)
    first.printx()
    first.imply_all()
    first.printx()
    first.print() 
    # The problem is solved through simple implications (no need for trial and error)
    sum(first)
    return 0

if __name__ == "__main__":
    main()
        
