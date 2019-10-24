#include <iostream>
#include <vector>
#include <algorithm>
#include <cassert>

using namespace std;
/*
  Jane Street Puzzle. September 2019. 
  see https://www.janestreet.com/puzzles/
 
  Fill each region with the digits 1 though N, where N is the number of
  cells in the given region. For every cell in the grid, if K denotes the
  number in that cell, then the nearest value of K (looking only horizontallys
  or vertically) must be located exactly K cells away. (As in the example,s
  below.) Some of the cells have already been filled in.
  
  Once the grid is completed, take the largest “horizontally concatenate
  number” from each region and compute the sum of these values. Enter this sum
  as your answer. Good luck!
*/

//******************************************************************************
// We need to represent the table from the puzzle. In our version of region
// defining table each region gets its unique number from 0 to 19. An 8-bit
// integer is enough to represent numbers from 0 to 19.
// Also, note that no region has more than 8 squares, so an 8-bit int is more
// than enough to represent "1 through N" stated in the puzzle. 
// To solve the problem, it is conveninent to mark values/digits excluded ats
//  each square. 8-bit unsigned int is sufficient for this task.
//  Thus, we use the following typedefs.

typedef int8_t   numb_t; // to represent "1 through N" or region number
typedef uint8_t  mark_t; // excluded values marks

//******************************************************************************
// We have a table of 9 x 9 squares

constexpr int tr = 9; // table, number of rows
constexpr int tc = 9; // table, number of columns

//******************************************************************************
// Our representation of regions -- directly from the puzzle table.
// Each table square is labled with its region id.
// Note that regions are non-overlaping and cover the table
                          
constexpr numb_t
       sq2reg[tr][tc] = { { 0,  1,  1,  1,  2,  3,  4,  5,  6 },
                          { 0,  7,  1,  2,  2,  2,  4,  6,  6 },
                          { 0,  1,  1,  1,  2,  2,  4,  6,  6 },
                          { 0,  0,  1,  8,  2,  9,  9,  6,  6 },
                          {10,  0, 11,  8, 12,  9, 13, 14, 14 },
                          {10, 10, 11, 11, 12,  9, 13, 14, 14 },
                          {10, 15, 16, 16, 12, 17, 17, 14, 14 },
                          {15, 15, 15, 16, 16, 16, 18, 19, 19 },
                          {15, 16, 16, 16, 18, 18, 18, 18, 19 } };

//******************************************************************************
// Region is a collection of squares (vector of coordinate pairs)
vector<vector<pair<int,int>>> regs; // lists of regions, to be initialized

//******************************************************************************
// some squares have initially known values

constexpr numb_t
     known[tr][tc]   =  { { 0,  0,  0,  0,  0,  0,  0,  0,  0 },
                          { 0,  0,  0,  0,  0,  0,  0,  1,  0 },
                          { 0,  3,  0,  0,  0,  0,  0,  0,  0 },
                          { 0,  0,  0,  0,  0,  0,  0,  0,  2 },
                          { 0,  0,  0,  0,  1,  0,  0,  0,  0 },
                          { 2,  0,  0,  0,  0,  0,  0,  0,  0 },
                          { 0,  0,  0,  0,  0,  0,  0,  4,  0 },
                          { 0,  2,  0,  0,  0,  0,  0,  0,  0 },
                          { 0,  0,  0,  0,  0,  0,  0,  0,  0 } };
                          

//******************************************************************************
// solution found "by hand"
                          
constexpr numb_t  // 
     knownH[tr][tc]  =  { { 4,  7,  8,  1,  1,  1,  3,  1,  7 },
                          { 1,  1,  2,  7,  2,  6,  1,  1,  3 },
                          { 5,  3,  6,  4,  3,  5,  2,  4,  6 },
                          { 3,  2,  5,  2,  4,  2,  3,  5,  2 },
                          { 4,  6,  3,  1,  1,  4,  2,  6,  3 },
                          { 2,  1,  2,  1,  3,  1,  1,  1,  2 },
                          { 3,  1,  1,  5,  2,  1,  2,  4,  5 },
                          { 5,  2,  3,  2,  4,  6,  1,  1,  3 },
                          { 4,  3,  8,  7,  3,  4,  2,  5,  2 } };


//******************************************************************************
// To solve the problem it is conveninent to keep track of known/assumed
// values and excluded values for each sqare. At this stage we may not know
// if all subsequent values are implied, or we will have apply a
// "try-and-error" procedure. Thus, compuational state should not be
// defined as a static table -- we may need a stack of them.
                          
struct state {
    numb_t n[tr][tc]; // known or assumed value
    mark_t x[tr][tc]; // marks excluded values
    void set_n(int r, int c, int m) { 
      assert(m >= 0 && m <= regs[sq2reg[r][c]].size() );
      n[r][c] = m;
      if ( !m ) x[r][c] = ~0 & ~((0x01 << regs[sq2reg[r][c]].size()) - 1);
      else      x[r][c] = ~(0x01 << (m-1)); // exclude all but m
//       cout << r << ' ' << c << ' ' << m << ' ' << (int)x[r][c] << endl;
    }
  public:
    state() {
      // x - excludes values larger than reg size
      // n - according to known;
      for ( int i = 0; i < tr; i++ ) {
        for ( int j = 0; j < tc; j++ ) {
          set_n(i,j,known[i][j]); 
        }
      }
    }
    state(const state& a)         { copy(a.n[0],a.n[0]+tr*tc,n[0]);  
                                    copy(a.x[0],a.x[0]+tr*tc,x[0]); } 
    void operator=(const state& a){ copy(a.n[0],a.n[0]+tr*tc,n[0]);
                                    copy(a.x[0],a.x[0]+tr*tc,x[0]); } 
    int constrain(int r, int c) { 
      // excludes the value of n[r][c] around n[r][c] (up,down,left,right)
      // excludes the value of n[r][c] in the region of (r,c)
      // returns -1 if it detects assignment conflicts, 1 otherwise 
      assert( n[r][c] != 0 && n[r][c] <= (int)regs[sq2reg[r][c]].size());
      const auto   m   = n[r][c];
      const mark_t bit = 0x01 << (m-1);
      
      auto set = [this,m,bit](int i, int j){
        if ( n[i][j] ) return n[i][j] != m; // false if same value too close
        mark_t& e  = x[i][j]; 
                e |= bit;
        return ~e != 0; // false if all values excluded
      };
      if ( m > 1 ) { 
        for ( auto i = r + 1; i <  tr && i < r + m; i++ ) if(!set(i,c)) return -1;
        for ( auto i = r - 1; i >=  0 && i > r - m; i-- ) if(!set(i,c)) return -1;
        for ( auto j = c + 1; j <  tr && j < c + m; j++ ) if(!set(r,j)) return -1;
        for ( auto j = c - 1; j >=  0 && j > c - m; j-- ) if(!set(r,j)) return -1;
      }
      for ( auto e : regs[sq2reg[r][c]] ) {
        auto i = e.first;
        auto j = e.second;
        if ( r != i || c != j ) if(!set(i,j)) return -1;
      }
      return true;
    }
    int x_choices (int r, int c) const {
      // find how many choices we have for n[r][c] accoring to x[r][c]
      // return -1 if all choices are excluded
      int cntr = 0;
      if ( n[r][c] == 0 ) {
        const auto w  = x[r][c];
        const auto s  = sq2reg[r][c];
        const auto ss = regs[s].size();
        for ( int k = 0; k < ss; k++ ) cntr += ((w >> k) & 0x01) ? 0 : 1;
//         cout << "ss " << ss << endl;
//         cout << "cntr " << cntr << endl;
//         cout << "w " << (int)w << endl;
        if ( !cntr ) cntr = -1;
      }
      return cntr;
    }
    int z_choices ( int r, int c) {
      // find what choices we have to satisfy min distance
      int op = 0;
      if ( n[r][c] ) {
        const auto   m   = n[r][c];
        const mark_t bit = 0x01 << ( m-1);
        if ( r - m >= 0 ) { int i = r - m;
          if ( n[i][c] == m ) return 0;
          if ( n[i][c] == 0 && (x[i][c] & bit) == 0) op |= 0x01;
        }
        if ( r + m < tr ) { int i = r + m;
          if ( n[i][c] == m ) return 0;
          if ( n[i][c] == 0 && (x[i][c] & bit) == 0) op |= 0x02;
        }
        if ( c - m >= 0 ) { int j = c - m;
          if ( n[r][j] == m ) return 0;
          if ( n[r][j] == 0 && (x[r][j] & bit) == 0) op |= 0x04;
        }
        if ( c + m < tc ) { int j = c + m;
          if ( n[r][j] == m ) return 0;
          if ( n[r][j] == 0 && (x[r][j] & bit) == 0) op |= 0x08;
        }
      }
      return op;
    }
    void messageX(int r, int c, int m) {
        cout << "X implied " << m << " at (" << r << "," <<c<<"}" << endl;
    }
    void messageR(int r, int c, int m) {
        cout << "R implied " << m << " at (" << r << "," <<c<<"}" << endl;
    }
    void messageZ(int r, int c, int m) {
        cout << "Z impled " << m << " at (" << r << "," <<c<<"}" << endl;
    }
    void messageY(int r, int c, int m) {
        cout << "X excluded " << m << " at (" << r << "," <<c<<"}" << endl;
    }
    int x_imply( int r, int c ) {
      // find a new implication due to exclusion of all but one value
      // return : 0 = no implication
      //          1 = new implication, no conflicts from constrain()
      //         -1 = new implication, a  conflict  from constrain()
      const int choices = x_choices(r,c);
      if ( choices < 0 ) return -1; // already a conflict?
      if ( choices < 1 ) return  0; // already cosen
      if ( choices > 1 ) return  0; // too many, no implication

      const auto w  = x[r][c];
      const auto s  = sq2reg[r][c];
      const auto ss = regs[s].size();
      int k = 0;
      for ( ; k < ss; k++ ) if ( ((w >> k) & 0x01) == 0 ) { k++; break; }
      set_n(r,c,k); assert( k > 0 && k <= ss );
      messageX(r,c,k);
      return constrain(r,c);
    }
    int r_imply( int r, int c ) {
      cout << "IN r_imply" << endl;
      // find a new implication due to exclusion in the rest of the region
      // return : 0 = no implication
      //          1 = new implication, no conflicts from constrain()
      //         -1 = new implication, a  conflict  from constrain()
      if ( n[r][c] ) return 0;
      const auto w  = ~x[r][c]; // w identifies possible values
//       for(auto i = 0; i < 9; i++) {
//           for(auto j = 0; j < 9; j++) {
//               cout << (int)x[i][j] << " ";
//         }
//         cout << endl;
//     } 
//       cout << "W: " << (char)w << endl;
      const auto s  = sq2reg[r][c];
      const auto ss = regs[s].size();
      mark_t     y  = w; // finds values possible to set only at (r,c)
      for ( auto e : regs[s] ) {
        auto i = e.first; 
        auto j = e.second;
//         cout << "i,r,j,c " << i << " " << r << " " << j << " " << c << endl;
        if ( i == r && j == c ) continue;
        y &= x[i][j]; // eliminate bits not excluded elsewhere
//         cout << "y: " << (int)y << endl;
      }
      if ( y == 0 ) return 0; // no unique values at (r,c)
      int m    = 0;
      int cntr = 0;
      for ( int k = 0; k < ss; k++ ) {
//         cout << "Y >> k: " << ((y >> k) & 0x01) << endl;
        if ( (y >> k) & 0x01 ) { cntr++; m = k+1; }
      }
      if ( cntr > 1 ) return -1;
      messageR(r,c,m);
      set_n(r,c,m);
      return 1;
    }
    int z_imply( int r, int c ) {
      // find a new implication due to unique min distance satifiability
      // return : 0 = no implication
      //          1 = new implication, no conflicts from constrain()
      //         -1 = new implication, a  conflict  from constrain()
      const int m  = n[r][c];
            int op = z_choices ( r,c );
      if ( op == 0x01 ) { messageZ(r-m,c,m); set_n(r-m,c,m); return constrain( r-m,c ); }
      if ( op == 0x02 ) { messageZ(r+m,c,m); set_n(r+m,c,m); return constrain( r+m,c ); }
      if ( op == 0x04 ) { messageZ(r,c-m,m); set_n(r,c-m,m); return constrain( r,c-m ); }
      if ( op == 0x08 ) { messageZ(r,c+m,m); set_n(r,c+m,m); return constrain( r,c+m ); }
      return false;
    }
    int z_constrain( int r, int c, int m) { 
      // find a new constrain due to min distance unsatisfiability
      // return : 0 = no new constrain
      //          1 = new constrain, no conflicts from x[r][c]
      //         -1 = new constrain, a  conflict  from x[r][c]
//       cout << "n" << endl;
//       for(auto i = 0; i < 9; i++) {
//           for(auto j = 0; j < 9; j++) {
//               cout << (int)n[i][j] << " ";
//         }
//         cout << endl;
//       }
      
      if ( n[r][c] ) return 0;

      const mark_t bit = 0x01 << (m-1);
//       cout << "r,c,m" << r << " " << c << " " << m << endl;
      
      if ( x[r][c] & bit ) return 0; // if already excluded

      if ( r - m >= 0 ) { int i = r - m; if (( x[i][c] & bit ) == 0 ) return 0; }
      if ( r + m < tr ) { int i = r + m; if (( x[i][c] & bit ) == 0 ) return 0; }
      if ( c - m >= 0 ) { int j = c - m; if (( x[r][j] & bit ) == 0 ) return 0; }
      if ( c + m < tc ) { int j = c + m; if (( x[r][j] & bit ) == 0 ) return 0; }
      messageY(r,c,m);
      x[r][c] |= bit; 
      cout << "after bit " << (int)x[r][c] << endl;
      for(auto i = 0; i < 9; i++) {
          for(auto j = 0; j < 9; j++) {
              cout << (int)x[i][j] << " ";
        }
        cout << endl;
      }
      return ~x[r][c] == 0 ? -1 : 1;
    }
    int x_imply() // apply all possible x implications
    {
      cout << "In x_imply" << endl;
      int  cntr = 0;
      bool ok   = true; // no-conflic
      for ( int i = 0; i < tr && ok; i++ ) {
        for ( int j = 0; j < tc && ok; j++ ) {
          auto res = x_imply(i,j);
          cout << "i,j " << i << " " << j << endl;
          ok   &= res >= 0;
          cntr += res == 1;
        }
      }
      cout << "x_imply result: " << ok << ' '<< cntr << endl;
      return ok ? cntr : -1;
    }
    int r_imply() // apply all possible r implications
    {
      int  cntr = 0;
      bool ok   = true; // no-conflic
      for ( int i = 0; i < tr && ok; i++ ) {
        for ( int j = 0; j < tc && ok; j++ ) {
          auto res = r_imply(i,j);
          ok   &= res >= 0;
          cntr += res == 1;
        }
      }
      return ok ? cntr : -1;
    }
    int z_imply() // apply all possible z implications
    {
      int  cntr = 0;
      bool ok   = true; // no-conflic
      for ( int i = 0; i < tr && ok; i++ ) {
        for ( int j = 0; j < tc && ok; j++ ) {
          auto res = z_imply(i,j);
          ok   &= res >= 0;
          cntr += res == 1;
        }
      }
      return ok ? cntr : -1;
    }
    int z_constrain() // apply all possible z constraints
    {
      int  cntr = 0;
      bool ok   = true; // no-conflic
      for ( int i = 0; i < tr && ok; i++ ) {
        for ( int j = 0; j < tc && ok; j++ ) {
          for ( int m = 1; m <= regs[sq2reg[i][j]].size(); m++ ) {
            auto res = z_constrain(i,j,m);
            ok   &= res >= 0;
            cntr += res == 1;
          }
        }
      }
      return ok ? cntr : -1;
    }
    int imply() // find all possible implications and constraints
    {
      int cntr  = 0; // total number of implications
      int delta = 0; // change in cntr;
      int res   = 0; // partial result
      do {
        delta = 0;
        res = x_imply(); if ( res < 0 ) return -1; delta += res;
        res = r_imply(); if ( res < 0 ) return -1; delta += res;
        res = z_imply(); if ( res < 0 ) return -1; delta += res;
        res = z_constrain(); if ( res < 0 ) return -1;
        cntr += delta;
      } while ( delta > 0 || res > 1 );
      cout << "result: " << cntr << endl;
      return cntr;
    }

    void print() const { // print known/assumed values, show region boundaries
      for ( int i = 0; i < tr; i++ ) {
        if ( !i ) cout << "------------------------------------" << endl;
        for ( int j = 0; j < tc; j++ ) {
            cout << ((!j || regs[sq2reg[i][j-1]] != regs[sq2reg[i][j]]) ? "| " : "  ");
            cout << abs(n[i][j]) << ' ';
        }
        cout << endl;
        if ( i + 1 == tr ) cout << "------------------------------------" << endl;
        else
        for ( int j = 0; j < tc; j++ ) {
            cout << (( regs[sq2reg[i+1][j]] != regs[sq2reg[i][j]]) ? " ---" : "    ");
        }
        cout << endl;
      }
    }
    void printx() const { // print excluded values
      for ( int i = 0; i < tr; i++ ) {
        for ( int j = 0; j < tc; j++ ) {
            for ( int k = 0; k < 8; k++ ) {
//                 cout << (int)x[i][j] << endl;
                int b = 0x01 & ( x[i][j] >> k ); 
//                 cout << b;
            }
            cout << "  ";
        }
        cout << endl;
      }
    }
};

void init_regs()
{
  regs.resize(20);
  for ( int i = 0; i < tr; i++ ) {
    for ( int j = 0; j < tc; j++ ) {
        regs[sq2reg[i][j]].emplace_back(i,j);
    }
  }
}

int sum( const state& st) {
  int sig = 0;
  for ( auto sh : regs ) {
    int max = 0;
    for ( auto e : sh ) {
      int s = 0;
      int i = e.first;
      int j = e.second;
      while ( j && sq2reg[i,j-1] == sq2reg[i,j] ) j--; 
      do {
        s *= 10;
        s += st.n[i][j];
        //cout << "loop - reg: " << sq2reg[i][j] << " i: " << i << " j: " << j << " s: " << s << " sh': " << sq2reg[i][j+1] << endl; 
        j++;
        //cout << "loop - reg: " << sq2reg[i][j-1] << " -> " << sq2reg[i][j] << endl; 
        //cout << "loop - reg: " << (sq2reg[i][j-1] == sq2reg[i][j]) << endl; 
        //cout << "loop - cond:  " << (j < tc) << " " << ( sq2reg[i,j-1] == sq2reg[i,j] ) << " " << (j <  tc && sq2reg[i,j-1] == sq2reg[i,j]) << endl;
      }
      while ( j <  tc && sq2reg[i][j-1] == sq2reg[i][j] );
      s = abs(s);
      if ( s > max ) max = s;
    }
    cout << "max: " << max << endl;
    sig += max;
  }
  cout << "SIG: " << sig << endl;
}

int main(int argc, char **argv) {
    cout << "Hello, world!" << endl;
    init_regs();
    
    state first;
    first.print();
    // apply initial constrains
    bool ok = true;
    for ( int i = 0; i < tr && ok; i++ ) {
      for ( int j = 0; j < tc && ok; j++ ) {
          if ( first.n[i][j] > 0 ) {
//               cout << i << " " << j << endl;
              ok &= first.constrain(i,j);
          }
      }
    }
    cout << "ok: " << ok << endl;
    first.printx();
    first.imply();
    first.printx();
    first.print();
    sum(first);
    return 0;
}
