/*
 * AVL PFF PS
 *
 *	Sergio Leon <slgaixas@upc.edu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "ps.h"

int pff_avl_add (
		struct pff_ps * ps,
		struct mod_pff_entry * entry) {

	t_priv priv = (t_priv) ps->priv;
	if (!priv) { return -1; }

	/* read lock priv */
	read_lock(&priv->lock);

	t_entry  n = avl_search(priv->tableroot, (avl_kt) entry->fwd_info);

	/* unlock read priv */
	read_unlock(&priv->lock);

	if(!n) {
		n = getNewEntry();
		if(!n) {return -1; }

		/* insert lock priv */
		write_lock(&priv->lock);

		priv->tableroot = avl_insert(priv->tableroot, (avl_kt) entry->fwd_info, n);

		/* unlock insert priv*/
		write_unlock(&priv->lock);
	}

	/* insert lock entry */
	write_lock(&n->lock);

	t_qosNode qos;
	for(qos = n->QoSs; qos; qos = qos->next) {
		if(qos->id == entry->qos_id) { break; }
	}
	if(!qos) {
		qos = getNewQoSNode(entry->qos_id);
		if(!qos) {return -1; }
		qos->next = n->QoSs;
		n->QoSs = qos;
	}

	/* foreach portid : entry->port_id_altlists */
	{
		port_id_t portid;
		t_portNode port;
		for(port = qos->ports; port; port = port->next) {
			if(port->id == portid) { break; }
		}
		if(!port) {
			port = getNewPortNode(portid);
			if(!port) {return -1; }
			port->next = qos->ports;
			qos->ports = port;
		}
	}
	/* unlock insert entry */
	write_unlock(&n->lock);

	return 0;
}

int pff_avl_remove (
		struct pff_ps * ps,
		struct mod_pff_entry * entry) {

	t_priv priv = (t_priv) ps->priv;
	if (!priv) { return -1; }

	/* read lock priv */
	read_lock(&priv->lock);

	t_entry  n = avl_search(priv->tableroot, (avl_kt) entry->fwd_info);

	/* unlock read priv */
	read_unlock(&priv->lock);

	if(!n) { return 0; }

	/* insert lock entry */
	write_lock(&n->lock);

	t_qosNode qos;
	for(qos = n->QoSs; qos; qos = qos->next) {
		if(qos->id == entry->qos_id) { break; }
	}
	if(!qos) { return 0; }


	/* foreach portid : entry->port_id_altlists */
	{
		port_id_t portid;
		t_portNode port = qos->ports;
		t_portNode prev = NULL;

		if(!port) { break; }
		while(port) {
			if(port->id == portid) {
				if(prev) { prev->next = port->next; }
				else { qos->ports = port->next; }
				freePortNode(port);
				port = NULL;
			} else {
				prev = port;
				port = port->next;
			}
		}
	}

	if(!qos->ports) {
		if(n->QoSs == qos) {
			n->QoSs = qos->next;
		} else {
			for(t_qosNode sqos = n->QoSs; sqos; sqos = sqos->next) {
				if(sqos->next == qos) {
					sqos->next = qos->next;
					break;
				}
			}
		}
		freeQoSNode(qos);
	}

	/* unlock insert entry */
	write_unlock(&n->lock);

	if(!n->QoSs) {
		/* insert lock priv */
		write_unlock(&priv->lock);

		priv->tableroot = avl_remove(priv->tableroot, (avl_kt) entry->fwd_info);

		/* unlock insert priv*/
		write_unlock(&priv->lock);

		freeEntry(n);
	}

	return 0;
}

int pff_avl_port_state_change (
		struct pff_ps * ps,
		port_id_t port_id, bool up) {

	t_priv priv = (t_priv) ps->priv;
	if (!priv) { return -1; }

	/* write lock privports */
	write_lock(&priv->lockPorts);

	for(t_portNode sport = priv->downPorts; sport; sport = sport->next) {
		if(sport->id == port_id) { port = sport; break; }
	}

	if(up) {
		t_portNode prev = NULL;
		t_portNode port = priv->downPorts;
		while(port) {
			if(port->id == port_id) {
				if(prev) { prev->next = port->next; }
				else { priv->downPorts = port->next; }
				freePortNode(port);
				port = NULL;
			} else {
				prev = port;
				port = port->next;
			}
		}

	} else {
		t_portNode port;
		for(t_portNode sport = priv->downPorts; sport; sport = sport->next) {
			if(sport->id == port_id) { port = sport; break; }
		}
		if(!port) {
			port = getNewPortNode(port_id);
			if(!port) {return -1; }
			port->next = priv->downPorts;
			priv->downPorts = port->next;
		}
	}
	/* unlock write privports */
	write_unlock(&priv->lockPorts);

	return 0;
}

bool pff_avl_is_empty (
		struct pff_ps * ps) {

	t_priv priv = (t_priv) ps->priv;
	if (!priv) { return -1; }
	/* read lock priv */
	read_lock(&priv->lock);

	bool ret = (priv->tableroot != NULL);

	/* unlock read priv */
	read_unlock(&priv->lock);

	return ret;
}

int  pff_avl_flush(
		struct pff_ps * ps) {
	/* Empty table?? */
	return 0;
}

int  pff_avl_nhop (
		struct pff_ps * ps,
		struct pci *    pci,
		port_id_t **    ports,
		size_t *        count) {

	t_priv priv = (t_priv) ps->priv;
	if (!priv) { return -1; }

	/* read lock priv */
	read_lock(&priv->lock);

	t_entry  n = avl_search(priv->tableroot, (avl_kt) pci->destination);

	if(n) {
		/* read lock entry */
		read_lock(&n->lock);
	}
	/*** check if lock priv -> lock entry -> unlock priv -> unlock entry
	 *   solves problem read entry -> !!entry removed -> lock entry
	 *   or creates deadlock
	 ***/

	/* unlock read priv */
	read_unlock(&priv->lock);

	if(n) {

		t_qosNode qos;
		for(qos = n->QoSs; qos; qos = qos->next) {
			if(qos->id == pci->connection_id->qos_id) { break; }
		}
		if(qos) {
			for(t_portNode port = qos->ports; port; port = port->next) {
				/* read lock privports */
				read_lock(&priv->lockPorts);

				bool down = false;
				for(t_portNode dport = priv->downPorts; dport; dport = dport->next) {
					if(dport == port) {
						down = true;
						break;
					}
				}
				if(!down) {
					/* Insert port into results */
				}

				/* unlock read privports */
				read_unlock(&priv->lockPorts);

			}
		}
		/* unlock read entry */
		read_unlock(&n->lock);
	}

	return 0;
}

int  pff_avl_dump (
		struct pff_ps *    ps,
		struct list_head * entries) {
	/* WTD??*/
	return 0;
}

static struct ps_base * pff_avl_ps_default_create(
		struct rina_component * component) {

        struct pff_ps * ps;
        struct pff_avl_ps_priv * priv;
        struct pff * pff = pff_from_component(component);

        priv = rkzalloc(sizeof(*priv), GFP_KERNEL);
        if (!priv) {
                return NULL;
        }

        rwlock_init(&priv->lock);
        priv->tableroot = NULL;

        ps->base.set_policy_set_param = pff_avl_ps_set_policy_set_param;
        ps->dm = pff;
        ps->priv = (void *) priv;

        ps->pff_add = pff_avl_add;
        ps->pff_remove = pff_avl_remove;
        ps->pff_port_state_change = NULL;
        ps->pff_is_empty = pff_avl_is_empty;
        ps->pff_flush = pff_avl_flush;
        ps->pff_nhop = pff_avl_nhop;
        ps->pff_dump = pff_avl_dump;

        return &ps->base;
}

static void pff_avl_ps_default_destroy(struct ps_base * bps) {
        struct pff_ps * ps = container_of(bps, struct pff_ps, base);

        if (bps) {
                struct pff_ps_priv * priv;

                priv = (struct pff_ps_priv *) ps->priv;
                if(!priv_is_ok(priv)) {
                        return;
                }

                write_lock(&priv->lock);

                	/*Do deletion*/

                write_unlock(&priv->lock);

                rkfree(priv);
                rkfree(ps);
        }
}


t_entry getNewEntry() {
	t_entry ret = (t_entry) kmalloc(sizeof(*t_entry), GFP_KERNEL);
    rwlock_init(&ret->lock);
	ret->QoSs = NULL;
	return ret;
}

t_qosNode getNewQoSNode(qos_id_t id) {
	t_qosNode ret = (t_qosNode) kmalloc(sizeof(*t_qosNode), GFP_KERNEL);
	ret->id = id;
	ret->next = NULL;
	ret->ports = NULL;
	return ret;
}

t_portNode getNewPortNode(t_portNode id) {
	t_portNode ret = (t_portNode) kmalloc(sizeof(*t_portNode), GFP_KERNEL);
	ret->id = id;
	ret->next = NULL;
	return ret;
}

void freeEntry(t_entry o) {
	kfree(o);
}

void freeQoSNode(t_qosNode o) {
	kfree(o);
}

void freePortNode(t_qosNode o) {
	kfree(o);
}

