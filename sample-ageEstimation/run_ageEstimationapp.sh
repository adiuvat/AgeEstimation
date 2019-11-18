#!/bin/bash
#
#   =======================================================================
#
# Copyright (C) 2018, Hisilicon Technologies Co., Ltd. All Rights Reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#   1 Redistributions of source code must retain the above copyright notice,
#     this list of conditions and the following disclaimer.
#
#   2 Redistributions in binary form must reproduce the above copyright notice,
#     this list of conditions and the following disclaimer in the documentation
#     and/or other materials provided with the distribution.
#
#   3 Neither the names of the copyright holders nor the names of the
#   contributors may be used to endorse or promote products derived from this
#   software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#   =======================================================================

# ************************Variable*********************************************

script_path="$( cd "$(dirname "$0")" ; pwd -P )"

remote_host=$1
#presenter_view_app_name=$2
data_source=$2

. ${script_path}/script/func_util.sh
. ${script_path}/script/func_deploy.sh

function kill_remote_running()
{
    echo -e "run.sh exit, kill ${remote_host}:ascend_ageEstimationapp running..."
    parse_remote_port
	iRet=$(IDE-daemon-client --host ${remote_host}:${remote_port} --hostcmd "kill \$(pidof ~/HIAI_PROJECTS/ascend_workspace/ageEstimationapp/out/ascend_ageEstimationapp)")
	if [[ $? -eq 0 ]];then
		echo "$iRet in ${remote_host}."
		return 0
	fi

    iRet=$(IDE-daemon-client --host ${remote_host}:${remote_port} --hostcmd "for p in \`pidof ascend_ageEstimationapp\`; do { echo \"kill \$p\"; kill \$p; }; done")
    if [[ $? -ne 0 ]];then
        echo "ERROR: kill ${remote_host}:ascend_ageEstimationapp running failed, please login to kill it manually."
        return 1
    else
        echo "$iRet in ${remote_host}."
    fi
    return 0
}

trap 'kill_remote_running' 2 15

function main()
{
    if [[ $# -lt 2 ]];then
        echo "ERROR: invalid command, please check your command format: ./run_ageEstimationapp.sh host_ip camera_channel_name."
        exit 1
    fi

    check_ip_addr ${remote_host}
    if [[ $? -ne 0 ]];then
        echo "ERROR: invalid host ip, please check your command format: ./run_ageEstimationapp.sh host_ip camera_channel_name."
        exit 1
    fi

#    bash ${script_path}/script/prepare_param.sh ${remote_host} ${presenter_view_app_name} ${data_source}
    bash ${script_path}/script/prepare_param.sh ${remote_host} ${data_source}
    if [[ $? -ne 0 ]];then
        exit 1
    fi

#    #start presenter
#    #cd {script_path}/script
#    presenter_server_pid=`ps -ef | grep "presenter_server\.py" | grep "age_estimation" | awk -F ' ' '{print $2}'`
#    if [[ ${presenter_server_pid}"X" == "X" ]];then
#        echo "presenter server for age estimation is not started, please start it."
#        exit 1
#    fi


    parse_remote_port

    echo "[Step] run ${remote_host}:ascend_ageEstimationapp..."

    #start app
    iRet=`IDE-daemon-client --host $remote_host:${remote_port} --hostcmd "cd ~/HIAI_PROJECTS/ascend_workspace/ageEstimationapp/out/;./ascend_ageEstimationapp"`
    if [[ $? -ne 0 ]];then
        echo "ERROR: excute ${remote_host}:./HIAI_PROJECTS/ascend_workspace/ageEstimationapp/out/ascend_ageEstimationapp failed, please check /var/log/syslog and board running log from IDE Log Module for details."
        exit 1
    fi
    exit 0
}

main $*
