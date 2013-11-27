#ifndef HELPERS_H_
#define HELPERS_H_

/// Macro to set a bit y in variable x
#define SETBIT(x,y)	(x |= (y))
/// Macro to reset a bit y in variable x
#define CLEARBIT(x,y)	(x &= ~(y))
/// Macro to toggle a bit y in variable x
#define INVERTBIT(x,y)	(x ^= (y))
//macro to read a bit
#define GETBIT(x,y) (x &= y)
#endif
