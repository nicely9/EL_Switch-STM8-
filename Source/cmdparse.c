/************************************************************************************
	Project		: 전자식스위치
	File Name	: CMDPARSE.C
	Target		: EL_Switch
	Revision	: Rev 1.0
	Compiler	: ST Visual Develop(COSMIC)
	Date		: 2020/04/20
	Designed by	: J.S Yoon
	Modify		:
	Section		: NEO INFORMATION SYSTEMS CO.,LTD. R&D
************************************************************************************/

#include "header.h"
#include "cmdparse.h"

// Function Prototypes
void rip (char *s);
char *index(char *s, char c);
//int cmdparse(const struct cmds cmds[], char *line);


int cmdparse(const struct cmds cmds[], char *line)
{
  	const struct cmds *cmdp;
  	char *argv[NARG], *cp;
  	int argc, rslt;

  	rip (line);		/* replace terminating end of line marker(s) with null */

  	for (argc = 0; argc < NARG; argc++)		// initialize 
    		argv[argc] = NULLCHAR;

  	for (argc = 0; argc < NARG && *line != '\0';)
    	{
		/* Skip leading white space */
      	while(*line == ' ' || *line == '\t')
			line++;
      	if (*line == '\0') break;
	     argv[argc++] = line;	/* Beginning of token */

		/* Find terminating delimiter */
      	for (cp = line; *cp; cp++)
			if (*cp == ' ' || *cp == '\t')
	  			break;
      	if (*cp != '\0') *cp++ = '\0';
      	line = cp;
    	}
  	if (argc < 1)
    	{				/* empty command line */
    		return -1;		//eklee
    	}
	/* Look up command in table; prefix matches are OK */
  	for (cmdp = cmds; cmdp->name != NULLCHAR; cmdp++)
    	{
     	if (strncmp (argv[0], cmdp->name, strlen(cmdp->name)) == 0)
			break;
    	}
  	if (cmdp->name == NULLCHAR)
    	{
		printf("\nInvalid Command!\n");
      		return -1;
    	} else {
      	if (argc < cmdp->argcmin) {
			/* Insufficient arguments */
	  		printf("\nUsage: %s\n", cmdp->argc_errmsg);
	  		return -1;
		} else {
	  		printf("\n");		//eklee
	  		argv[0] = cmdp->name;
	  		rslt = (*cmdp->func)(argc, (void*)argv);
	  		if ((rslt < 0) && (cmdp->exec_errmsg != NULLCHAR))
	    			printf("\n%s\n", cmdp->exec_errmsg);
	  		return (rslt);
		}
    	}
}


// replace terminating end of line marker(s) with null 
void rip(char *s)
{
  	char *cp;
  	
  	if ((cp = (char *)index(s, '\r')) != NULLCHAR)		
    		*cp = '\0';
  	if ((cp = (char *)index (s, '\n')) != NULLCHAR)
    		*cp = '\0';
}

char *index(char *s, char c)
{
	for( ; *s != c && *s != '\0'; s++)
		;
	return s;
}

