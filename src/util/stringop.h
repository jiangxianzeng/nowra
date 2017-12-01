#ifndef _STRING_OP_H_INCLUDE_
#define _STRING_OP_H_INCLUDE_

extern char *split_at(char *string, int delimiter);

extern char *split_at_right(char *string, int delimiter);

extern char *mystrtok(char **src, const char *step);

extern char *host_port(const char *addr, char **hostp, char **portp);

extern char *concatenate(const char *arg0,...);

extern char *basename(const char *path);

extern int string2ll(const char *s, size_t slen, long long *value);

#endif
