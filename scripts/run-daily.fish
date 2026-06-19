#!/usr/bin/env fish

# Run maa daily task inside redroid, always stop the container on exit.
#
# Usage: run-daily.fish
# Logs:  ~/.local/share/maa/run-logs/daily-YYYYMMDD-HHMMSS.log

set -g MAA_DAILY_CONTAINER redroid
set -g MAA_DAILY_ADB localhost:5555
set -g MAA_DAILY_TASK daily
set -g MAA_DAILY_LOG_DIR "$HOME/.local/share/maa/run-logs"
set -g MAA_DAILY_BOOT_TIMEOUT 180
set -g MAA_DAILY_VERBOSE 1  # 1=info, 2=debug, 3=trace

mkdir -p $MAA_DAILY_LOG_DIR
set -g MAA_DAILY_LOG_FILE "$MAA_DAILY_LOG_DIR/daily-"(date +%Y%m%d-%H%M%S)".log"

function maa_daily_verbose_args
    set -l args
    for i in (seq $MAA_DAILY_VERBOSE)
        set args $args -v
    end
    echo $args
end

function maa_daily_log
    set -l line "["(date '+%Y-%m-%d %H:%M:%S')"] "$argv
    echo $line | tee -a $MAA_DAILY_LOG_FILE
end

function maa_daily_stop_redroid
    if docker inspect -f '{{.State.Running}}' $MAA_DAILY_CONTAINER 2>/dev/null | grep -qx true
        maa_daily_log "停止容器 $MAA_DAILY_CONTAINER ..."
        docker stop $MAA_DAILY_CONTAINER &>> $MAA_DAILY_LOG_FILE
        or maa_daily_log "警告: docker stop 失败 (exit $status)"
    else
        maa_daily_log "容器 $MAA_DAILY_CONTAINER 未在运行，跳过停止"
    end
end

function maa_daily_on_signal --on-signal INT --on-signal TERM --on-signal QUIT
    maa_daily_log "收到信号，执行清理 ..."
    maa_daily_stop_redroid
    exit 130
end

function maa_daily_wait_for_adb
    set -l elapsed 0
    while test $elapsed -lt $MAA_DAILY_BOOT_TIMEOUT
        adb connect $MAA_DAILY_ADB &>> $MAA_DAILY_LOG_FILE
        if adb -s $MAA_DAILY_ADB shell true &>> $MAA_DAILY_LOG_FILE
            maa_daily_log "ADB 已就绪 ($MAA_DAILY_ADB)"
            return 0
        end
        sleep 3
        set elapsed (math $elapsed + 3)
        maa_daily_log "等待 ADB ... ($elapsed/$MAA_DAILY_BOOT_TIMEOUT 秒)"
    end
    return 1
end

set -l exit_code 0

maa_daily_log "========== 开始 daily 任务 =========="
maa_daily_log "日志: $MAA_DAILY_LOG_FILE"

maa_daily_log "启动容器 $MAA_DAILY_CONTAINER ..."
if not docker start $MAA_DAILY_CONTAINER &>> $MAA_DAILY_LOG_FILE
    maa_daily_log "错误: docker start 失败 (exit $status)"
    maa_daily_stop_redroid
    exit 1
end

if not maa_daily_wait_for_adb
    maa_daily_log "错误: 等待 ADB 超时"
    maa_daily_stop_redroid
    exit 1
end

maa_daily_log "运行 maa run $MAA_DAILY_TASK (verbose=$MAA_DAILY_VERBOSE) ..."
maa run $MAA_DAILY_TASK --batch &| tee -a $MAA_DAILY_LOG_FILE
set exit_code $pipestatus[1]

if test $exit_code -eq 0
    maa_daily_log "maa 任务完成"
else
    maa_daily_log "maa 任务失败 (exit $exit_code)"
end

maa_daily_stop_redroid

maa_daily_log "========== 结束 (exit $exit_code) =========="
exit $exit_code