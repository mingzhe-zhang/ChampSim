#!/bin/bash

if [ "$#" -gt 18 ]; then
    echo "Illegal number of parameters"
    echo "Usage: ./run_champsim.sh [BINARY] [N_WARM] [N_SIM] [TRACE] [MOSAIC_CACHE_OPTIONS] [OPTION]"
    echo "MOSAIC_CACHE_OPTIONS: MOSAIC_CACHE_WORK_MODE"
    echo "                      MOSAIC_CACHE_WRITEBACK_MODE"
    echo "                      MOSAIC_CACHE_TARGET_DELTA"
    echo "                      MOSAIC_CACHE_CHECK_PERIOD"
    echo "                      MOSAIC_CACHE_L1_ADAPTIVE_WAY_NUM"
    echo "                      MOSAIC_CACHE_L1_RECONFIG_THRESHOLD"
    echo "                      MOSAIC_CACHE_L1_RATIO"
    echo "                      MOSAIC_CACHE_L2_ADAPTIVE_WAY_NUM"
    echo "                      MOSAIC_CACHE_L2_RECONFIG_THRESHOLD"
    echo "                      MOSAIC_CACHE_L2_RATIO"
    echo "                      MOSAIC_CACHE_L3_ADAPTIVE_WAY_NUM"
    echo "                      MOSAIC_CACHE_L3_RECONFIG_THRESHOLD"
    echo "                      MOSAIC_CACHE_L3_RATIO"
    exit 1
fi

TRACE_DIR=$PWD/dpc3_traces
BINARY=${1}
N_WARM=${2}
N_SIM=${3}
TRACE=${4}
# MOSAIC_CACHE_OPT=${5}" "${6}" "${7}" "${8}" "${9}" "${10}" "${11}" "${12}" "${13}" "${14}" "${15}" "${16}" "${17}
MOSAIC_CACHE_OPT="-mosaic_cache_work_mode "${5}" -mosaic_cache_writeback_mode "${6}" -mosaic_cache_target_delta "${7}
MOSAIC_CACHE_OPT=${MOSAIC_CACHE_OPT}" -mosaic_cache_check_period "${8}
MOSAIC_CACHE_OPT=${MOSAIC_CACHE_OPT}" -mosaic_cache_l1_adaptive_way_num "${9}" -mosaic_cache_l1_reconfig_threshold "${10}" -mosaic_cache_l1_ratio "${11}
MOSAIC_CACHE_OPT=${MOSAIC_CACHE_OPT}" -mosaic_cache_l2_adaptive_way_num "${12}" -mosaic_cache_l2_reconfig_threshold "${13}" -mosaic_cache_l2_ratio "${14}
MOSAIC_CACHE_OPT=${MOSAIC_CACHE_OPT}" -mosaic_cache_l3_adaptive_way_num "${15}" -mosaic_cache_l3_reconfig_threshold "${16}" -mosaic_cache_l3_ratio "${17}
OPTION=${18}

echo ${MOSAIC_CACHE_OPT}
echo ${OPTION}

# Sanity check
if [ -z $TRACE_DIR ] || [ ! -d "$TRACE_DIR" ] ; then
    echo "[ERROR] Cannot find a trace directory: $TRACE_DIR"
    exit 1
fi

if [ ! -f "bin/$BINARY" ] ; then
    echo "[ERROR] Cannot find a ChampSim binary: bin/$BINARY"
    exit 1
fi

re='^[0-9]+$'
if ! [[ $N_WARM =~ $re ]] || [ -z $N_WARM ] ; then
    echo "[ERROR]: Number of warmup instructions is NOT a number" >&2;
    exit 1
fi

re='^[0-9]+$'
if ! [[ $N_SIM =~ $re ]] || [ -z $N_SIM ] ; then
    echo "[ERROR]: Number of simulation instructions is NOT a number" >&2;
    exit 1
fi

if [ ! -f "$TRACE_DIR/$TRACE" ] ; then
    echo "[ERROR] Cannot find a trace file: $TRACE_DIR/$TRACE"
    exit 1
fi

mkdir -p results_${N_SIM}M
(./bin/${BINARY} -warmup_instructions ${N_WARM}000000 -simulation_instructions ${N_SIM}000000 ${MOSAIC_CACHE_OPT} ${OPTION} -traces ${TRACE_DIR}/${TRACE}) &> results_${N_SIM}M/${TRACE}-${BINARY}${OPTION}.txt
