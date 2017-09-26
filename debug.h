#ifndef __debug_h__
#define __debug_h__

#define NPREDICT
//#define DEBUG

#ifdef NDEBUG
#define DEBUG(x)
#else
#define DEBUG(x) do { std::cerr << x; } while (0)
#endif

#ifdef NPREDICT
#define PREDICT(x)
#else
#define PREDICT(x) do { std::cout << x; } while(0)
#endif

#endif
