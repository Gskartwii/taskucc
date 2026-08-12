/* https://www.scs.stanford.edu/~dm/blog/va-opt.html */

#define ID(x) x
#define LPAREN (
#define ID2(arg) arg

ID(ID2)(ID)(X)        // => X
ID(ID2 LPAREN)ID)(X)  // => X
ID(ID2 LPAREN ID))(X) // => ID(X)
ID(ID2 (ID))(X)       // => ID(X)
