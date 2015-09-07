/*
 * wl command-line swiss-army-knife utility. Generic (not OS-specific)
 * implementation that interfaces to WLAN driver API (wl_drv.h).
 *
 *
 * $ Copyright Broadcom Corporation 2008 $
 * $Id: wlu_generic.c 378881 2013-01-15 05:41:08Z richarch $
 */

/* ---- Include Files ---------------------------------------------------- */
#undef BCMDRIVER
#include "typedefs.h"
#include "bcmutils.h"
#include <stdio.h>
#include <string.h>

#include "internal/wwd_sdpcm.h"
#include "network/wwd_buffer_interface.h"
#include "RTOS/wwd_rtos_interface.h"
#include "wiced_wifi.h"
#include "wlu.h"


/* ---- Public Variables ------------------------------------------------- */
/* ---- Private Constants and Types -------------------------------------- */

#define NUM_ARGS	64


/* ---- Private Variables ------------------------------------------------ */

/* ---- Private Function Prototypes -------------------------------------- */

static cmd_t* wl_find_cmd(char* name);
extern int wl_ioctl(void *wl, int cmd, void *buf, int len, bool set);
extern const char *wlu_av0;

/* ---- Functions -------------------------------------------------------- */

/****************************************************************************
* Function:   wl_find_cmd
*
* Purpose:    Search the wl_cmds table for a matching command name.
*             Return the matching command or NULL if no match found.
*
* Parameters: name (in) Name of command to find.
*
* Returns:    Command structure. NULL if not found.
*****************************************************************************
*/
static cmd_t *
wl_find_cmd(char* name)
{
	cmd_t *cmd = NULL;

	/* search the wl_cmds for a matching name */
	for (cmd = wl_cmds; cmd->name && strcmp(cmd->name, name); cmd++);

	if (cmd->name == NULL)
		cmd = NULL;

	return cmd;
}



/* ----------------------------------------------------------------------- */
int
wl_get(void *wl, int cmd, void *buf, int len)
{
   return wl_ioctl( wl, cmd, buf, len, FALSE );
}

/* ----------------------------------------------------------------------- */
int
wl_set(void *wl, int cmd, void *buf, int len)
{
    return wl_ioctl( wl, cmd, buf, len, TRUE );
}


/* ----------------------------------------------------------------------- */
int
wlu_main_args(int argc, char **argv)
{
	cmd_t *cmd = NULL;
	int err = 0;
	int help = 0;
	int status = CMD_WL;
	char *notused;

	wlu_av0 = argv[0];

	/* Skip first arg. */
	argc--;
	argv++;

	wlu_init();

	while (*argv != NULL) {

		/* command option */
		if ((status = wl_option(&argv, &notused, &help)) == CMD_OPT) {
			if (help)
				break;

			continue;
		}
		/* parse error */
		else if (status == CMD_ERR)
			break;
		/* wl command */
		/*
		 * else if (status == CMD_WL)
		 *	;
		 */


		/* search for command */
		cmd = wl_find_cmd(*argv);

		/* defaults to using the set_var and get_var commands */
		if (!cmd)
			cmd = &wl_varcmd;

		/* do command */
		err = (*cmd->func)(NULL, cmd, argv);

		break;
	}

	if (help && *argv) {
		cmd = wl_find_cmd(*argv);
		if (cmd)
			wl_cmd_usage(stdout, cmd);
		else {
			printf("%s: Unrecognized command \"%s\", type -h for help\n",
			       wlu_av0, *argv);
		}
	} else if (!cmd)
		wl_usage(stdout, NULL);
	else if (err == BCME_ERROR)
		wl_cmd_usage(stderr, cmd);
	else if (err != BCME_OK)
		wl_printlasterror(NULL);

	return 0;
}

