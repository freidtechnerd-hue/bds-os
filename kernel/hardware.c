#include "os.h"
#include <cpuid.h>
u32 hardware_fxsr;
static volatile u32 timer;
static int serial_present;
static int queue[128],qh,qt,events[128],eh,et,shift,alt,ctrl;
int mouse_x=400,mouse_y=300,mouse_buttons;
static int relative_x,relative_y;
int input_relative(int*x,int*y){input_poll();*x=relative_x;*y=relative_y;relative_x=relative_y=0;return mouse_buttons;}
int input_modifiers(void){input_poll();return shift|(ctrl<<1)|(alt<<2);}
static u8 cmos(u8 r){outb(0x70,r);return inb(0x71);}
void input_date_time(int*d){u8 v[6],again[6],regs[]={9,8,7,4,2,0};for(int tries=0;tries<4;tries++){for(int wait=0;wait<100000&&(cmos(10)&128);wait++);for(int i=0;i<6;i++)v[i]=cmos(regs[i]);for(int i=0;i<6;i++)again[i]=cmos(regs[i]);if(!memcmp(v,again,6))break;}int flags=cmos(11),pm=v[3]&128;v[3]&=127;for(int i=0;i<6;i++)d[i]=(flags&4)?v[i]:(v[i]>>4)*10+(v[i]&15);if(!(flags&2))d[3]=(d[3]%12)+(pm?12:0);d[0]+=2000;}
static u8 mouse_packet[3];static int mp;
#ifdef __x86_64__
struct __attribute__((packed)) Gate{u16 low,selector;u8 zero,flags;u16 high;u32 top,reserved;};
#else
struct __attribute__((packed)) Gate{u16 low,selector;u8 zero,flags;u16 high;};
#endif
static struct Gate idt[256];
extern void irq_timer(void),fault(void),load_gdt(void);
extern void (*fault_vectors[32])(void);
void serial_init(void){
    /* Absent UART ports read as 0xff, including the receive-ready bit. */
    u8 scratch=inb(0x3ff);outb(0x3ff,0x5a);serial_present=inb(0x3ff)==0x5a;outb(0x3ff,scratch);
    if(!serial_present)return;
    outb(0x3f9,0);outb(0x3fb,0x80);outb(0x3f8,1);outb(0x3f9,0);outb(0x3fb,3);outb(0x3fa,0xc7);
}
void serial(const char*s){if(!serial_present)return;while(*s){while(!(inb(0x3fd)&32));outb(0x3f8,*s++);}}
void timer_irq(void){timer++;outb(0x20,0x20);}
static void fault_hex(u32 n){char s[9];for(int i=0;i<8;i++)s[i]="0123456789ABCDEF"[(n>>(28-i*4))&15];s[8]=0;serial(s);}
void fault_c(const usize*frame){
    u16 control,status;__asm__ volatile("fnstcw %0; fnstsw %1":"=m"(control),"=m"(status));
    serial("FATAL: CPU exception vector=");fault_hex(frame[0]);serial(" error=");fault_hex(frame[1]);serial(" eip=");fault_hex(frame[2]);serial(" x87cw=");fault_hex(control);serial(" x87sw=");fault_hex(status);serial("\n");
    __asm__ volatile("cli");for(;;)__asm__ volatile("hlt");
}
static void gate(int n,void(*f)(void)){usize a=(usize)f;idt[n]=(struct Gate){a,8,0,0x8e,a>>16};
#ifdef __x86_64__
    idt[n].top=a>>32;
#endif
}
static void pswait(void){for(int i=0;i<100000;i++)if(!(inb(0x64)&2))break;}
void hardware_init(void){
    usize cr0;__asm__ volatile("mov %%cr0,%0":"=r"(cr0));cr0=(cr0&~(usize)12)|32u;__asm__ volatile("mov %0,%%cr0; fninit"::"r"(cr0));
    u32 a,b,c,d;
    if(__get_cpuid(1,&a,&b,&c,&d)&&(d&(1u<<24))&&(d&(1u<<25))){
        usize cr4;u32 mxcsr=0x1f80;__asm__ volatile("mov %%cr4,%0":"=r"(cr4));cr4|=0x600;
        __asm__ volatile("mov %0,%%cr4; ldmxcsr %1"::"r"(cr4),"m"(mxcsr));hardware_fxsr=1;
    }
    load_gdt();for(int i=0;i<256;i++)gate(i,fault);for(int i=0;i<32;i++)gate(i,fault_vectors[i]);gate(32,irq_timer);
    struct __attribute__((packed)){u16 size;usize addr;}ptr={sizeof(idt)-1,(usize)idt};
    __asm__ volatile("lidt %0"::"m"(ptr));
    outb(0x20,0x11);outb(0xa0,0x11);outb(0x21,32);outb(0xa1,40);outb(0x21,4);outb(0xa1,2);outb(0x21,1);outb(0xa1,1);
    outb(0x21,0xfe);outb(0xa1,0xff);outb(0x43,0x36);outb(0x40,1193&255);outb(0x40,1193>>8);
    pswait();outb(0x64,0xa8);pswait();outb(0x64,0xd4);pswait();outb(0x60,0xf4);
    __asm__ volatile("sti");
}
u32 ticks(void){return timer;}
static const char normal[128]={[2]='1',[3]='2',[4]='3',[5]='4',[6]='5',[7]='6',[8]='7',[9]='8',[10]='9',[11]='0',[12]='-',[13]='=',[14]=8,[15]=9,[16]='q',[17]='w',[18]='e',[19]='r',[20]='t',[21]='y',[22]='u',[23]='i',[24]='o',[25]='p',[26]='[',[27]=']',[28]=13,[30]='a',[31]='s',[32]='d',[33]='f',[34]='g',[35]='h',[36]='j',[37]='k',[38]='l',[39]=';',[40]='\'',[41]='`',[43]='\\',[44]='z',[45]='x',[46]='c',[47]='v',[48]='b',[49]='n',[50]='m',[51]=',',[52]='.',[53]='/',[57]=' '};
static void enqueue(int k,int release){if(!k)return;int n=(et+1)%128;if(n!=eh){events[et]=k|(release?KEY_RELEASE:0);et=n;}if(!release){n=(qt+1)%128;if(n!=qh){queue[qt]=k;qt=n;}}}
void input_poll(void){
    while(inb(0x64)&1){u8 st=inb(0x64),s=inb(0x60);if(st&32){if(!mp&&(s==0xfa||!(s&8)))continue;mouse_packet[mp++]=s;if(mp==3){mp=0;if(!(mouse_packet[0]&0xc0)){relative_x+=(signed char)mouse_packet[1];relative_y-=(signed char)mouse_packet[2];mouse_x+=(signed char)mouse_packet[1];mouse_y-=(signed char)mouse_packet[2];if(mouse_x<0)mouse_x=0;if(mouse_x>=screen_w)mouse_x=screen_w-1;if(mouse_y<0)mouse_y=0;if(mouse_y>=screen_h)mouse_y=screen_h-1;mouse_buttons=mouse_packet[0]&7;}}continue;}
    if(s==0xe0)continue;int release=s&128;s&=127;if(s==42||s==54){shift=!release;continue;}if(s==56){alt=!release;continue;}if(s==29){ctrl=!release;continue;}if(ctrl&&s==33){if(!release)app_fullscreen(-1);continue;}if(ctrl&&(s==46||s==47||s==45||s==30)){enqueue(s==46?KEY_COPY:s==47?KEY_PASTE:s==45?KEY_CUT:KEY_SELECT_ALL,release);continue;}if(ctrl&&s==38){enqueue(KEY_ADDRESS,release);continue;}int k=0;
    if(s==87)k=KEY_F1+10;else if(s==88)k=KEY_F1+11;else if(s==1)k=27;else if(s>=59&&s<=68)k=KEY_F1+s-59;else if(s==72)k=KEY_UP;else if(s==80)k=KEY_DOWN;else if(s==75)k=KEY_LEFT;else if(s==77)k=KEY_RIGHT;else k=normal[s];
    if(shift&&k>='a'&&k<='z')k-=32;else if(shift){const char*lo="1234567890-=[];'`,./\\";const char*hi="!@#$%^&*()_+{}:\"~<>?|";for(int j=0;lo[j];j++)if(k==lo[j]){k=hi[j];break;}}if(alt&&k>=KEY_UP&&k<=KEY_RIGHT)k+=256;enqueue(k,release);
    }
    while(serial_present&&(inb(0x3fd)&1)){int c=inb(0x3f8);if(c=='\n')c=13;enqueue(c,0);enqueue(c,1);}
}
int keyevent(void){input_poll();if(eh==et)return 0;int k=events[eh];eh=(eh+1)%128;return k;}
extern void console_full(void);
int key(int wait){if(wait)console_full();do{input_poll();if(qh!=qt){int k=queue[qh];qh=(qh+1)%128;return k;}if(wait)__asm__ volatile("hlt");}while(wait);return 0;}
void sleep(u32 ms){u32 end=ticks()+ms;while((int)(ticks()-end)<0){input_poll();audio_pump();net_poll();__asm__ volatile("hlt");}}
