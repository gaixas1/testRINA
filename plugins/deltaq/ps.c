/*
 * DeltaQ RMT PS
 *
 *    Sergio Leon <slgaixas@upc.edu>
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

#define RINA_PREFIX "rmt-deltaq"

#include "logs.h"
#include "rds/rmem.h"
#include "rmt-ps.h"

#define TTime ktime
#define TSyze u32
#define TQoSId qos_id_t
#define TPortId port_id_t

struct size_time_node {
    TSyze               size;
    TTime               time;
    struct size_time_node    * next;
};

struct time_node {
    TTime               time;
    struct time_node         * next;
};

struct size_prob_node {
    TSyze               size;
    int                 prob; // 0 = 0% prob, 10.000 = 100,00% prob
    struct size_prob_node    * next;
};

struct pdu_node {
    pdu              * p;
    struct pdu_node         * next;
};

struct qos_params_node {
    //QoS ID
    TQoSId                  key;
    
    //QoS Limits Input?
    bool                    limit_in;
    //Burst duration
    TTime                   in_burst_time;
    //Probability of drop if burst size surpassed
    struct size_prob_node        * in_burst_size_drop_prob_begin;
            
    //QoS Limits Output?
    bool                    limit_out;
    //Burst duration
    TTime                   out_burst_time;
    //Wait if burst size surpassed on burst
    TSyze                   out_burst_size;
    
    //Probability of cherish drop
    struct size_prob_node        * out_drop_prob_begin;
    
    //Expected max time waiting on queue
    TTime                   out_expected_waiting_time;
    //Multiplier when real waiting time surpases max expected
    double                  out_multiplier_over_expected_waiting_time;
    
    //Next QoS
    struct qos_params_node       * next;
}

struct qos_vals_node {
    //QoS ID
    TQoSId                  key;   
    //Params for QoS
    struct qos_params_node       * params;

    //Out queue for QoS
    struct pdu_node              * queue_begin;
    struct pdu_node              * queue_end;
    
    //In case of params->limit_in
    TSyze                   received_size;
    struct size_time_node        * received_size_time_begin;
    struct size_time_node        * received_size_time_end;
    
    //In case of params->limit_out
    TSyze                   send_size;
    struct size_time_node        * send_size_time_begin;
    struct size_time_node        * send_size_time_end;
    TTime                   last_stime;
    
    //Store arriving time
    struct time_node             * wtime_begin;
    struct time_node             * wtime_end;
    
    //Next QoS
    struct qos_vals_node         * next;
};

struct port_vals_node {
    //Port ID
    TPortId             key;
    
    //PDUs waiting on output queues
    int count;
    
    //Next QoS
    struct qos_vals_node     * qos_vals_begin;
    
    //Next Port
    struct port_vals_node    * next;
};

struct int_rmt_data {
    struct rmt_ps       base;
    
    struct port_vals_node    * port_vals_begin;
    struct qos_params_node   * qos_params_begin;
    
    struct size_time_node    * buffer_size_time_begin;
    struct time_node         * buffer_time_begin;
    struct pdu_node         * buffer_pdu_begin;
};

struct qos_vals_node * getPortQos(struct int_rmt_data * ps, TPortId port_id, TQoSId qos_id, struct port_vals_node** ret_port) {
    for(struct port_vals_node * it_port = ps->port_vals_begin; it_port!= NULL; it_port = it_port->next) {
        if(it_port->key == port_id) {
            if(ret_port != NULL) { *ret_port = it_port; }
            for(struct qos_vals_node * it_qos it_qos = it_port->qos_vals_begin; it_qos!= NULL; it_qos = it_qos->next) {
                if(it_qos->key == qos_id) {
                    return it_qos;
                }
            }
            return NULL;
        }
    }
}

struct port_vals_node * getPort(struct int_rmt_data * ps, TPortId port_id) {
    for(port_vals_node * it_port = ps->port_vals_begin; it_port!= NULL; it_port = it_port->next) {
        if(it_port->key == port_id) {
            return it_port;
        }
    }
    return NULL;
}

struct size_time_node * getSizeTimeNode(struct int_rmt_data * ps, TSyze size, TTime time, struct size_time_node * next) {
    struct size_time_node * ret = NULL;
    if(ps->buffer_size_time_begin == NULL) {
        ret = (struct size_time_node *) kmalloc(sizeof(struct size_time_node), GFP_ATOMIC);
    } else {
        ret = ps->buffer_size_time_begin;
        ps->buffer_size_time_begin = ret->next;
    }
    
    ret->next = next;
    ret->size = size;
    ret->time = time;
    return ret;
}

void releaseSizeTimeNode(struct int_rmt_data * ps, struct size_time_node * node) {
    node->next = ps->buffer_size_time_begin;
    ps->buffer_size_time_begin = node;
}

struct time_node * getTimeNode(struct int_rmt_data * ps, TTime time, struct time_node * next) {
    struct time_node * ret = NULL;
    if(ps->buffer_time_begin == NULL) {
        ret = (struct time_node *) kmalloc(sizeof(struct time_node), GFP_ATOMIC);
    } else {
        ret = ps->buffer_time_begin;
        ps->buffer_time_begin = ret->next;
    }
    
    ret->next = next;
    ret->time = time;
    return ret;
}

void releaseTimeNode(struct int_rmt_data * ps, struct time_node * node) {
    node->next = ps->buffer_time_begin;
    ps->buffer_time_begin = node;
}

struct pdu_node * getPDUNode(struct int_rmt_data * ps, pdu * p, struct pdu_node * next) {
    struct pdu_node * ret = NULL;
    if(ps->buffer_pdu_begin == NULL) {
        ret = (struct pdu_node *) kmalloc(sizeof(struct pdu_node), GFP_ATOMIC);
    } else {
        ret = ps->buffer_pdu_begin;
        ps->buffer_pdu_begin = ret->next;
    }
    
    ret->next = next;
    ret->p = p;
    return ret;
}

void releasePDUNode(struct int_rmt_data * ps, struct pdu_node * node) {
    node->next = ps->buffer_pdu_begin;
    ps->buffer_pdu_begin = node;
}

/*
 * Limiter In::
 * - DECIDES IF DROP given # of bits received in burst time
 * - IF not DROP, RECORDS size/time of incoming PDUs
 * ::
 * IF NOT PORT-QOS-limitIn : ACCEPT, RETURN 0
 * GET time
 * CLEAN QoS_PORT-received_size_time / UPDATE QoS_PORT-received_size
 * GET QoS-[received_size+PDU.size<=SIZE]-limit_drop_prob
 * IF limit_drop_prob >= 1 DROP, RETURN -1; if limit_drop_prob <= 0 ACCEPT; (random < limit_drop_prob)? ACCEPT : DROP, RETURN -1;
 * INSERT {time, PDU.size} INTO QoS_PORT-received_size_time
 * ADDS PDU.size TO QoS_PORT-received_size
 */
int imp_rmt_limiter_rx(struct rmt_ps * _ps, struct rmt_n1_port * _port, struct pdu * p) {
    printk("%s: called()\n", __func__);
    
    struct int_rmt_data * ps = container_of(_ps, struct int_rmt_data, base);
    
    TQoSId qos_id = p->pci->connection_id.qos_id;
    TPortId port_id = _port->port_id;
    
    struct qos_vals_node * qosVals = getPortQos(ps, port_id, qos_id, NULL);
    
    if(qosVals == NULL) {
        LOG_WARN("QoS not initialicied at given input port structure.");
        return -1;
    }
    
    //Check if current QoS limits input
    if(!qosVals->params->limit_in) { return 0; }
    
    TTime time =  ktime_get();
    TTime mtime =  time - qosVals->params->in_burst_time;
    
    //Clean old stored received time/size log
    struct size_time_node * it_stime = qosVals->received_size_time_begin;
    
    while(it_stime != NULL && it_stime->time < mtime) {
        qosVals->received_size -= it_stime->size;
        qosVals->received_size_time_begin = it_stime->next;
        releaseSizeTimeNode(ps, it_stime);
        it_stime = qosVals->received_size_time_begin;
    }
    if(it_stime == NULL) { qosVals->received_size_time_end = NULL; }
    
    TSyze pdu_size = p->buffer->size;
    
    //Get drop probability. [0-10.000]
    TSyze next_size = qosVals->received_size + pdu_size;
    int dropProb = 0;
    for(struct size_prob_node * it_dprob = qosVals->params->in_burst_size_drop_prob_begin; it_dprob != NULL && it_dprob->size <= next_size; it_dprob = it_dprob->next) {
        dropProb = it_dprob->prob;
    }
    
    //Decide if drop
    bool drop = false;
    if (dropProb >= 10000) { drop = true; }
    else if(dropProb > 0) {
        int r;
        get_random_bytes ( &r, sizeof (r) );
        r %= 10000;
        drop = r < dropProb;
    }
    
    if(drop) {
        //drop pdu here?
        return 1;
    }
    
    //Log current Pdu
    qosVals->received_size += pdu_size;
    
    if(qosVals->received_size_time_begin == NULL) {
        qosVals->received_size_time_begin = getSizeTimeNode(ps, pdu_size, time, NULL);
        qosVals->received_size_time_end = qosVals->received_size_time_begin;
    } else {
        qosVals->received_size_time_end->next = getSizeTimeNode(ps, pdu_size, time, NULL);
        qosVals->received_size_time_end = qosVals->received_size_time_end->next;
    }
    
    return 0;
}



/*
 * Enqueue OUT::
 * - DROP given # of bits received in burst time
 * - IF not DROP, store pdu and current time
 * ::
 * GET PORT-count
 * GET QoS-[count<=PCOUNT]-count_drop_prob
 * IF count_drop_prob >= 1 DROP, RETURN -1; if count_drop_prob <= 0 ACCEPT; (random < count_drop_prob)? ACCEPT : DROP, RETURN -1;
 * INCREMENT PORT-count
 * PORT-QOS-queue PUSH BACK pdu
 * GET time
 * PORT-QOS-wtime PUSH BACK stime
 */
int imp_rmt_enqueue_tx (struct rmt_ps * _ps, struct rmt_n1_port * _port, struct pdu * p) {
    printk("%s: called()\n", __func__);
    struct int_rmt_data * ps = container_of(_ps, struct int_rmt_data, base);
    
    TQoSId qos_id = p->pci->connection_id.qos_id;
    TPortId port_id = _port->port_id;
    
    struct port_vals_node * portVals = NULL;
    struct qos_vals_node * qosVals = getPortQos(ps, port_id, qos_id, &portVals);
    
    if(portVals == NULL || qosVals == NULL) {
        LOG_WARN("Port or QoS not initialicied for given input port structure.");
        return -1;
    }
    
    
    //Get drop probability. [0-10.000]
    int dropProb = 0;
    for(struct size_prob_node * it_dprob = qosVals->params->out_drop_prob_begin; it_dprob != NULL && portVals->count <= qosVals->received_size; it_dprob = it_dprob->next) {
        dropProb = it_dprob->prob;
    }
    
    //Decide if drop
    bool drop = false;
    if (dropProb >= 10000) { drop = true; }
    else if(dropProb > 0) {
        int r;
        get_random_bytes ( &r, sizeof (r) );
        r %= 10000;
        drop = r < dropProb;
    }
    
    if(drop) {
        //drop pdu here?
        return 1;
    }
    
    //PUSH PDU
    portVals->count += 1;
    if(qosVals->queue_begin == NULL) {
        qosVals->queue_begin = getPDUNode(ps, p, NULL);
        qosVals->queue_end = qosVals->queue_begin;
    } else {
        qosVals->queue_end->next = getPDUNode(ps, p, NULL);
        qosVals->queue_end = qosVals->queue_end->next;
    }
    
    //STORE time log
    TTime time =  ktime_get();
    if(qosVals->wtime_begin == NULL) {
        qosVals->wtime_begin = getTimeNode(ps, time, NULL);
        qosVals->wtime_end = qosVals->wtime_begin;
    } else {
        qosVals->wtime_end->next = getTimeNode(ps, time, NULL);
        qosVals->wtime_end = qosVals->wtime_end->next;
    }
}



/*
 * Dequeue OUT::
 * - GET PDU with less (accepted-real) hop delay
 * ::
 * SET min_hop_delay INF
 * SET ret_qos NULL
 * GET time
 * FOR q : QOS
 * * IF EMPTY PORT-QOS-queue : CONTINUE
 * * IF PORT-QOS-limitOut
 * * * CLEAN QoS_PORT-send_size_time / UPDATE QoS_PORT-send_size
 * * * ID QoS_PORT-send_size_time > QoS-accepted_send_size : CONTINUE
 * * GET FRONT PORT-QOS-wtime stime
 * * GET QOS-accepted_delay
 * * SET t AS accepted_delay - ( time - stime )
 * * IF t < min_hop_delay
 * * * min_hop_delay = t
 * * * ret_qos = q
 * IF IS NULL ret_qos : RETURN NULL
 * SET ret_qos-last_stime (FRONT ret_qos-wtime) 
 * POP FRONT ret_qos-wtime
 * GET FRONT ret_qos-queue pdu
 * POP FRONT ret_qos-queue;
 * IF ret_qos-limitOut
 * * INSERT {time, PDU.size} INTO ret_qos-send_size_time
 * * ADDS PDU.size TO ret_qos-send_size
 * RETURN pdu
 */
struct pdu * imp_rmt_dequeue_tx(struct rmt_ps * _ps , struct rmt_n1_port * _port) {
    printk("%s: called()\n", __func__);
    struct int_rmt_data * ps = container_of(_ps, struct int_rmt_data, base);
    
    TPortId port_id = _port->port_id;
    
    struct port_vals_node * portVals = getPort(ps, port_id);
    
    if(portVals == NULL) {
        LOG_WARN("Port not initialicied for given input port structure.");
        return -1;
    }
    
    TTime time =  ktime_get();
    TTime min_hop_delay = KTIME_MAX ;
    struct qos_vals_node * ret_qos = NULL;
    
    for(struct qos_vals_node * it_qos = portVals->qos_vals_begin; it_qos != NULL; it_qos = it_qos->next ) {
        if(it_qos->queue_begin == NULL) { continue; }
        
        if(it_qos->params->limit_out) {
            TTime mtime =  time - it_qos->params->out_burst_time;
    
            //Clean old stored send time/size log
            struct size_time_node * it_stime = it_qos->send_size_time_begin;

            while(it_stime != NULL && it_stime->time < mtime) {
                it_qos->send_size -= it_stime->size;
                it_qos->send_size_time_begin = it_stime->next;
                releaseSizeTimeNode(ps, it_stime);
                it_stime = it_qos->send_size_time_begin;
            }
            if(it_stime == NULL) { it_qos->send_size_time_end = NULL; }
            
            //Skip if next PDU exceeds burst size
            if(it_qos->send_size + it_qos->queue_begin->p->buffer->size > it_qos->params->out_burst_size ) { continue; }
        }
        
        //Compute t_hop_delay as (max expected + insert time - current time) [*multiplier <if negative>]
        TTime t_hop_delay = it_qos->params->out_expected_waiting_time - time + it_qos->wtime_begin->time ;
        if(t_hop_delay < 0) {
            t_hop_delay *= it_qos->params->out_multiplier_over_expected_waiting_time;
        }
        
        if(t_hop_delay < min_hop_delay) {
            min_hop_delay = t_hop_delay;
            ret_qos = it_qos;
        }
    }
    
    //If any PDU can be served, return null
    if(ret_qos == NULL) { return NULL; }
    
    //GET front pdu and release from queue
    struct pdu_node * t_pnode = ret_qos->queue_begin;
    ret_qos->queue_begin = t_pnode->next;
    if(ret_qos->queue_begin == NULL) { ret_qos->queue_end = NULL; }
    struct pdu * ret = t_pnode->p; 
    releasePDUNode(ps, t_pnode);
    
    //Store next serving pdu insert time as last served and remove from log
    ret_qos->last_stime = ret_qos->wtime_begin->time;
    struct time_node * t_tnode = ret_qos->wtime_begin;
    ret_qos->wtime_begin = t_tnode->next;
    if(ret_qos->wtime_begin == NULL) { ret_qos->wtime_end = NULL; }
    releaseTimeNode(ps, t_tnode);
    
    //If Qos limits output, log current pdu
    if(ret_qos->params->limit_out) {
        if(ret_qos->send_size_time_begin == NULL) {
            ret_qos->send_size_time_begin = getSizeTimeNode(ps, ret->buffer->size,NULL);
            ret_qos->send_size_time_end = ret_qos->send_size_time_begin;
        } else {
            ret_qos->send_size_time_end->next = getSizeTimeNode(ps, ret->buffer->size,NULL);
            ret_qos->send_size_time_end = ret_qos->send_size_time_end->next;
        }
        ret_qos->send_size += ret->buffer->size;
    }
    
    
    //Reduce port count
    portVals->count -= 1;
    
    return ret;
}

/*
 * Requeue OUT::
 * - RESTORE LAST PDU
 * ::
 * SET stime PDU.qos-last_stime 
 * PORT-QOS-queue PUSH FRONT pdu
 * PORT-QOS-wtime PUSH FRONT stime
 * IF PORT-QOS-limitOut
 * * POP_BACK FROM PORT-QOS-send_size_time
 * * SUBSTRACT PDU.size FROM PORT-QOS-send_size
 */
int imp_rmt_requeue_tx(struct rmt_ps * _ps, struct rmt_n1_port *, struct pdu *) {
    printk("%s: called()\n", __func__);
    struct int_rmt_data * ps = container_of(_ps, struct int_rmt_data, base);
    
    TQoSId qos_id = pdu->pci->connection_id.qos_id;
    TPortId port_id = rmt_n1_port->port_id;
    
    struct port_vals_node * portVals = NULL;
    struct qos_vals_node * qosVals = getPortQos(ps, port_id, qos_id, &portVals);
    
    if(portVals == NULL || qosVals == NULL) {
        LOG_WARN("Port or QoS not initialicied for given input port structure.");
        return -1;
    }
    
    
    //RE-PUSH PDU
    portVals->count += 1;
    qosVals->queue_begin = getPDUNode(ps, p, qosVals->queue_begin);
    if(qosVals->queue_end == NULL) {
        qosVals->queue_end = qosVals->queue_begin;
    }
    
    //RE-STORE time log
    qosVals->wtime_begin = getTimeNode(ps, qosVals->last_stime, qosVals->wtime_begin);
    if(qosVals->wtime_end == NULL) {
        qosVals->wtime_end = qosVals->wtime_begin;
    } 
    
    if(qosVals->params->limit_out) {
        qosVals->send_size -= qosVals->send_size_time_begin->size;
        qosVals->send_size_time_begin = qosVals->send_size_time_begin->next;
        if(qosVals->send_size_time_begin == NULL) {
            qosVals->send_size_time_end = qosVals->send_size_time_begin;
        }
    }
}
        

/**
 * Policy
 **/

/*
 * SET param
 */
static int rmt_ps_set_policy_set_param(struct ps_base * bps,
        const char * name,
        const char * value) {
    struct rmt_ps * _ps = container_of(bps, struct rmt_ps, base);
    struct int_rmt_data * ps = container_of(_ps, struct int_rmt_data, base);


    if (!name) {
        LOG_ERR("Null parameter name");
        return -1;
    }

    if (!value) {
        LOG_ERR("Null parameter value");
        return -1;
    }
    
    // SET PORT <port, port_id>
    /*
     * port = alloc new port node;
     * port.key = port_id
     * port.count = 0
     * foreach QoS_Params
     * * port.qos <- alloc new QoS<key, qos_params>
     * insert port to ports list
     */
    
    // SET QoS <qos, {qos_id, expectedWtime, multiplier
    //                  limitIn:(active, burstDuration, [(size, dropProb),..,(size,dropProb)]),  
    //                  limitOut:(active, burstDuration, burstSize),  
    //                  cherishDrop:([(size, dropProb),..,(size,dropProb)])}
    /*
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
static struct ps_base *
rmt_ps_deltaq_create(struct rina_component * component) {
    struct rmt * rmt = rmt_from_component(component);
    struct int_rmt_data * ps = rkzalloc(sizeof (*ps), GFP_KERNEL);

    if (!ps) {
        return NULL;
    }

    ps->base.base.set_policy_set_param = rmt_ps_set_policy_set_param;
    ps->base.dm = rmt;
    ps->base.priv = NULL;
    
    ps->base.rmt_q_monitor_policy_rx = NULL;
    ps->base.max_q_policy_rx = NULL;
    ps->base.rmt_scheduling_policy_rx = imp_rmt_limiter_rx;
    
    ps->base.rmt_q_monitor_policy_tx_enq = NULL;
    ps->base.rmt_enqueue_scheduling_policy_tx = imp_rmt_enqueue_tx;
    ps->base.max_q_policy_tx = NULL;
    
    ps->base.rmt_next_scheduled_policy_tx = imp_rmt_dequeue_tx;
    ps->base.rmt_q_monitor_policy_tx_deq = NULL;
    ps->base.rmt_requeue_scheduling_policy_tx = imp_rmt_requeue_tx;
    
    ps->base.rmt_scheduling_create_policy_tx = imp_rmt_scheduling_create_policy_tx;
    ps->base.rmt_scheduling_destroy_policy_tx = imp_rmt_scheduling_destroy_policy_tx;

    //ps->base.rmt_rescheduling_if_null_tx = true;
    
    ps->port_vals_begin = NULL;
    ps->qos_params_begin = NULL;
    
    ps->buffer_size_time_begin = NULL;
    ps->buffer_time_begin = NULL;
    ps->buffer_pdu_begin = NULL;
    
    
    return &ps->base.base;
}

/*
 * DESTROY
 */
static void rmt_ps_deltaq_destroy(struct ps_base * bps) {
    struct rmt_ps * _ps = container_of(bps, struct rmt_ps, base);
    struct int_rmt_data * ps = container_of(_ps, struct int_rmt_data, base);
    
    //Release buffers
    while(ps->buffer_size_time_begin != NULL) {
        struct size_time_node * t = ps->buffer_size_time_begin;
        ps->buffer_size_time_begin = t->next;
        kfree(t);
    }
    
    while(ps->buffer_time_begin != NULL) {
        struct time_node * t = ps->buffer_time_begin;
        ps->buffer_time_begin = t->next;
        kfree(t);
    }
    
    while(ps->buffer_pdu_begin != NULL) {
        struct pdu_node * t = ps->buffer_pdu_begin;
        ps->buffer_pdu_begin = t->next;
        kfree(t);
    }
    
    //Release ports data
    while(ps->port_vals_begin != NULL) {
        struct port_vals_node * t_port = ps->port_vals_begin;
        
        //Release Port Qos values
        while(t_port->qos_vals_begin != NULL){
            struct qos_vals_node * t_qos = t_port->qos_vals_begin;
            
            //Release remaining PDUs. !!should be empty on destroy
            while(t_qos->queue_begin != NULL){
                struct pdu_node * t_node = t_qos->queue_begin;
                t_qos->queue_begin = t_node->next;
                kfree(t_node->p);// Release also pdu data? ask
                kfree(t_node);
            }
            
            //Release remaining PDUs in time. !!should be empty on destroy
            while(t_qos->wtime_begin != NULL) {
                struct time_node * t_node = t_qos->wtime_begin;
                t_qos->wtime_begin = t_node->next;
                kfree(t_node);
            }
            
            //Release received size/time log for inputLimit
            while(t_qos->received_size_time_begin != NULL) {
                struct size_time_node * t_node = t_qos->received_size_time_begin;
                t_qos->received_size_time_begin = t_node->next;
                kfree(t_node);
            }
            
            //Release send size/time log for outputLimit
            while(t_qos->send_size_time_begin != NULL) {
                struct size_time_node * t_node = t_qos->send_size_time_begin;
                t_qos->send_size_time_begin = t_node->next;
                kfree(t_node);
            }
            
            t_port->qos_vals_begin = t_qos->next;
            kfree(t_qos);
        }
        
        ps->port_vals_begin = t_port->next;
        kfree(t_port);
    }
    
    if (bps) {
        rkfree(ps);
    }
}

/*
 * FACTORY DATA
 */
struct ps_factory rmt_factory = {
    .owner = THIS_MODULE,
    .create = rmt_ps_deltaq_create,
    .destroy = rmt_ps_deltaq_destroy,
};
