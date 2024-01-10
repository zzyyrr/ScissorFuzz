#!/usr/bin/env python
import os
import sys
import subprocess
import time

import psutil

afl_instance_py = "./afl_instance_trim.py"
run_log_dir = "./run_logs"

job_int = 0
job_container = []
SLEEP_TIME = 30


def start_job(app_id):
    # print "start_job"

    cmd = ["python", afl_instance_py, app_id]
    if len(sys.argv) >= 2:
        cmd += [sys.argv[1]]
    print(cmd)
    with open(os.path.join(run_log_dir, app_id + ".log"), "w") as fp:
        p = subprocess.Popen(cmd, stdout=fp, stderr=fp, close_fds=True)
    return p


def waitPid(pid):
    for proc in psutil.process_iter():
        if proc.pid == pid:
            print("%s is still running!" % proc.name())
            return True
    return False


def main():
    global job_int
    if not os.path.exists(run_log_dir):
        os.makedirs(run_log_dir)
    app_id_list = ["1"]
    # app_id_list = ["1", "2", "3", "4", "5", "6", "7", "8", "9", "14", "15", "16", "17", "18", "19", "20", "21", "22",
    #                "23", "24"]

    for app_id in app_id_list:
        while job_int >= 8:
            time.sleep(SLEEP_TIME)
            for i in job_container:
                if i.poll() is not None:
                    job_container.remove(i)
                    job_int = job_int - 1
        job_container.append(start_job(app_id))
        time.sleep(SLEEP_TIME)
        job_int = job_int + 1


if __name__ == "__main__":
    main()
