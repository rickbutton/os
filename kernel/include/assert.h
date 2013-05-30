#ifndef ASSERT_H
#define ASSERT_H

#define _stringify(x) (#x)
#define assert(cond) ( (cond) ? (void)0 : assert_fail(#cond, __FILE__, __LINE__) )

#endif
