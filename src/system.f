: dup  ( x -- x x )  0 pick ;
: over  ( x1 x2 -- x1 x2 x1 ) 1 pick ;
: swap  ( x1 x2 -- x2 x1 )  1 roll ;
: rot  ( x1 x2 x3 -- x2 x3 x1 ) 2 roll ;
