#ifndef TWOFTPD__LOG__H__
#define TWOFTPD__LOG__H__

extern const char program[];

extern void log_start(void);
#define log_str(S) obuf_puts(&errbuf, S)
#define log_uint(U) obuf_putu(&errbuf, U)
#define log_end() obuf_putsflush(&errbuf, "\n")

extern void log1(const char*);
extern void log2(const char*, const char*);

#endif
