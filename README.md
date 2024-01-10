This repository is the implementation of paper "Grammar-aware Test Case Trimming for Efficient Hybrid Fuzzing".


# install DISSInput
```bash
#!/bin/bash
set -eux
PREFIX=${PREFIX:-${HOME}}

TAR_NAME="pin-3.26-98690-g1fc9d60e6-gcc-linux"

wget https://software.intel.com/sites/landingpage/pintool/downloads/${TAR_NAME}.tar.gz
tar -C ${PREFIX} -xzf ${TAR_NAME}.tar.gz
rm ${TAR_NAME}.tar.gz

set +x
echo "Please set:"
echo "export PIN_ROOT=${PREFIX}/${TAR_NAME}"

make clean && make
```

# Usage
```bash
    python afl_run.py 1
```