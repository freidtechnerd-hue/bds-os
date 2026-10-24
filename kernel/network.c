#include "os.h"
#include "tls.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#include "netif/ethernet.h"
#include "lwip/etharp.h"
#include "lwip/dhcp.h"
#include "lwip/dns.h"
#include "lwip/tcp.h"
#include "lwip/timeouts.h"
/* Intel 82540EM/82545EM, legacy descriptors; polled, one CPU, no IRQ use. */
typedef struct __attribute__((packed)){u32 lo,hi;u16 length,checksum;u8 status,errors;u16 special;}Rx;
typedef struct __attribute__((packed)){u32 lo,hi;u16 length;u8 cso,cmd,status,css;u16 special;}Tx;
static volatile u32*mmio;static volatile Rx rx[32] __attribute__((aligned(128)));static volatile Tx tx[16] __attribute__((aligned(128)));
static u8 rxbuf[32][2048] __attribute__((aligned(16))),txbuf[16][2048] __attribute__((aligned(16)));
int rtl_init(u8*);int rtl_link(void);int rtl_send(const void*,u32);int rtl_receive(void*);
int pcnet_init(u8*);int pcnet_link(void);int pcnet_send(const void*,u32);int pcnet_receive(void*);
static int realtek;static u8 packet[2048];
static int rr,tt,ready,busy,fault;static u32 random_state;static struct netif nic;static struct dhcp lease;
static u32 reg(u32 r){return mmio[r/4];}static void wr(u32 r,u32 v){mmio[r/4]=v;(void)reg(8);}
static u32 pci(u32 a,int off){outl(0xcf8,0x80000000|a|(off&252));return inl(0xcfc);}
u32 sys_now(void){return ticks();}
u32 net_random(void){random_state^=random_state<<13;random_state^=random_state>>17;random_state^=random_state<<5;return random_state;}
void net_assert(const char*s){fault=1;serial("NET assertion: ");serial(s);serial("\n");}
static err_t output(struct netif*n,struct pbuf*p){
    if(realtek){if(p->tot_len>1518)return ERR_IF;pbuf_copy_partial(p,packet,p->tot_len,0);return (realtek==1?rtl_send(packet,p->tot_len):pcnet_send(packet,p->tot_len))?ERR_IF:ERR_OK;}
    static int logged;if(!logged){logged=1;char v[16];serial("NET first TX length ");number(v,p->tot_len);serial(v);serial(" status ");number(v,reg(8));serial(v);serial("\n");}
    if(!(reg(8)&2)||p->tot_len>1518||!(tx[tt].status&1))return ERR_IF;
    int index=tt;pbuf_copy_partial(p,txbuf[index],p->tot_len,0);tx[index].length=p->tot_len;tx[index].cmd=1|2|8;tx[index].status=0;
    __asm__ volatile("":::"memory");tt=(tt+1)&15;wr(0x3818,tt);return ERR_OK;
}
static err_t nic_init(struct netif*n){n->name[0]='e';n->name[1]='0';n->hwaddr_len=6;n->mtu=1500;n->flags=NETIF_FLAG_BROADCAST|NETIF_FLAG_ETHARP;n->output=etharp_output;n->linkoutput=output;n->hostname="bds-os";return ERR_OK;}
static void status(struct netif*n){if(!ip4_addr_isany_val(*netif_ip4_addr(n))){print("NET DHCP address: ");print(ip4addr_ntoa(netif_ip4_addr(n)));print(" gateway ");print(ip4addr_ntoa(netif_ip4_gw(n)));print("\n");}}
void net_init(void){
    u8 mac[6];u32 low=0,high=0;u32 found=0xffffffff;for(int bus=0;bus<256&&found==0xffffffff;bus++)for(int slot=0;slot<32&&found==0xffffffff;slot++)for(int fn=0;fn<8;fn++){u32 a=(bus<<16)|(slot<<11)|(fn<<8),v=pci(a,0);if((v&65535)==65535){if(!fn)break;continue;}if((v&65535)==0x8086&&(v>>16==0x100e||v>>16==0x100f||v>>16==0x100c)){found=a;break;}if(!fn&&!(pci(a,12)&0x800000))break;}
    if(found==0xffffffff){if(!rtl_init(mac))realtek=1;else if(!pcnet_init(mac))realtek=2;else{print("NET: no supported Intel, Realtek or AMD Ethernet adapter\n");return;}low=mac[0]|mac[1]<<8|mac[2]<<16|mac[3]<<24;high=mac[4]|mac[5]<<8;goto stack;}
    u32 bar=pci(found,16);if((bar&7)||!(bar&~15u)){print("NET: unsupported Ethernet BAR\n");return;}mmio=(volatile u32*)(bar&~15u);
    outl(0xcf8,0x80000000|found|4);outw(0xcfc,(u16)pci(found,4)|6);wr(0xd8,0xffffffff);wr(0,reg(0)|(1u<<26));sleep(20);wr(0xd8,0xffffffff);wr(0,((reg(0)&~((1u<<31)|8))|0x40));sleep(20);
    low=reg(0x5400);high=reg(0x5404);for(int i=0;i<4;i++)mac[i]=low>>(i*8);mac[4]=high;mac[5]=high>>8;
    if(!(high&(1u<<31))){for(int i=0;i<3;i++){wr(0x14,1|(i<<8));u32 end=ticks()+100;while(!(reg(0x14)&16)&&(int)(end-ticks())>0)sleep(1);if(!(reg(0x14)&16)){print("NET: could not read Ethernet MAC\n");return;}u32 v=reg(0x14)>>16;mac[i*2]=v;mac[i*2+1]=v>>8;}wr(0x5400,mac[0]|mac[1]<<8|mac[2]<<16|mac[3]<<24);wr(0x5404,mac[4]|mac[5]<<8|0x80000000u);}
    for(int i=0;i<128;i++)wr(0x5200+i*4,0);memset((void*)rx,0,sizeof(rx));memset((void*)tx,0,sizeof(tx));
    for(int i=0;i<32;i++)rx[i].lo=(u32)rxbuf[i];for(int i=0;i<16;i++){tx[i].lo=(u32)txbuf[i];tx[i].status=1;}
    rr=tt=0;wr(0x100,0);wr(0x400,0);wr(0x2800,(u32)rx);wr(0x2804,0);wr(0x2808,sizeof(rx));wr(0x2810,0);wr(0x2818,31);wr(0x2820,0);wr(0x282c,0);
    wr(0x3800,(u32)tx);wr(0x3804,0);wr(0x3808,sizeof(tx));wr(0x3810,0);wr(0x3818,0);wr(0x3820,0);wr(0x410,10|(8<<10)|(6<<20));wr(0x400,2|8|(15<<4)|(64<<12));wr(0x100,2|(1<<15)|(1<<26));
    stack:
    random_state=ticks()^low^high^0xbd505a13u;if(!random_state)random_state=1;
    lwip_init();ip4_addr_t zero;ip4_addr_set_zero(&zero);netif_add(&nic,&zero,&zero,&zero,0,nic_init,ethernet_input);memcpy(nic.hwaddr,mac,6);netif_set_default(&nic);netif_set_status_callback(&nic,status);dhcp_set_struct(&nic,&lease);ready=1;netif_set_up(&nic);if(realtek?(realtek==1?rtl_link():pcnet_link()):!!(reg(8)&2))netif_set_link_up(&nic);dhcp_start(&nic);print(realtek==2?"NET: AMD PCnet ready, DHCP requested\n":realtek?"NET: Realtek RTL8139 ready, DHCP requested\n":"NET: Intel PRO/1000 ready, DHCP requested\n");
}
void net_poll(void){
    if(!ready||busy||fault)return;busy=1;int link=realtek?(realtek==1?rtl_link():pcnet_link()):!!(reg(8)&2);if(link!=!!netif_is_link_up(&nic)){if(link)netif_set_link_up(&nic);else netif_set_link_down(&nic);}
    if(realtek){for(int count=0;count<32;count++){int len=realtek==1?rtl_receive(packet):pcnet_receive(packet);if(!len)break;struct pbuf*p=pbuf_alloc(PBUF_RAW,len,PBUF_POOL);if(p){pbuf_take(p,packet,len);if(nic.input(p,&nic)!=ERR_OK)pbuf_free(p);}}}
    else for(int count=0;count<32&&(rx[rr].status&1);count++){
        static int logged;if(!logged){logged=1;char v[16];serial("NET first RX length ");number(v,rx[rr].length);serial(v);serial(" errors ");number(v,rx[rr].errors);serial(v);serial("\n");}
        int index=rr;u16 len=rx[index].length;u8 s=rx[index].status;if((s&2)&&!rx[index].errors&&len>=14&&len<=1518){struct pbuf*p=pbuf_alloc(PBUF_RAW,len,PBUF_POOL);if(p){pbuf_take(p,rxbuf[index],len);if(nic.input(p,&nic)!=ERR_OK)pbuf_free(p);}}
        rx[index].status=0;__asm__ volatile("":::"memory");wr(0x2818,index);rr=(rr+1)&31;
    }sys_check_timeouts();busy=0;
}
int net_info(char*out,u32 cap){
    char s[180];strcpy(s,!ready?"No supported Ethernet adapter":fault?"Ethernet stack fault":!netif_is_link_up(&nic)?"Ethernet cable disconnected":ip4_addr_isany_val(*netif_ip4_addr(&nic))?"Ethernet: waiting for DHCP":"Ethernet: ");
    if(ready&&!fault&&netif_is_link_up(&nic)&&!ip4_addr_isany_val(*netif_ip4_addr(&nic))){strcpy(s+strlen(s),ip4addr_ntoa(netif_ip4_addr(&nic)));strcpy(s+strlen(s)," gateway ");strcpy(s+strlen(s),ip4addr_ntoa(netif_ip4_gw(&nic)));}
    if(!out||cap<=strlen(s))return -1;strcpy(out,s);return ready&&!fault&&netif_is_link_up(&nic)&&!ip4_addr_isany_val(*netif_ip4_addr(&nic));
}
typedef struct{struct tcp_pcb*pcb;char*out;u32 cap,used,token;int done,error,resolved,secure,connected,eof;ip_addr_t address;char request[1600];}Fetch;
static Fetch fetch;static u32 token;
static u8 tls_rx[32768];static u32 tls_head,tls_used;
static int tls_send(const void*data,unsigned n){if(!fetch.pcb)return -4;unsigned room=tcp_sndbuf(fetch.pcb);if(n>room)n=room;if(n>1460)n=1460;if(!n)return 0;err_t r=tcp_write(fetch.pcb,data,n,TCP_WRITE_FLAG_COPY);if(r==ERR_MEM)return 0;if(r!=ERR_OK)return -4;tcp_output(fetch.pcb);return n;}
static int tls_receive(void*data,unsigned n){if(n>tls_used)n=tls_used;if(!n)return fetch.eof?-7:0;for(unsigned i=0;i<n;i++)((u8*)data)[i]=tls_rx[(tls_head+i)%sizeof(tls_rx)];tls_head=(tls_head+n)%sizeof(tls_rx);tls_used-=n;return n;}
static int tls_plain(const void*data,unsigned n){if(n>fetch.cap-1-fetch.used)return -5;memcpy(fetch.out+fetch.used,data,n);fetch.used+=n;fetch.out[fetch.used]=0;return n;}

static void resolved(const char*name,const ip_addr_t*addr,void*arg){if((u32)arg!=fetch.token)return;fetch.resolved=1;if(addr)fetch.address=*addr;else fetch.error=-3;}
static void tcp_error(void*arg,err_t error){fetch.pcb=0;fetch.error=-4;fetch.done=1;}
static err_t receive(void*arg,struct tcp_pcb*p,struct pbuf*b,err_t error){
    if(!b){if(fetch.secure)fetch.eof=1;else fetch.done=1;return ERR_OK;}
    if(fetch.secure){if(b->tot_len>sizeof(tls_rx)-tls_used)return ERR_MEM;
        for(struct pbuf*q=b;q;q=q->next)for(unsigned i=0;i<q->len;i++)tls_rx[(tls_head+tls_used++)%sizeof(tls_rx)]=((u8*)q->payload)[i];
        tcp_recved(p,b->tot_len);pbuf_free(b);return ERR_OK;}
    if(b->tot_len>fetch.cap-1-fetch.used){pbuf_free(b);fetch.error=-5;fetch.done=1;return ERR_OK;}
    pbuf_copy_partial(b,fetch.out+fetch.used,b->tot_len,0);fetch.used+=b->tot_len;fetch.out[fetch.used]=0;tcp_recved(p,b->tot_len);pbuf_free(b);return ERR_OK;
}
static err_t connected(void*arg,struct tcp_pcb*p,err_t error){if(error){fetch.error=-4;fetch.done=1;return error;}tcp_recv(p,receive);fetch.connected=1;if(fetch.secure)return ERR_OK;if(tcp_write(p,fetch.request,strlen(fetch.request),TCP_WRITE_FLAG_COPY)!=ERR_OK){fetch.error=-4;fetch.done=1;return ERR_OK;}tcp_output(p);return ERR_OK;}
static int service(u32 end){net_poll();audio_pump();input_poll();if(key(0)==27)return -6;if((int)(ticks()-end)>=0)return -4;sleep(10);return 0;}
int http_get(const char*url,char*out,u32 cap){
    if(!url||!out||cap<1024||cap>4*1024*1024)return -2;int secure=!strncmp(url,"https://",8);int scheme=secure?8:7;if(!secure&&strncmp(url,"http://",7))return -2;if(strlen(url)>1000)return -2;
    const char*p=url+scheme;char host[254],authority[270],path[1024];int n=0;while(*p&&*p!='/'&&*p!=':'&&*p!='?'&&*p!='#'){if(n>=253||(u8)*p<=32||*p=='@')return -2;host[n++]=*p++;}host[n]=0;if(!n)return -2;
    int port=secure?443:80;if(*p==':'){p++;port=0;if(*p<'0'||*p>'9')return -2;while(*p>='0'&&*p<='9'){port=port*10+*p++-'0';if(port>65535)return -2;}if(!port)return -2;}
    const char*pathstart=p;strcpy(path,*p=='/'?p:"/");if(*p=='?')strcpy(path+1,p);for(int i=0;path[i];i++)if((u8)path[i]<32||path[i]==' ')return -2;
    if(*p&&*p!='/'&&*p!='?'&&*p!='#')return -2;int authoritylen=pathstart-(url+scheme);if(authoritylen>=270)return -2;memcpy(authority,url+scheme,authoritylen);authority[authoritylen]=0;
    for(int i=0;path[i];i++)if(path[i]=='#'){path[i]=0;break;}
    if(!ready||fault||!netif_is_link_up(&nic))return -1;u32 end=ticks()+15000;while(ip4_addr_isany_val(*netif_ip4_addr(&nic))){int r=service(end);if(r)return r;}
    memset(&fetch,0,sizeof(fetch));fetch.out=out;fetch.cap=cap;fetch.secure=secure;tls_head=tls_used=0;fetch.token=++token;out[0]=0;strcpy(fetch.request,"GET ");strcpy(fetch.request+4,path);strcpy(fetch.request+strlen(fetch.request)," HTTP/1.0\r\nHost: ");strcpy(fetch.request+strlen(fetch.request),authority);strcpy(fetch.request+strlen(fetch.request),"\r\nUser-Agent: BDS-Browser/1.0\r\nAccept: text/html,text/plain,*/*;q=0.5\r\nAccept-Encoding: identity\r\nConnection: close\r\n\r\n");
    err_t r=dns_gethostbyname(host,&fetch.address,resolved,(void*)fetch.token);if(r==ERR_OK)fetch.resolved=1;else if(r!=ERR_INPROGRESS)return -3;
    end=ticks()+15000;while(!fetch.resolved&&!fetch.error){int e=service(end);if(e){fetch.token=++token;return e;}}if(fetch.error)return fetch.error;
    if(secure){int e=tls_start(host);if(e){tls_clear();return e;}}
    fetch.pcb=tcp_new();if(!fetch.pcb){tls_clear();return -4;}tcp_err(fetch.pcb,tcp_error);r=tcp_connect(fetch.pcb,&fetch.address,port,connected);if(r!=ERR_OK){tcp_abort(fetch.pcb);fetch.pcb=0;tls_clear();return -4;}
    end=ticks()+60000;while(!fetch.done&&!fetch.error){
        if(secure&&fetch.connected){int r=tls_step(tls_send,tls_receive,tls_plain,fetch.request);if(r<0)fetch.error=r;else if(r>0)fetch.done=1;}
        if(fetch.done||fetch.error)break;int e=service(end);if(e){fetch.error=e;break;}}

    int result=fetch.error?fetch.error:(int)fetch.used;if(fetch.error){char n[16];number(n,fetch.used);serial("NET failed HTTP received bytes: ");serial(n);serial("\n");}if(fetch.pcb){struct tcp_pcb*p=fetch.pcb;tcp_arg(p,0);tcp_err(p,0);tcp_recv(p,0);if(fetch.error||tcp_close(p)!=ERR_OK)tcp_abort(p);fetch.pcb=0;}fetch.out=0;fetch.token=++token;tls_clear();memset(tls_rx,0,sizeof(tls_rx));
    return result;
}
