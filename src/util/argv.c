#include <stdlib.h> 
#include <string.h>
#include <stdarg.h>
#include "stringop.h"
#include "mymalloc.h"
#include "argv.h"

/* argv_alloc - initialize string array */

ARGV *argv_alloc(int len)
{
    const char *myname = "argv_alloc";
    
    ARGV  *argvp;
    
    /*
     * Make sure that always argvp->argc < argvp->len.
     */

    argvp = (ARGV *) mymalloc(sizeof(*argvp));
    argvp->len = (len < 2 ? 2 : len);
    argvp->argv = (char **) mymalloc((argvp->len + 1) * sizeof(char *));

    argvp->argc = 0;
    argvp->argv[0] = 0;


    return (argvp);
}


/* argv_free - destroy string array */
ARGV  *argv_free(ARGV *argvp)
{
    char  **cpp;

    for (cpp = argvp->argv; cpp < argvp->argv + argvp->argc; cpp++)
        myfree(*cpp);

    myfree((char *) argvp->argv);
    myfree((char *) argvp);

    return (0);
}

/* argv_add - add string to vector */
void argv_add(ARGV *argvp,...)
{
    char *arg;
    va_list ap;

    /*
    * * Make sure that always argvp->argc < argvp->len.
    * */
    va_start(ap, argvp);
    while((arg = va_arg(ap, char *)) !=0)
    {
        if (argvp->argc + 1 >= argvp->len) 
        {
            argvp->len *= 2;
            argvp->argv = (char **)myrealloc((char *) argvp->argv, (argvp->len + 1) * sizeof(char *));
        }
        argvp->argv[argvp->argc++] = mystrdup(arg);
    }

}

/* argv_terminate - terminate string array */

void argv_terminate(ARGV *argvp)
{
        /*
         *      * Trust that argvp->argc < argvp->len.
         *           */
        argvp->argv[argvp->argc] = 0;
}

/* argv_split - split string into token array */

ARGV   *argv_split(const char *string, const char *delim)
{
	ARGV  *argvp = argv_alloc(1);
	char  *saved_string = mystrdup(string);
	char   *bp = saved_string;
	char   *arg;

	while ((arg = mystrtok(&bp, delim)) != 0)
	{
		argv_add(argvp, arg, (char *) 0);
	}

	argv_terminate(argvp);
	myfree(saved_string);

	return (argvp);
}

/* argv_split_append - split string into token array, append to array */

ARGV   *argv_split_append(ARGV *argvp, const char *string, const char *delim)
{
	char   *saved_string = mystrdup(string);
	char   *bp = saved_string;
	char   *arg;

	while ((arg = mystrtok(&bp, delim)) != 0)
	{
		argv_add(argvp, arg, (char *) 0);
	}

	argv_terminate(argvp);
	myfree(saved_string);

	return (argvp);
}
