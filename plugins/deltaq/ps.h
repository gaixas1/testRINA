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

#ifndef H_PS
#define H_PS

#include "rmt-ps.h"

#define TTime ktime_t
#define TSyze u32
#define TQoSId qos_id_t
#define TPortId port_id_t

struct size_time_node {
	TSyze size;
	TTime time;
	struct size_time_node * next;
};
typedef struct size_time_node * t_sizetime_n;

struct time_node {
	TTime time;
	struct time_node * next;
};
typedef struct time_node * t_time_n;

struct size_prob_node {
	TSyze size;
	int prob;
	struct size_prob_node * next;
};
typedef struct size_prob_node * t_sizeprob_n;

struct pdu_node {
	struct pdu * p;
	struct pdu_node * next;
};
typedef struct pdu_node * t_pdu_n;

struct qos_params_node {
	TQoSId key;

	/* Not used, in-drop at EFCP
	bool limit_in;
	Burst duration
	TTime iBurstT;
	t_sizeprob_n iBurstSizeDrop_begin;
	*/

	bool limit_out;
	TTime oBurstT;
	TSyze oBurstS;


	t_sizeprob_n oDropProb_begin;
	TTime oExWaitT;
	int oMulOver;
		/*Default 100*/

	struct qos_params_node * next;
};
typedef struct qos_params_node * t_qos_n;

struct qos_vals_node {
	TQoSId		  key;

	t_qos_n params;

	t_pdu_n queue_begin;
	t_pdu_n queue_end;

	/* Not used, in-drop at EFCP
	/*
	TSyze received_size;
	t_sizetime_n recivedST_begin;
	t_sizetime_n recivedST_end;
	*/

	TSyze send_size;
	t_sizetime_n sendST_begin;
	t_sizetime_n sendST_end;
	TTime lastST;

	t_time_n wtime_begin;
	t_time_n wtime_end;

	struct qos_vals_node * next;
};
typedef struct qos_vals_node * t_qosvals_n;

struct port_portvals_node {
	TPortId key;

	int count;

	t_qosvals_n qosvals_begin;

	struct port_portvals_node * next;
};
typedef struct port_portvals_node * t_portvals_n;

struct int_rmt_data {
	struct rmt_ps base;

	t_portvals_n portvals_begin;
	t_qos_n qosparams_begin;

	t_sizetime_n bufferST_begin;
	t_time_n bufferT_begin;
	t_pdu_n bufferPDU_begin;
};
typedef struct int_rmt_data * rmtdata_p;



t_qosvals_n getPortQos(
		rmtdata_p ps,
		TPortId port_id,
		TQoSId qos_id,
		t_portvals_n * ret_port);

t_portvals_n getPort(
		rmtdata_p ps,
		TPortId port_id) ;

t_sizetime_n getSizeTimeNode(
		rmtdata_p ps,
		TSyze size, TTime time,
		t_sizetime_n next);
void releaseSizeTimeNode (
		rmtdata_p ps,
		t_sizetime_n node);

t_time_n getTimeNode(
		rmtdata_p ps,
		TTime time,
		t_time_n next);
void releaseTimeNode(
		rmtdata_p ps,
		t_time_n node);

t_pdu_n getPDUNode(
		rmtdata_p ps,
		struct pdu * p,
		t_pdu_n next);
void releasePDUNode(
		rmtdata_p ps,
		t_pdu_n node);

/* Not used, in-drop at EFCP
/*
int limiter_rx(
		struct rmt_ps * _ps,
		struct rmt_n1_port * _port,
		struct pdu * p);
*/

int enqueue_tx(
		struct rmt_ps * _ps,
		struct rmt_n1_port * _port,
		struct pdu * p) ;

struct pdu * dequeue_tx(
		struct rmt_ps * _ps,
		struct rmt_n1_port * _port);

int requeue_tx(
		struct rmt_ps * _ps,
		struct rmt_n1_port * port,
		struct pdu * p);



static int rmt_ps_set_policy_set_param(
		struct ps_base * bps,
		const char * name,
		const char * value);


static struct ps_base * rmt_ps_deltaq_create(
		struct rina_component * component);


static void rmt_ps_deltaq_destroy(
		struct ps_base * bps);
#endif
