#!/usr/bin/env python

import os
import sys
import shutil
import functools
import time
import subprocess

SLEEP_TIME = 10

def get_score(testcase):
    # New coverage is the best
    score1 = testcase.endswith("+cov")
    # NOTE: seed files are not marked with "+cov"
    # even though it contains new coverage
    score2 = "orig:" in testcase
    # Smaller size is better
    score3 = -os.path.getsize(testcase)
    # Since name contains id, so later generated one will be chosen earlier
    score4 = testcase
    return score1, score2, score3, score4


def testcase_compare(a, b):
    a_score = get_score(a)
    b_score = get_score(b)
    return 1 if a_score > b_score else -1


class TminExecutor:
    def __init__(self, cmd, root_dir, dissinput_root):
        self.cmd = cmd
        self.root_dir = root_dir
        self.slave_afl_dir = os.path.join(root_dir, "fuzzer-slave")
        self.trim_in_dir = os.path.join(self.slave_afl_dir, "queue")
        self.trim_out_dir = os.path.join(self.slave_afl_dir, "trim_queue")
        self.afl_bitmap = os.path.join(self.slave_afl_dir, "fuzz_bitmap")
        self.dissinput_bitmap = os.path.join(root_dir, "tmin/bitmap")
        self.trim_new_cov_out_dir = os.path.join(root_dir, "tmin/queue")
        if not os.path.exists(self.trim_new_cov_out_dir):
            os.makedirs(self.trim_new_cov_out_dir)
        if not os.path.exists(self.trim_out_dir):
            os.makedirs(self.trim_out_dir)
        self.done = set()
        self.dissinput_root = dissinput_root
        self.tmin_path = os.path.join(dissinput_root, "dissinput-tmin")
        self.afl_bitmap_update_time = 0

    def merge_bitmap(self):
        if not os.path.exists(self.dissinput_bitmap):
            with open(self.afl_bitmap, "rb") as f:
                afl_bitmap = f.read()
            merge_bitmap = b''
            for i in range(0, 65536):
                if not ord(afl_bitmap[i]) == 255:
                    merge_bitmap = merge_bitmap + b'\x01'
                else:
                    merge_bitmap = merge_bitmap + b'\x00'
            with open(self.dissinput_bitmap, "wb") as f:
                f.write(merge_bitmap)
            return
        else:
            with open(self.dissinput_bitmap, "rb") as f:
                dissinput_bitmap = f.read()
            with open(self.afl_bitmap, "rb") as f:
                afl_bitmap = f.read()
            merge_bitmap = b''
            for i in range(0, 65536):
                if not ord(afl_bitmap[i]) == 255 or not ord(dissinput_bitmap[i]) == 0:
                    merge_bitmap = merge_bitmap + b'\x01'
                else:
                    merge_bitmap = merge_bitmap + b'\x00'
            with open(self.dissinput_bitmap, "wb") as f:
                f.write(merge_bitmap)

    def sync_files(self):
        files = []
        for name in os.listdir(self.trim_in_dir):
            path = os.path.join(self.trim_in_dir, name)
            if os.path.isfile(path):
                files.append(path)
        files = list(set(files) - self.done)
        return sorted(files,
                      key=functools.cmp_to_key(testcase_compare),
                      reverse=True)

    def run(self):
        while not os.path.exists(self.afl_bitmap):
            time.sleep(SLEEP_TIME)
        while True:
            files = self.sync_files()

            if not files:
                print("Sleep for getting files")
                time.sleep(5)
                continue

            for fp in files:
                self.run_file(fp)
                break

    def run_file(self, fp):
        bitmap_update_time = os.path.getmtime(self.afl_bitmap)
        if bitmap_update_time > self.afl_bitmap_update_time:
            self.merge_bitmap()
            self.afl_bitmap_update_time = bitmap_update_time

        new_fp = os.path.join(self.trim_out_dir, os.path.basename(fp))
        tmin_cmd = [self.tmin_path, "-i", fp, "-o", new_fp, "-B", self.dissinput_bitmap, "-O", self.trim_new_cov_out_dir, "-e", "--"]
        tmin_cmd.extend(self.cmd)

        print("Run tmin: cmd=%s" % tmin_cmd)
        start = time.time()
        p = subprocess.Popen(tmin_cmd, bufsize=0, cwd=self.dissinput_root)
        tmin_ret = p.wait()
        print("Run finish time=%s (fp=%s)" % (str(time.time() - start), fp))
        if tmin_ret != 0 or not os.path.exists(new_fp):
            shutil.copy2(fp, new_fp)
        self.done.add(fp)


def main(argv):
    app_cmd = []
    for i in range(3, len(argv)):
        app_cmd += [argv[i]]
    TminExecutor(app_cmd, argv[1], argv[2]).run()


if __name__ == "__main__":
    main(sys.argv)
