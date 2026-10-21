/* Native TLS 1.2. No host proxy, permissive verifier or weak entropy fallback. */
#include "bearssl.h"
#include <string.h>
#include "tls.h"
#include "tls_anchors.h"
extern void input_date_time(int*);
extern void serial(const char*);
extern void number(char*,unsigned);
static br_ssl_client_context client;
static br_x509_minimal_context chain;
static unsigned char buffers[BR_SSL_BUFSIZE_BIDI];
static unsigned request_pos;
static void wipe(void*p,unsigned n){volatile unsigned char*b=p;while(n--)*b++=0;}
void tls_clear(void){wipe(&client,sizeof(client));wipe(&chain,sizeof(chain));wipe(buffers,sizeof(buffers));request_pos=0;}
static int leap(int y){return !(y%4)&&((y%100)||!(y%400));}
int tls_start(const char*host){
    unsigned a,b,c,d,entropy[8];int date[6];
    tls_clear();__asm__ volatile("cpuid":"=a"(a),"=b"(b),"=c"(c),"=d"(d):"a"(1));
    if(!(c&(1u<<30)))return -8;
    for(int i=0;i<8;i++){unsigned char ok=0;for(int n=0;n<32&&!ok;n++)__asm__ volatile("rdrand %0; setc %1":"=r"(entropy[i]),"=qm"(ok));if(!ok){wipe(entropy,sizeof(entropy));return -8;}}
    input_date_time(date);static const int mdays[]={31,28,31,30,31,30,31,31,30,31,30,31};
    if(date[0]<2026||date[0]>2099||date[1]<1||date[1]>12||date[2]<1||date[2]>mdays[date[1]-1]+(date[1]==2&&leap(date[0]))||date[3]<0||date[3]>23||date[4]<0||date[4]>59||date[5]<0||date[5]>59){wipe(entropy,sizeof(entropy));return -9;}
    unsigned days=0;for(int y=0;y<date[0];y++)days+=365+leap(y);
    for(int m=1;m<date[1];m++)days+=mdays[m-1]+(m==2&&leap(date[0]));days+=date[2]-1;
    br_ssl_client_init_full(&client,&chain,TAs,TAs_NUM);
    br_ssl_engine_set_versions(&client.eng,BR_TLS12,BR_TLS12);
    static const uint16_t suites[]={BR_TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256,BR_TLS_ECDHE_ECDSA_WITH_CHACHA20_POLY1305_SHA256,BR_TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256,BR_TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256,BR_TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384,BR_TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384};
    br_ssl_engine_set_suites(&client.eng,suites,sizeof(suites)/sizeof(*suites));
    br_x509_minimal_set_time(&chain,days,date[3]*3600+date[4]*60+date[5]);
    br_ssl_engine_set_buffer(&client.eng,buffers,sizeof(buffers),1);
    br_ssl_engine_inject_entropy(&client.eng,entropy,sizeof(entropy));wipe(entropy,sizeof(entropy));
    return br_ssl_client_reset(&client,host,0)?0:-7;
}
/* Nonblocking engine: keep the desktop/audio/network service loop responsive.
   Callbacks return a consumed byte count, zero for would-block, or an error. */
int tls_step(int(*send)(const void*,unsigned),int(*receive)(void*,unsigned),int(*plain)(const void*,unsigned),const char*request){
    for(int turn=0;turn<32;turn++){
        unsigned state=br_ssl_engine_current_state(&client.eng);size_t n;unsigned char*p;int used,progress=0;
        if(state&BR_SSL_CLOSED){int err=br_ssl_engine_last_error(&client.eng);if(err){char v[16];number(v,err);serial("TLS validation/handshake error: ");serial(v);serial("\n");return -7;}return 1;}
        if(state&BR_SSL_SENDREC){p=br_ssl_engine_sendrec_buf(&client.eng,&n);used=send(p,n);if(used<0)return used;if(used){br_ssl_engine_sendrec_ack(&client.eng,used);progress=1;}}
        if(state&BR_SSL_RECVAPP){p=br_ssl_engine_recvapp_buf(&client.eng,&n);used=plain(p,n);if(used<0)return used;br_ssl_engine_recvapp_ack(&client.eng,n);progress=1;}
        if((state&BR_SSL_SENDAPP)&&request[request_pos]){p=br_ssl_engine_sendapp_buf(&client.eng,&n);unsigned left=strlen(request+request_pos);if(n>left)n=left;memcpy(p,request+request_pos,n);request_pos+=n;br_ssl_engine_sendapp_ack(&client.eng,n);br_ssl_engine_flush(&client.eng,0);progress=1;}
        if(state&BR_SSL_RECVREC){p=br_ssl_engine_recvrec_buf(&client.eng,&n);used=receive(p,n);if(used<0)return used;if(used){br_ssl_engine_recvrec_ack(&client.eng,used);progress=1;}}
        if(!progress)break;
    }return 0;
}
