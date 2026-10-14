/* Generic read-only guest exception/IPC diagnostics. MIT; IOSretroMob.
 * Included after the existing exact-kernel-layout-gated diagnostic readers.
 * No syscall result, register, guest memory or scheduling decision is changed.
 * Numeric PID selection is a convenience, never a substitute for task identity.
 */
#ifndef IOSRETROMOB_GUEST_RUNTIME_DIAGNOSTICS_H
#define IOSRETROMOB_GUEST_RUNTIME_DIAGNOSTICS_H
#include "guest_ta_capture.h"
#define RD_THREADS 256u
#define RD_EVENTS 8192u
#define RD_BYTES (8u * 1024u * 1024u)
enum { RD_TRIAGE, RD_SIGNAL, RD_EXIT, RD_PORT_TASK, RD_PORT_THREAD,
       RD_TASK_TERM, RD_EXCEPTION_DELIVER, RD_MQUEUE, RD_BLOCK, RD_RUN,
       RD_SWI, RD_PORT_DESTROY, RD_PORT_DEALLOCATE, RD_PORT_MEMBER, RD_KILL, RD_PORT_CLEAR, RD_PORT_RELEASE, RD_PORT_DESTROY_OBJECT, RD_MQUEUE_CHANGED, RD_RIGHT_DESTROY, RD_SYMBOLS };
static const char *const rd_names[RD_SYMBOLS] = {
    "_exception_triage", "_psignal", "_exit1", "_convert_port_to_task",
    "_convert_port_to_thread", "_task_terminate", "_exception_deliver",
    "_ipc_mqueue_copyin", "_thread_block", "_thread_run", "_fleh_swi",
    "_mach_port_destroy", "_mach_port_deallocate", "_mach_port_move_member",
    "_kill", "_ipc_port_clear_receiver", "_ipc_port_release_receive",
    "_ipc_port_destroy", "_ipc_mqueue_changed", "_ipc_right_destroy" };
typedef struct {
    bool open;
    uint32_t thread, ttbr, context, sp, pc, thumb;
    uint32_t args[7];
    uint64_t at;
} rd_pending_t;
typedef struct {
    uint32_t thread;
    diagnostic_thread_identity_t id;
    uint64_t validated_at, samples, mach_calls, mach_returns, copyins, copyreturns;
    uint64_t faults, undefs, user_samples, kernel_samples, last_mach_at;
    uint32_t user_open, stack_error, port_ops; rd_pending_t user[8];
    uint32_t last_pc, last_ret, named_proc;
    uint32_t recent_pc[16],recent_n;uint64_t recent_at[16];
    rd_pending_t mach, copy, task, port_thread;
    rd_pending_t bl;unsigned bl_hits,bl_steps;
} rd_thread_t;
enum { RF_RUN, RF_SLEEP, RF_SLEEP_TIMED, RF_WAKE, RF_CLOSE, RF_LOOP_SLEEP,
       RF_LOOP_SLEEP_TIMED, RF_LOOP_WAKE, RF_MUTEX_SLEEP, RF_ASSERT_WAIT,
       RF_WAKE_PRIM, RF_LOCK, RF_BLOCK, RF_RECURSIVE_SLEEP, RF_ENABLE_IRQ, RF_DISABLE_IRQ, RF_TIMER_SET, RF_TIMER_FIRE, RF_TIMER_CANCEL, RF_COUNT };
static const char *const rf_names[RF_COUNT]={
    "__ZN13IOCommandGate9runActionEPFiP8OSObjectPvS2_S2_S2_ES2_S2_S2_S2_",
    "__ZN13IOCommandGate12commandSleepEPvm",
    "__ZN13IOCommandGate12commandSleepEPv12UnsignedWidem",
    "__ZN13IOCommandGate13commandWakeupEPvb",
    "__ZN10IOWorkLoop9closeGateEv", "__ZN10IOWorkLoop9sleepGateEPvm",
    "__ZN10IOWorkLoop9sleepGateEPv12UnsignedWidem",
    "__ZN10IOWorkLoop10wakeupGateEPvb", "_lck_mtx_sleep", "_assert_wait",
    "_thread_wakeup_prim", "_IORecursiveLockLock", "_thread_block",
    "_IORecursiveLockSleep", "__ZN9IOService15enableInterruptEi",
    "__ZN9IOService16disableInterruptEi", "__ZN18IOTimerEventSource10setTimeoutE13mach_timespec",
    "__ZN18IOTimerEventSource7timeoutEPv", "__ZN18IOTimerEventSource13cancelTimeoutEv" };
static struct {
    bool enabled, failed, finishing, entry_gate;
    const char *path;
    uint32_t pid, symbols[RD_SYMBOLS];
    uint64_t start, end, events, bytes, dropped, unknown_threads;
    uint64_t kinds[RD_SYMBOLS], faults, undefs, overwritten_pending, exec_calls;
    FILE *file;
    char exec_paths[2][256];uint32_t selected_tasks[2];unsigned exec_count;
    uint32_t sites[32],site_hits[32][3][2];unsigned site_count;
    uint32_t bl_sites[8];unsigned bl_count;bool graphics;
    unsigned graphics_records,access_hits[8],fifo_kept,fifo_seen,fifo_scenes;
    uint32_t fifo_chunk[36];unsigned fifo_chunk_n;
    uint64_t graphics_last[12],accepted_seen[S5L_MBX_3D_ACCEPT_HISTORY],rejected_seen[S5L_MBX_3D_REJECTION_HISTORY];bool graphics_have;
    struct {
        uint32_t trigger, symbols[RF_COUNT], gate, action, command, owner, loop;
        uint32_t thread, task, ttbr, context, action_pending_pc;
        uint32_t events[16], event_count, sessions, site_hits[32];
        uint32_t field_offsets[8], field_objects[8];unsigned field_count;
        uint64_t records, suppressed, early_triggers;
        bool active, action_executed;
        rd_pending_t returned;
    } focus;
    struct { uint32_t space,name,entry,object,bits,last[8]; unsigned changes; } ports[64];unsigned port_count;
    rd_thread_t threads[RD_THREADS];
    uint32_t cached_thread; rd_thread_t *cached;
} RD;
static bool rd_parse(uint64_t steps) {
    const char *pid = getenv("IOSRETROMOB_RUNTIME_DIAG_PID");
    const char *path = getenv("IOSRETROMOB_RUNTIME_DIAG_LOG");
    const char *start = getenv("IOSRETROMOB_RUNTIME_DIAG_START");
    const char *end = getenv("IOSRETROMOB_RUNTIME_DIAG_END");
    const char *execs=getenv("IOSRETROMOB_RUNTIME_DIAG_EXECUTABLES");
    const char *sites=getenv("IOSRETROMOB_RUNTIME_DIAG_KERNEL_SITES");
    const char *trigger=getenv("IOSRETROMOB_RUNTIME_DIAG_GATE_CALL");
    const char *fields=getenv("IOSRETROMOB_RUNTIME_DIAG_GATE_FIELDS");
    const char *bls=getenv("IOSRETROMOB_RUNTIME_DIAG_USER_BL_SITES");
    const char *graphics=getenv("IOSRETROMOB_RUNTIME_DIAG_GRAPHICS");
    if (!pid && !path && !start && !end && !execs && !sites && !trigger && !fields && !bls && !graphics) return true;
    if(graphics){if(strcmp(graphics,"1")&&strcmp(graphics,"0"))goto invalid;RD.graphics=!strcmp(graphics,"1");}
    if(bls){const char *a=bls;while(*a){char *z=NULL;errno=0;unsigned long v=strtoul(a,&z,0);
        if(errno||z==a||v>UINT32_MAX-4||v>=0xc0000000ul||(v&3)||RD.bl_count>=8||(*z&&*z!=','))goto invalid;
        RD.bl_sites[RD.bl_count++]=(uint32_t)v;if(!*z)break;a=z+1;if(!*a)goto invalid;}}
    if(execs) {
        const char *a=execs;
        while(*a) { const char *z=strchr(a,'|');size_t n=z?(size_t)(z-a):strlen(a);
            if(!n||n>=256||RD.exec_count>=2||*a!='/')goto invalid;
            memcpy(RD.exec_paths[RD.exec_count++],a,n);if(!z)break;a=z+1;if(!*a)goto invalid; }
    }
    if(sites) { const char *a=sites;
        while(*a) {char *z=NULL;errno=0;unsigned long v=strtoul(a,&z,0);
            if(errno||z==a||v<0xc0000000ul||v>UINT32_MAX||RD.site_count>=32||(*z&&*z!=','))goto invalid;
            RD.sites[RD.site_count++]=(uint32_t)v&~1u;if(!*z)break;a=z+1;if(!*a)goto invalid;}
    }
    uint64_t value;
    if(trigger) {
        if(!parse_u64_arg("runtime gate call",trigger,&value)||value<0xc0000000ul||
           value>UINT32_MAX-2||(value&1)||!RD.exec_count)goto invalid;
        RD.focus.trigger=(uint32_t)value;
    }
    if(fields) {const char *a=fields;if(!trigger||!*a)goto invalid;
        while(*a) {char *z=NULL;errno=0;unsigned long v=strtoul(a,&z,0);
            if(errno||z==a||v>4092||(v&3)||RD.focus.field_count>=8||(*z&&*z!=','))goto invalid;
            for(unsigned q=0;q<RD.focus.field_count;q++)if(RD.focus.field_offsets[q]==v)goto invalid;
            RD.focus.field_offsets[RD.focus.field_count++]=(uint32_t)v;
            if(!*z)break;a=z+1;if(!*a)goto invalid;}
    }
    value=0;
    if (!path || !*path || (!pid&&!RD.exec_count) ||
        (pid&&(!parse_u64_arg("runtime diagnostic PID",pid,&value)||!value||value>INT32_MAX)))goto invalid;
    RD.pid = (uint32_t)value;
    RD.start = 0; RD.end = steps;
    if ((start && !parse_u64_arg("runtime diagnostic start", start, &RD.start)) ||
        (end && !parse_u64_arg("runtime diagnostic end", end, &RD.end)) ||
        RD.start >= RD.end || RD.end > steps) goto invalid;
    RD.enabled = true; RD.path = path;
    printf("runtime diagnostics: read-only PID=%u detail=[%" PRIu64 ",%" PRIu64
           ") max-events=%u max-bytes=%u; exceptions before detail retained\n",
           RD.pid, RD.start, RD.end, RD_EVENTS, RD_BYTES);
    return true;
invalid:
    fprintf(stderr, "runtime diagnostics require a positive PID, new log path and reachable detail window\n");
    return false;
}
static bool rd_return_matches(const rd_pending_t *p, uint32_t thread,
        uint32_t ttbr, uint32_t context, uint32_t sp, uint32_t pc, uint32_t thumb) {
    return p->open && p->thread == thread && p->ttbr == ttbr &&
        p->context == context && p->sp == sp && p->pc == (pc & ~1u) && p->thumb == thumb;
}
static bool rd_selfcheck(void) {
    rd_pending_t p = {.open=true,.thread=1,.ttbr=2,.context=3,.sp=4,.pc=6,.thumb=ARM_CPSR_T};
    return rd_return_matches(&p,1,2,3,4,7,ARM_CPSR_T) &&
        !rd_return_matches(&p,9,2,3,4,6,ARM_CPSR_T) &&
        !rd_return_matches(&p,1,9,3,4,6,ARM_CPSR_T) &&
        !rd_return_matches(&p,1,2,9,4,6,ARM_CPSR_T) &&
        !rd_return_matches(&p,1,2,3,9,6,ARM_CPSR_T) &&
        !rd_return_matches(&p,1,2,3,4,6,0) &&
        !rd_return_matches(&p,1,2,3,4,8,ARM_CPSR_T);
}
static rd_thread_t *rd_lookup(uint32_t thread) {
    if (RD.cached_thread == thread) return RD.cached;
    RD.cached_thread=thread;RD.cached=NULL;
    for(unsigned i=0;i<RD_THREADS;i++) if(RD.threads[i].thread==thread) {RD.cached=&RD.threads[i];break;}
    return RD.cached;
}
static rd_thread_t *rd_thread(arm_cpu_t *cpu, uint64_t at, bool fresh) {
    uint32_t thread = cpu->cp15.tpidrprw;
    if (!thread || !G.springboard_child_config.enabled) return NULL;
    rd_thread_t *s = NULL;
    for (unsigned i=0; i<RD_THREADS; ++i) {
        if (RD.threads[i].thread == thread) { s=&RD.threads[i]; break; }
        if (!s && !RD.threads[i].thread) s=&RD.threads[i];
    }
    if (!s) { RD.unknown_threads++; return NULL; }
    if (s->thread != thread) { memset(s,0,sizeof *s); s->thread=thread; fresh=true; }
    if (fresh || !s->id.task_valid || at-s->validated_at >= UINT64_C(1000000)) {
        diagnostic_read_thread_identity(cpu,thread,&s->id); s->validated_at=at;
    }
    if(s->id.task_valid && s->named_proc!=s->id.task_proc && RD.file && RD.events<RD_EVENTS && RD.bytes+2048<RD_BYTES) {
        char name[32]={0};uint32_t off=0;
        bool ok=proc_name_scan(cpu,s->id.task_proc,name,sizeof name,&off);
        long begin=ftell(RD.file);
        fprintf(RD.file,"{\"kind\":\"process_name_heuristic\",\"at\":%" PRIu64 ",\"thread\":%u,\"task\":%u,\"proc\":%u,\"pid\":%u,\"offset\":%u,\"available\":%s,\"name\":\"",at,thread,s->id.task,s->id.task_proc,s->id.task_pid,off,ok?"true":"false");
        for(unsigned j=0;name[j];j++) { if(name[j]=='"'||name[j]=='\\')fputc('\\',RD.file);fputc(name[j],RD.file); }
        fputs("\"}\n",RD.file);fflush(RD.file);
        long end=ftell(RD.file);if(begin>=0&&end>=begin)RD.bytes+=(uint64_t)(end-begin);else RD.failed=true;
        RD.events++;s->named_proc=s->id.task_proc;
    }
    RD.cached_thread=thread;RD.cached=s;
    return s;
}
static void rd_emit(const char *kind, arm_cpu_t *cpu, uint64_t at,
        rd_thread_t *s, const uint32_t *extra, unsigned count) {
    if (!RD.file || RD.failed) return;
    if (RD.events >= RD_EVENTS-(RD.finishing?0:64) || RD.bytes + (RD.finishing?2048:65536) > RD_BYTES) { RD.dropped++; return; }
    long begin=ftell(RD.file);
    fprintf(RD.file,"{\"kind\":\"%s\",\"at\":%" PRIu64 ",\"thread\":%u,\"identity_valid\":%s,\"identity_at\":%" PRIu64
        ",\"pid\":",kind,at,s?s->thread:cpu->cp15.tpidrprw,s&&s->id.task_valid?"true":"false",s?s->validated_at:0);
    if (s&&s->id.task_valid) fprintf(RD.file,"%u",s->id.task_pid); else fputs("null",RD.file);
    fprintf(RD.file,",\"task\":%u,\"proc\":%u,\"effective_pid\":",s?s->id.task:0,s?s->id.task_proc:0);
    if(s&&s->id.effective_valid) fprintf(RD.file,"%u",s->id.effective_pid); else fputs("null",RD.file);
    fprintf(RD.file,",\"ttbr0\":%u,\"ttbr1\":%u,\"context\":%u,\"cpsr\":%u,\"spsr_svc\":%u,\"spsr_abt\":%u,\"spsr_und\":%u,\"user_sp\":%u,\"user_lr\":%u,\"dfsr\":%u,\"dfar\":%u,\"ifsr\":%u,\"ifar\":%u,\"r\":[",
        cpu->cp15.ttbr0,cpu->cp15.ttbr1,cpu->cp15.context_id,cpu->cpsr,
        cpu->spsr[ARM_BANK_SVC],cpu->spsr[ARM_BANK_ABT],cpu->spsr[ARM_BANK_UND],
        diagnostic_user_reg(cpu,13),diagnostic_user_reg(cpu,14),cpu->cp15.dfsr,cpu->cp15.dfar,cpu->cp15.ifsr,cpu->cp15.ifar);
    for(unsigned i=0;i<16;i++) fprintf(RD.file,"%s%u",i?",":"",cpu->r[i]);
    fputs("],\"extra\":[",RD.file);
    for(unsigned i=0;i<count;i++) fprintf(RD.file,"%s%u",i?",":"",extra[i]);
    fputs("]}\n",RD.file);fflush(RD.file);
    long end=ftell(RD.file);
    if(begin<0||end<begin||ferror(RD.file)) RD.failed=true;
    else RD.bytes+=(uint64_t)(end-begin);
    RD.events++;
}
static bool rd_target(const rd_thread_t *s) {
    if(!s||!s->id.task_valid)return false;
    if(RD.pid&&s->id.task_pid==RD.pid)return true;
    for(unsigned j=0;j<RD.exec_count;j++)if(RD.selected_tasks[j]&&RD.selected_tasks[j]==s->id.task)return true;
    return false;
}
static void rd_exec_select(arm_cpu_t *cpu,uint64_t at,rd_thread_t *s,const char *path) {
    if(!s||!s->id.task_valid||!path)return;
    for(unsigned j=0;j<RD.exec_count;j++) {
        if(!strcmp(path,RD.exec_paths[j])) {
            RD.selected_tasks[j]=s->id.task;uint32_t e[]={j,s->id.task,s->id.task_pid};rd_emit("executable_selected",cpu,at,s,e,3);
        }else if(RD.selected_tasks[j]==s->id.task)RD.selected_tasks[j]=0;
    }
}
static bool rd_field(arm_cpu_t *cpu,uint32_t p,uint32_t off,uint32_t *out) {
    uint32_t va=0,fsr=0;
    return p>=g_virt_base&&p<=UINT32_MAX-off-4&&springboard_child_read_field(cpu,p,off,out,&va,&fsr);
}
static void rd_stack_words(arm_cpu_t *cpu,uint64_t at,rd_thread_t *s) {
    uint32_t e[35]={diagnostic_user_reg(cpu,13),0,0};uint8_t b[128]={0};
    bool ok=guest_read_user_bytes(cpu,e[0],b,sizeof b,&e[1],&e[2]);
    for(unsigned i=0;i<32;i++)e[3+i]=ld32(b+4*i);
    rd_emit(ok?"user_stack_words":"user_stack_unreadable",cpu,at,s,e,35);
    if(s)for(unsigned i=0;i<16&&i<s->recent_n;i++) {unsigned j=(s->recent_n-1-i)%16;
        uint32_t r[]={s->recent_pc[j],(uint32_t)s->recent_at[j],(uint32_t)(s->recent_at[j]>>32)};
        rd_emit("recent_user_pc",cpu,at,s,r,3);}
}
/* The first entry pointer comes from actual ipc_entry_lookup. Later observations
 * recompute the table location under a separate exact instruction-shape gate.
 * Ownership fields are decoded only under the existing exact 7E18 layout gate. */
/* Exact current 7E18 table path, re-derived at every observation. A retained
 * entry address is never assumed to survive growth/migration. Splay collisions
 * and the fast-space variant are explicitly unsupported, never approximated. */
static unsigned rd_entry_resolve(arm_cpu_t *cpu,uint32_t space,uint32_t name,
        uint32_t *entry,uint32_t *object,uint32_t *bits) {
    *entry=*object=*bits=0;
    if(!RD.entry_gate)return 0;
    uint32_t active=0,table=0,count=0,fast=0;
    if(!rd_field(cpu,space,0x1c,&active)||!rd_field(cpu,space,0x24,&table)||
       !rd_field(cpu,space,0x28,&count)||!rd_field(cpu,space,0x54,&fast))return 5;
    if(!active)return 4;
    if(fast||count>1048576)return 3;
    uint32_t index=name>>8;
    if(index>=count)return 3; /* Might be a splay-tree name: do not guess. */
    uint64_t addr=(uint64_t)table+(uint64_t)index*16;
    if(addr>UINT32_MAX-8||addr<g_virt_base)return 5;
    *entry=(uint32_t)addr;
    if(!rd_field(cpu,*entry,4,bits)||!rd_field(cpu,*entry,0,object))return 5;
    if((*bits>>24)!=(name&255u))return (*bits&0x00800000u)?3:2;
    if(!(*bits&0x001f0000u))return 2;
    return 1;
}
static void rd_port(arm_cpu_t *cpu,uint64_t at,rd_thread_t *s,unsigned j,bool force) {
    if(j>=RD.port_count||!G.commcenter_layout.enabled)return;
    uint32_t e[14]={RD.ports[j].space,RD.ports[j].name,0,0};unsigned mask=0;
    e[13]=rd_entry_resolve(cpu,e[0],e[1],&e[2],&e[3],&e[4]);
    if(e[13]==1)mask|=3;else {e[3]=0;}
    if(e[3]&&rd_field(cpu,e[3],4,&e[5]))mask|=4;
    if(e[3]&&rd_field(cpu,e[3],8,&e[6]))mask|=8;
    if(e[3]&&rd_field(cpu,e[3],0x3c,&e[7]))mask|=16;
    if((e[5]&0x80000000)&&rd_field(cpu,e[7],0x58,&e[8]))mask|=32;
    uint32_t proc=0;
    if(e[8]&&rd_field(cpu,e[8],G.springboard_child_config.task_proc_offset,&proc)&&rd_field(cpu,proc,G.springboard_child_config.proc_pid_offset,&e[9]))mask|=64;
    e[10]=mask;e[11]=RD.ports[j].object;e[12]=RD.ports[j].bits;
    if((force||memcmp(RD.ports[j].last,e+3,8*sizeof(uint32_t))||RD.ports[j].entry!=e[2])&&RD.ports[j].changes++<32) {
        rd_emit("ipc_entry_state_raw",cpu,at,s,e,14);memcpy(RD.ports[j].last,e+3,8*sizeof(uint32_t));RD.ports[j].entry=e[2];}
}
static bool rd_port_object(uint32_t p) {
    for(unsigned j=0;j<RD.port_count;j++)if(RD.ports[j].object==p)return true;
    return false;
}
static bool rd_init(void) {
    if(!RD.enabled) return true;
    RD.entry_gate=diagnostic_kernel_symbol_window_matches("_ipc_entry_lookup",UINT32_C(0xc0011b14),0x30,
        (const uint8_t[]){0x83,0x6a,0x9a,0x42,0x0f,0xd2,0x40,0x6a},8)&&
        diagnostic_kernel_symbol_window_matches("_ipc_entry_lookup",UINT32_C(0xc0011b14),4,(const uint8_t[]){0x43,0x6d,0x04,0x1c},4);
    RD.file=fopen(RD.path,"wx");
    if(!RD.file) { perror("new runtime diagnostic log"); return false; }
    for(unsigned i=0;i<RD_SYMBOLS;i++) {
        RD.symbols[i]=ksym_value(rd_names[i])&~1u;
        fprintf(RD.file,"{\"kind\":\"symbol\",\"name\":\"%s\",\"address\":%u,\"resolved\":%s}\n",rd_names[i],RD.symbols[i],RD.symbols[i]?"true":"false");
    }
    fprintf(RD.file,"{\"kind\":\"layout\",\"identity_gate\":%s,\"wait_gate\":%s}\n",G.springboard_child_config.enabled?"true":"false",G.commcenter_wait_config.enabled?"true":"false");
    if(RD.focus.trigger)for(unsigned j=0;j<RF_COUNT;j++) {
        RD.focus.symbols[j]=ksym_value(rf_names[j])&~1u;
        fprintf(RD.file,"{\"kind\":\"gate_symbol\",\"name\":\"%s\",\"address\":%u}\n",rf_names[j],RD.focus.symbols[j]);
    }
    fflush(RD.file); RD.bytes=(uint64_t)ftell(RD.file);return !ferror(RD.file);
}
static void rd_arm(rd_pending_t *p, arm_cpu_t *cpu, uint64_t at, bool mach) {
    if(p->open)RD.overwritten_pending++;
    p->open=true;p->thread=cpu->cp15.tpidrprw;p->ttbr=cpu->cp15.ttbr0;
    p->context=cpu->cp15.context_id;p->at=at;
    p->pc=cpu->r[14]&~1u;
    p->sp=mach?diagnostic_user_reg(cpu,13):cpu->r[13];
    p->thumb=mach?(cpu->spsr[ARM_BANK_SVC]&ARM_CPSR_T):((cpu->r[14]&1u)?ARM_CPSR_T:0);
    memcpy(p->args,cpu->r,sizeof p->args);
}
static void rd_complete(rd_pending_t *p,const char *kind,arm_cpu_t *cpu,uint64_t at,rd_thread_t *s) {
    if(!rd_return_matches(p,cpu->cp15.tpidrprw,cpu->cp15.ttbr0,cpu->cp15.context_id,
        cpu->r[13],cpu->r[15],cpu->cpsr&ARM_CPSR_T)) return;
    uint32_t extra[9];memcpy(extra,p->args,sizeof p->args);
    extra[7]=(uint32_t)(at-p->at);extra[8]=p->pc;
    rd_emit(kind,cpu,at,s,extra,9);p->open=false;
    if(!strcmp(kind,"copyin_return")) s->copyreturns++;
}
/* Raw frame-chain candidates; ABI interpretation is deliberately separate.
 * Samples are small, read-only, user-permission checked and finite. */
static void rd_stack(arm_cpu_t *cpu,uint64_t at,rd_thread_t *s) {
    uint32_t sp=diagnostic_user_reg(cpu,13),fp=cpu->r[7];
    uint32_t e[29]={sp,fp,0,0,0};
    uint8_t bytes[8];uint32_t va=0,fsr=0;
    for(unsigned i=0;i<8;i++) {
        if(fp<sp||fp-sp>8u*1024u*1024u||(fp&3u)||fp>UINT32_MAX-8u)break;
        if(!guest_read_user_bytes(cpu,fp,bytes,8,&va,&fsr)){e[3]=va;e[4]=fsr;break;}
        uint32_t next=ld32(bytes),lr=ld32(bytes+4);
        e[5+i*3]=fp;e[6+i*3]=next;e[7+i*3]=lr;e[2]++;
        if(next<=fp)break;fp=next;
    }
    rd_emit("user_frame_candidates",cpu,at,s,e,29);
}
/* Raw argument bytes support offline RPC/string interpretation. A pointer is
 * never assumed to designate text, and translation failures remain explicit. */
static void rd_arg_bytes(arm_cpu_t *cpu,uint64_t at,rd_thread_t *s,unsigned arg) {
    if(arg>=3)return;
    uint32_t e[29]={arg,cpu->r[arg],0,0,0};uint8_t b[96]={0};
    e[4]=guest_read_user_bytes(cpu,e[1],b,sizeof b,&e[2],&e[3]);
    for(unsigned j=0;j<24;j++)e[5+j]=ld32(b+4*j);
    rd_emit("user_argument_bytes",cpu,at,s,e,29);
}
static void rd_user_site(arm_cpu_t *cpu,uint64_t at,unsigned slot) {
    if(slot>=8||(cpu->cpsr&ARM_CPSR_MODE_MASK)!=ARM_MODE_USR)return;
    rd_thread_t *s=rd_thread(cpu,at,true);if(!rd_target(s))return;
    rd_emit("user_site",cpu,at,s,&cpu->r[15],1);rd_stack(cpu,at,s);
    for(unsigned j=0;j<3;j++)rd_arg_bytes(cpu,at,s,j);
    rd_arm(&s->user[slot],cpu,at,false);s->user_open|=1u<<slot;
}
/* Actual ARM BL immediate, checked against native bytes before arming a
 * post-BL return. This is separate from the entry/LR method observer. */
static bool rd_arm_bl_target(uint32_t pc,uint32_t op,uint32_t *target) {
    if((pc&3u)||pc>UINT32_MAX-8u||(op&0xff000000u)!=0xeb000000u)return false;
    int64_t delta=(int64_t)(op&0xffffffu)*4;
    if(op&0x800000u)delta-=INT64_C(0x4000000);
    int64_t to=(int64_t)pc+8+delta;
    if(to<0||to>UINT32_MAX)return false;
    *target=(uint32_t)to;return true;
}
static void rd_bl_pre(arm_cpu_t *cpu,uint64_t at,rd_thread_t *s) {
    if(!s)return;
    if(s->bl.open){
        if(rd_return_matches(&s->bl,cpu->cp15.tpidrprw,cpu->cp15.ttbr0,cpu->cp15.context_id,
                cpu->r[13],cpu->r[15],cpu->cpsr&ARM_CPSR_T)) {
            rd_complete(&s->bl,"user_bl_return",cpu,at,s);return;
        }
        if(s->bl_steps<96 && s->bl.thread==cpu->cp15.tpidrprw &&
           s->bl.ttbr==cpu->cp15.ttbr0&&s->bl.context==cpu->cp15.context_id){
            uint8_t b[4]={0};uint32_t va=0,fsr=0;
            bool ok=guest_read_user_bytes(cpu,cpu->r[15],b,4,&va,&fsr);
            uint32_t e[]={s->bl_steps++,cpu->r[15],ld32(b),ok,va,fsr};
            rd_emit("user_bl_path",cpu,at,s,e,6);
        }
    }
    if(at<RD.start||at>=RD.end||!rd_target(s)||s->bl_hits>=8||(cpu->cpsr&ARM_CPSR_T))return;
    for(unsigned j=0;j<RD.bl_count;j++)if(cpu->r[15]==RD.bl_sites[j]){
        uint8_t b[4];uint32_t va=0,fsr=0,to=0;
        if(!guest_read_user_bytes(cpu,cpu->r[15],b,4,&va,&fsr)||
           !rd_arm_bl_target(cpu->r[15],ld32(b),&to))return;
        rd_arm(&s->bl,cpu,at,false);s->bl.pc=cpu->r[15]+4;s->bl.thumb=0;
        s->bl_hits++;s->bl_steps=0;uint32_t e[]={cpu->r[15],ld32(b),to,s->bl.pc,s->bl_hits};
        rd_emit("user_bl_call",cpu,at,s,e,5);for(unsigned k=0;k<3;k++)rd_arg_bytes(cpu,at,s,k);
    }
}
/* Read-only hardware/ledger copies and actual bus transactions. Caps are
 * host observation limits, never GPU idle/completion or guest scheduling. */
static void rd_graphics_emit(const char *kind,uint64_t at,const uint32_t *e,unsigned n) {
    if(!RD.graphics||!g_mach||at<RD.start||at>=RD.end||RD.graphics_records>=1024)return;
    RD.graphics_records++;rd_emit(kind,&g_mach->cpu,at,rd_lookup(g_mach->cpu.cp15.tpidrprw),e,n);
}
static void rd_graphics_sample(uint64_t at,bool force) {
    if(!RD.graphics||!g_mach||at<RD.start||at>=RD.end||(!force&&(at&0xfffffu)))return;
    s5l_mbx_t *m=&g_mach->mbx;s5l_mbx_telemetry_t *t=&g_mach->mbx_telemetry;
    uint64_t key[]={t->candidates_2d,t->completed_2d,t->rejected_2d,t->bytes_2d,
        t->candidates_3d,t->completed_3d,t->rejected_3d,t->pixels_3d,
        m->status,m->reg[S5L_MBX_FBSTART/4],m->reg[S5L_MBX_OBJBASE/4],m->ta_fifo_complete};
    if(!force&&RD.graphics_have&&!memcmp(key,RD.graphics_last,sizeof key))return;
    memcpy(RD.graphics_last,key,sizeof key);RD.graphics_have=true;
    uint32_t e[80];unsigned n=0;for(unsigned j=0;j<12;j++){e[n++]=(uint32_t)key[j];e[n++]=(uint32_t)(key[j]>>32);}
    const uint32_t offsets[]={0x12c,0x130,0x134,0x608,0x60c,0x61c,0x650,0x654,0x658,0x65c,0x660,0x680,0x800,0x810,0x83c,0x840,0x844};
    for(unsigned j=0;j<sizeof offsets/sizeof offsets[0];j++){e[n++]=offsets[j];e[n++]=m->reg[offsets[j]/4];}
    e[n++]=m->ta_fifo_words;e[n++]=m->ta_fifo_complete;
    rd_graphics_emit("graphics_ledger",at,e,n);
    for(unsigned j=0;j<S5L_MBX_3D_ACCEPT_HISTORY;j++){
        const s5l_mbx_3d_accept_witness_t *w=&t->accepted_3d_history[j];if(!w->sequence||RD.accepted_seen[j]==w->sequence)continue;RD.accepted_seen[j]=w->sequence;
        uint32_t a[]={j,(uint32_t)w->sequence,(uint32_t)(w->sequence>>32),w->kind,w->pixels,w->region,w->object,w->target,w->target_physical,w->target_mapping_span,w->xclip,w->yclip,w->pixel_sample,w->framebuffer_control,w->framebuffer_stride,w->record_base,w->record_valid_words};
        rd_graphics_emit("graphics_accepted",at,a,sizeof a/sizeof a[0]);
    }
    for(unsigned j=0;j<S5L_MBX_3D_REJECTION_HISTORY;j++){
        const s5l_mbx_3d_rejection_witness_t *w=&t->rejected_3d_history[j];if(!w->sequence||RD.rejected_seen[j]==w->sequence)continue;RD.rejected_seen[j]=w->sequence;
        /* Explicit raw copy: host witness struct has no pointers. */
        uint32_t a[sizeof *w/4+1];a[0]=j;memcpy(a+1,w,sizeof *w);
        rd_graphics_emit("graphics_rejected_raw",at,a,sizeof a/sizeof a[0]);
    }
}
static void rd_graphics_access(uint32_t addr,uint32_t val,unsigned bytes,bool wr) {
    rta_access(addr,val,bytes,wr);
    if(!RD.graphics||!g_mach)return;uint64_t at=G.hot_now;
    if(at<RD.start||at>=RD.end)return;
    uint32_t off=addr-S5L8900_MBX_BASE;
    if(wr&&bytes==4&&off==S5L_MBX_3D_DATA_FIFO){
        RD.fifo_seen++;
        if(RD.fifo_scenes<4&&RD.fifo_kept<1024){RD.fifo_chunk[RD.fifo_chunk_n++]=val;RD.fifo_kept++;
            if(RD.fifo_chunk_n==32||val==S5L_MBX_3D_SUBMIT){uint32_t e[36]={RD.fifo_scenes,RD.fifo_seen,RD.fifo_kept,RD.fifo_chunk_n};
                memcpy(e+4,RD.fifo_chunk,RD.fifo_chunk_n*4);rd_graphics_emit("graphics_ta_words",at,e,4+RD.fifo_chunk_n);RD.fifo_chunk_n=0;}}
        if(val==S5L_MBX_3D_SUBMIT){uint32_t e[]={RD.fifo_scenes,RD.fifo_seen,RD.fifo_kept,RD.fifo_seen>RD.fifo_kept};rd_graphics_emit("graphics_ta_end",at,e,4);RD.fifo_scenes++;RD.fifo_seen=RD.fifo_kept=RD.fifo_chunk_n=0;}
        return;
    }
    unsigned category=8;
    if(off<S5L_MBX_SIZE && (off==0x12c||off==0x130||off==0x134))category=wr?0:1;
    else if(wr&&off<S5L_MBX_SIZE)category=2;
    else if(wr&&off>=S5L_MBX_2D_RING_BASE&&off<S5L_MBX_2D_RING_BASE+S5L_MBX_2D_RING_SIZE)category=3;
    else if(wr&&addr>=S5L8900_CLCD_BASE&&addr<S5L8900_CLCD_BASE+4096)category=4;
    if(category<8&&RD.access_hits[category]++<64){uint32_t e[]={addr,val,bytes,wr,category};rd_graphics_emit("graphics_bus",at,e,5);}
}

/* Early service traffic must not consume the later diagnostic-window reserve.
 * Per-role/per-phase counts remain subordinate to the global byte/event cap. */
static bool rd_site_capture(unsigned site,unsigned role,bool detail) {
    if(site>=32||role>=3)return false;
    unsigned *hits=&RD.site_hits[site][role][detail?1:0];
    if(*hits>=48)return false;
    ++*hits;return true;
}
/* Opt-in call-correlated observation, not an IOKit implementation. The trigger
 * must be a readable Thumb BLX-register calling the resolved runAction entry.
 * Objects/owner/action/thread are read from the actual call, never configured
 * addresses. Capture quotas belong to the triggered call, not startup traffic. */
static bool rd_focus_code(arm_cpu_t *cpu,uint32_t va,uint8_t *bytes,size_t count,uint32_t *failure_va,uint32_t *failure_fsr) {
    *failure_va=*failure_fsr=0;
    if(!g_mach||!g_mach->ram||va<g_virt_base||!count||count>16||
       (uint64_t)va+count>UINT32_MAX)return false;
    for(size_t j=0;j<count;j++) {
        uint32_t pa=0;uint32_t fsr=arm_mmu_translate(cpu,va+(uint32_t)j,ARM_ACCESS_FETCH,true,&pa);
        if(fsr||pa<g_mach->ram_base||(uint64_t)pa-g_mach->ram_base>=g_mach->ram_size) {
            *failure_va=va+(uint32_t)j;*failure_fsr=fsr;return false;
        }
        bytes[j]=g_mach->ram[pa-g_mach->ram_base];
    }
    return true;
}
static bool rd_focus_event(uint32_t event) {
    if(!event)return false;
    for(unsigned j=0;j<RD.focus.event_count;j++)if(RD.focus.events[j]==event)return true;
    return false;
}
static void rd_focus_add_event(uint32_t event) {
    if(event&&!rd_focus_event(event)&&RD.focus.event_count<16)
        RD.focus.events[RD.focus.event_count++]=event;
}
static bool rd_focus_related(const arm_cpu_t *cpu) {
    if(!RD.focus.active)return false;
    if(cpu->cp15.tpidrprw==RD.focus.thread&&cpu->cp15.ttbr0==RD.focus.ttbr&&
       cpu->cp15.context_id==RD.focus.context)return true;
    for(unsigned j=0;j<3;j++) {
        uint32_t v=cpu->r[j];
        if(v&&(v==RD.focus.gate||v==RD.focus.command||v==RD.focus.owner||
               v==RD.focus.loop||rd_focus_event(v)))return true;
        for(unsigned q=0;q<RD.focus.field_count;q++)
            if(v&&v==RD.focus.field_objects[q])return true;
    }
    return false;
}
static void rd_focus_emit(const char *kind,arm_cpu_t *cpu,uint64_t at,
        rd_thread_t *s,const uint32_t *e,unsigned n) {
    if(RD.focus.records>=1024){RD.focus.suppressed++;return;}
    RD.focus.records++;rd_emit(kind,cpu,at,s,e,n);
}
static void rd_focus_object(arm_cpu_t *cpu,uint64_t at,rd_thread_t *s,uint32_t object,unsigned role) {
    /* 256 raw bytes, with per-word validity. Field semantics remain offline. */
    uint32_t e[69]={RD.focus.sessions,role,object,0,0};
    for(unsigned j=0;j<64;j++)if(rd_field(cpu,object,j*4,&e[5+j]))e[3+j/32]|=1u<<(j%32);
    rd_focus_emit("gate_object_raw",cpu,at,s,e,69);
}
/* One-level configured pointer fields. No guessed layouts or recursive walk;
 * RAM-only reader reports validity, and fresh identities are retained per call. */
static void rd_focus_fields(arm_cpu_t *cpu,uint64_t at,rd_thread_t *s,bool discover) {
    rd_focus_object(cpu,at,s,RD.focus.owner,2);
    for(unsigned q=0;q<RD.focus.field_count;q++) {
        if(discover){RD.focus.field_objects[q]=0;
            rd_field(cpu,RD.focus.owner,RD.focus.field_offsets[q],&RD.focus.field_objects[q]);}
        uint32_t e[]={RD.focus.sessions,q,RD.focus.owner,RD.focus.field_offsets[q],RD.focus.field_objects[q]};
        rd_focus_emit("gate_pointer_field_raw",cpu,at,s,e,5);
        rd_focus_object(cpu,at,s,RD.focus.field_objects[q],5+q);
    }
}
/* Direct host model fields, never MMIO reads: no latch/counter side effects.
 * No hardware semantics are inferred here. Values are decoded offline. */
static void rd_focus_board(arm_cpu_t *cpu,uint64_t at,rd_thread_t *s) {
    if(!g_mach||!RD.focus.field_count)return;
    uint32_t e[97]={RD.focus.sessions,(uint32_t)g_mach->timer.ticks,(uint32_t)(g_mach->timer.ticks>>32)};
    unsigned n=3;
    for(unsigned q=0;q<S5L8900_I2S_COUNT;q++)for(unsigned j=0;j<S5L_I2S_REGS;j++)e[n++]=g_mach->i2s[q].regs[j];
    for(unsigned q=0;q<S5L_GPIOIC_GROUPS;q++) {
        e[n++]=g_mach->gpioic.en[q];e[n++]=g_mach->gpioic.stat[q];e[n++]=g_mach->gpioic.type[q];
        e[n++]=g_mach->gpioic.level[q];e[n++]=g_mach->gpioic.raw[q];e[n++]=g_mach->gpioic.driven[q];
    }
    for(unsigned q=0;q<S5L8900_DMAC_COUNT;q++) {
        e[n++]=g_mach->dmac[q].config;e[n++]=g_mach->dmac[q].raw_tc;e[n++]=g_mach->dmac[q].raw_err;
        e[n++]=(uint32_t)g_mach->dmac[q].transfers;e[n++]=(uint32_t)g_mach->dmac[q].bytes_moved;
        for(unsigned j=0;j<S5L_PL080_CHANNELS;j++)e[n++]=g_mach->dmac[q].ch[j].cfg;
    }
    if(g_mach->i2s[0].experimental_hz) {
        e[n++]=g_mach->i2s[0].experimental_hz;e[n++]=(uint32_t)g_mach->i2s[0].frame_phase;
        e[n++]=(uint32_t)g_mach->i2s[0].frame_edges;e[n++]=(uint32_t)(g_mach->i2s[0].frame_edges>>32);
        e[n++]=(uint32_t)g_mach->i2s[0].clock_generation;
        e[n++]=g_mach->i2s[0].tx_level;
        e[n++]=(uint32_t)g_mach->i2s[0].tx_bytes_in;e[n++]=(uint32_t)(g_mach->i2s[0].tx_bytes_in>>32);
        e[n++]=(uint32_t)g_mach->i2s[0].tx_bytes_out;e[n++]=(uint32_t)(g_mach->i2s[0].tx_bytes_out>>32);
        e[n++]=(uint32_t)g_mach->i2s[0].tx_underflow_frames;e[n++]=(uint32_t)g_mach->i2s[0].tx_refused;
    }
    rd_focus_emit("gate_board_raw",cpu,at,s,e,n);
}
static bool rd_focus_trigger(arm_cpu_t *cpu,uint64_t at,rd_thread_t *s,bool detail) {
    if(!RD.focus.trigger||cpu->r[15]!=RD.focus.trigger||!rd_target(s))return false;
    if(!detail){RD.focus.early_triggers++;return false;}
    if(RD.focus.active||RD.focus.sessions>=4)return false;
    uint8_t bytes[2]={0};uint32_t va=0,fsr=0;
    if(!(cpu->cpsr&ARM_CPSR_T)||!rd_focus_code(cpu,cpu->r[15],bytes,2,&va,&fsr))return false;
    unsigned op=bytes[0]|((unsigned)bytes[1]<<8),reg=(op>>3)&15;
    if((op&0xff87u)!=0x4780u||!RD.focus.symbols[RF_RUN]||
       cpu->r[reg]!=(RD.focus.symbols[RF_RUN]|1u)||!(cpu->r[1]&1u)||
       (cpu->r[1]&~1u)<g_virt_base)return false;
    RD.focus.gate=cpu->r[0];RD.focus.action=cpu->r[1]&~1u;RD.focus.command=cpu->r[2];
    RD.focus.owner=RD.focus.loop=0;rd_field(cpu,RD.focus.gate,12,&RD.focus.owner);
    rd_field(cpu,RD.focus.gate,24,&RD.focus.loop);
    RD.focus.thread=cpu->cp15.tpidrprw;RD.focus.task=s->id.task;
    RD.focus.ttbr=cpu->cp15.ttbr0;RD.focus.context=cpu->cp15.context_id;
    RD.focus.event_count=0;RD.focus.active=true;RD.focus.action_executed=false;
    RD.focus.action_pending_pc=0;memset(RD.focus.site_hits,0,sizeof RD.focus.site_hits);
    RD.focus.sessions++;rd_arm(&RD.focus.returned,cpu,at,false);
    RD.focus.returned.pc=cpu->r[15]+2;RD.focus.returned.thumb=ARM_CPSR_T;
    uint32_t e[]={RD.focus.sessions,RD.focus.gate,RD.focus.action,RD.focus.command,
        RD.focus.owner,RD.focus.loop,op,reg,RD.focus.symbols[RF_RUN],RD.focus.returned.pc};
    rd_focus_emit("gate_call_trigger",cpu,at,s,e,10);
    rd_focus_object(cpu,at,s,RD.focus.gate,0);rd_focus_object(cpu,at,s,RD.focus.command,1);
    rd_focus_object(cpu,at,s,RD.focus.owner,2);rd_focus_object(cpu,at,s,RD.focus.loop,3);
    if(RD.focus.field_count){rd_focus_fields(cpu,at,s,true);rd_focus_board(cpu,at,s);}
    return true;
}
static void rd_focus_pre(arm_cpu_t *cpu,uint64_t at,bool detail) {
    if(!RD.focus.trigger)return;
    uint32_t pc=cpu->r[15]&~1u;
    if(pc==RD.focus.trigger)rd_focus_trigger(cpu,at,rd_thread(cpu,at,true),detail);
    if(!RD.focus.active)return;
    rd_thread_t *s=NULL;
    if(rd_return_matches(&RD.focus.returned,cpu->cp15.tpidrprw,cpu->cp15.ttbr0,
            cpu->cp15.context_id,cpu->r[13],pc,cpu->cpsr&ARM_CPSR_T)) {
        s=rd_thread(cpu,at,true);uint32_t e[]={RD.focus.sessions,(uint32_t)(at-RD.focus.returned.at),RD.focus.action_executed};
        rd_focus_emit("gate_call_return",cpu,at,s,e,3);rd_focus_object(cpu,at,s,RD.focus.gate,0);
        RD.focus.returned.open=false;RD.focus.active=false;return;
    }
    if(pc==RD.focus.action&&cpu->cp15.tpidrprw==RD.focus.thread&&
       cpu->cp15.ttbr0==RD.focus.ttbr&&cpu->cp15.context_id==RD.focus.context&&
       (cpu->cpsr&ARM_CPSR_T)&&!RD.focus.action_executed) {
        uint8_t bytes[16]={0};uint32_t e[9]={RD.focus.sessions,pc,0,0,0};
        e[2]=rd_focus_code(cpu,pc,bytes,16,&e[3],&e[4]);
        for(unsigned j=0;j<4;j++)e[5+j]=ld32(bytes+j*4);
        s=rd_thread(cpu,at,true);rd_focus_emit("gate_action_entry_candidate",cpu,at,s,e,9);
        if(e[2])RD.focus.action_pending_pc=pc;
    }
    for(unsigned j=0;j<RF_COUNT;j++)if(pc==RD.focus.symbols[j]&&rd_focus_related(cpu)) {
        s=rd_thread(cpu,at,true);
        if(j==RF_SLEEP||j==RF_SLEEP_TIMED||j==RF_LOOP_SLEEP||j==RF_LOOP_SLEEP_TIMED||j==RF_RECURSIVE_SLEEP)rd_focus_add_event(cpu->r[1]);
        if(j==RF_MUTEX_SLEEP)rd_focus_add_event(cpu->r[2]);
        if(j==RF_ASSERT_WAIT)rd_focus_add_event(cpu->r[0]);
        uint32_t e[]={RD.focus.sessions,j,RD.focus.gate,RD.focus.command,RD.focus.action_executed};
        rd_focus_emit("gate_primitive_entry",cpu,at,s,e,5);
        rd_focus_object(cpu,at,s,cpu->r[0],4);
        if(RD.focus.field_count){rd_focus_fields(cpu,at,s,false);rd_focus_board(cpu,at,s);}
        if(j==RF_SLEEP||j==RF_SLEEP_TIMED||j==RF_CLOSE||j==RF_MUTEX_SLEEP)
            rd_focus_object(cpu,at,s,RD.focus.gate,0);
        if(j==RF_BLOCK&&G.commcenter_wait_config.enabled) {
            commcenter_wait_snapshot_t w;memset(&w,0,sizeof w);commcenter_wait_snapshot_capture(&w,cpu,RD.focus.thread);
            uint32_t e2[]={w.valid_bits,w.wait_queue,w.event_low,w.event_high,w.state,w.result,w.continuation,w.parameter,w.failure_va,w.failure_fsr};
            rd_focus_emit("gate_thread_wait_raw",cpu,at,s,e2,10);
        }
    }
}
static void rd_focus_post(arm_cpu_t *cpu,uint64_t at,uint32_t before_cpsr,uint32_t before_pc) {
    if(!RD.focus.active||!RD.focus.action_pending_pc||before_pc!=RD.focus.action_pending_pc||
       cpu->cp15.tpidrprw!=RD.focus.thread||cpu->cp15.ttbr0!=RD.focus.ttbr||
       cpu->cp15.context_id!=RD.focus.context)return;
    uint32_t mode=cpu->cpsr&ARM_CPSR_MODE_MASK;
    if((before_cpsr&ARM_CPSR_T)&&mode==ARM_MODE_SVC&&(cpu->cpsr&ARM_CPSR_T)&&cpu->r[15]==before_pc+2) {
        RD.focus.action_executed=true;uint32_t e[]={RD.focus.sessions,before_pc,before_cpsr};
        rd_focus_emit("gate_action_first_instruction_executed",cpu,at,rd_thread(cpu,at,true),e,3);
    }
    RD.focus.action_pending_pc=0;
}

static void rd_pre(arm_cpu_t *cpu,uint64_t at) {
    uint32_t pc=cpu->r[15]&~1u, mode=cpu->cpsr&ARM_CPSR_MODE_MASK;
    bool detail=at>=RD.start&&at<RD.end;
    rd_graphics_sample(at,false);
    /* Pending return checks use raw thread/address-space identity before any reads. */
    rd_thread_t *s=rd_lookup(cpu->cp15.tpidrprw);
    if(s && mode!=ARM_MODE_USR) {
        rd_complete(&s->copy,"copyin_return",cpu,at,s);
        rd_complete(&s->task,"port_to_task_return",cpu,at,s);
        rd_complete(&s->port_thread,"port_to_thread_return",cpu,at,s);
    }
    if((at&0xffffu)==0) {
        if((at&0xfffffu)==0)for(unsigned j=0;j<RD.port_count;j++)rd_port(cpu,at,s,j,false);
        s=rd_thread(cpu,at,false);
        if(rd_target(s)) {
            s->samples++;s->last_pc=pc;
            if(mode==ARM_MODE_USR){unsigned j=s->recent_n++%16;s->recent_pc[j]=pc;s->recent_at[j]=at;}
            if(s->id.task==RD.selected_tasks[0]&&s->samples%512==0)rd_stack_words(cpu,at,s);
            if(mode==ARM_MODE_USR)s->user_samples++;else s->kernel_samples++;
            if(s->samples<=2 || s->samples%512==0) {
                commcenter_wait_snapshot_t w;memset(&w,0,sizeof w);
                if(G.commcenter_wait_config.enabled)commcenter_wait_snapshot_capture(&w,cpu,s->thread);
                uint32_t e[]={w.valid_bits,w.wait_queue,w.event_low,w.event_high,w.state,w.result,w.continuation,w.parameter,w.failure_va,w.failure_fsr};
                rd_emit("thread_checkpoint",cpu,at,s,e,10);
            }
        }
    }
    if(mode==ARM_MODE_USR) {
        if(RD.bl_count){if(!s)for(unsigned j=0;j<RD.bl_count;j++)if(pc==RD.bl_sites[j]){s=rd_thread(cpu,at,true);break;}rd_bl_pre(cpu,at,s);}
        if(s&&s->user_open)for(unsigned j=0;j<8;j++)if(s->user_open&(1u<<j)) {
            rd_complete(&s->user[j],"user_site_return",cpu,at,s);
            if(!s->user[j].open)s->user_open&=~(1u<<j);
        }
        return;
    }
    rd_focus_pre(cpu,at,detail);
    /* Numeric kernel sites expose real driver calls/returns, before the detail window too.
     * No semantics are assigned to registers until offline byte verification. */
    for(unsigned j=0;j<RD.site_count;j++)if(pc==RD.sites[j]) {
        if(RD.focus.trigger&&(!RD.focus.active||!rd_focus_related(cpu)))continue;
        s=rd_thread(cpu,at,true);unsigned role=2;
        if(s&&s->id.task_valid)for(unsigned q=0;q<RD.exec_count;q++)if(RD.selected_tasks[q]==s->id.task)role=q;
        if(RD.focus.trigger) {
            if(RD.focus.site_hits[j]>=96){RD.focus.suppressed++;continue;}
            RD.focus.site_hits[j]++;
        }else if(!rd_site_capture(j,role,detail))continue;
        uint32_t e[40]={j,pc};
        uint32_t objects[]={cpu->r[0],cpu->r[4],cpu->r[12]};
        for(unsigned q=0;q<3;q++){unsigned o=2+q*12;e[o]=objects[q];
            for(unsigned f=0;f<10;f++){uint32_t value=0;
                if(rd_field(cpu,objects[q],f*4,&value))e[o+1]|=1u<<f;e[o+2+f]=value;}}
        rd_emit("kernel_site_raw",cpu,at,s,e,38);
        if(RD.focus.field_count&&RD.focus.active) {rd_focus_fields(cpu,at,s,false);rd_focus_board(cpu,at,s);}
        uint32_t f[40]={pc,cpu->r[4],0,cpu->r[0],0};
        const uint32_t off[]={0x74,0x78,0x80,0x84,0x90,0x94,0x98,0x9c};
        for(unsigned q=0;q<8;q++) {if(rd_field(cpu,cpu->r[4],off[q],&f[5+q]))f[2]|=1u<<q;}
        for(unsigned q=0;q<8;q++) {if(rd_field(cpu,cpu->r[0],off[q],&f[13+q]))f[4]|=1u<<q;}
        f[21]=cpu->r[12];if(rd_field(cpu,cpu->r[12],0x358,&f[22]))f[23]=1;
        rd_emit("kernel_object_fields_raw",cpu,at,s,f,24);

    }
    /* Actual lookup return in copyin: space/name are r4/r6, entry returned in r0.
     * Verified instruction shape prevents interpreting another kernel's ABI. */
    if(pc==UINT32_C(0xc001425e)&&G.commcenter_layout.enabled&&rd_target(rd_thread(cpu,at,false))) {
        s=rd_thread(cpu,at,false);bool seen=false;
        for(unsigned j=0;j<RD.port_count;j++)if(RD.ports[j].space==cpu->r[4]&&RD.ports[j].name==cpu->r[6]){seen=true;break;}
        if(!seen&&cpu->r[0]&&RD.port_count<64&&diagnostic_kernel_symbol_window_matches("_ipc_mqueue_copyin",UINT32_C(0xc0014224),0x3a,(const uint8_t[]){0x00,0x28,0xf1,0xd0,0x04,0x68,0x40,0x68},8)) {
            unsigned j=RD.port_count++;RD.ports[j].space=cpu->r[4];RD.ports[j].name=cpu->r[6];RD.ports[j].entry=cpu->r[0];
            rd_field(cpu,cpu->r[0],0,&RD.ports[j].object);rd_field(cpu,cpu->r[0],4,&RD.ports[j].bits);rd_port(cpu,at,s,j,true);
        }
    }
    for(unsigned k=0;k<RD_SYMBOLS;k++) {
        if(!RD.symbols[k]||pc!=RD.symbols[k])continue;
        if(k==RD_SWI && (int32_t)cpu->r[12]!=-31 && cpu->r[12]!=244 && cpu->r[12]!=59 && cpu->r[12]!=37)break;
        if(k!=RD_TRIAGE&&k!=RD_SIGNAL&&k!=RD_EXIT&&!detail&&k!=RD_SWI&&k<RD_PORT_DESTROY)break;
        s=rd_thread(cpu,at,k==RD_TRIAGE||k==RD_SIGNAL||k==RD_EXIT);RD.kinds[k]++;
        if(k>=RD_KILL) {
            bool interesting=k==RD_KILL?rd_target(s):(rd_port_object(cpu->r[0])||rd_port_object(cpu->r[1])||(k==RD_MQUEUE_CHANGED&&cpu->r[0]>=0x18&&rd_port_object(cpu->r[0]-0x18)));
            if(interesting&&RD.kinds[k]<8192){uint32_t e[]={k,cpu->r[0],cpu->r[1],cpu->r[2],cpu->r[3]};rd_emit("kernel_lifecycle",cpu,at,s,e,5);rd_stack_words(cpu,at,s);}
        } else if(k==RD_TRIAGE && RD.kinds[k]<=256) {
            uint32_t e[8]={cpu->r[0],cpu->r[1],cpu->r[2],0,0,0,0,0};
            for(unsigned i=0;i<4&&i<cpu->r[2];i++) {
                uint32_t fva=0,fsr=0,v=0;
                if(springboard_child_read_field(cpu,cpu->r[1],4*i,&v,&fva,&fsr)){e[3]|=1u<<i;e[4+i]=v;}
            }
            rd_emit("exception_triage",cpu,at,s,e,8);
        } else if((k==RD_SIGNAL||k==RD_EXIT)&&RD.kinds[k]<=128) {
            uint32_t pid=0,fva=0,fsr=0;bool ok=G.springboard_child_config.enabled&&springboard_child_read_field(cpu,cpu->r[0],G.springboard_child_config.proc_pid_offset,&pid,&fva,&fsr);
            uint32_t e[]={cpu->r[0],cpu->r[1],ok,pid,fva,fsr};rd_emit(k==RD_SIGNAL?"signal":"exit",cpu,at,s,e,6);
        } else if(k==RD_SWI) {
            if(cpu->r[12]==37&&rd_target(s)&&RD.kinds[k]<10000000){rd_emit("kill_syscall",cpu,at,s,NULL,0);rd_stack_words(cpu,at,s);
            }else if(cpu->r[12]==244||cpu->r[12]==59) {
                if(RD.exec_calls++<128) {
                    rd_emit("exec_or_spawn",cpu,at,s,NULL,0);
                    uint32_t pva=cpu->r[cpu->r[12]==244?1:0],fva=0,fsr=0,e[67]={pva,0,0};uint8_t path[256]={0};
                    unsigned length=0;
                    for(;length<sizeof path-1;length++) {
                        if(pva>UINT32_MAX-length||!guest_read_user_bytes(cpu,pva+length,path+length,1,&fva,&fsr))break;
                        if(!path[length])break;
                    }
                    e[1]=fva;e[2]=fsr;for(unsigned j=0;j<64;j++)e[3+j]=ld32(path+4*j);
                    rd_emit("exec_path_raw",cpu,at,s,e,67);
                    if(s&&s->id.task_valid&&!fva&&!fsr&&length<sizeof path-1) {
                        rd_exec_select(cpu,at,s,(char*)path);
                    }
                }
            }else if(rd_target(s)|| (detail&&s&&s->id.task_valid)) {
                s->mach_calls++;
                if(s->mach_calls<=16 || at-s->last_mach_at>=UINT64_C(10000000)) {
                    uint8_t header[24]={0};uint32_t va=0,fsr=0;
                    bool ok=guest_read_user_bytes(cpu,cpu->r[0],header,sizeof header,&va,&fsr);
                    uint32_t e[9]={ok,va,fsr};for(unsigned j=0;j<6;j++)e[3+j]=ld32(header+4*j);
                    rd_emit("mach_msg_entry",cpu,at,s,e,9);rd_arm(&s->mach,cpu,at,true);s->last_mach_at=at;
                }
            }
        } else if(k==RD_MQUEUE&&rd_target(s)) {
            s->copyins++;
            if(s->copyins<=16||s->copyins%4096==0){rd_emit("copyin_entry",cpu,at,s,NULL,0);rd_arm(&s->copy,cpu,at,false);}
        } else if((k==RD_PORT_TASK||k==RD_PORT_THREAD)&&RD.kinds[k]<=256) {
            rd_emit(k==RD_PORT_TASK?"port_to_task":"port_to_thread",cpu,at,s,NULL,0);
            if(s)rd_arm(k==RD_PORT_TASK?&s->task:&s->port_thread,cpu,at,false);
        } else if(k>=RD_PORT_DESTROY&&rd_target(s)&&s->port_ops++<128) {
            uint32_t e[]={k,cpu->r[0],cpu->r[1],cpu->r[2]};
            rd_emit("port_operation",cpu,at,s,e,4);rd_stack(cpu,at,s);
        } else if((k==RD_TASK_TERM||k==RD_EXCEPTION_DELIVER)&&RD.kinds[k]<=128) {
            rd_emit(k==RD_TASK_TERM?"task_terminate":"exception_deliver",cpu,at,s,NULL,0);
        } else if((k==RD_BLOCK||k==RD_RUN)&&rd_target(s)&&RD.kinds[k]<=512) {
            rd_emit(k==RD_BLOCK?"thread_block":"thread_run",cpu,at,s,NULL,0);
        }
        break;
    }
}
static void rd_post(arm_cpu_t *cpu,uint64_t at,uint32_t before_cpsr,uint32_t before_pc,uint32_t before_lr,uint32_t before_sp) {
    rd_focus_post(cpu,at,before_cpsr,before_pc);
    uint32_t mode=cpu->cpsr&ARM_CPSR_MODE_MASK;
    if((before_cpsr&ARM_CPSR_MODE_MASK)==ARM_MODE_USR&&(mode==ARM_MODE_ABT||mode==ARM_MODE_UND)) {
        RD.faults++;rd_thread_t *s=rd_thread(cpu,at,true);
        if(mode==ARM_MODE_UND)RD.undefs++;
        if(s){s->faults++;if(mode==ARM_MODE_UND)s->undefs++;}
        if((mode==ARM_MODE_UND&&(RD.undefs<=32||(rd_target(s)&&s->undefs<=32)))||(rd_target(s)&&(s->faults<=16||s->faults%1024==0))) {
            uint8_t b[16]={0};uint32_t va=0,fsr=0;
            bool ok=guest_read_user_bytes(cpu,before_pc,b,16,&va,&fsr);
            uint32_t e[]={before_pc,before_lr,before_sp,before_cpsr,ok,va,fsr,ld32(b),ld32(b+4),ld32(b+8),ld32(b+12)};
            rd_emit(mode==ARM_MODE_UND?"user_undefined":"user_abort",cpu,at,s,e,11);
        }
    }
    if(mode==ARM_MODE_USR&&(before_cpsr&ARM_CPSR_MODE_MASK)==ARM_MODE_SVC) {
        rd_thread_t *s=rd_lookup(cpu->cp15.tpidrprw);
        if(s&&s->mach.open && rd_return_matches(&s->mach,s->thread,cpu->cp15.ttbr0,cpu->cp15.context_id,cpu->r[13],cpu->r[15],cpu->cpsr&ARM_CPSR_T)) {
            rd_complete(&s->mach,"mach_msg_return",cpu,at,s);s->mach_returns++;s->last_ret=cpu->r[0];
            if(cpu->r[0]&&!s->stack_error&&rd_target(s)) {rd_stack(cpu,at,s);s->stack_error=cpu->r[0];}
        }
    }
}
static void rd_finish(arm_cpu_t *cpu,uint64_t at) {
    if(!RD.file)return;
    RD.finishing=true;
    if(RD.focus.trigger) {
        uint32_t e[]={RD.focus.sessions,RD.focus.active,RD.focus.action_executed,
            (uint32_t)RD.focus.records,(uint32_t)RD.focus.suppressed,(uint32_t)RD.focus.early_triggers,
            RD.focus.gate,RD.focus.command,RD.focus.action,RD.focus.event_count};
        rd_emit("gate_summary",cpu,at,rd_lookup(RD.focus.thread),e,10);
    }
    for(unsigned i=0;i<RD_THREADS;i++) {
        rd_thread_t *s=&RD.threads[i];if(!rd_target(s))continue;
        uint32_t e[]={(uint32_t)s->samples,(uint32_t)s->user_samples,(uint32_t)s->kernel_samples,(uint32_t)s->mach_calls,(uint32_t)s->mach_returns,(uint32_t)s->copyins,(uint32_t)s->copyreturns,s->last_ret,s->last_pc,(uint32_t)s->faults};
        rd_emit("thread_summary",cpu,at,s,e,10);
        commcenter_wait_snapshot_t w;memset(&w,0,sizeof w);
        if(G.commcenter_wait_config.enabled)commcenter_wait_snapshot_capture(&w,cpu,s->thread);
        uint32_t we[]={w.valid_bits,w.wait_queue,w.event_low,w.event_high,w.state,w.result,w.continuation,w.parameter,w.failure_va,w.failure_fsr,cpu->cp15.tpidrprw};
        rd_emit("thread_final_wait",cpu,at,s,we,11);
    }
    fprintf(RD.file,"{\"kind\":\"summary\",\"at\":%" PRIu64 ",\"events\":%" PRIu64 ",\"bytes\":%" PRIu64 ",\"dropped\":%" PRIu64 ",\"faults_all\":%" PRIu64 ",\"undefined_all\":%" PRIu64 ",\"failed\":%s,\"unknown_threads\":%" PRIu64 ",\"symbol_hits\":[",at,RD.events,RD.bytes,RD.dropped,RD.faults,RD.undefs,RD.failed?"true":"false",RD.unknown_threads);
    for(unsigned i=0;i<RD_SYMBOLS;i++)fprintf(RD.file,"%s%" PRIu64,i?",":"",RD.kinds[i]);
    fprintf(RD.file,"] ,\"overwritten_pending\":%" PRIu64 "}\n",RD.overwritten_pending);fclose(RD.file);RD.file=NULL;
}
#endif
