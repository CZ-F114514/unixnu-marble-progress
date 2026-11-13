/* DEVELOPMENT-only bounded RAM bootstrap observer; no native return policy changes. */
struct marble_ld_event {
 uint64_t pc,lr,slot0,slot1;
 unsigned sequence,generation,code,effective,kind,phase,slots_read,returned;
 unsigned path_attempted,path_cleaned;int path_error;char path[64];
};
struct marble_ld_sysctl {
 unsigned sampled,namelen,mib_count,mib_truncated;
 int mib_error,pre_error,post_error;unsigned oldlen_present;
 int32_t mib[8];uint64_t oldptr,oldlenp,newptr,newlen,pre_len,post_len;
};
struct marble_ld_pending {struct marble_ld_event event;struct marble_ld_sysctl sysctl;};
struct marble_ld_bresult {struct marble_ld_event event;int error,r0,r1;};
struct marble_ld_dup {uint64_t lr;unsigned sequence,generation,from,to;int error,r0,r1;};
struct marble_ld_error {unsigned sequence,code,effective;int error;uint64_t slot0,slot1;unsigned path_flags;int path_error;char path[24];};
struct marble_ld_view {
 unsigned enabled,budget,limit,uuid_ok,generation,total,armed,ndup,bpre,bret,lost,errors,has_first,nerrors;
 uint64_t header,dyld,start,end;
 struct marble_ld_dup dup[3];struct marble_ld_bresult post[4],first;
 struct marble_ld_sysctl first_sysctl;struct marble_ld_error error[16];
};
struct marble_ld_state {struct marble_ld_view v;struct marble_ld_pending pending[8];};
static struct marble_ld_state marble_ld;
static LCK_SPIN_DECLARE(marble_ld_lock, &proc_lck_grp);
static void
marble_ld_begin(void)
{
 int n=4096,mode=0,limit=64;
 PE_parse_boot_argn("marble-launchd-budget",&n,sizeof(n));
 PE_parse_boot_argn("marble-launchd-follow",&mode,sizeof(mode));
 PE_parse_boot_argn("marble-bootstrap-budget",&limit,sizeof(limit));
 if(n<1 || n>4096 || mode!=3 || limit<1 || limit>256) panic("LD ERR invalid budget/mode/limit %d/%d/%d",n,mode,limit);
 lck_spin_lock(&marble_ld_lock);bzero(&marble_ld,sizeof(marble_ld));
 marble_ld.v.enabled=1;marble_ld.v.budget=(unsigned)n;marble_ld.v.limit=(unsigned)limit;
 lck_spin_unlock(&marble_ld_lock);
}
void
marble_ld_image_ready(proc_t p,uint64_t header,uint64_t dyld,const uint8_t *uuid)
{
 static const uint8_t expected[16]={0x4b,0x52,0x59,0x92,0x8c,0x69,0x31,0x5e,0xa5,0x1a,0xcc,0x94,0x33,0xe4,0x5b,0x51};
 if(proc_getpid(p)!=1) return;
 unsigned valid=memcmp(uuid,expected,sizeof(expected))==0;
 uint64_t start=header+5008,end=start+336872;
 if(header<4096 || header>=UINT64_C(0x0000ffffffffffff) || start<header || end<start || end>=UINT64_C(0x0000ffffffffffff)) valid=0;
 lck_spin_lock(&marble_ld_lock);
 struct marble_ld_view *v=&marble_ld.v;
 if(v->enabled) {
  unsigned generation=v->generation<0xffffffffu?v->generation+1:v->generation,budget=v->budget,limit=v->limit;
  bzero(&marble_ld,sizeof(marble_ld));v=&marble_ld.v;
  v->enabled=1;v->budget=budget;v->limit=limit;v->generation=generation;
  v->uuid_ok=valid;v->header=header;v->dyld=dyld;v->start=start;v->end=end;
 }
 lck_spin_unlock(&marble_ld_lock);
}
static void
marble_ld_print_event(char *message,size_t cap,size_t *used,const char *tag,const struct marble_ld_event *e)
{
 marble_stdio_append(message,cap,used,"%s #%u g%u %u/%u a=%llx/%llx\n",tag,e->sequence,e->generation,e->code,e->effective,(unsigned long long)e->slot0,(unsigned long long)e->slot1);
 marble_stdio_append(message,cap,used," pc=%llx lr=%llx k%u n%u\n",(unsigned long long)e->pc,(unsigned long long)e->lr,e->kind,e->slots_read);
}
static void
marble_ld_dump(const struct marble_ld_view *v,unsigned stop,const struct marble_ld_event *cause)
{
 const struct marble_ld_event *finger=v->has_first?&v->first.event:cause;
 uint64_t center=finger->kind==2?finger->pc:finger->lr;uint32_t sig[8]={0};int copied=EINVAL;
 if(center>=4096 && center<UINT64_C(0x0000ffffffffffff) && !(center&3)) copied=copyin((user_addr_t)(center-16),sig,sizeof(sig));
 char message[2048]={0};size_t used=0;
 marble_stdio_append(message,sizeof(message),&used,"LD ERR n=%u pre=%u ret=%u limit=%u stop=%u\n",v->total,v->bpre,v->bret,v->limit,stop);
 marble_stdio_append(message,sizeof(message),&used,"u%u g%u arm%u dup%u lost%u errors%u kept=%u over=%u\n",v->uuid_ok,v->generation,v->armed,v->ndup,v->lost,v->errors,v->nerrors,v->errors-v->nerrors);
 marble_stdio_append(message,sizeof(message),&used,"main=%llx text=%llx..%llx d%llx\n",(unsigned long long)v->header,(unsigned long long)v->start,(unsigned long long)v->end,(unsigned long long)v->dyld);
 for(unsigned i=0;i<v->ndup && i<3;i++) {
  const struct marble_ld_dup *d=&v->dup[i];
  marble_stdio_append(message,sizeof(message),&used,"D%u#%u %u>%u e%d r%d/%d;",i,d->sequence,d->from,d->to,d->error,d->r0,d->r1);
 }
 if(v->ndup) marble_stdio_append(message,sizeof(message),&used,"\n");
 marble_ld_print_event(message,sizeof(message),&used,"CAUSE",cause);
 marble_stdio_append(message,sizeof(message),&used,"sig %s center=%llx copy=%d\n",v->has_first?"FIRST":"CAUSE",(unsigned long long)center,copied);
 for(unsigned i=0;i<8;i++) marble_stdio_append(message,sizeof(message),&used,"%08x%s",sig[i],i==7?"\n":" ");
 if(v->has_first) {
  marble_stdio_append(message,sizeof(message),&used,"FIRST e%d r%x/%x\n",v->first.error,(unsigned)v->first.r0,(unsigned)v->first.r1);
  marble_ld_print_event(message,sizeof(message),&used,"F",&v->first.event);
  if(v->first.event.effective==SYS_sysctl) {
   const struct marble_ld_sysctl *s=&v->first_sysctl;
   marble_stdio_append(message,sizeof(message),&used,"SC a6=%u n=%u got=%u t%u me=%d\n",s->sampled,s->namelen,s->mib_count,s->mib_truncated,s->mib_error);
   for(unsigned i=0;i<s->mib_count;i++) {
    if(!(i%4)) marble_stdio_append(message,sizeof(message),&used,"M%u",i);
    marble_stdio_append(message,sizeof(message),&used," %d",s->mib[i]);
    if(i%4==3 || i+1==s->mib_count) marble_stdio_append(message,sizeof(message),&used,"\n");
   }
   marble_stdio_append(message,sizeof(message),&used,"OLD p%u ptr=%llx lenp=%llx\n",s->oldlen_present,(unsigned long long)s->oldptr,(unsigned long long)s->oldlenp);
   marble_stdio_append(message,sizeof(message),&used,"LEN pre=%llu e%d\n",(unsigned long long)s->pre_len,s->pre_error);
   marble_stdio_append(message,sizeof(message),&used,"LEN post=%llu e%d\n",(unsigned long long)s->post_len,s->post_error);
   marble_stdio_append(message,sizeof(message),&used,"NEW ptr=%llx len=%llu\n",(unsigned long long)s->newptr,(unsigned long long)s->newlen);
  }
 }
 for(unsigned i=0;i<v->nerrors && i<16;i++) {
  const struct marble_ld_error *e=&v->error[i];
  marble_stdio_append(message,sizeof(message),&used,"E%u #%u s%u/%u e%d",i+1,e->sequence,e->code,e->effective,e->error);
  if(e->path_flags&1) marble_stdio_append(message,sizeof(message),&used," P%d%c:%.20s",e->path_error,e->path_flags&2?'C':'R',e->path);
  marble_stdio_append(message,sizeof(message),&used,"\n");
 }
 if(v->bret) {
  const struct marble_ld_bresult *last=&v->post[(v->bret-1)%4];
  marble_stdio_append(message,sizeof(message),&used,"LAST #%u e%d r%x/%x\n",last->event.sequence,last->error,(unsigned)last->r0,(unsigned)last->r1);
 }
 marble_stdio_append(message,sizeof(message),&used,"7post;2/3/6/9/10pre;5dupErr;Pprefix;C-return only");
 panic("%s",message);
}
static unsigned
marble_ld_path_call(unsigned code)
{
 return code==SYS_open || code==SYS_open_nocancel || code==SYS_access || code==SYS_chdir || code==SYS_stat64 || code==SYS_lstat64 || code==SYS_getattrlist || code==SYS_readlink;
}
static void
marble_ld_sysctl_pre(struct marble_ld_sysctl *s,unsigned argc,const void *args)
{
 if(argc<6) return;
 uint64_t slots[6];memcpy(slots,args,sizeof(slots));
 s->sampled=1;s->namelen=(uint32_t)slots[1];s->mib_count=s->namelen>8?8:s->namelen;s->mib_truncated=s->namelen>8;
 s->oldptr=slots[2];s->oldlenp=slots[3];s->newptr=slots[4];s->newlen=slots[5];s->oldlen_present=s->oldlenp!=0;
 if(s->mib_count) s->mib_error=copyin((user_addr_t)slots[0],s->mib,s->mib_count*sizeof(s->mib[0]));
 if(s->oldlen_present) s->pre_error=copyin((user_addr_t)s->oldlenp,&s->pre_len,sizeof(s->pre_len));
}
uint64_t
marble_ld_syscall(proc_t p,arm_saved_state_t *s,unsigned code,unsigned effective,unsigned argc,const void *args,unsigned reboot_requested)
{
 if(p!=initproc || p!=current_proc() || proc_getpid(p)!=1 || !is_saved_state64(s)) return 0;
 struct marble_ld_pending record={0};struct marble_ld_event *event=&record.event;
 event->pc=get_saved_state_pc(s);event->lr=get_saved_state_lr(s);event->code=code;event->effective=effective;
 event->slots_read=argc>2?2:argc;
 if(event->slots_read) {uint64_t slots[2]={0};memcpy(slots,args,event->slots_read*sizeof(uint64_t));event->slot0=slots[0];event->slot1=slots[1];}
 struct marble_ld_view snapshot;unsigned stop=0;uint64_t ticket=0;
 lck_spin_lock(&marble_ld_lock);
 struct marble_ld_view *v=&marble_ld.v;
 if(v->enabled && v->generation) {
  if(v->total<0xffffffffu) v->total++;
  event->sequence=v->total;event->generation=v->generation;
  if(v->uuid_ok) {
   if(event->lr>=v->start && event->lr<v->end) event->kind=1;
   else if(event->pc>=v->start && event->pc<v->end) event->kind=2;
  }
  event->phase=v->armed;
  if(event->phase && v->bpre<0xffffffffu) v->bpre++;
  marble_ld.pending[(event->sequence-1)%8]=record;
  stop=reboot_requested?3:(code==SYS_exit?6:(!v->uuid_ok?10:(v->total>=v->budget?2:(event->phase && v->bpre>=v->limit*4?9:0))));
  if(event->phase || (event->kind && code==SYS_dup2 && effective==SYS_dup2 && event->slots_read==2)) ticket=((uint64_t)event->generation<<32)|event->sequence;
  if(stop) snapshot=*v;
 }
 lck_spin_unlock(&marble_ld_lock);
 if(event->phase && event->slots_read && marble_ld_path_call(event->effective)) {
  size_t copied=0;event->path_attempted=1;
  event->path_error=copyinstr((user_addr_t)event->slot0,event->path,sizeof(event->path),&copied);event->path[sizeof(event->path)-1]=0;
  for(unsigned i=0;i<sizeof(event->path) && event->path[i];i++) if((unsigned char)event->path[i]<32 || (unsigned char)event->path[i]>126) {event->path[i]='?';event->path_cleaned=1;}
 }
 if(event->phase && event->effective==SYS_sysctl) marble_ld_sysctl_pre(&record.sysctl,argc,args);
 if(event->phase) {
  lck_spin_lock(&marble_ld_lock);
  struct marble_ld_pending *pending=&marble_ld.pending[(event->sequence-1)%8];
  if(marble_ld.v.generation==event->generation && pending->event.sequence==event->sequence && pending->event.generation==event->generation && !pending->event.returned) *pending=record;
  lck_spin_unlock(&marble_ld_lock);
 }
 if(stop) marble_ld_dump(&snapshot,stop,event);
 return ticket;
}
void
marble_ld_return(proc_t p,uint64_t ticket,int error,int r0,int r1)
{
 if(!ticket || p!=initproc || p!=current_proc() || proc_getpid(p)!=1) return;
 unsigned sequence=(unsigned)ticket,generation=(unsigned)(ticket>>32),stop=0,valid=0;
 if(!sequence) return;
 struct marble_ld_view snapshot;struct marble_ld_pending record={0};
 lck_spin_lock(&marble_ld_lock);
 struct marble_ld_view *v=&marble_ld.v;struct marble_ld_pending *pending=&marble_ld.pending[(sequence-1)%8];
 struct marble_ld_event *e=&pending->event;
 if(!v->enabled || !v->uuid_ok || generation!=v->generation || e->sequence!=sequence || e->generation!=generation || e->returned) {
  if(v->lost<0xffffffffu) v->lost++;
 } else {e->returned=1;record=*pending;valid=1;}
 lck_spin_unlock(&marble_ld_lock);
 if(!valid) return;
 if(record.sysctl.sampled && record.sysctl.oldlen_present) record.sysctl.post_error=copyin((user_addr_t)record.sysctl.oldlenp,&record.sysctl.post_len,sizeof(record.sysctl.post_len));
 lck_spin_lock(&marble_ld_lock);
 v=&marble_ld.v;e=&record.event;
 if(!v->enabled || !v->uuid_ok || generation!=v->generation) {
  if(v->lost<0xffffffffu) v->lost++;
 } else if(e->phase) {
  if(v->bret<0xffffffffu) v->bret++;
  struct marble_ld_bresult *r=&v->post[(v->bret-1)%4];r->event=*e;r->error=error;r->r0=r0;r->r1=r1;
  if(error) {
   if(v->errors<0xffffffffu) v->errors++;
   if(!v->has_first) {v->first=*r;v->first_sysctl=record.sysctl;v->has_first=1;}
   if(v->nerrors<16) {
    struct marble_ld_error *row=&v->error[v->nerrors++];
    row->sequence=sequence;row->code=e->code;row->effective=e->effective;row->error=error;row->slot0=e->slot0;row->slot1=e->slot1;
    row->path_flags=e->path_attempted|(e->path_cleaned<<1);row->path_error=e->path_error;memcpy(row->path,e->path,sizeof(row->path)-1);row->path[sizeof(row->path)-1]=0;
   }
  }
  if(v->bret>=v->limit) stop=7;
 } else if(e->kind && e->code==SYS_dup2 && e->effective==SYS_dup2 && e->slots_read==2 && v->ndup<3) {
  struct marble_ld_dup *d=&v->dup[v->ndup++];
  d->sequence=e->sequence;d->generation=e->generation;d->from=(uint32_t)e->slot0;d->to=(uint32_t)e->slot1;d->lr=e->lr;d->error=error;d->r0=r0;d->r1=r1;
  if(error) stop=5;else if(v->ndup==3) v->armed=1;
 }
 if(stop) snapshot=*v;
 lck_spin_unlock(&marble_ld_lock);
 if(stop) marble_ld_dump(&snapshot,stop,&record.event);
}
