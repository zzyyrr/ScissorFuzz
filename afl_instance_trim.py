#!/usr/bin/env python
import os
import sys
import subprocess
import time
import shutil
import psutil

binarise_dir = "/d/p/justafl/"
seed_dir = "/workdir/seeds/"
AFL_bin = "/afl/afl-fuzz"
output_dir = "/workdir/result/"
qsym_bin = "/workdir/qsym/bin/run_qsym_afl.py"
tmin_process_bin = "./tmin_instance.py"
dissinput_root = "/workdir/pathto/dissinput/"

job_int = 0
job_container = []
SLEEP_TIME = 30
TIME_LIMIT = 12*3600
# TIME_LIMIT = 600

app_map = {
    # id, prog, commandline, seed_folder
    "1": [1, "exiv2-0.25", ["@@"], "jpg"],
    "2": [2, "tiffsplit", ["@@"], "tiff"],
    "3": [3, "mp3gain", ["@@"], "mp3"],
    "4": [4, "wav2swf", ["-o", "/dev/null", "@@"], "wav"],
    "5": [5, "pdftotext", ["@@", "/dev/null"], "pdf"],
    "6": [6, "infotocap", ["-o", "/dev/null", "@@"], "text"],
    "7": [7, "mp42aac", ["@@", "/dev/null"], "mp4"],
    "8": [8, "flvmeta", ["@@"], "flv"],
    "9": [9, "objdump", ["-S", "@@"], "obj"],
    "14": [14, "tcpdump", ["-e", "-vv", "-nr", "@@"], "tcpdump100"],
    "15": [15, "ffmpeg", ["-y", "-i", "@@", "-c:v", "mpeg4", "-c:a", "copy", "-f", "mp4", "/dev/null"], "ffmpeg100"],
    "16": [16, "gdk-pixbuf-pixdata", ["@@", "/dev/null"], "pixbuf"],
    "17": [17, "cflow", ["@@"], "cflow"],
    "18": [18, "nm",
           ["-A", "-a", "-l", "-S", "-s", "--special-syms", "--synthetic", "--with-symbol-versions", "-D", "@@"], "nm"],
    # "19": [19, "sqlite3", ["<", "@@"], "sql"],
    "19": [19, "sqlite3", [], "sql"],
    "20": [20, "lame", ["@@", "/dev/null"], "lame3.99.5"],
    "21": [21, "jhead", ["@@"], "jhead"],
    "22": [22, "imginfo", ["-f", "@@"], "imginfo"],
    "23": [23, "jq", [".", "@@"], "json"],
    "24": [24, "mujs", ["@@"], "mujs"],
    # below is the LAVA-M settings
    "10": [10, "uniq", ["@@"], "uniq"],
    "11": [11, "base64", ["-d", "@@"], "base64"],
    "12": [12, "md5sum", ["-c", "@@"], "md5sum"],
    "13": [13, "who", ["@@"], "who"],
}


def start_afl_instance(app, flag):
    print("start_afl_instance")
    # target app
    loc_binary = os.path.join(binarise_dir, app[1])
    # print(loc_binary)
    if not os.path.isfile(loc_binary):
        return 0

    base_dir = os.path.join(output_dir, app[1])
    if not os.path.exists(base_dir):
        os.makedirs(base_dir)
    in_dir = os.path.join(base_dir, "in")
    if not os.path.exists(in_dir):
        os.makedirs(in_dir)
    out_dir = os.path.join(base_dir, "out")
    if not os.path.exists(out_dir):
        os.makedirs(out_dir)

    # copy the seed
    seed_path = os.path.join(seed_dir, app[3])
    src_files = os.listdir(seed_path)
    for file_name in src_files:
        full_file_name = os.path.join(seed_path, file_name)
        if os.path.isfile(full_file_name):
            shutil.copy(full_file_name, in_dir)

    if not os.listdir(in_dir):
        with open(os.path.join(in_dir, "seed"), "wb") as f:
            f.write("fuzz\n".encode())

    memory = "none"
    args = [AFL_bin]
    args += ["-i", in_dir]
    args += ["-o", out_dir]
    args += ["-m", memory]
    '''if loc_binary.find('cflow') > -1 or loc_binary.find('imginfo') > -1 or loc_binary.find('jhead') > -1 or loc_binary.find('lame') > -1 or loc_binary.find('mp3gain') > -1 or loc_binary.find('objdum$
            args += ["-t","30000+"]'''
    # args += ["-Q"]
    # args += ["-d"]
    if flag == 0:
        args += ["-M", "fuzzer-master"]
    elif flag == 1:
        args += ["-S", "fuzzer-slave"]
    else:
        args += ["-S", "fuzzer-slavetwo"]

    args += ["--"]
    args += [loc_binary]
    args += app[2]

    if flag == 0:
        outfile = os.path.join(base_dir, "master.log")
    elif flag == 1:
        outfile = os.path.join(base_dir, "slave.log")
    else:
        outfile = os.path.join(base_dir, "slavetwo.log")

    print(args)
    with open(outfile, "w") as fp:
        p = subprocess.Popen(args, stdout=fp, stderr=fp, close_fds=True)
        return p


def start_tmin_executor(app):
    print("start_trim_executor")
    # target app
    loc_binary = os.path.join(binarise_dir, app[1])
    # print(loc_binary)
    if not os.path.isfile(loc_binary):
        return 0
    base_dir = os.path.join(output_dir, app[1])
    out_dir = os.path.join(base_dir, "out")
    afl_dir = os.path.join(out_dir, "fuzzer-slave")
    fuzzer_queue_path = os.path.join(afl_dir, "queue")
    while not os.path.exists(fuzzer_queue_path):
        time.sleep(SLEEP_TIME)

    outfile = os.path.join(base_dir, "tmin.log")
    args = [tmin_process_bin]
    args += [out_dir]
    args += [dissinput_root]
    args += [loc_binary]
    args += app[2]
    print(args)
    with open(outfile, "w") as fp:
        p = subprocess.Popen(args, stdout=fp, stderr=fp, bufsize=0, close_fds=True)
        return p


def start_qsym_instance(app):
    print("start_qsym_instance")
    # target app
    loc_binary = os.path.join(binarise_dir, app[1])
    # print(loc_binary)
    if not os.path.isfile(loc_binary):
        return 0

    base_dir = os.path.join(output_dir, app[1])
    out_dir = os.path.join(base_dir, "out")
    fuzzer_state_path = os.path.join(out_dir, "fuzzer-slave/fuzzer_stats")
    while not os.path.exists(fuzzer_state_path):
        time.sleep(SLEEP_TIME)
    if not os.path.exists(out_dir):
        os.makedirs(out_dir)

    memory = "none"
    args = [qsym_bin]
    args += ["-a", "fuzzer-slave"]
    args += ["-o", out_dir]
    args += ["-n", "qsym"]
    args += ["--"]
    args += [loc_binary]
    args += app[2]

    outfile = os.path.join(base_dir, "qsym.log")
    print(args)
    with open(outfile, "w") as fp:
        p = subprocess.Popen(args, stdout=fp, stderr=fp, close_fds=True)
        return p


def main(argv):
    app_id = argv[1]
    use_tmin = True if len(argv) >= 3 and argv[2] == '1' else False
    app = app_map[app_id]
    if app is None:
        print("unknown app")
        return

    p = start_afl_instance(app, 0)
    time.sleep(SLEEP_TIME)
    p1 = start_afl_instance(app, 1)
    time.sleep(SLEEP_TIME)
    p2 = None
    if use_tmin:
        p2 = start_tmin_executor(app)
        time.sleep(SLEEP_TIME)
    p3 = start_qsym_instance(app)
    start_time = time.time()
    print("begin(app_id=%s)" % app_id)
    num = 0
    num1 = 0
    with open(output_dir + app[1] + "/" + "master.txt", "a+") as f:
        f.write("time,cpu_percent%,cputimes().user,cputimes().system,mem MB \n")

    with open(output_dir + app[1] + "/" + "slave.txt", "a+") as f1:
        f1.write("time,cpu_percent%,cputimes().user,cputimes().system,mem MB \n")

    '''with open("/home/zyr/hybrid-fuzzing/result/"+app[1]+"/"+"slavetwo.txt","a+") as f2:
            f2.write("time,cpu_percent%,cputimes().user,cputimes().system,mem MB,memsum MB \n")'''

    while True:
        time.sleep(SLEEP_TIME)
        mem_sum = 0.0
        with open(output_dir + app[1] + "/" + "master.txt", "a+") as f:
            current_time = time.strftime("%Y%m%d-%H%M%S", time.localtime(time.time()))
            pmaster = psutil.Process(p.pid)
            cpu_master = pmaster.cpu_percent(interval=1.0)
            mem_master = pmaster.memory_info().rss / 1024.0 / 1024.0
            mem_sum += mem_master
            line_master = current_time + ',' + str(cpu_master) + ',' + str(pmaster.cpu_times().user) + ',' + str(
                pmaster.cpu_times().system) + ',' + str(mem_master)
            f.write(line_master + '\n')

        with open(output_dir + app[1] + "/" + "slave.txt", "a+") as f1:
            current_time = time.strftime("%Y%m%d-%H%M%S", time.localtime(time.time()))
            pslave = psutil.Process(p1.pid)
            cpu_slave = pslave.cpu_percent(interval=1.0)
            mem_slave = pslave.memory_info().rss / 1024.0 / 1024.0
            mem_sum += mem_slave
            line_slave = current_time + ',' + str(cpu_slave) + ',' + str(pslave.cpu_times().user) + ',' + str(
                pslave.cpu_times().system) + ',' + str(mem_slave) + ',' + str(mem_sum)
            f1.write(line_slave + '\n')

        '''with open("/home/zyr/hybrid-fuzzing/result/"+app[1]+"/"+"slavetwo.txt","a+") as f2:
                current_time = time.strftime("%Y%m%d-%H%M%S",time.localtime(time.time()))
                pslavetwo = psutil.Process(p2.pid)
                cpu_slavetwo = pslavetwo.cpu_percent(interval = 1.0)
                mem_slavetwo = pslavetwo.memory_info().rss / 1024.0 /1024.0
                mem_sum += mem_slavetwo
                line_slavetwo = current_time + ',' + str(cpu_slavetwo) + ',' +str(pslavetwo.cpu_times().user)+','+str(pslavetwo.cpu_times().system)  + ',' + str(mem_slavetwo) + ',' + str(mem_sum)
                f2.write(line_slavetwo + '\n')'''

        running_time = time.time()

        if (running_time - start_time) >= TIME_LIMIT:
            if p.poll() is None:
                parent = psutil.Process(p.pid)
                for child in parent.children(recursive=True):
                    try:
                        child.kill()
                    except:
                        pass
                parent.kill()
            if p1.poll() is None:
                parent = psutil.Process(p1.pid)
                for child in parent.children(recursive=True):
                    try:
                        child.kill()
                    except:
                        pass
                parent.kill()
            if p2 is not None and p2.poll() is None:
                parent = psutil.Process(p2.pid)
                for child in parent.children(recursive=True):
                    try:
                        child.kill()
                    except:
                        pass
                parent.kill()
            if p3.poll() is None:
                parent = psutil.Process(p3.pid)
                for child in parent.children(recursive=True):
                    try:
                        child.kill()
                    except:
                        pass
                parent.kill()
            # f.close()
            # f1.close()
            # f2.close()
            break

    print("All done(app_id=%s)!" % app_id)


if __name__ == "__main__":
    main(sys.argv)
