#include "aurum.h"
#include <string.h>
#include <signal.h>

static int invocation_error(void) {
 fputs("usage: aurum --seed seed.txt --commands commands.txt|-\n",stderr);
 return 2;
}
int main(int argc, char **argv) {
#ifdef SIGPIPE
 /* Convert a closed output pipe into the same checked I/O failure as a full file. */
 if (signal(SIGPIPE,SIG_IGN) == SIG_ERR) { fputs("cannot configure output errors\n",stderr); return 2; }
#endif
 const char *seed_path = NULL, *commands_path = NULL;
 if (argc != 5) return invocation_error();
 for (int i = 1; i < argc; i += 2) {
  if (!strcmp(argv[i],"--seed") && !seed_path) seed_path = argv[i + 1];
  else if (!strcmp(argv[i],"--commands") && !commands_path) commands_path = argv[i + 1];
  else return invocation_error();
 }
 if (!seed_path || !commands_path || !strcmp(seed_path,"-")) return invocation_error();
 FILE *seed = fopen(seed_path,"rb");
 if (!seed) { fputs("cannot open seed\n",stderr); return 2; }
 Engine e;
 engine_init(&e);
 bool loaded = seed_load(seed,&e);
 if (fclose(seed) != 0) loaded = false;
 if (!loaded) { fputs("invalid or unreadable seed\n",stderr); engine_free(&e); return 2; }
 FILE *commands = !strcmp(commands_path,"-") ? stdin : fopen(commands_path,"rb");
 if (!commands) { fputs("cannot open command stream\n",stderr); engine_free(&e); return 2; }
 int exit_code = 0;
 for (;;) {
  char line[LINE_MAX_BYTES + 1];
  int status = read_line(commands,line,sizeof(line));
  if (status == 0) break;
  if (status == -2) { fputs("cannot read command stream\n",stderr); exit_code = 2; break; }
  Command c;
  command_default(&c,INVALID_OP);
  if (status == 1) status = parse_command(line,&c);
  if (status == 0) continue;
  Result r;
  if (status < 0) { r = result_default(&c); r.decision = ERROR; r.reason = INVALID_INPUT; }
  else r = execute(&e,&c);
  write_result(stdout,&e,&c,&r);
  if (ferror(stdout)) { fputs("cannot write results\n",stderr); exit_code = 2; break; }
 }
 if (commands != stdin && fclose(commands) != 0) { fputs("cannot close command stream\n",stderr); exit_code = 2; }
 if (fflush(stdout) != 0) { fputs("cannot flush results\n",stderr); exit_code = 2; }
 engine_free(&e);
 return exit_code;
}
