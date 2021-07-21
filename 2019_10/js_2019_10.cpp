#include <iostream>
#include <vector>
#include <algorithm>
#include <cassert>

using namespace std;

/*
  Jane Street Puzzle. October 2019. 
  see https://www.janestreet.com/puzzles/
  
  Place a collection of right triangles into the grid below. 
  The triangles must have integer-length legs, and the legs must be along 
  grid lines.
  
  Each triangle must contain exactly one number. That number represents the 
  area of the triangle containing it. (Every number must eventually be 
  contained in exactly one triangle.) The entire square (1-by-1 cell) 
  containing the number must be inside the triangle.
  
  Triangles’ interiors may not overlap. (But triangles’ boundaries may 
  intersect, as seen in the example.)
  
  As your answer to this month’s puzzle, please send in the product of the 
  odd horizontal leg lengths.
*/

//******************************************************************************
// Let's retype target-square table given in the puzzle.
// Below, empty squares are represented as 0s and they are non-target.

constexpr int tor = 17; // n of original table rows
constexpr int toc = 17; // n of original columns

constexpr int tt[tor][toc] = // target table from the puzzle
{ {  0,  0,  0,  0,  0,  0,  0,  0,  0,  8,  0,  0,  0,  2,  0,  0,  0, },
  {  0,  0,  0, 12,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  7,  0,  0, },
  {  4,  0,  0,  0,  0, 10,  0,  0,  0,  0,  0,  0,  3,  0,  0,  0,  0, },
  {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  5, },
  {  0,  0,  0,  0,  0,  0,  0,  7,  0,  0,  0,  0,  0, 10,  0,  0,  0, },
  {  0,  0,  3,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, },
  {  0,  7,  0,  0,  0,  0,  0,  0,  0,  0,  0,  3,  0,  0,  0,  0,  3, },
  {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, },
  {  0,  0,  0,  0,  0,  0,  0,  0, 20,  0,  0,  0,  0,  0,  0,  0,  0, },
  {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, },
  {  4,  0,  0,  0,  0, 14,  0,  0,  0,  0,  0,  0,  0,  0,  0, 18,  0, },
  {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8,  0,  0, },
  {  0,  0,  0,  9,  0,  0,  0,  0,  0, 11,  0,  0,  0,  0,  0,  0,  0, },
  {  6,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, },
  {  0,  0,  0,  0,  3,  0,  0,  0,  0,  0,  0,  7,  0,  0,  0,  0,  6, },
  {  0,  0, 12,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  4,  0,  0,  0, },
  {  0,  0,  0,  2,  0,  0,  0, 18,  0,  0,  0,  0,  0,  0,  0,  0,  0, }, };
  
//******************************************************************************
// When writing the code it will be convenient to use table coordinates wrapped
// in a single object rather than two single numbers.
// Table coordinates are defined as follows

struct TCoord : public pair<int,int> { // Table Coordinates
  void swap() { std::swap(first,second); }
  void operator+=(const TCoord& o) {
    first  += o.first;
    second += o.second;
  }
  TCoord()             { first = 0; second = 0; }
  TCoord(int i, int j) { first = i; second = j; }
};

inline bool in_table( const TCoord& loc ) {
  return loc.first >= 0   && loc.second >= 0 &&
         loc.first <  tor && loc.second <  toc;
}

//******************************************************************************
// For convenience, let's create a list of target square coordinates

vector<TCoord> ltsq; // list of target squares; list of coordinates

void init_ltsq() {
  for ( int i = 0; i < tor; i++ )
    for ( int j = 0; j < toc; j++ ) 
      if ( tt[i][j] ) {
        ltsq.emplace_back(i,j);
      }
}

//******************************************************************************
// To solve the problem we look at the grid of tt square corners
// Note, however, that triangle interiors cannot overlap and we will need
// some functionality to check it. This problem can be solved in various
// ways. Conceptually, the easiest way is to make the grid more  -
// dense enough to make any two overlaping triangles share an internal
// point, i.e. point that does not belong to the triangle's line segments.
// If we take into account all possilble triangle sizes (we will look
// at this issue later) our grid needs more than 7 times as many lines.
// For simplicity, a scale of 10 will serve the purpose.
// Note that the original target-square table is 17x17, and grid 18x18.

constexpr int scale = 10;
constexpr int gr = tor * scale + 1; // n of our grid rows
constexpr int gc = toc * scale + 1; // n of our grid columns

inline int t2g(int i) { return i* scale; } // translates to our grid index


//******************************************************************************
// When writing the code it will be convenient to use grid coordinates
// wrapped in a single object rather than two single numbers.
// The code will be clearer if grid coordinates are represented by a 
// structure different from TCoord.

struct GCoord : public pair<int,int> { // coordinates
  void swap() { std::swap(first,second); }
  void operator+=(const GCoord& o) {
    first  += o.first;
    second += o.second;
  }
  GCoord()             { first = 0; second = 0; }
  GCoord(int i, int j) { first = i; second = j; }
};

inline bool in_grid( const GCoord& loc ) {
  return loc.first >= 0  && loc.second >= 0 &&
         loc.first <= gr && loc.second <= gc;
}

inline GCoord t2g( const TCoord loc ) {
    GCoord gloc(t2g(loc.first),t2g(loc.second));
    return gloc;
}

//******************************************************************************
// A triangle must cover only its target square. Since we are looking at a 
// more dense grid, it is enough to check if the triangle we are placing on
// the grid shares any of its internal points with another target square.
// Let's create a grid table that marks internal points of all original
// tt target squares with their tt table coordinates.
// Note that tt square (0,0) is not a target, so we can use it as "empty".
// So, since c++ initializes all static storage to 0, all we need to do
// is to mark non-empty grid points.

TCoord gmsqi[gr][gc]; // grid marked tt square interiors

void init_gmsqi() {
  for ( auto sq : ltsq ) {
    int r = sq.first;  
    int c = sq.second;  
    for ( int i = t2g(r)+1; i < t2g(r+1); i++ )
      for ( int j = t2g(c)+1; j < t2g(c+1); j++ ) {
        gmsqi[i][j] = sq; 
      }
  }
}

//******************************************************************************
// A triangle has boundary grid points and internal/gut grid points.
// Since we are using a dense grid, all we need to pay attention to are
// internal points.
// Class gps is used for this purpose.
// '.' -- "empty" grid point 
// '*' -- internal


struct Gps { // grid point status: empty or internal
  char s[gr][gc];
    Gps() { for(int i = 0; i < gr; i++ )
      for( int j = 0; j < gc; j++ ) s[i][j] = '.'; }
    Gps(const Gps& o) { 
      for(int i = 0; i < gr; i++ ) 
        for( int j = 0; j < gc; j++ ) s[i][j] = o.s[i][j]; }
    bool is_empty(const GCoord& loc) const {
      return s[loc.first][loc.second] == '.';
    }     
    bool is_internal(const GCoord& loc) const {
      return s[loc.first][loc.second] == '*';
    }     
    void mark_internal(const GCoord& loc) {
      s[loc.first][loc.second] = '*';
    }     
    bool is_covered(const TCoord& tloc) const {
      int i = t2g(tloc.first)  + 1;
      int j = t2g(tloc.second) + 1;
      return s[i][j] == '*';
    }
    void print() const {
      cout << "Gps: " << endl;
      for(int i = 0; i < gr; i++ ) {
        for( int j = 0; j < gc; j++ ) cout << ' ' << s[i][j]; 
        cout << endl;
      }
    }
};

//******************************************************************************
// An attempt to solve the puzzle manually may involve cutting paper triangles of
// various sizes. Let's call them templates. A template square entirely contained
// in the template may cover the target square. It defines the template position. 
// A template can be flipped horizonally, vertically, or longer and shorter arms
// can be swapped. This leads to the notion of triangle configuration.

struct Config { // triangle configuration
  bool s = 0; // flip asymmetry
  bool h = 0; // flip horizonally
  bool v = 0; // flip verically
  int  p = 0; // use this position index -- a template has a list
};

//******************************************************************************
// A template defines its corners, boundary points, and positions.
// The square angle corner is located at (0,0). For position (0,0),
// to get coordinates of grid points covered by the template, add its corners,
// boundary, and internal coordinates to the grid coordinates of the target 
// square.
// transform() computes the relative location of each triangle point taking
// into account triangle configuration. 

struct Tm {
  vector<GCoord> v; // vertices/corners
  vector<GCoord> b; // boundary but not corners
  vector<GCoord> g; // internal "guts"
  vector<TCoord> p; // target square positions - upper left corner
  Tm(const int r, const int c) { // r, c == original table size
    v.emplace_back(t2g(0),t2g(0)); // scaled coordinates
    v.emplace_back(t2g(0),t2g(c)); 
    v.emplace_back(t2g(r),t2g(0));

    // find t.b
    for ( int j = 1; j < t2g(c); j++ ) { b.emplace_back(0,j); }
    for ( int i = 1; i < t2g(r); i++ ) { b.emplace_back(i,0); }
    const int rf = gcd(r,c); // reduction factor
    const int rr = r / rf; // reduced r -- we want relatively prime pair
    const int rc = c / rf; // reduced c
    for ( int k  = 1; k < t2g(rf); k++ ) { b.emplace_back(r-k*rr,0+k*rc); }

    // find t.g 
    for ( int i = 1; i < t2g(r); i++ ) // skip original (0,c) and (r,0)
      // only columns that satisfy floating point j < (t2g(r)-i)*(c/r)
      for ( int j = 1; j*rr < (t2g(r)-i)*rc; j++) 
        g.emplace_back(i,j);

    // find t.p -- (i+1,j+1) must be inside or at boundary
    for ( int i = 0; i < r; i++ ) 
      // only columns that satisfy floating point (j+1) <= (r-i-1)*(c/r)
      for ( int j = 0; (j+1)*rr <= (r-i-1)*rc; j++) 
        p.emplace_back(i,j);
  }
  bool symmetrical() const { return v[1].second == v[2].first; }
  GCoord transform( const Config& cnfg, const TCoord loc ) const {
    TCoord res = loc;
    res.first  -= p[cnfg.p].first;
    res.second -= p[cnfg.p].second;
    if ( cnfg.s ) swap(res.first,res.second);
    if ( cnfg.h ) res.first  = -res.first  + 1;
    if ( cnfg.v ) res.second = -res.second + 1;
    return t2g(res);
  }
  GCoord transform( const Config& cnfg, const GCoord loc ) const {
    GCoord res = loc;
    res.first  -= t2g(p[cnfg.p].first);
    res.second -= t2g(p[cnfg.p].second);
    if ( cnfg.s ) swap(res.first,res.second);
    if ( cnfg.h ) res.first  = -res.first  + t2g(1);
    if ( cnfg.v ) res.second = -res.second + t2g(1);
    return res;
  }
  int gcd (int a, int b) const {
    int x;
    while (b)
      {
        x = a % b;
        a = b;
        b = x;
      }
    return a;
  }

};

//******************************************************************************
// It is easy to find all temlate sizes needed for the targets in tt table

Tm tm2_2x2(2,2);
Tm tm3_2x3(2,3);
Tm tm4_2x4(2,4);
Tm tm5_2x5(2,5);
Tm tm6_2x6(2,6);
Tm tm6_4x3(4,3);
Tm tm7_2x7(2,7);
Tm tm8_2x8(2,8);
Tm tm8_4x4(4,4);
Tm tm9_2x9(2,9);
Tm tm9_3x6(3,6);
Tm tm10_2x10(2,10);
Tm tm10_4x5(4,5);
Tm tm11_2x11(2,11);
Tm tm12_2x12(2,12);
Tm tm12_4x6(4,6);
Tm tm12_8x3(8,3);
Tm tm14_2x14(2,14);
Tm tm14_4x7(4,7);
Tm tm18_2x18(2,18); // does not fit tt
Tm tm18_4x9(4,9);
Tm tm18_3x12(3,12);
Tm tm18_6x6(6,6);
Tm tm20_2x20(2,20); // does not fit tt
Tm tm20_4x10(4,10);
Tm tm20_8x5(8,5);

//******************************************************************************
// We will need to easily look up all templates by target number.

vector<vector<const Tm*>> n2tm; // number to template list

void init_n2tm() {
  n2tm.resize(21);
  auto e = [](int i, Tm& t){ n2tm[i].emplace_back(&t); }; 
  e( 2,tm2_2x2);
  e( 3,tm3_2x3);
  e( 4,tm4_2x4);
  e( 5,tm5_2x5);
  e( 6,tm6_2x6);   e( 6,tm6_4x3);
  e( 7,tm7_2x7);  
  e( 8,tm8_2x8);   e( 8,tm8_4x4);
  e( 9,tm9_2x9);   e( 9,tm9_3x6);
  e(10,tm10_2x10); e(10,tm10_4x5);
  e(11,tm11_2x11);
  e(12,tm12_2x12); e(12,tm12_4x6); e(12,tm12_8x3);
  e(14,tm14_2x14); e(14,tm14_4x7);
  e(18,tm18_4x9);  e(18,tm18_3x12); e(18,tm18_6x6);
  e(20,tm20_4x10); e(20,tm20_8x5);
}


//******************************************************************************
// We will need to print some info

void print( const pair<int,int>& p) {
  cout << "(" << p.first << ',' << p.second << ')';
}

#if 0
void print( const Tm& t)
{
  cout << endl;
  cout << "v: "; for( auto e : t.v) print(e); cout << endl;
  cout << "b: "; for( auto e : t.b) print(e); cout << endl;
  cout << "g: "; for( auto e : t.g) print(e); cout << endl;
  cout << "p: "; for( auto e : t.p) print(e); cout << endl;
}
#endif

//******************************************************************************
// Now the algorithmic part.
// fits()  checks if a template in a particular configuration can be used to 
//         cover a given target square.
// place() places a template in a particular configuration 
// hll()   find horizonal leg length

bool fits( const Gps& gps, const TCoord& tl, const Tm& tm, const Config& cnfg) {
  // tl   -- target square location
  // tm   -- triangle template
  // cnfg -- triangle configuration
  // assert triangle area == template area / 2 --- wrong template?
  assert( 2 * tt[tl.first][tl.second] * scale * scale == tm.v[1].second * tm.v[2].first );
  // check if corners are in table, are they in another triangle
  auto image = [&tl,&tm,&cnfg](const GCoord& templloc) {
    auto al  = tm.transform(cnfg,templloc);
         al += t2g(tl); // actual grid location
    return al;
  };
  auto v0 = image(tm.v[0]); if (!in_grid(v0) || gps.is_internal(v0)) return 0;
  auto vc = image(tm.v[1]); if (!in_grid(vc) || gps.is_internal(vc)) return 0;
  auto vr = image(tm.v[2]); if (!in_grid(vr) || gps.is_internal(vr)) return 0;
  // check if template guts overlap another triangle or target square
  for ( const auto& gl : tm.g ) {
    GCoord al  = image(gl);
    TCoord mk  = gmsqi[al.first][al.second];
    if ( (mk.first != 0 || mk.second != 0) && mk != tl ) return false;
    if ( gps.is_internal(al) )              return false;
  }
  return true;
}

void place( Gps& gps, const TCoord& tl, const Tm& tm, const Config& cnfg) {
  // place triangle, i.e., mark internal points in gps. 
  // tl   -- target square location
  // tm   -- triangle template
  // cnfg -- triangle configuration
  for ( const auto& gl : tm.g ) {
    GCoord al  = tm.transform(cnfg,gl);
           al += t2g(tl); // actual grid location
    gps.mark_internal(al);
  }
}

void print_troc ( const TCoord& tl, const Tm& tm, const Config& cnfg) {
  // print triangle in original grid coordinates
  // tl   -- target square location
  // tm   -- triangle template
  // cnfg -- triangle configuration
  // assert triangle area == template area / 2 --- wrong template?
  assert( 2 * tt[tl.first][tl.second]*scale*scale == tm.v[1].second * tm.v[2].first );
  for ( const auto& e : tm.v ) {
    GCoord al  = tm.transform(cnfg,e);
           al += t2g(tl); // actual grid location
           al.first  /= scale;
           al.second /= scale;
    cout << "\t"; print(al);
  }
}

int hll( const TCoord& tl, const Tm& tm, const Config& cnfg) {
  // find horizontal leg length -- just check the template
  // tl   -- target square location
  // tm   -- triangle template
  // cnfg -- triangle configuration
  // assert triangle area == template area / 2 --- wrong template?
  assert( 2 * tt[tl.first][tl.second]*scale*scale == tm.v[1].second * tm.v[2].first );
  if ( !cnfg.s ) return tm.v[1].second / scale;
  else           return tm.v[2].first  / scale;
}

//******************************************************************************
// options() finds the toal number of all templates configurations that fit.
// Let's define data structures that can be used to save options.

struct Option {
  const Tm*    tm = 0;
  const Config cnfg;
  Option(){;}
  Option(const Tm* t, const Config& cg ) : tm(t), cnfg(cg) {;}
};

vector<vector<Option>> topt; // target options record

void init_topt() {
  topt.resize(ltsq.size());
}

int options( const Gps& gps, const TCoord& tl ) {
  // Finds options that fit and records them in topt, skips covered targers
  const int tn = tt[tl.first][tl.second];
  if ( gps.is_covered(tl) ) return 0;
  int k = 0; while( ltsq[k] != tl) k++; // find the index of tl
  topt[k].resize(0);
  int res = 0;
  for ( auto tm : n2tm[tt[tl.first][tl.second]] ) {
    Config cnfg; 
    auto count = [gps,tl,k,&res,tm,&cnfg]() {
      for ( int p = 0; p < tm->p.size(); p++ ) { 
        cnfg.p = p;
        if ( fits(gps,tl,*tm,cnfg) ) { res++;
          topt[k].emplace_back(tm,cnfg);
        }
      }
    };
                    count(); // h==0, v==0
    cnfg.h = true;  count(); // h==1, v==0
    cnfg.v = true;  count(); // h==1, v==1
    cnfg.h = false; count(); // h==0, v==1
    if ( tm->symmetrical() ) continue;
    cnfg.s = true;  count(); // h==0, v==1
    cnfg.v = false; count(); // h==0, v==0
    cnfg.h = true;  count(); // h==1, v==0
    cnfg.v = true;  count(); // h==1, v==1
  }
#if 0
  cout << "target ";
  print(tl); 
  cout << " \t" << tt[tl.first][tl.second] << " \tres: " << res << endl;
#endif
  return res;
}

void all_options(const Gps& gps ) {
  // find all options for targets that have not been covered
  long tot = 0L;
  for ( auto sq : ltsq ) tot += options(gps,sq);
#if 0
  cout << "all options: " << tot << endl;
#endif
}

//******************************************************************************
// Solve()

void solve() {
  vector<Gps> stack;   // For placement decisions
    stack.reserve(ltsq.size()+2);
    stack.resize(1);   // initial gps is empty
  vector<int> picked;  // keeps the sequence of targets covered so far
  auto pick_target = [&stack]() { // pick target by max size
    auto& Top = stack[stack.size()-1];
    int res = -2; int max = -1;
    for( int k = 0; k < ltsq.size(); k++ ) {
      auto t = ltsq[k];
      if ( Top.is_covered(t) ) continue;
      if ( topt[k].size() == 0 ) return -1; 
      auto n = tt[t.first][t.second];
      if ( n > 0 && n >= max ) { max = n;  res = k; }
    }
    return res;
  };
  auto put_triangle = [&stack](int pick) {
    auto& Top = stack[stack.size()-1];
        stack.emplace_back(Top);
    const auto& vo = topt[pick];
    auto& opt = vo[vo.size()-1];
    auto& NewTop = stack[stack.size()-1];
    place (NewTop,ltsq[pick],*opt.tm,opt.cnfg);
  };
  bool found = false;
  while( true ) {
    all_options( stack[stack.size()-1]);
    auto pick = pick_target();
    if ( pick < 0 ) {
      if ( pick < -1 ) { found = true;
        cout << " SOLVED !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! " << endl;
        auto& Top = stack[stack.size()-1];
        int sig = 1;
        for ( int pck = 0; pck < picked.size(); pck++ ) {
            const auto& vo = topt[pck];
            auto& opt = vo[vo.size()-1];
            print(ltsq[pck]);
                    print_troc (ltsq[pck],*opt.tm,opt.cnfg);
            cout << "\t" << hll(ltsq[pck],*opt.tm,opt.cnfg) << endl;
            auto tmp = hll(ltsq[pck],*opt.tm,opt.cnfg);
            if ( tmp % 2 ) sig *= tmp;
        }
        cout << "Answer: " << sig << endl;
      }
      while( true ) {
        stack.pop_back();      // erase failed attemp
        pick = picked.back();  // get last target decision
        topt[pick].pop_back(); // eliminate unsuccesful option
        if ( topt[pick].size() ) {
          goto PUT_TRIANGLE;   // more options for last pick
        }
        if ( picked.size() ) {
          picked.pop_back();
        } 
        if ( !picked.size() ) {
            if ( !found ) cout << " FINISHED !!!!!  NO SOLUTION  !!!!!!!! " << endl;
            else          cout << " FINISHED !!!!!  NO MORE SOLUTIONS  !!!!!!!! " << endl;
          return;
        } 
      }
    }
    picked.push_back(pick);
    PUT_TRIANGLE:
    put_triangle(pick);
  }
}


int main(int argc, char **argv) {
    init_ltsq();
    init_n2tm();
    init_gmsqi();
    init_topt();
    solve();
    return 0;
}
