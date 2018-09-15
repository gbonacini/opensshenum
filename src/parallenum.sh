#!/usr/bin/env bash

clisyntax(){
 echo "Usage: $0 [-P <number_of_parallel_processes>] [-i identity]"                  >&2
 echo "          [ -s -m min -M max -n -r regexp -c clientIdstr]"                    >&2
 echo "          | [-h] | [-V]"                                                      >&2
 echo "          hostname or address"                                                >&2
 echo "Parameters:"                                                                  >&2
 echo "[-P <n>] Forces the number of parallel processes to n"                        >&2
 echo "         If this parameter isn't specified, 2*<cpu_number> is used"           >&2
 echo "[-i <k>] Specifies the ssh key prefix files (i.e. id_rsa) "                   >&2
 echo "         The files must be presents in ~/.ssh"                                >&2
 echo "[-n]     only scan the target to individuare ssh port(s) "                    >&2
 echo "         No user enumeration is performed."                                   >&2
 echo "[-m <n>] lower port for scan mode: the scan will be performed"                >&2
 echo "         starting from this port number to that specified with -M"            >&2
 echo "[-M <n>] higher port for scan mode: the scan will be performed"               >&2
 echo "         starting from port specified with -m  to this port number."          >&2
 echo "[-t <n>] port scanning timeout"                                               >&2
 echo "[-r <e>] regular expression used in scan mode to identify openssh running"    >&2
 echo "         on ports other then 22. "                                            >&2
 echo "[-c <i>  Client id string used in initial handshaking to identify the ssh"    >&2
 echo "         client. If no custom string is specified,'SSH-2.0-enum' will be sent">&2
 echo "[-h]     print this help message."                                            >&2
 echo "[-V]     version information."                                                >&2
 exit 1
}

CMDLINE=""
VERSION="0.1.0"
START_PORT=""
END_PORT=""
PARALLEL_PROCESSES=""
HAS_IDENTITY="0"
HAS_REGEXP="0"
while getopts "P:nt:m:M:r:c:i:hV" opar; do
    case "${opar}" in
        n)
            CMDLINE="$CMDLINE -n "
            ;;
        t)
            CMDLINE="$CMDLINE -t${OPTARG}"
            ;;
        m)
            START_PORT="${OPTARG}"
            ;;
        M)
            END_PORT="${OPTARG}"
            ;;
        r)
            CMDLINE="$CMDLINE -r${OPTARG}"
            HAS_REGEXP="1"
            ;;
        c)
            CMDLINE="$CMDLINE -c${OPTARG}"
            ;;
        P)
            PARALLEL_PROCESSES="${OPTARG}"
            ;;
        i)
            CMDLINE="$CMDLINE -i${OPTARG}"
            HAS_IDENTITY="1"
            ;;
        h)
            clisyntax
            ;;
        V)
            echo "parallenum: an opensshenum wrapper" >&2
            echo "Version:    ${VERSION}"             >&2
            echo "Using:      $(opensshenum -V)"      >&2
            exit 1
            ;;
        *)
            clisyntax
            ;;
    esac
done
shift $((OPTIND-1))

THOST="$1"

[[ -z "${THOST}" ]]               && echo "Missing Host name or addr." >&2 && clisyntax
[[ "$#" -ne "1" ]]                && echo "Unexpected arg at the end." >&2 && clisyntax
[[ -z "${START_PORT}" ]]          && echo "-m is a mandatory param."   >&2 && clisyntax
[[ -z "${END_PORT}" ]]            && echo "-M is a mandatory param."   >&2 && clisyntax
[[ "${HAS_IDENTITY}" -ne "1" ]]   && echo "-i is a mandatory param."   >&2 && clisyntax
[[ "${HAS_REGEXP}" -ne "1" ]]     && echo "-r is a mandatory param."   >&2 && clisyntax
[[ "x$(which parallel)" = "x" ]]  && echo "Can't find GNU Parallel."   >&2 && exit 1

[[ -z "${PARALLEL_PROCESSES}" ]]  && PARALLEL_PROCESSES="$(lscpu | grep '^CPU(s):' | awk '{ print $2 }')"

LAST_PORT="65535"
GROUP_SIZE=$((((${END_PORT}-${START_PORT})/${PARALLEL_PROCESSES})))
[[ "${GROUP_SIZE}" -eq "0" ]] && echo "Invalid processes number or range." >&2 && clisyntax
echo "start $CMDLINE $START_PORT $END_PORT ${PARALLEL_PROCESSES} ${GROUP_SIZE}"

LOW="${START_PORT}"
IDX="1"
#TODO: DEFAULT VAL from installed CPUs
PARALLEL_CDM_PARAMS=""
for i in $(seq ${START_PORT} ${GROUP_SIZE} ${END_PORT} ); do 
        HIGH=${i}
        [[ "${LOW}" = "${HIGH}" ]]  && continue
        [[ "${IDX}" -eq "${PARALLEL_PROCESSES}" ]] && HIGH=${END_PORT}
        PARALLEL_CDM_PARAMS="${PARALLEL_CDM_PARAMS} -m${LOW} -M${HIGH}" 
        LOW=$(((${HIGH}+1)))
        IDX=$(((${IDX}+1)))
done
echo ${PARALLEL_CDM_PARAMS}

parallel --bar --jobs ${PARALLEL_PROCESSES} -m opensshenum ${CMDLINE} -s {} ${THOST} ::: ${PARALLEL_CDM_PARAMS}
