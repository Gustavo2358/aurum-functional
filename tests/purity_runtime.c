#define _GNU_SOURCE
#include "core.h"
#include <assert.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#define EACH(X) \
 X(Account,accounts,account,TABLE_ACCOUNT) X(Card,cards,card,TABLE_CARD) \
 X(Merchant,merchants,merchant,TABLE_MERCHANT) X(Authorization,auths,auth,TABLE_AUTH) \
 X(Capture,captures,capture,TABLE_CAPTURE) X(Lot,lots,lot,TABLE_LOT) \
 X(Invoice,invoices,invoice,TABLE_INVOICE) X(History,history,history,TABLE_HISTORY) \
 X(Entry,ledger,entry,TABLE_LEDGER) X(Cached,keys,key,TABLE_KEY) \
 X(Payment,payments,payment,TABLE_PAYMENT) X(Refund,refunds,refund,TABLE_REFUND) \
 X(Reward,rewards,reward,TABLE_REWARD)
#define ARRAY(T,N,F,E) T N[64]; size_t N##_len;
typedef struct { Config config; int64_t now; Command command; EACH(ARRAY) } Snapshot;
#undef ARRAY
static uint64_t hash_bytes(uint64_t h,const void *p,size_t n) {
 const unsigned char *b=p; for(size_t i=0;i<n;i++) h=(h^b[i])*UINT64_C(1099511628211); return h;
}
static uint64_t hash_string(uint64_t h,const char *s,size_t limit) {
 size_t n=0;while(n<limit && s[n])n++; h=hash_bytes(h,s,n);return hash_bytes(h,&n,sizeof n);
}
#include "purity_semantic.h"
static const uint64_t basis=UINT64_C(1469598103934665603);
static uint64_t raw_hash(const Snapshot *s) { return hash_bytes(basis,s,sizeof *s); }
static uint64_t semantic_hash(const Snapshot *s) {
 uint64_t h=hash_Config(basis,&s->config); h=hash_bytes(h,&s->now,sizeof s->now);
 h=hash_Command(h,&s->command);
#define HASH(T,N,F,E) h=hash_bytes(h,&s->N##_len,sizeof s->N##_len); for(size_t i=0;i<s->N##_len;i++)h=hash_##T(h,&s->N[i]);
 EACH(HASH)
#undef HASH
 return h;
}
static Snapshot *allocate(void) {
 Snapshot *s=mmap(NULL,sizeof *s,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
 assert(s!=MAP_FAILED);return s;
}
static void protect(Snapshot *s) { assert(mprotect(s,sizeof *s,PROT_READ)==0); }
static void release(const Snapshot *s) { assert(munmap((void *)s,sizeof *s)==0); }
static StateView view(const Snapshot *s) {
 StateView v={.config=s->config,.now=s->now};
#define VIEW(T,N,F,E) v.N=s->N; v.N##_len=s->N##_len;
 EACH(VIEW)
#undef VIEW
 return v;
}
static Snapshot *seed(void) {
 Snapshot *s=allocate();s->config=core_config_default();
#define CAP(T,N,F,E) s->config.N##_capacity=64;
 EACH(CAP)
#undef CAP
 s->accounts[0]=(Account){.id="A",.active=true,.credit_limit=100000,.daily_limit=100000,.per_operation_limit=100000};s->accounts_len=1;
 s->cards[0]=(Card){.id="C",.account_id="A",.active=true,.expiry_day=10000,.allow_international=true,.allow_contactless=true};s->cards_len=1;
 s->merchants[0]=(Merchant){.id="M",.active=true,.country="BR"};s->merchants_len=1;
 s->command=core_command_default(AUTHORIZE);s->command.amount=10000;
 strcpy(s->command.request_id,"R");strcpy(s->command.account_id,"A");strcpy(s->command.card_id,"C");strcpy(s->command.merchant_id,"M");
 protect(s);return s;
}
static uint64_t edit_hash(uint64_t h,Edit e) {
 h=hash_bytes(h,&e.table,sizeof e.table);h=hash_bytes(h,&e.index,sizeof e.index);h=hash_bytes(h,&e.append,sizeof e.append);
 switch(e.table) {
#define EDITHASH(T,N,F,E) case E:return hash_##T(h,&e.value.F);
 EACH(EDITHASH)
#undef EDITHASH
 case TABLE_CLOCK:return hash_bytes(h,&e.value.now,sizeof e.value.now);
 }
 assert(0);return 0;
}
static Snapshot *apply(const Snapshot *before,Edit e) {
 Snapshot *s=allocate();memcpy(s,before,sizeof *s);
 switch(e.table) {
#define APPLY(T,N,F,E) case E: assert(e.index<64); if(e.append){assert(e.index==s->N##_len);s->N##_len++;}else assert(e.index<s->N##_len);s->N[e.index]=e.value.F;break;
 EACH(APPLY)
#undef APPLY
 case TABLE_CLOCK:s->now=e.value.now;break;
 }
 protect(s);return s;
}
typedef struct { Result result; uint64_t edits, state; size_t steps; CoreStatus status; } Run;
static Run transaction(const Snapshot *published,Command command) {
 const Snapshot *staged=published;Continuation continuation=core_begin();Run run={.edits=basis};
 uint64_t original_raw=raw_hash(published),original_semantic=semantic_hash(published);
 for(size_t count=0;count<512;count++) {
  uint64_t raw=raw_hash(staged),semantic=semantic_hash(staged);
  Step step=core_step(view(staged),command,continuation);
  assert(raw_hash(staged)==raw && semantic_hash(staged)==semantic);
  assert(raw_hash(published)==original_raw && semantic_hash(published)==original_semantic);
  run.steps++;run.status=step.status;
  if(step.status!=CORE_EDIT) {
   assert(step.status==CORE_COMMIT || step.status==CORE_ABORT || step.status==CORE_READ);
   run.result=step.result;run.state=semantic_hash(staged);
   if(staged!=published)release(staged);
   return run;
  }
  run.edits=edit_hash(run.edits,step.edit);
  Snapshot *next=apply(staged,step.edit);if(staged!=published)release(staged);staged=next;continuation=step.next;
 }
 assert(!"core continuation failed to terminate in bounded test fixture");return run;
}
static void same(Run a,Run b) {
 assert(a.status==b.status && a.steps==b.steps && a.edits==b.edits && a.state==b.state);
 assert(hash_Result(basis,&a.result)==hash_Result(basis,&b.result));
}
static void arithmetic(void) {
 Number n=core_money_add(MONEY_MAX-1,1);assert(n.ok && n.value==MONEY_MAX);
 assert(!core_money_add(MONEY_MAX,1).ok);assert(!core_money_add(-1,1).ok);
 n=core_money_sub(MONEY_MAX,MONEY_MAX);assert(n.ok && n.value==0);assert(!core_money_sub(0,1).ok);
 assert(!core_checked_mul(INT64_MAX,2).ok);assert(!core_checked_mul(-1,0).ok);
 n=core_checked_mul(INT64_MAX,1);assert(n.ok && n.value==INT64_MAX);
 n=core_floor_rate(101,100);assert(n.ok && n.value==1);n=core_ceil_rate(101,100);assert(n.ok && n.value==2);
 n=core_money_fx(1,5000);assert(n.ok && n.value==1);assert(!core_money_fx(MONEY_MAX,100000).ok);
 n=core_proportional(MONEY_MAX,MONEY_MAX-1,MONEY_MAX);assert(n.ok && n.value==MONEY_MAX-1);
 assert(!core_proportional(1,1,0).ok);n=core_cumulative_delta(10,1,1,3);assert(n.ok && n.value==3);
 assert(core_split_part(10,3,0)==4 && core_split_part(10,3,1)==3 && core_split_part(10,3,2)==3);
 assert(core_split_part(10,0,0)==-1);
 for(Money total=0;total<=300;total++)for(int64_t parts=1;parts<=12;parts++){
  Money sum=0;for(int64_t i=0;i<parts;i++)sum+=core_split_part(total,parts,i);assert(sum==total);
 }
}
int main(void) {
 arithmetic();Snapshot *s=seed();uint64_t raw_before=raw_hash(s),semantic_before=semantic_hash(s);
 assert(core_config_valid(&s->config) && core_command_valid(&s->command));
 assert(core_find_account(view(s),"A")==&s->accounts[0]);assert(core_find_card(view(s),"C")==&s->cards[0]);
 Price p=core_quote(&s->config,&s->command);assert(p.reason==NONE && p.principal==10000 && p.fee==0);
 Projection projection=core_project_account(view(s),&s->accounts[0]);assert(projection.debt==0 && projection.held==0 && projection.available_credit==100000);
 assert(core_ledger_balanced(view(s)));Reconciliation rec=core_reconcile(view(s),"A");assert(rec.valid && rec.debt_difference==0 && rec.held_difference==0);
 Run first=transaction(s,s->command);assert(first.status==CORE_COMMIT && first.result.decision==APPROVED && first.result.reserved==10000);
 Command quote=core_command_default(QUOTE);quote.amount=20000;Run q=transaction(s,quote);assert(q.status==CORE_READ && q.result.principal==20000);
 Command invalid=core_command_default(INVALID_OP);Run bad=transaction(s,invalid);assert(bad.status==CORE_ABORT && bad.result.reason==INVALID_INPUT);
 Command get=core_command_default(GET_ACCOUNT);strcpy(get.account_id,"A");Run read=transaction(s,get);assert(read.status==CORE_READ);
 Command tick=core_command_default(TICK);tick.now=7;Run timed=transaction(s,tick);assert(timed.status==CORE_COMMIT);
 Snapshot *other=allocate();memcpy(other,s,sizeof *s);other->accounts[0].credit_limit=5000;protect(other);
 Run declined=transaction(other,s->command);assert(declined.result.decision==DECLINED && declined.result.reason==CREDIT_LIMIT);
 for(int i=0;i<40;i++) { same(first,transaction(s,s->command));same(declined,transaction(other,s->command));same(q,transaction(s,quote));same(timed,transaction(s,tick)); }
 assert(raw_hash(s)==raw_before && semantic_hash(s)==semantic_before);
 printf("{\"status\":\"PASS\",\"readonly_collections\":13,\"repetitions\":40,\"interleaved_commands\":4,\"arithmetic_split_cases\":3612,\"raw_before\":\"%016" PRIx64 "\",\"raw_after\":\"%016" PRIx64 "\",\"semantic_before\":\"%016" PRIx64 "\",\"semantic_after\":\"%016" PRIx64 "\",\"authorize_steps\":%zu,\"tick_steps\":%zu}\n",raw_before,raw_hash(s),semantic_before,semantic_hash(s),first.steps,timed.steps);
 release(other);release(s);return 0;
}
