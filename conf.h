#ifndef CONF__H__
#define CONF__H__

#include <bglibs/iobuf.h>
extern obuf conf_out;
extern void start_file(const char* filename, int mode);
extern void end_file(void);
extern void make_file(const char* filename, int mode,
		      const char* s1,
		      const char* s2,
		      const char* s3,
		      const char* s4,
		      const char* s5,
		      const char* s6,
		      const char* s7);
extern void make_fileu(const char* filename, unsigned val);

#endif
