#define _DARWIN_C_SOURCE 1
/* Synthetic observer tests: no firmware, guest execution or game data. MIT. */
#define main bootkernel_program_main
#include "../../tools/bootkernel.c"
#undef main
#include <assert.h>
static void putword(uint8_t *p,uint32_t value) {
    for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(value>>(8*i));
}
int main(void) {
    /* Assertions remain active for this test even in a Release build. */
    if(!rd_selfcheck())return 1;
    if(!tap_guard_parse(NULL)||tap_guard_parse("1:9:00")||tap_guard_parse("1:4:e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855")||tap_guard_parse("1:2:bad")||
       !tap_guard_parse("100:2:e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"))return 60;
    uint32_t target=0;
    if(!rd_arm_bl_target(0x11514,0xeb235a19,&target)||target!=0x8e7d80 ||
       !rd_arm_bl_target(0x1000,0xebfffffe,&target)||target!=0x1000 ||
       rd_arm_bl_target(0x1002,0xeb235a19,&target)||rd_arm_bl_target(0x1000,0xea000000,&target)||
       rd_arm_bl_target(0xfffffffc,0xeb000000,&target))return 52;
    s5l8900_t *m=calloc(1,sizeof *m);arm_cpu_t *cpu=calloc(1,sizeof *cpu);
    arm_cpu_t *before=calloc(1,sizeof *before);uint8_t *ram=calloc(1,16384);
    if(!m||!cpu||!before||!ram)return 2;
    m->ram=ram;m->ram_base=UINT32_C(0xc0000000);m->ram_size=16384;
    g_mach=m;g_virt_base=m->ram_base;
    G.springboard_child_config.enabled=true;
    G.springboard_child_config.thread_task_offset=8;
    G.springboard_child_config.task_proc_offset=16;
    G.springboard_child_config.proc_pid_offset=8;
    putword(ram+0x108,0xc0001000);putword(ram+0x1010,0xc0001800);putword(ram+0x1808,7);
    cpu->cp15.tpidrprw=0xc0000100;cpu->cp15.ttbr0=0x12340018;
    cpu->cp15.ttbr1=0x56780018;cpu->cp15.context_id=9;
    cpu->cp15.dfar=0x13579bdf;cpu->cp15.dfsr=0xfeed;
    cpu->cpsr=ARM_MODE_SVC;cpu->spsr[ARM_BANK_SVC]=ARM_MODE_USR|ARM_CPSR_T;
    cpu->r[12]=(uint32_t)-31;cpu->r[14]=0x2002;
    cpu->bank_r13[ARM_BANK_USR]=0x3000;cpu->bank_r14[ARM_BANK_USR]=0x4001;
    cpu->r[0]=0x5000;cpu->r[1]=2;cpu->r[4]=0x1703;
    RD.enabled=true;RD.pid=7;RD.end=1000;RD.file=tmpfile();if(!RD.file)return 3;
    RD.symbols[RD_SWI]=0xc0000200;cpu->r[15]=RD.symbols[RD_SWI];
    memcpy(before,cpu,sizeof *cpu);rd_pre(cpu,1);
    if(memcmp(before,cpu,sizeof *cpu))return 4;
    rd_thread_t *s=rd_lookup(cpu->cp15.tpidrprw);
    if(!rd_target(s)||s->mach_calls!=1||!s->mach.open||s->mach.args[4]!=0x1703)return 5;
    cpu->cpsr=ARM_MODE_USR|ARM_CPSR_T;cpu->r[13]=0x3000;cpu->r[15]=0x2002;cpu->r[0]=0x10004003;
    cpu->cp15.ttbr0^=0x4000;rd_post(cpu,2,ARM_MODE_SVC,0,0,0);
    if(!s->mach.open||s->mach_returns)return 6;
    cpu->cp15.ttbr0^=0x4000;cpu->r[13]+=4;rd_post(cpu,3,ARM_MODE_SVC,0,0,0);
    if(!s->mach.open||s->mach_returns)return 7;
    cpu->r[13]-=4;cpu->r[15]+=2;rd_post(cpu,4,ARM_MODE_SVC,0,0,0);
    if(!s->mach.open||s->mach_returns)return 8;
    cpu->r[15]-=2;memcpy(before,cpu,sizeof *cpu);rd_post(cpu,5,ARM_MODE_SVC,0,0,0);
    if(memcmp(before,cpu,sizeof *cpu)||s->mach.open||s->mach_returns!=1||s->last_ret!=0x10004003)return 9;
    cpu->cpsr=ARM_MODE_UND;cpu->spsr[ARM_BANK_UND]=ARM_MODE_USR;
    memcpy(before,cpu,sizeof *cpu);rd_post(cpu,6,ARM_MODE_USR,0x9000,0xa000,0xb000);
    if(memcmp(before,cpu,sizeof *cpu)||RD.undefs!=1)return 10;
    RD.undefs=64;s->undefs=0;uint64_t events=RD.events;
    memcpy(before,cpu,sizeof *cpu);rd_post(cpu,7,ARM_MODE_USR,0x9000,0xa000,0xb000);
    if(memcmp(before,cpu,sizeof *cpu)||RD.events!=events+1||RD.undefs!=65||s->undefs!=1)return 13;
    /* Dynamic selection follows actual task and exact executable, not a PID guess. */
    RD.pid=0;RD.exec_count=1;strcpy(RD.exec_paths[0],"/synthetic/service");
    rd_exec_select(cpu,8,s,"/synthetic/service-suffix");if(rd_target(s))return 14;
    rd_exec_select(cpu,9,s,"/synthetic/service");if(!rd_target(s))return 15;
    s->id.task_pid=1234;if(!rd_target(s))return 16;
    rd_exec_select(cpu,10,s,"/synthetic/other");if(rd_target(s))return 17;
    RD.pid=1234;
    /* Two spaces using the same name remain two objects. Exact layout gate
     * belongs to the caller; this fixture provides synthetic known fields. */
    G.commcenter_layout.enabled=true;RD.entry_gate=true;RD.port_count=2;
    RD.ports[0].space=0xc0002000;RD.ports[0].name=0x5503;RD.ports[0].entry=0xc0000280;
    RD.ports[1].space=0xc0002100;RD.ports[1].name=0x5503;RD.ports[1].entry=0xc0000290;
    putword(ram+0x201c,1);putword(ram+0x2024,0xc0000200);putword(ram+0x2028,128);
    putword(ram+0x211c,1);putword(ram+0x2124,0xc0000300);putword(ram+0x2128,128);
    RD.ports[0].entry=0xc0000750;RD.ports[1].entry=0xc0000850;
    putword(ram+0x750,0xc0003000);putword(ram+0x754,0x03020001);
    putword(ram+0x850,0xc0003100);putword(ram+0x854,0x03020001);
    putword(ram+0x3004,0x80000000);putword(ram+0x3008,0x5503);putword(ram+0x303c,0xc0002000);putword(ram+0x2058,0xc0001000);
    memcpy(before,cpu,sizeof *cpu);rd_port(cpu,11,s,0,true);rd_port(cpu,11,s,1,true);
    if(memcmp(before,cpu,sizeof *cpu)||RD.ports[0].last[0]!=0xc0003000||RD.ports[1].last[0]!=0xc0003100)return 18;
    uint64_t unchanged=RD.events;rd_port(cpu,12,s,0,false);if(RD.events!=unchanged)return 19;
    /* Grow/migrate the table, poison the obsolete entry, preserve logical right. */
    putword(ram+0x2024,0xc0000400);putword(ram+0x950,0xc0003000);putword(ram+0x954,0x03020001);
    putword(ram+0x750,0x41414141);putword(ram+0x754,0xdeadbeef);rd_port(cpu,13,s,0,false);
    if(RD.ports[0].entry!=0xc0000950||RD.ports[0].last[0]!=0xc0003000)return 21;
    unchanged=RD.events;putword(ram+0x950,0);putword(ram+0x954,0x03000000);rd_port(cpu,14,s,0,false);
    if(RD.events!=unchanged+1||RD.ports[0].last[0]!=0)return 20;
    uint32_t entry=0,object=0,bits=0;
    putword(ram+0x954,0x04020001);
    if(rd_entry_resolve(cpu,0xc0002000,0x5503,&entry,&object,&bits)!=2)return 22;
    putword(ram+0x2054,1);
    if(rd_entry_resolve(cpu,0xc0002000,0x5503,&entry,&object,&bits)!=3)return 23;
    putword(ram+0x2054,0);putword(ram+0x201c,0);
    if(rd_entry_resolve(cpu,0xc0002000,0x5503,&entry,&object,&bits)!=4)return 24;
    /* Raw pointer arguments preserve bytes without treating them as strings,
     * and invalid pointers cannot alter CPU/CP15 state. */
    cpu->r[0]=0xc0003000;cpu->r[1]=0x1234;
    putword(ram+0x3000,0x44434241);putword(ram+0x3004,0);
    long argpos=ftell(RD.file);memcpy(before,cpu,sizeof *cpu);
    rd_arg_bytes(cpu,15,s,0);rd_arg_bytes(cpu,15,s,1);
    if(memcmp(before,cpu,sizeof *cpu))return 25;
    long argend=ftell(RD.file);fseek(RD.file,argpos,SEEK_SET);
    char args[4096]={0};size_t argn=fread(args,1,(size_t)(argend-argpos),RD.file);
    args[argn]=0;fseek(RD.file,argend,SEEK_SET);
    if(!strstr(args,"1145258561")||!strstr(args,"user_argument_bytes"))return 26;
    /* A startup storm cannot starve the selected service's later driver call,
     * nor consume another selected role's independent observation reserve. */
    for(unsigned j=0;j<128;j++)if(rd_site_capture(0,0,false)!=(j<48))return 27;
    if(!rd_site_capture(0,0,true)||!rd_site_capture(0,1,false)||
       rd_site_capture(32,0,true)||rd_site_capture(0,3,true))return 28;
    /* Synthetic BLX call: never arm on an ARM PC, unreadable/non-BLX code,
     * wrong native target, unselected task, or startup call. Objects are
     * discovered anew; nothing depends on historical guest addresses. */
    RD.focus.trigger=0xc0000200;RD.focus.symbols[RF_RUN]=0xc0000400;
    cpu->cpsr=ARM_MODE_SVC|ARM_CPSR_T;cpu->r[15]=RD.focus.trigger;
    cpu->r[13]=0xc0003f00;cpu->r[4]=0xc0000401;
    cpu->r[0]=0xc0003000;cpu->r[1]=0xc0000501;cpu->r[2]=0xc0003100;
    ram[0x200]=0xa0;ram[0x201]=0x47;ram[0x500]=0xf0;ram[0x501]=0xb5;
    putword(ram+0x300c,0xc0003200);putword(ram+0x3018,0xc0003300);
    cpu->cpsr&=~ARM_CPSR_T;if(rd_focus_trigger(cpu,16,s,true))return 29;
    cpu->cpsr|=ARM_CPSR_T;cpu->r[4]^=2;if(rd_focus_trigger(cpu,17,s,true))return 30;cpu->r[4]^=2;
    if(rd_focus_trigger(cpu,18,s,false)||RD.focus.sessions||RD.focus.records)return 31;
    memcpy(before,cpu,sizeof *cpu);if(!rd_focus_trigger(cpu,19,s,true)||memcmp(before,cpu,sizeof *cpu))return 32;
    if(RD.focus.gate!=0xc0003000||RD.focus.owner!=0xc0003200||RD.focus.loop!=0xc0003300||RD.focus.command!=0xc0003100)return 33;
    cpu->r[15]=0xc0000500;rd_focus_pre(cpu,20,true);
    if(!RD.focus.action_pending_pc||RD.focus.action_executed)return 34;
    /* A failed fetch/exception is not reported as an executed prologue. */
    cpu->cpsr=ARM_MODE_ABT;cpu->r[15]=0x0c;rd_focus_post(cpu,21,ARM_MODE_SVC|ARM_CPSR_T,0xc0000500);
    if(RD.focus.action_executed)return 35;
    cpu->cpsr=ARM_MODE_SVC|ARM_CPSR_T;cpu->r[15]=0xc0000500;rd_focus_pre(cpu,22,true);
    cpu->r[15]+=2;memcpy(before,cpu,sizeof *cpu);rd_focus_post(cpu,23,ARM_MODE_SVC|ARM_CPSR_T,0xc0000500);
    if(!RD.focus.action_executed||memcmp(before,cpu,sizeof *cpu))return 36;
    rd_focus_add_event(0xc0003500);rd_focus_add_event(0xc0003500);
    if(RD.focus.event_count!=1||rd_focus_event(0xc0003504))return 37;
    cpu->cp15.tpidrprw=0x2222;cpu->r[0]=0x1111;cpu->r[1]=0;cpu->r[2]=0;
    if(rd_focus_related(cpu))return 38;
    cpu->r[0]=0xc0003500;if(!rd_focus_related(cpu))return 39;
    cpu->r[0]=0xc0003504;if(rd_focus_related(cpu))return 40;
    cpu->cp15.tpidrprw=RD.focus.thread;cpu->r[15]=RD.focus.returned.pc;cpu->r[13]=RD.focus.returned.sp;
    cpu->cp15.context_id^=1;rd_focus_pre(cpu,24,true);if(!RD.focus.active)return 41;
    cpu->cp15.context_id^=1;memcpy(before,cpu,sizeof *cpu);rd_focus_pre(cpu,25,true);
    if(RD.focus.active||RD.focus.returned.open||memcmp(before,cpu,sizeof *cpu))return 42;
    /* Nested fields and raw hardware snapshots are RAM/model observations.
     * Poisoned/overflow pointers never reach MMIO and no machine byte changes. */
    RD.focus.active=true;RD.focus.field_count=2;RD.focus.field_offsets[0]=0x70;RD.focus.field_offsets[1]=0x84;
    putword(ram+0x3270,0xc0003600);putword(ram+0x3284,0xfffffff0);
    s5l8900_t *machine_before=malloc(sizeof *m);uint8_t ram_before[16384];
    if(!machine_before)return 44;memcpy(machine_before,m,sizeof *m);memcpy(ram_before,ram,sizeof ram_before);
    memcpy(before,cpu,sizeof *cpu);rd_focus_fields(cpu,26,s,true);rd_focus_board(cpu,26,s);
    if(memcmp(before,cpu,sizeof *cpu)||memcmp(machine_before,m,sizeof *m)||memcmp(ram_before,ram,sizeof ram_before))return 45;
    if(RD.focus.field_objects[0]!=0xc0003600||RD.focus.field_objects[1]!=0xfffffff0)return 46;
    cpu->cp15.tpidrprw=0x2222;cpu->r[0]=0xc0003600;cpu->r[1]=cpu->r[2]=0;
    if(!rd_focus_related(cpu))return 47;cpu->r[0]=0xc0003604;if(rd_focus_related(cpu))return 48;
    /* Optional clock/TX raw supplement keeps the 85-word legacy prefix.
     * Capturing it must not tick the device or mutate architectural state. */
    m->i2s[0].experimental_hz=44100;m->i2s[0].frame_phase=1234;
    m->i2s[0].frame_edges=UINT64_C(0x300000008);m->i2s[0].clock_generation=11;
    m->i2s[0].tx_level=31;m->i2s[0].tx_bytes_in=UINT64_C(0x100000003);
    m->i2s[0].tx_bytes_out=UINT64_C(0x200000005);
    m->i2s[0].tx_underflow_frames=7;m->i2s[0].tx_refused=9;
    memcpy(machine_before,m,sizeof *m);memcpy(before,cpu,sizeof *cpu);
    FILE *saved_file=RD.file;RD.file=tmpfile();if(!RD.file)return 49;
    rd_focus_board(cpu,27,s);
    if(memcmp(machine_before,m,sizeof *m)||memcmp(before,cpu,sizeof *cpu))return 50;
    rewind(RD.file);char board_json[8192]={0};
    if(!fgets(board_json,sizeof board_json,RD.file) ||
       !strstr(board_json,",44100,1234,8,3,11,31,3,1,5,2,7,9]}"))return 51;
    fclose(RD.file);RD.file=saved_file;
    free(machine_before);RD.focus.active=false;
    /* Synthetic post-BL pair explicitly ignores pre-BL LR and rejects wrong
     * context/SP/PC. Captures native BOOL without mutating CPU/RAM/device. */
    RD.bl_count=1;RD.bl_sites[0]=0xc0000200;RD.start=0;RD.end=1000;
    cpu->cpsr=ARM_MODE_USR;cpu->r[15]=0xc0000200;cpu->r[13]=0xc0003f00;
    cpu->r[14]=0xdeadbeef;putword(ram+0x200,0xeb000006);
    s->id.task_pid=RD.pid;s->bl_hits=0;memcpy(before,cpu,sizeof *cpu);
    rd_bl_pre(cpu,28,s);
    if(!s->bl.open||s->bl.pc!=0xc0000204||memcmp(before,cpu,sizeof *cpu))return 53;
    cpu->r[15]=0xc0000204;cpu->r[0]=1;cpu->cp15.context_id^=1;rd_bl_pre(cpu,29,s);
    if(!s->bl.open)return 54;cpu->cp15.context_id^=1;cpu->r[13]+=4;rd_bl_pre(cpu,30,s);
    if(!s->bl.open)return 55;cpu->r[13]-=4;memcpy(before,cpu,sizeof *cpu);rd_bl_pre(cpu,31,s);
    if(s->bl.open||memcmp(before,cpu,sizeof *cpu))return 56;
    RD.graphics=true;RD.graphics_records=0;m->cpu=*cpu;m->cpu.cycles=32;G.hot_now=32;
    memcpy(machine_before=malloc(sizeof *m),m,sizeof *m);if(!machine_before)return 57;
    rd_graphics_sample(32,true);rd_graphics_access(S5L8900_MBX_BASE+0x680,1,4,true);
    rd_graphics_access(S5L8900_MBX_BASE+S5L_MBX_3D_DATA_FIFO,0xf0000000,4,true);
    if(memcmp(machine_before,m,sizeof *m)||RD.graphics_records<4||RD.fifo_scenes!=1)return 58;
    unsigned capbefore=RD.graphics_records;RD.graphics_records=1024;
    rd_graphics_emit("synthetic_cap",33,NULL,0);if(RD.graphics_records!=1024||!capbefore)return 59;
    RD.graphics=false;free(machine_before);
    if(!tap_guard_parse("100:0:e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855;200:2:e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"))return 64;
    tap_guard_select(200);if(!tap_guard.enabled||tap_guard.window!=2)return 65;
    tap_guard_select(150);if(tap_guard.enabled)return 66;
    if(tap_guard_parse("100:0:e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855;"))return 67;
    tap_guard_select(100);if(!tap_guard.enabled||tap_guard.window!=0)return 68;
    memset(&m->clcd,0,sizeof m->clcd);m->clcd.ctrl=0x40;tap_guard.window=0;
    m->clcd.win[0].control=0x300;m->clcd.win[0].geometry=(1u<<16)|1u;
    m->clcd.win[0].stride=2;m->clcd.win[0].fbaddr=m->ram_base;
    /* Native view geometry fixture uses declared register names below. */
    clcd_native_view_t view;clcd_native_status_t status=clcd_native_view(&m->clcd,0,m->ram,m->ram_base,m->ram_size,&view);
    if(status!=CLCD_NATIVE_OK)return 61;
    ios3_sha256(view.pixels,view.bytes,tap_guard.hash);memcpy(machine_before=malloc(sizeof *m),m,sizeof *m);
    if(!machine_before||!tap_guard_matches(m)||memcmp(machine_before,m,sizeof *m))return 62;
    tap_guard.hash[0]^=1;if(tap_guard_matches(m))return 63;free(machine_before);tap_guard.enabled=false;
    /* Native FIFO recorder: exact words, independent hard bounds and
     * byte-for-byte guest immutability. Temporary files contain synthetic data. */
    char ta_dir[]="/private/tmp/iosretromob-ta-observer-XXXXXX";
    if(!mkdtemp(ta_dir))return 69;
    setenv("IOSRETROMOB_TA_CAPTURE_DIR",ta_dir,1);
    setenv("IOSRETROMOB_TA_CAPTURE_START","0",1);
    setenv("IOSRETROMOB_TA_CAPTURE_END","1000",1);
    if(!rta_parse(1000))return 70;
    memcpy(machine_before=malloc(sizeof *m),m,sizeof *m);if(!machine_before)return 71;
    G.hot_now=40;
    const uint32_t synthetic_ta[]={0x10000010,0x12345678,0xf0000000};
    for(unsigned j=0;j<3;j++)rta_access(S5L8900_MBX_BASE+S5L_MBX_3D_DATA_FIFO,synthetic_ta[j],4,true);
    if(RTA.scene!=1||RTA.total!=12||memcmp(machine_before,m,sizeof *m))return 72;
    char cap_path[PATH_MAX];snprintf(cap_path,sizeof cap_path,"%s/packet-00-complete.bin",ta_dir);
    FILE *cap=fopen(cap_path,"rb");uint8_t expected_ta[]={0x10,0,0,0x10,0x78,0x56,0x34,0x12,0,0,0,0xf0},got[12];
    if(!cap||fread(got,1,12,cap)!=12||memcmp(got,expected_ta,12)||fgetc(cap)!=EOF)return 73;fclose(cap);
    /* Preserve an overlong prefix explicitly, never mark it complete. */
    for(unsigned j=0;j<RTA_WORDS;j++)rta_access(S5L8900_MBX_BASE+S5L_MBX_3D_DATA_FIFO,j?0:0x10000010,4,true);
    rta_access(S5L8900_MBX_BASE+S5L_MBX_3D_DATA_FIFO,0xf0000000,4,true);
    if(RTA.scene!=2||RTA.total!=12+RTA_WORDS*4u||RTA.failed||memcmp(machine_before,m,sizeof *m))return 74;
    for(unsigned j=2;j<RTA_SCENES;j++)for(unsigned k=0;k<3;k++)rta_access(S5L8900_MBX_BASE+S5L_MBX_3D_DATA_FIFO,synthetic_ta[k],4,true);
    uint64_t old_total=RTA.total;
    for(unsigned k=0;k<3;k++)rta_access(S5L8900_MBX_BASE+S5L_MBX_3D_DATA_FIFO,synthetic_ta[k],4,true);
    if(RTA.scene!=RTA_SCENES||RTA.total!=old_total||RTA.failed||memcmp(machine_before,m,sizeof *m))return 75;
    rta_finish();free(machine_before);
    unsetenv("IOSRETROMOB_TA_CAPTURE_DIR");unsetenv("IOSRETROMOB_TA_CAPTURE_START");unsetenv("IOSRETROMOB_TA_CAPTURE_END");
    puts("PASS: synthetic native FIFO bytes, overlong-prefix label, 32-packet cap and immutable guest machine");
    RD.focus.records=1024;uint64_t focusbefore=RD.events;rd_focus_emit("gate_synthetic",cpu,26,s,NULL,0);
    if(RD.events!=focusbefore||RD.focus.suppressed!=1)return 43;
    RD.events=RD_EVENTS-64;uint64_t drops=RD.dropped;
    memcpy(before,cpu,sizeof *cpu);rd_emit("bounded",cpu,7,s,NULL,0);
    if(RD.dropped!=drops+1||memcmp(before,cpu,sizeof *cpu))return 11;
    rewind(RD.file);char log[8192];size_t n=fread(log,1,sizeof log-1,RD.file);log[n]=0;
    if(!strstr(log,"mach_msg_return")||!strstr(log,"user_undefined")||strstr(log,"\"kind\":\"bounded\""))return 12;
    fclose(RD.file);free(ram);free(before);free(cpu);free(m);
    puts("PASS: real observer routing, isolated return identities, raw error code, undefined entry, bounds and CPU/CP15 immutability");
    return 0;
}
