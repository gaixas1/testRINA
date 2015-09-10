/*
 * DeltaQ RMT PS
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
#include <linux/ktime.h>

#include "ps.h";

#define RINA_PREFIX "rmt-deltaq"

#include "logs.h"
#include "rds/rmem.h"

t_qosvals_n getPortQos(
		rmtdata_p ps,
		TPortId port_id,
		TQoSId qos_id,
		t_portvals_n * ret_port) {

	for (t_portvals_n it_port = ps->portvals_begin;
			it_port != NULL;
			it_port = it_port->next) {
		if (it_port->key == port_id) {
			if (ret_port != NULL) {
				*ret_port = it_port;
			}
			for(t_qosvals_n it_qos = it_port->qosvals_begin;
					it_qos != NULL;
					it_qos = it_qos->next ) {
				if(it_qos->key == qos_id) {
					return it_qos;
				}
			}
			return NULL;
		}
	}
	return NULL;
}

t_portvals_n getPort(
		rmtdata_p ps,
		TPortId port_id) {

	for (t_portvals_n it_port = ps->portvals_begin;
			it_port != NULL;
			it_port = it_port->next) {
		if (it_port->key == port_id) {
			return it_port;
		}
	}
	return NULL;
}

t_sizetime_n getSizeTimeNode(
		rmtdata_p ps,
		TSyze size, TTime time,
		t_sizetime_n next) {

	t_sizetime_n ret = NULL;
	if (ps->bufferST_begin == NULL) {
		ret = (t_sizetime_n) kmalloc(sizeof(*t_sizetime_n), GFP_ATOMIC);
	} else {
		ret = ps->bufferST_begin;
		ps->bufferST_begin = ret->next;
	}

	ret->next = next;
	ret->size = size;
	ret->time = time;
	return ret;
}

void releaseSizeTimeNode(
		rmtdata_p ps,
		t_sizetime_n node) {

	node->next = ps->bufferST_begin;
	ps->bufferST_begin = node;
}

t_time_n getTimeNode(
		rmtdata_p ps,
		TTime time,
		t_time_n next) {

	t_time_n ret = NULL;
	if (ps->bufferT_begin == NULL) {
		ret = (t_time_n) kmalloc(sizeof(*t_time_n), GFP_ATOMIC);
	} else {
		ret = ps->bufferT_begin;
		ps->bufferT_begin = ret->next;
	}

	ret->next = next;
	ret->time = time;
	return ret;
}

void releaseTimeNode(
		rmtdata_p ps,
		t_time_n node) {

	node->next = ps->bufferT_begin;
	ps->bufferT_begin = node;
}

t_pdu_n getPDUNode(
		rmtdata_p ps,
		struct pdu * p,
		t_pdu_n next) {

	t_pdu_n ret = NULL;
	if (ps->bufferPDU_begin == NULL) {
		ret = (t_pdu_n) kmalloc(sizeof(*t_pdu_n), GFP_ATOMIC);
	} else {
		ret = ps->bufferPDU_begin;
		ps->bufferPDU_begin = ret->next;
	}

	ret->next = next;
	ret->p = p;
	return ret;
}

void releasePDUNode(
		rmtdata_p ps,
		t_pdu_n node) {

	node->next = ps->bufferPDU_begin;
	ps->bufferPDU_begin = node;
}

/*
 * Limiter In::
 * - DECIDES IF DROP given # of bits received in burst time
 * - IF not DROP, RECORDS size/time of incoming PDUs
 */
/* Not used, in-drop at EFCP
/*
int limiter_rx(
		struct rmt_ps * _ps,
		struct rmt_n1_port * _port,
		struct pdu * p) {

	printk("%s: called()\n", __func__);

	rmtdata_p ps = container_of(_ps, struct int_rmt_data, base);

	TQoSId qos_id = p->pci->connection_id.qos_id;
	TPortId port_id = _port->port_id;

	t_qosvals_n qosVals = getPortQos(ps, port_id, qos_id,
			NULL);

	if (qosVals == NULL) {
		LOG_WARN("QoS not initialicied at given input port structure.");
		return -1;
	}

	if (!qosVals->params->limit_in) {
		return 0;
	}

	TTime time = ktime_get();
	TTime mtime = time - qosVals->params->iBurstT;

	t_sizetime_n it_stime = qosVals->recivedST_begin;

	while (it_stime != NULL && it_stime->time < mtime) {
		qosVals->received_size -= it_stime->size;
		qosVals->recivedST_begin = it_stime->next;
		releaseSizeTimeNode(ps, it_stime);
		it_stime = qosVals->recivedST_begin;
	}
	if (it_stime == NULL) {
		qosVals->recivedST_end = NULL;
	}

	TSyze pdu_size = p->buffer->size;

	TSyze next_size = qosVals->received_size + pdu_size;
	int dropProb = 0;
	for (t_sizeprob_n it_dprob =
			qosVals->params->iBurstSizeDrop_begin;
			it_dprob != NULL && it_dprob->size <= next_size; it_dprob =
					it_dprob->next) {
		dropProb = it_dprob->prob;
	}

	bool drop = false;
	if (dropProb >= 10000) {
		drop = true;
	} else if (dropProb > 0) {
		int r;
		get_random_bytes(&r, sizeof(r));
		r %= 10000;
		drop = r < dropProb;
	}

	if (drop) {
		pdu_destroy(p);
		return 1;
	}

	qosVals->received_size += pdu_size;

	if (qosVals->recivedST_begin == NULL) {
		qosVals->recivedST_begin = getSizeTimeNode(ps,
				pdu_size, time, NULL);
		qosVals->recivedST_end =
				qosVals->recivedST_begin;
	} else {
		qosVals->recivedST_end->next = getSizeTimeNode(ps,
				pdu_size, time, NULL);
		qosVals->recivedST_end =
				qosVals->recivedST_end->next;
	}

	return 0;
}
*/

/*
 * Enqueue OUT::
 * - DROP given # of pdus waiting on port
 * - IF not DROP, store pdu and current time
 */
int enqueue_tx(
		struct rmt_ps * _ps,
		struct rmt_n1_port * _port,
		struct pdu * p) {

	printk("%s: called()\n", __func__);
	rmtdata_p ps = container_of(_ps, struct int_rmt_data, base);

	TQoSId qos_id = p->pci->connection_id.qos_id;
	TPortId port_id = _port->port_id;

	t_portvals_n portVals = NULL;
	t_qosvals_n qosVals = getPortQos(ps, port_id, qos_id,
			&portVals);

	if (portVals == NULL || qosVals == NULL) {
		LOG_WARN("Port or QoS not initialicied.");
		return -1;
	}

	int dropProb = 0;
	for (t_sizeprob_n it_dprob = qosVals->params->oDropProb_begin;
			it_dprob != NULL && portVals->count <= qosVals->send_size;
			it_dprob = it_dprob->next) {
		dropProb = it_dprob->prob;
	}

	bool drop = false;
	if (dropProb >= 10000) {
		drop = true;
	} else if (dropProb > 0) {
		int r;
		get_random_bytes(&r, sizeof(r));
		r %= 10000;
		drop = r < dropProb;
	}

	if (drop) {
		pdu_destroy(p);
		return 1;
	}

	portVals->count += 1;
	if (qosVals->queue_begin == NULL) {
		qosVals->queue_begin = getPDUNode(ps, p, NULL);
		qosVals->queue_end = qosVals->queue_begin;
	} else {
		qosVals->queue_end->next = getPDUNode(ps, p, NULL);
		qosVals->queue_end = qosVals->queue_end->next;
	}

	TTime time = ktime_get();
	if (qosVals->wtime_begin == NULL) {
		qosVals->wtime_begin = getTimeNode(ps, time, NULL);
		qosVals->wtime_end = qosVals->wtime_begin;
	} else {
		qosVals->wtime_end->next = getTimeNode(ps, time, NULL);
		qosVals->wtime_end = qosVals->wtime_end->next;
	}

	return 0;
}

/*
 * Dequeue OUT::
 * - GET PDU with less (accepted-real) hop delay
 * - CAN decide to not return if limited output
 */
struct pdu * dequeue_tx(
		struct rmt_ps * _ps,
		struct rmt_n1_port * _port) {

	printk("%s: called()\n", __func__);
	rmtdata_p ps =
			container_of(_ps, struct int_rmt_data, base);

	TPortId port_id = _port->port_id;

	t_portvals_n portVals = getPort(ps, port_id);

	if (portVals == NULL) {
		LOG_WARN("Port not initialicied for given input port structure.");
		return -1;
	}

	TTime time = ktime_get();
	TTime min_hop_delay = KTIME_MAX;
	t_qosvals_n ret_qos = NULL;

	for (t_qosvals_n it_qos = portVals->qosvals_begin;
			it_qos != NULL;
			it_qos = it_qos->next) {
		if (it_qos->queue_begin == NULL) {
			continue;
		}

		if (it_qos->params->limit_out) {
			TTime mtime = time - it_qos->params->oBurstT;

			t_sizetime_n it_stime =
					it_qos->sendST_begin;

			while (it_stime != NULL && it_stime->time < mtime) {
				it_qos->send_size -= it_stime->size;
				it_qos->sendST_begin = it_stime->next;
				releaseSizeTimeNode(ps, it_stime);
				it_stime = it_qos->sendST_begin;
			}
			if (it_stime == NULL) {
				it_qos->sendST_end = NULL;
			}

			if (it_qos->send_size + it_qos->queue_begin->p->buffer->size
					> it_qos->params->oBurstS) {
				continue;
			}
		}

		TTime t_hop_delay = it_qos->params->oExWaitT - time + it_qos->wtime_begin->time;
		if (t_hop_delay < 0) {
			t_hop_delay *= it_qos->params->oMulOver;
		} else {
			t_hop_delay *= 100;
		}
		if (t_hop_delay < min_hop_delay) {
			min_hop_delay = t_hop_delay;
			ret_qos = it_qos;
		}
	}

	if (ret_qos == NULL) {
		return NULL;
	}

	t_pdu_n t_pnode = ret_qos->queue_begin;
	ret_qos->queue_begin = t_pnode->next;
	if (ret_qos->queue_begin == NULL) {
		ret_qos->queue_end = NULL;
	}

	struct pdu * ret = t_pnode->p;
	releasePDUNode(ps, t_pnode);

	ret_qos->lastST = ret_qos->wtime_begin->time;
	t_time_n t_tnode = ret_qos->wtime_begin;
	ret_qos->wtime_begin = t_tnode->next;
	if (ret_qos->wtime_begin == NULL) {
		ret_qos->wtime_end = NULL;
	}
	releaseTimeNode(ps, t_tnode);

	if (ret_qos->params->limit_out) {
		if (ret_qos->sendST_begin == NULL) {
			ret_qos->sendST_begin = getSizeTimeNode(ps, ret->buffer->size, NULL);
			ret_qos->sendST_end = ret_qos->sendST_begin;
		} else {
			ret_qos->sendST_end->next = getSizeTimeNode(ps, ret->buffer->size, NULL);
			ret_qos->sendST_end =
					ret_qos->sendST_end->next;
		}
		ret_qos->send_size += ret->buffer->size;
	}

	portVals->count -= 1;

	return ret;
}

/*
 * Requeue OUT::
 * - RESTORE LAST PDU
 */
int requeue_tx(
		struct rmt_ps * _ps,
		struct rmt_n1_port * port,
		struct pdu * p) {

	printk("%s: called()\n", __func__);
	rmtdata_p ps = container_of(_ps, struct int_rmt_data, base);

	TQoSId qos_id = p->pci->connection_id.qos_id;
	TPortId port_id = port->port_id;

	t_portvals_n portVals = NULL;
	t_qosvals_n qosVals = getPortQos(ps, port_id, qos_id, &portVals);

	if (portVals == NULL || qosVals == NULL) {
		LOG_WARN("Port or QoS not initialicied.");
		return -1;
	}

	portVals->count += 1;
	qosVals->queue_begin = getPDUNode(ps, p, qosVals->queue_begin);
	if (qosVals->queue_end == NULL) {
		qosVals->queue_end = qosVals->queue_begin;
	}

	qosVals->wtime_begin = getTimeNode(ps, qosVals->lastST, qosVals->wtime_begin);
	if (qosVals->wtime_end == NULL) {
		qosVals->wtime_end = qosVals->wtime_begin;
	}

	if (qosVals->params->limit_out) {
		qosVals->send_size -= qosVals->sendST_begin->size;
		qosVals->sendST_begin = qosVals->sendST_begin->next;
		if (qosVals->sendST_begin == NULL) {
			qosVals->sendST_end = qosVals->sendST_begin;
		}
	}
	return  0;
}

/**
 * Policy
 **/

/*
 * SET param
 */
static int rmt_ps_set_policy_set_param(
		struct ps_base * bps,
		const char * name,
		const char * value) {

	struct rmt_ps * _ps =
			container_of(bps, struct rmt_ps, base);
	rmtdata_p ps =
			container_of(_ps, struct int_rmt_data, base);

	if (!name) {
		LOG_ERR("Null parameter name");
		return -1;
	}

	if (!value) {
		LOG_ERR("Null parameter value");
		return -1;
	}

	/* SET PORT <port, port_id>*/
	/*
	 * port = alloc new port node;
	 * port.key = port_id
	 * port.count = 0
	 * foreach QoS_Params
	 * * port.qos <- alloc new QoS<key, qos_params>
	 * insert port to ports list
	 */

	/* SET QoS <qos, {qos_id, expectedWtime, multiplier
	 *		  limitIn:(active, burstDuration, [(size, dropProb),..,
	 *			(size,dropProb)]),
	 *		  limitOut:(active, burstDuration, burstSize),
	 *		  cherishDrop:([(size, dropProb),..,(size,dropProb)])}
	 *****
	 * qos_param = alloc new qos param node;
	 * set qos_param vals;
	 * foreach Port
	 * * port.qos <- alloc new QoS<qos_id, qos_param>
	 * insert qos_param to qos params list
	 */

	LOG_ERR("No such parameter to set");

	return -1;
}

/*
 * CREATE
 */
static struct ps_base * rmt_ps_deltaq_create(
		struct rina_component * component) {

	struct rmt * rmt = rmt_from_component(component);
	rmtdata_p ps = rkzalloc(sizeof(*ps), GFP_KERNEL);

	if (!ps) {
		return NULL;
	}

	ps->base.base.set_policy_set_param = rmt_ps_set_policy_set_param;
	ps->base.dm = rmt;
	ps->base.priv = NULL;

	ps->base.rmt_q_monitor_policy_rx = NULL;
	ps->base.max_q_policy_rx = NULL;

	/* Not used, in-drop at EFCP
	/*
	ps->base.rmt_scheduling_policy_rx = limiter_rx;
	* instead
	*/
	ps->base.rmt_scheduling_policy_rx = NULL;

	ps->base.rmt_q_monitor_policy_tx_enq = NULL;
	ps->base.rmt_enqueue_scheduling_policy_tx = enqueue_tx;
	ps->base.max_q_policy_tx = NULL;

	ps->base.rmt_next_scheduled_policy_tx = dequeue_tx;
	ps->base.rmt_q_monitor_policy_tx_deq = NULL;
	ps->base.rmt_requeue_scheduling_policy_tx = requeue_tx;
	ps->base.rmt_next_scheduled_policy_allownull = true;

	ps->base.rmt_scheduling_create_policy_tx = rmt_ps_deltaq_create;
	ps->base.rmt_scheduling_destroy_policy_tx = rmt_ps_deltaq_destroy;


	ps->portvals_begin = NULL;
	ps->qosparams_begin = NULL;

	ps->bufferST_begin = NULL;
	ps->bufferT_begin = NULL;
	ps->bufferPDU_begin = NULL;

	return &ps->base.base;
}

/*
 * DESTROY
 */
static void rmt_ps_deltaq_destroy(
		struct ps_base * bps) {

	struct rmt_ps * _ps =
			container_of(bps, struct rmt_ps, base);
	rmtdata_p ps =
			container_of(_ps, struct int_rmt_data, base);

	/*Release buffers*/
	while (ps->bufferST_begin != NULL) {
		t_sizetime_n t = ps->bufferST_begin;
		ps->bufferST_begin = t->next;
		kfree(t);
	}

	while (ps->bufferT_begin != NULL) {
		t_time_n t = ps->bufferT_begin;
		ps->bufferT_begin = t->next;
		kfree(t);
	}

	while (ps->bufferPDU_begin != NULL) {
		t_pdu_n t = ps->bufferPDU_begin;
		ps->bufferPDU_begin = t->next;
		kfree(t);
	}

	/*Release ports data*/
	while (ps->portvals_begin != NULL) {
		t_portvals_n t_port = ps->portvals_begin;

		/*Release Port Qos values*/
		while (t_port->qosvals_begin != NULL) {
			t_qosvals_n t_qos = t_port->qosvals_begin;

			/*Release remaining PDUs. !!should be empty on destroy*/
			while (t_qos->queue_begin != NULL) {
				t_pdu_n t_node = t_qos->queue_begin;
				t_qos->queue_begin = t_node->next;
				kfree(t_node->p);/* Release also pdu data? ask*/
				kfree(t_node);
			}

			/*Release remaining PDUs in time. !!should be empty on destroy*/
			while (t_qos->wtime_begin != NULL) {
				t_time_n t_node = t_qos->wtime_begin;
				t_qos->wtime_begin = t_node->next;
				kfree(t_node);
			}

			/*Release received size/time log for inputLimit*/

			/* Not used, in-drop at EFCP
			/*
			while (t_qos->recivedST_begin != NULL) {
				t_sizetime_n t_node =
						t_qos->recivedST_begin;
				t_qos->recivedST_begin = t_node->next;
				kfree(t_node);
			}
			*/

			/*Release send size/time log for outputLimit*/
			while (t_qos->sendST_begin != NULL) {
				t_sizetime_n t_node = t_qos->sendST_begin;
				t_qos->sendST_begin = t_node->next;
				kfree(t_node);
			}

			t_port->qosvals_begin = t_qos->next;
			kfree(t_qos);
		}

		ps->portvals_begin = t_port->next;
		kfree(t_port);
	}

	if (bps) {
		rkfree(ps);
	}
}

/*
 * FACTORY DATA
 */
struct ps_factory rmt_factory = { .owner = THIS_MODULE, .create =
		rmt_ps_deltaq_create, .destroy = rmt_ps_deltaq_destroy, };
