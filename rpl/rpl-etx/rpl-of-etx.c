/**
 * \addtogroup uip6
 * @{
 */
/*
 * Copyright (c) 2010, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */
/**
 * \file
 *         The minrank-hysteresis objective function (OCP 1).
 *
 *         This implementation uses the estimated number of 
 *         transmissions (ETX) as the additive routing metric.
 *
 * \author Joakim Eriksson <joakime@sics.se>, Nicolas Tsiftes <nvt@sics.se>
 */

#include "net/rpl/rpl-private.h"
#include "net/neighbor-info.h"

#define DEBUG DEBUG_NONE
#include "net/uip-debug.h"

static void reset(rpl_dag_t *);
static void parent_state_callback(rpl_parent_t *, int, int);
static rpl_parent_t *best_parent(rpl_parent_t *, rpl_parent_t *);
static rpl_rank_t calculate_rank(rpl_parent_t *, rpl_rank_t);
static void update_metric_container(rpl_dag_t *);

rpl_of_t rpl_of_etx = {
  reset,
  parent_state_callback,
  best_parent,
  calculate_rank,
  update_metric_container,
  1
};

#define NI_ETX_TO_RPL_ETX(etx)						\
	((etx) * (RPL_DAG_MC_ETX_DIVISOR / NEIGHBOR_INFO_ETX_DIVISOR))
#define RPL_ETX_TO_NI_ETX(etx)					\
	((etx) / (RPL_DAG_MC_ETX_DIVISOR / NEIGHBOR_INFO_ETX_DIVISOR))

/* Reject parents that have a higher link metric than the following. */
#define MAX_LINK_METRIC			10

/* Reject parents that have a higher path cost than the following. */
#define MAX_PATH_COST			100

/*
 * The rank must differ more than 1/PARENT_SWITCH_THRESHOLD_DIV in order
 * to switch preferred parent.
 */
#define PARENT_SWITCH_THRESHOLD_DIV	2

typedef uint16_t rpl_path_metric_t;

static rpl_path_metric_t
calculate_path_metric(rpl_parent_t *p)
{



  if(p == NULL || (p->mc.obj.etx == 0 && p->rank > ROOT_RANK(p->dag))) {
    //printf("etx nullo, path metric is %d\n", MAX_PATH_COST * RPL_DAG_MC_ETX_DIVISOR);
return MAX_PATH_COST * RPL_DAG_MC_ETX_DIVISOR;
  }

  return p->mc.obj.etx + NI_ETX_TO_RPL_ETX(p->link_metric);
}

static void
reset(rpl_dag_t *dag)
{
}

static void
parent_state_callback(rpl_parent_t *parent, int known, int etx)
{
}

static rpl_rank_t
calculate_rank(rpl_parent_t *p, rpl_rank_t base_rank)
{
  rpl_rank_t new_rank;
  rpl_rank_t rank_increase;

  if(p == NULL) {
    if(base_rank == 0) {
      return INFINITE_RANK;
    }
    rank_increase = NEIGHBOR_INFO_FIX2ETX(INITIAL_LINK_METRIC) * DEFAULT_MIN_HOPRANKINC;
  } else {
    rank_increase = NEIGHBOR_INFO_FIX2ETX(p->link_metric) * p->dag->min_hoprankinc;
    if(base_rank == 0) {
      base_rank = p->rank;
    }
  }

  if(INFINITE_RANK - base_rank < rank_increase) {
    /* Reached the maximum rank. */
    new_rank = INFINITE_RANK;
  } else {
   /* Calculate the rank based on the new rank information from DIO or
      stored otherwise. */
    new_rank = base_rank + rank_increase;
 //printf("base rank %d rank_increase %d new rank %d\n",base_rank,rank_increase,new_rank);
  }
 //printf("new rank %d\n",new_rank);
 /*printf("RPL_DAG_MC_ETX_DIVISOR %d NEIGHBOR_INFO_ETX_DIVISOR %d DEFAULT_MIN_HOPRANKINC %d INFINITE_RANK %d INITIAL_LINK_METRIC %d\n",RPL_DAG_MC_ETX_DIVISOR,NEIGHBOR_INFO_ETX_DIVISOR,DEFAULT_MIN_HOPRANKINC,INFINITE_RANK,INITIAL_LINK_METRIC);*/
  return new_rank;
}

static rpl_parent_t *
best_parent(rpl_parent_t *p1, rpl_parent_t *p2)
{
  rpl_dag_t *dag;
  rpl_path_metric_t min_diff;
  rpl_path_metric_t p1_metric;
  rpl_path_metric_t p2_metric;

  dag = p1->dag; /* Both parents must be in the same DAG. */

  min_diff = RPL_DAG_MC_ETX_DIVISOR / 
             PARENT_SWITCH_THRESHOLD_DIV;

  p1_metric = calculate_path_metric(p1);
  p2_metric = calculate_path_metric(p2);

  /* Maintain stability of the preferred parent in case of similar ranks. */
  if(p1 == dag->preferred_parent || p2 == dag->preferred_parent) {
    if(p1_metric < p2_metric + min_diff &&
       p1_metric > p2_metric - min_diff) {
      /*PRINTF("RPL: MRHOF hysteresis: %u <= %u <= %u\n",
             p2_metric - min_diff,
             p1_metric,
             p2_metric + min_diff);*/
      return dag->preferred_parent;
    }
  }

  return p1_metric < p2_metric ? p1 : p2;
}

static void
update_metric_container(rpl_dag_t *dag)
{
  rpl_path_metric_t path_metric;
#if RPL_DAG_MC == RPL_DAG_MC_ENERGY
  uint8_t type;
#endif

  dag->mc.flags = RPL_DAG_MC_FLAG_P;
  dag->mc.aggr = RPL_DAG_MC_AGGR_ADDITIVE;
  dag->mc.prec = 0;

  if(dag->rank == ROOT_RANK(dag)) {
    path_metric = 0;
  } else {
    path_metric = calculate_path_metric(dag->preferred_parent);
  }

#if RPL_DAG_MC == RPL_DAG_MC_ETX

  dag->mc.type = RPL_DAG_MC_ETX;
  dag->mc.length = sizeof(dag->mc.obj.etx);
  dag->mc.obj.etx = path_metric;

  PRINTF("RPL: My path ETX to the root is %u.%u\n",
	dag->mc.obj.etx / RPL_DAG_MC_ETX_DIVISOR,
	(dag->mc.obj.etx % RPL_DAG_MC_ETX_DIVISOR * 100) / RPL_DAG_MC_ETX_DIVISOR);
//----------------------------------------------------------------------------------------------
 /*static uint32_t last_cpu, last_lpm, last_transmit;
uint32_t cpu, lpm, transmit, listen;
uint32_t avg_power;
static uint32_t  last_listen;
uint32_t maxbattery = 32400000;*/
/*1)supply voltage range = 2,1 - 3.6 V --------> 3V (in Tmote's sky datasheet)
2)A*h range = 500 - 3000 mAh --------------> 3000 mAh
3)1 W*h = 3600 Joule
initial value (3000 * 3 * 3600) mJ*/

 /*energest_flush();
cpu = energest_type_time(ENERGEST_TYPE_CPU);
   lpm = energest_type_time(ENERGEST_TYPE_LPM);
   transmit = energest_type_time(ENERGEST_TYPE_TRANSMIT);
  listen = energest_type_time(ENERGEST_TYPE_LISTEN);
  


last_cpu = energest_type_time(ENERGEST_TYPE_CPU) - cpu;
last_lpm = energest_type_time(ENERGEST_TYPE_LPM) - lpm;
last_transmit = energest_type_time(ENERGEST_TYPE_TRANSMIT) -  transmit;
last_listen = energest_type_time(ENERGEST_TYPE_LISTEN) - listen;



avg_power =  (unsigned long) ((1.8 * energest_type_time(ENERGEST_TYPE_CPU)  +0.0545 * energest_type_time(ENERGEST_TYPE_LPM) + 20.0 
 * energest_type_time(ENERGEST_TYPE_LISTEN) + 17.7 * energest_type_time(ENERGEST_TYPE_TRANSMIT)) * 3 / RTIMER_SECOND  );
printf("avg power %lu energy %lu\n",avg_power,(((maxbattery - avg_power) *100) / maxbattery));*/
//----------------------------------------------------------------------------------------------------
#elif RPL_DAG_MC == RPL_DAG_MC_ENERGY

  dag->mc.type = RPL_DAG_MC_ENERGY;
  dag->mc.length = sizeof(dag->mc.obj.energy);

  if(dag->rank == ROOT_RANK(dag)) {
    type = RPL_DAG_MC_ENERGY_TYPE_MAINS;
  } else {
    type = RPL_DAG_MC_ENERGY_TYPE_BATTERY;
  }

  dag->mc.obj.energy.flags = type << RPL_DAG_MC_ENERGY_TYPE;
  dag->mc.obj.energy.energy_est = path_metric;

#else

#error "Unsupported RPL_DAG_MC configured. See rpl.h."

#endif /* RPL_DAG_MC */
}
