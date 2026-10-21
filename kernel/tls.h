#ifndef BDS_TLS_H
#define BDS_TLS_H
int tls_start(const char*host);
int tls_step(int(*send)(const void*,unsigned),int(*receive)(void*,unsigned),int(*plain)(const void*,unsigned),const char*request);
void tls_clear(void);
#endif
