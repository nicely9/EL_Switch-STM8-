#ifndef _CMDPARSE_H
#define _CMDPARSE_H

#define NARG		10	/* Max number of args to commands */

struct cmds {
	char *name;		/* Name of command */
	//int (*func)();		/* Function to execute command */
	int (*func)(int, char *);		/* Function to execute command */
	int  argcmin;		/* Minimum number of args */
	char *argc_errmsg;	/* Message to print if insufficient args */
	char *exec_errmsg;	/* Message to print if function fails */
};


extern int cmdparse(const struct cmds cmds[], char *line);

#ifndef NULLCHAR
#define NULLCHAR	(char *)0
#endif

#endif
