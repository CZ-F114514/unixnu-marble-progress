#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <setjmp.h>
#include <assert.h>
typedef uint64_t user_addr_t;
typedef struct {int pid;} *proc_t;
typedef struct {uint64_t pc,lr;} arm_saved_state_t;
static struct {int pid;} initial={1}, other={2};
static proc_t initproc=(proc_t)&initial,current;
#ifndef TEST_MODE
#define TEST_MODE 2
#endif
static int mode=TEST_MODE,budget=4096,limit=64,held,panicked;
static char output[4096];static jmp_buf jump;
static const uint8_t uuid[16]={0x4b,0x52,0x59,0x92,0x8c,0x69,0x31,0x5e,0xa5,0x1a,0xcc,0x94,0x33,0xe4,0x5b,0x51};
#define SYS_dup2 90
#define SYS_exit 1
#define SYS_open 5
#define SYS_chdir 12
#define SYS_access 33
#define SYS_readlink 58
#define SYS_getattrlist 220
#define SYS_stat64 338
#define SYS_lstat64 340
#define SYS_open_nocancel 398
#define SYS_sysctl 202
#define LCK_SPIN_DECLARE(n,g) int n
#define bzero(p,n) memset(p,0,n)
static void lck_spin_lock(int*p){(void)p;assert(!held);held=1;}
static void lck_spin_unlock(int*p){(void)p;assert(held);held=0;}
static void PE_parse_boot_argn(const char*n,int*p,size_t s){(void)s;if(!strcmp(n,"marble-launchd-follow"))*p=mode;else if(!strcmp(n,"marble-launchd-budget"))*p=budget;else if(!strcmp(n,"marble-bootstrap-budget"))*p=limit;}
static int proc_getpid(proc_t p){return p->pid;}
static proc_t current_proc(void){return current;}
static int is_saved_state64(arm_saved_state_t*s){(void)s;return 1;}
static uint64_t get_saved_state_pc(arm_saved_state_t*s){return s->pc;}
static uint64_t get_saved_state_lr(arm_saved_state_t*s){return s->lr;}
static const void *allowed[8];static size_t sizes[8];static unsigned ranges,copy_count;static int force_fault;
static void (*copy_hook)(void);
static void allow(const void*p,size_t n){assert(ranges<8);allowed[ranges]=p;sizes[ranges++]=n;}
static int copyin(user_addr_t p,void*d,size_t n){assert(!held);copy_count++;if(copy_hook){void(*hook)(void)=copy_hook;copy_hook=NULL;hook();}if(force_fault)return EFAULT;for(unsigned i=0;i<ranges;i++)if(p>=(uintptr_t)allowed[i]&&p-(uintptr_t)allowed[i]<=sizes[i]&&n<=sizes[i]-(p-(uintptr_t)allowed[i])){memcpy(d,(const void*)(uintptr_t)p,n);return 0;}return EFAULT;}
static int copyinstr(user_addr_t p,void*d,size_t n,size_t*done){assert(!held);if(force_fault)return EFAULT;for(unsigned i=0;i<ranges;i++)if(p==(uintptr_t)allowed[i]){size_t l=strnlen((const char*)allowed[i],sizes[i]);if(l==sizes[i])return EFAULT;l++;if(l>n){memcpy(d,allowed[i],n);*done=n;return ENAMETOOLONG;}memcpy(d,allowed[i],l);*done=l;return 0;}return EFAULT;}
static void marble_stdio_append(char*d,size_t cap,size_t*u,const char*f,...){assert(!held);if(*u>=cap)return;va_list a;va_start(a,f);int n=vsnprintf(d+*u,cap-*u,f,a);va_end(a);if(n>0)*u+=(size_t)n<cap-*u?(size_t)n:cap-*u;}
__attribute__((noreturn)) static void panic(const char*f,...){assert(!held);va_list a;va_start(a,f);vsnprintf(output,sizeof(output),f,a);va_end(a);panicked++;longjmp(jump,1);}
#ifndef OBSERVER_HEADER
#define OBSERVER_HEADER "bootstrap-helper.c.inc"
#endif
#include OBSERVER_HEADER
static arm_saved_state_t regs={.pc=0x201000,.lr=0x101600};
static uint64_t enter(unsigned code,unsigned effective,unsigned n,const uint64_t*a){return marble_ld_syscall(initproc,&regs,code,effective,n,a,0);}
static void begin(void){held=panicked=ranges=copy_count=force_fault=0;copy_hook=NULL;output[0]=0;current=initproc;mode=TEST_MODE;budget=4096;limit=64;marble_ld_begin();marble_ld_image_ready(initproc,0x100000,0,uuid);}
static void arm(void){for(unsigned i=0;i<3;i++){uint64_t a[2]={i,i};uint64_t t=enter(90,90,2,a);assert(t);marble_ld_return(initproc,t,0,i,0);}assert(marble_ld.v.armed&&!panicked);}
static void finish(unsigned n){while(marble_ld.v.bret<n){uint64_t t=enter(20,20,0,NULL);if(!setjmp(jump)){marble_ld_return(initproc,t,0,1,0);assert(marble_ld.v.bret<n);}else assert(marble_ld.v.bret==n);}assert(panicked&&strstr(output,"stop=7"));}
static void test_all_eleven_errors_visible(void){begin();arm();for(unsigned i=0;i<11;i++){uint64_t t=enter(202,202,0,NULL);marble_ld_return(initproc,t,12,0,0);}finish(64);assert(strstr(output,"kept=11 over=0"));assert(strstr(output,"E11 #14 s202/202 e12"));}
static void test_sysctl_lengths(void){begin();arm();int32_t mib[2]={1,10};uint64_t len=4;allow(mib,sizeof(mib));allow(&len,sizeof(len));uint64_t a[6]={(uintptr_t)mib,UINT64_C(0xdeadbeef00000002),0x4000,(uintptr_t)&len,0,0};uint64_t t=enter(202,202,6,a);len=8;marble_ld_return(initproc,t,12,0,0);finish(64);assert(strstr(output,"SC a6=1 n=2 got=2 t0 me=0"));assert(strstr(output,"M0 1 10\n"));assert(strstr(output,"LEN pre=4 e0"));assert(strstr(output,"LEN post=8 e0"));}
static void test_short_null_and_bounded_args(void){begin();arm();uint64_t *short_args=malloc(2*sizeof(*short_args));short_args[0]=0;short_args[1]=2;uint64_t t=enter(202,202,2,short_args);assert(copy_count==0);marble_ld_return(initproc,t,9,0,0);free(short_args);finish(64);assert(strstr(output,"SC a6=0"));
 begin();arm();uint64_t a[6]={0,0,0,0,0,0};t=enter(202,202,6,a);assert(copy_count==0);marble_ld_return(initproc,t,12,0,0);finish(64);assert(strstr(output,"n=0 got=0 t0 me=0"));assert(strstr(output,"OLD p0"));
 begin();arm();int32_t mib[8]={1,2,3,4,5,6,7,8};allow(mib,sizeof(mib));a[0]=(uintptr_t)mib;a[1]=UINT32_MAX;t=enter(202,202,6,a);assert(copy_count==1);marble_ld_return(initproc,t,12,0,0);finish(64);assert(strstr(output,"n=4294967295 got=8 t1 me=0"));assert(strstr(output,"M4 5 6 7 8"));}
static void test_copy_errors_do_not_change_native_error(void){begin();arm();uint64_t len=4;allow(&len,sizeof(len));uint64_t a[6]={1,2,0x4000,(uintptr_t)&len,0,0};uint64_t t=enter(202,202,6,a);force_fault=1;marble_ld_return(initproc,t,12,0,0);force_fault=0;finish(64);assert(strstr(output,"me=14"));assert(strstr(output,"LEN pre=4 e0"));assert(strstr(output,"LEN post=0 e14"));assert(strstr(output,"FIRST e12"));}
static void new_generation(void){marble_ld_image_ready(initproc,0x100000,0,uuid);}
static void test_tickets_and_generation(void){begin();arm();uint64_t t=enter(20,20,0,NULL);marble_ld_return(initproc,t,12,0,0);unsigned copies=copy_count;marble_ld_return(initproc,t,12,0,0);assert(marble_ld.v.bret==1&&marble_ld.v.errors==1&&marble_ld.v.lost==1&&copy_count==copies);
 begin();arm();t=enter(20,20,0,NULL);for(unsigned i=0;i<8;i++)enter(20,20,0,NULL);marble_ld_return(initproc,t,0,0,0);assert(marble_ld.v.lost==1&&marble_ld.v.bret==0);
 begin();arm();uint64_t len=4;allow(&len,sizeof(len));uint64_t a[6]={0,0,0,(uintptr_t)&len,0,0};t=enter(202,202,6,a);copy_hook=new_generation;marble_ld_return(initproc,t,12,0,0);assert(marble_ld.v.generation==2&&marble_ld.v.lost==1&&!marble_ld.v.bret&&!marble_ld.v.has_first&&!marble_ld.v.armed);
 begin();arm();current=(proc_t)&other;assert(!enter(20,20,0,NULL));current=initproc;}
static void test_paths_overflow_and_limits(void){begin();arm();const char path[]="/missing\nfile";allow(path,sizeof(path));uint64_t a[2]={(uintptr_t)path,0};uint64_t t=enter(398,398,2,a);marble_ld_return(initproc,t,2,0,0);finish(64);assert(strstr(output,"P0C:/missing?file"));
 begin();arm();for(unsigned i=0;i<20;i++){t=enter(20,20,0,NULL);marble_ld_return(initproc,t,2,0,0);}finish(64);assert(strstr(output,"errors20 kept=16 over=4"));assert(strstr(output,"E16 #19")&&!strstr(output,"E17 #"));
 begin();limit=256;marble_ld_begin();marble_ld_image_ready(initproc,0x100000,0,uuid);arm();finish(256);assert(strstr(output,"ret=256 limit=256"));
 begin();arm();if(!setjmp(jump)){enter(1,1,0,NULL);assert(0);}assert(strstr(output,"stop=6"));
 begin();arm();if(!setjmp(jump)){marble_ld_syscall(initproc,&regs,55,55,0,NULL,1);assert(0);}assert(strstr(output,"stop=3"));
 begin();arm();marble_ld.v.limit=1;for(unsigned i=0;i<3;i++)enter(20,20,0,NULL);if(!setjmp(jump)){enter(20,20,0,NULL);assert(0);}assert(strstr(output,"stop=9"));}
static void test_display_stress(void){begin();arm();int32_t mib[8]={INT32_MIN,INT32_MAX,INT32_MIN,INT32_MAX,INT32_MIN,INT32_MAX,INT32_MIN,INT32_MAX};uint64_t len=UINT64_MAX;allow(mib,sizeof(mib));allow(&len,sizeof(len));uint64_t a[6]={(uintptr_t)mib,8,0xffffffffffff,(uintptr_t)&len,0xffffffffffff,UINT64_MAX};uint64_t t=enter(202,202,6,a);marble_ld_return(initproc,t,12,0,0);const char path[]="/System/Library/LaunchDaemons/long-test-path";allow(path,sizeof(path));a[0]=(uintptr_t)path;for(unsigned i=0;i<15;i++){t=enter(398,398,2,a);marble_ld_return(initproc,t,2,0,0);}finish(64);
 struct marble_ld_view view=marble_ld.v;view.total=4095;view.bpre=view.bret=view.limit=256;view.header=0x100300000;view.start=0x100301390;view.end=0x100353778;for(unsigned i=0;i<3;i++){view.dup[i].sequence=1062+3*i;view.dup[i].from=i==2?1:0;view.dup[i].to=i;}for(unsigned i=0;i<16;i++)view.error[i].sequence=2000+i;view.first.event.sequence=1071;view.first.event.slot0=0x16faff858;view.first.event.slot1=8;view.first.event.pc=0x1010ffaa0;view.first.event.lr=0x100dfa06c;view.first.event.kind=0;
 struct marble_ld_event cause={.sequence=4095,.generation=1,.code=46,.effective=46,.slots_read=2,.slot0=30,.slot1=0x16faff9e8,.pc=0x101101d30,.lr=0x1010ba808};view.post[3]=(struct marble_ld_bresult){.event=cause};if(!setjmp(jump)){marble_ld_dump(&view,7,&cause);assert(0);}FILE*f=fopen("display-stress.txt","wb");assert(f);assert(fwrite(output,1,strlen(output),f)==strlen(output));assert(!fclose(f));assert(strstr(output,"E16 #2015")&&strstr(output,"C-return only"));printf("DISPLAY_BYTES=%zu\n",strlen(output));}
int main(void){test_all_eleven_errors_visible();test_sysctl_lengths();test_short_null_and_bounded_args();test_copy_errors_do_not_change_native_error();test_tickets_and_generation();test_paths_overflow_and_limits();test_display_stress();puts("PASS:11errors,MIB/lengths,argc/null/bounds/copy faults,one-use/stale/overwrite/PID,paths/overflow,64/256/exits/pre-budget,display and lock discipline");return 0;}
