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

#include <linux/export.h>
#include <linux/module.h>
#include <linux/string.h>

#define RINA_PREFIX "pff-avl"

#include "pff-ps.h"
#include "avl.h"


struct pff_avl_ps_priv {
		spinlock_t      lock;
		spinlock_t      lockPorts;
        avl_n 			tableroot;
        t_portNode		downPorts;
};
typedef struct pff_avl_ps_priv* t_priv;

struct pff_avl_portNode {
	port_id_t id;
	struct pff_avl_portNode * next;
};

struct pff_avl_qosNode {
	qos_id_t id;
	struct pff_avl_portNode * ports;
	struct pff_avl_qosNode * next;
};

struct pff_avl_entry_container {
    spinlock_t lock;
	qos_id_t id;
	struct avl_qosNode * QoSs;
};

typedef struct pff_avl_portNode* t_portNode;
typedef struct pff_avl_qosNode* t_qosNode;
typedef struct pff_avl_entry_container* t_entry;


int pff_avl_add (
		struct pff_ps * ps,
		struct mod_pff_entry * entry);

int pff_avl_remove (
		struct pff_ps * ps,
		struct mod_pff_entry * entry);

int pff_avl_port_state_change (
		struct pff_ps * ps,
		port_id_t port_id, bool up);

bool pff_avl_is_empty (
		struct pff_ps * ps);

int  pff_avl_flush(
		struct pff_ps * ps);

int  pff_avl_nhop (
		struct pff_ps * ps,
		struct pci *    pci,
		port_id_t **    ports,
		size_t *        count);

int  pff_avl_dump (
		struct pff_ps *    ps,
		struct list_head * entries);


static int pff_avl_ps_set_policy_set_param(
		struct ps_base * bps,
		const char *     name,
		const char *     value);

static struct ps_base * pff_avl_ps_default_create(
		struct rina_component * component);

static void pff_avl_ps_default_destroy(
		struct ps_base * bps);

struct ps_factory pff_factory = {
        .owner   = THIS_MODULE,
        .create  = pff_avl_ps_default_create,
        .destroy = pff_avl_ps_default_destroy,
};




