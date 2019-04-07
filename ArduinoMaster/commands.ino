#define ESC 27
#define CR  13
#define LF  10

#define MAX_TOKENS 8

int command_scan(char *token[], char *cmd) {
  char *delim = cmd;
  int i;

  for(i = 0; i < MAX_TOKENS; i++) {
    while(*delim == ' ') {            // trim leading spaces
      delim++;
    }
    token[i] = delim;                 // token starts here
    delim = strchr(token[i], ' ');    // and ends at next space
    if(delim == NULL) {               // if no more spaces,
      return i + 1;                   // then return number of tokens
    }
    *delim++ = 0;                     // otherwise null-terminate and move to next token
  }
  return -1;                          // too many tokens
}

#define CMD_BUFSIZE 64
unsigned int cmd_index = 0;
char cmd[CMD_BUFSIZE];

void command_process(char *cmd) {
  char *token[MAX_TOKENS];
  int tokens;

  tokens = command_scan(token, cmd);
  if(strcmp(token[0], "download") == 0 && tokens == 3) {    // "download <file> <size>"
    profile_download(token[1], token[2]);
  } else if(strcmp(token[0], "profile") == 0 && tokens == 2) {
    profile_open_by_index(atoi(token[1]));
  }

  cmd_index = 0;                  // clear command after it has been processed (successfully or not)
}

void command_byte(int rx) {
  if(rx < 0 || rx == LF) {        // no data available (LF ignored)
    return;
  }
  
  if(rx == ESC) {
    cmd_index = 0;                // clear command
    return;
  }

  if(cmd_index < CMD_BUFSIZE) {   // avoid buffer overflow
    if(rx == CR) {                // end of command; process it
      cmd[cmd_index++] = 0;
      command_process(cmd);
    } else {                      // next byte of command
      cmd[cmd_index++] = rx;
    }
  }
}
