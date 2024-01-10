#include "ins_unitary_op.h"
#include "ins_helper.h"

/* threads context */
extern thread_ctx_t *threads_ctx;

static void PIN_FAST_ANALYSIS_CALL r2r_unitary_opb_u(THREADID tid,
                                                     uint32_t src) {
  RTAG[DFT_REG_RAX][0] = tag_traits::cleared_val;
  RTAG[DFT_REG_RAX][1] = tag_traits::cleared_val;
}

static void PIN_FAST_ANALYSIS_CALL r2r_unitary_opb_l(THREADID tid,
                                                     uint32_t src) {
  RTAG[DFT_REG_RAX][0] = tag_traits::cleared_val;
  RTAG[DFT_REG_RAX][1] = tag_traits::cleared_val;
}

static void PIN_FAST_ANALYSIS_CALL r2r_unitary_opw(THREADID tid, uint32_t src) {
  RTAG[DFT_REG_RDX][0] = tag_traits::cleared_val;
  RTAG[DFT_REG_RDX][1] = tag_traits::cleared_val;

  RTAG[DFT_REG_RAX][0] = tag_traits::cleared_val;
  RTAG[DFT_REG_RAX][1] = tag_traits::cleared_val;
}

static void PIN_FAST_ANALYSIS_CALL r2r_unitary_opq(THREADID tid, uint32_t src) {
  for (size_t i = 0; i < 8; i++) {
    RTAG[DFT_REG_RDX][i] = tag_traits::cleared_val;
    RTAG[DFT_REG_RAX][i] = tag_traits::cleared_val;
  }
}

static void PIN_FAST_ANALYSIS_CALL r2r_unitary_opl(THREADID tid, uint32_t src) {
  for (size_t i = 0; i < 4; i++) {
    RTAG[DFT_REG_RDX][i] = tag_traits::cleared_val;
    RTAG[DFT_REG_RAX][i] = tag_traits::cleared_val;
  }
}

static void PIN_FAST_ANALYSIS_CALL m2r_unitary_opb(THREADID tid, ADDRINT src) {
  RTAG[DFT_REG_RAX][0] = tag_traits::cleared_val;
  RTAG[DFT_REG_RAX][1] = tag_traits::cleared_val;
}

static void PIN_FAST_ANALYSIS_CALL m2r_unitary_opw(THREADID tid, ADDRINT src) {
  for (size_t i = 0; i < 2; i++) {
    RTAG[DFT_REG_RDX][i] = tag_traits::cleared_val;
    RTAG[DFT_REG_RAX][i] = tag_traits::cleared_val;
  }
}

static void PIN_FAST_ANALYSIS_CALL m2r_unitary_opq(THREADID tid, ADDRINT src) {
  for (size_t i = 0; i < 8; i++) {
    RTAG[DFT_REG_RDX][i] = tag_traits::cleared_val;
    RTAG[DFT_REG_RAX][i] = tag_traits::cleared_val;
  }
}

static void PIN_FAST_ANALYSIS_CALL m2r_unitary_opl(THREADID tid, ADDRINT src) {
  for (size_t i = 0; i < 4; i++) {
    RTAG[DFT_REG_RDX][i] = tag_traits::cleared_val;
    RTAG[DFT_REG_RAX][i] = tag_traits::cleared_val;
  }
}

void ins_unitary_op(INS ins) {
  if (INS_OperandIsMemory(ins, OP_0))
    switch (INS_MemoryWriteSize(ins)) {
    case BIT2BYTE(MEM_64BIT_LEN):
      M_CALL_R(m2r_unitary_opq);
      break;
    case BIT2BYTE(MEM_LONG_LEN):
      M_CALL_R(m2r_unitary_opl);
      break;
    case BIT2BYTE(MEM_WORD_LEN):
      M_CALL_R(m2r_unitary_opw);
      break;
    case BIT2BYTE(MEM_BYTE_LEN):
    default:
      M_CALL_R(m2r_unitary_opb);
      break;
    }
  else {
    REG reg_src = INS_OperandReg(ins, OP_0);
    if (REG_is_gr64(reg_src))
      R_CALL(r2r_unitary_opq, reg_src);
    else if (REG_is_gr32(reg_src))
      R_CALL(r2r_unitary_opl, reg_src);
    else if (REG_is_gr16(reg_src))
      R_CALL(r2r_unitary_opw, reg_src);
    else if (REG_is_Upper8(reg_src))
      R_CALL(r2r_unitary_opb_u, reg_src);
    else
      R_CALL(r2r_unitary_opb_l, reg_src);
  }
}