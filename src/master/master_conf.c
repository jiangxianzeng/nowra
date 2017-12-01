/*
 *
 * */

/* System libraries. */

#include <unistd.h>
#include <string.h>

/* Utility library. */

#include "msg.h"
#include "argv.h"

/* Application-specific. */

#include "master.h"

#define STR_DIFF    strcmp
#define STR_SAME    !strcmp
#define SWAP(type,a,b)  { type temp = a; a = b; b = temp; }

/* master_refresh - re-read configuration table */

void master_refresh(void)
{
	MASTER_SERV *serv;
	MASTER_SERV **servp;

	/*
	 * Mark all existing services.
	 */

	for (serv = master_head; serv != 0; serv = serv->next)
		serv->flags |= MASTER_FLAG_MARK;

	/*
	 * Read the master.cf configuration file. The master_conf() routine
	 * unmarks services upon update. New services are born with the mark bit
	 * off. After this, anything with the mark bit on should be removed.
	 */
	master_config();

	/*
	 * Delete all services that are still marked - they disappeared from the
	 * configuration file and are therefore no longer needed.
	 */
	for (servp = &master_head; (serv = *servp) != 0; /* void */ ) 
	{
		if ((serv->flags & MASTER_FLAG_MARK) != 0) 
		{
			*servp = serv->next;
			master_stop_service(serv);
			free_master_ent(serv);
		} 
		else 
		{
			servp = &serv->next;
		}
	}
}


/* master_config - read config file */

void master_config(void)
{
	MASTER_SERV *entry;
	MASTER_SERV *serv;

	/*
	 * A service is identified by its endpoint name AND by its transport
	 * type, not just by its name alone. The name is unique within its
	 * transport type. XXX Service privacy is encoded in the service name.
	 */
	set_master_ent();
	while ((entry = get_master_ent()) != 0) 
	{
		print_master_ent(entry);

		for (serv = master_head; serv != 0; serv = serv->next)
		{
			if (STR_SAME(serv->name, entry->name) && serv->type == entry->type)
			{
				break;
			}

		}

		/*
		 * Add a new service entry. We do not really care in what order the
		 * service entries are kept in memory.
		 */

		if (serv == 0) 
		{
			entry->next = master_head;
			master_head = entry;
			master_start_service(entry);
		}
		/*
		 * Update an existing service entry. Make the current generation of
		 * child processes commit suicide whenever it is convenient. The next
		 * generation of child processes will run with the new configuration
		 * settings.
		 */

		else 
		{
			serv->flags &= ~MASTER_FLAG_MARK;
			if (entry->flags & MASTER_FLAG_CONDWAKE)
				serv->flags |= MASTER_FLAG_CONDWAKE;
			else
				serv->flags &= ~MASTER_FLAG_CONDWAKE;
			serv->wakeup_time = entry->wakeup_time;
			serv->max_proc = entry->max_proc;
			serv->throttle_delay = entry->throttle_delay;
			SWAP(char *, serv->path, entry->path);
			SWAP(ARGV *, serv->args, entry->args);
			master_restart_service(serv);
			free_master_ent(entry);
		}
	}
	
	end_master_ent();
}	
