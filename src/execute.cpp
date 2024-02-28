// Copyright © 2019-2023
// 
// Licensed under the Apache License;
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <bitset>
#include <climits>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <util.h>
#include "instr.h"
#include "core.h"
#include "emulator.h"

using namespace tinyrv;

union reg_data_t {
  Word     u;
  WordI    i;
  WordF    f;
  float    f32;
  double   f64;
  uint32_t u32;
  uint64_t u64; 
  int32_t  i32;
  int64_t  i64;
};

void Emulator::execute(const Instr &instr, pipeline_trace_t *trace) {
  auto next_pc = PC_ + 4;

  auto func3  = instr.getFunc3();
  auto func7  = instr.getFunc7();
  auto opcode = instr.getOpcode();
  auto rdest  = instr.getRDest();
  auto rsrc0  = instr.getRSrc(0);
  auto rsrc1  = instr.getRSrc(1);
  auto immsrc = sext((Word)instr.getImm(), 32);

  // Register File access
  reg_data_t rsdata [2];
  auto num_rsrcs = instr.getNRSrc();
  if (num_rsrcs) {       
    for (uint32_t i = 0; i < num_rsrcs; ++i) {          
      auto type = instr.getRSType(i);
      auto reg = instr.getRSrc(i);        
      switch (type) {
      case RegType::Integer: 
        rsdata[i].u = reg_file_[reg];
        DPH(2, "Src" << std::dec << i << " Reg: " << type << std::dec << reg << "={0x" << std::hex << rsdata[i].i << "}" << std::endl);
        break;
      case RegType::None:
        break;
      default:
        std::abort();
      }      
    }
  }

  reg_data_t rddata;
  bool rd_write = false;
  
  switch (opcode) {  
  case LUI_INST: {
    // RV32I: LUI
    trace->exe_type = ExeType::ALU;
    trace->alu_type = AluType::ARITH;
    rddata.i = immsrc << 12;
    rd_write = true;
    break;
  }  
  case AUIPC_INST: {
    // RV32I: AUIPC
    trace->exe_type = ExeType::ALU;
    trace->alu_type = AluType::ARITH;
    rddata.i = (immsrc << 12) + PC_;
    rd_write = true;
    break;
  }
  case R_INST: {
    trace->exe_type = ExeType::ALU;    
    trace->alu_type = AluType::ARITH;
    trace->used_regs.set(rsrc0);
    trace->used_regs.set(rsrc1);
    if (func7 & 0x1) {
      switch (func3) {
      case 0: {
        // RV32M: MUL
        rddata.i = rsdata[0].i * rsdata[1].i;
        trace->alu_type = AluType::IMUL;
        break;
      }
      case 1: {
        // RV32M: MULH
        auto first = static_cast<DWordI>(rsdata[0].i);
        auto second = static_cast<DWordI>(rsdata[1].i);
        rddata.i = (first * second) >> XLEN;
        trace->alu_type = AluType::IMUL;
        break;
      }
      case 2: {
        // RV32M: MULHSU       
        auto first = static_cast<DWordI>(rsdata[0].i);
        auto second = static_cast<DWord>(rsdata[1].u);
        rddata.i = (first * second) >> XLEN;
        trace->alu_type = AluType::IMUL;
        break;
      } 
      case 3: {
        // RV32M: MULHU
        auto first = static_cast<DWord>(rsdata[0].u);
        auto second = static_cast<DWord>(rsdata[1].u);
        rddata.i = (first * second) >> XLEN;
        trace->alu_type = AluType::IMUL;
        break;
      } 
      case 4: {
        // RV32M: DIV
        auto dividen = rsdata[0].i;
        auto divisor = rsdata[1].i; 
        auto largest_negative = WordI(1) << (XLEN-1);  
        if (divisor == 0) {
          rddata.i = -1;
        } else if (dividen == largest_negative && divisor == -1) {
          rddata.i = dividen;
        } else {
          rddata.i = dividen / divisor;
        }
        trace->alu_type = AluType::IDIV;
        break;
      } 
      case 5: {
        // RV32M: DIVU
        auto dividen = rsdata[0].u;
        auto divisor = rsdata[1].u;
        if (divisor == 0) {
          rddata.i = -1;
        } else {
          rddata.i = dividen / divisor;
        }
        trace->alu_type = AluType::IDIV;
        break;
      } 
      case 6: {
        // RV32M: REM
        auto dividen = rsdata[0].i;
        auto divisor = rsdata[1].i;
        auto largest_negative = WordI(1) << (XLEN-1);
        if (rsdata[1].i == 0) {
          rddata.i = dividen;
        } else if (dividen == largest_negative && divisor == -1) {
          rddata.i = 0;
        } else {
          rddata.i = dividen % divisor;
        }
        trace->alu_type = AluType::IDIV;
        break;
      } 
      case 7: {
        // RV32M: REMU
        auto dividen = rsdata[0].u;
        auto divisor = rsdata[1].u;
        if (rsdata[1].i == 0) {
          rddata.i = dividen;
        } else {
          rddata.i = dividen % divisor;
        }
        trace->alu_type = AluType::IDIV;
        break;
      } 
      default:
        std::abort();
      }
    } else {
      switch (func3) {
      case 0: {
        if (func7) {
          // RV32I: SUB
          rddata.i = rsdata[0].i - rsdata[1].i;
        } else {
          // RV32I: ADD
          rddata.i = rsdata[0].i + rsdata[1].i;
        }
        break;
      }
      case 1: {
        // RV32I: SLL
        // TODO: rddata.i = ?
        rddata.i = rsdata[0].i << (rsdata[1].i & 0x1F);
        break;
      }
      case 2: {
        // RV32I: SLT
        // TODO: rddata.i = ?
        rddata.i = (int32_t)rsdata[0].i < (int32_t)rsdata[1].i;
        break;
      }
      case 3: {
        // RV32I: SLTU
        // TODO: rddata.i = ?
        rddata.i = rsdata[0].u32 < rsdata[1].u32;
        break;
      }
      case 4: {
        // RV32I: XOR
        // TODO: rddata.i = ?
        // rddata.i = rsrc0 ^ rsrc1
        rddata.i = rsdata[0].i ^ rsdata[1].i;
        break;
      }
      case 5: {
        if (func7) {
          // RV32I: SRA
          // TODO: rddata.i = ?
          rddata.i = rsdata[0].i32 >> (int32_t)(rsdata[1].i & 0x1F);
        } else {
          // RV32I: SRL
          // TODO: rddata.i = ?
          rddata.i = rsdata[0].u >> rsdata[1].u;
        }
        break;
      }
      case 6: {
        // RV32I: OR
        // TODO: rddata.i = ?
        rddata.i = rsdata[0].i | rsdata[1].i;
        break;
      }
      case 7: {
        // RV32I: AND
        // TODO: rddata.i = ?
        rddata.i = rsdata[0].i & rsdata[1].i;
        break;
      }
      default:
        std::abort();
      }
    }    
    rd_write = true;
    break;
  }
  case I_INST: {
    trace->exe_type = ExeType::ALU;    
    trace->alu_type = AluType::ARITH;    
    trace->used_regs.set(rsrc0);
    switch (func3) {
    case 0: {
      // RV32I: ADDI
      // TODO: rddata.i = ?
      rddata.i = (rsdata[0].i32 + immsrc);
      break;
    }
    case 1: {
      // RV32I: SLLI
      // TODO: rddata.i = ?
      uint32_t shamt = immsrc & 0x1F;
      uint32_t shtyp = (immsrc >> 5) & 0x7F;
      if (shtyp != 0){
        break;
      }
      rddata.i = (int32_t)(rsdata[0].i << shamt);
      break;
    }
    case 2: {
      // RV32I: SLTI
      // TODO: rddata.i = ?
      rddata.i = rsdata[0].i32 < (int32_t)immsrc;
      break;
    }
    case 3: {
      // RV32I: SLTIU
      // TODO: rddata.i = ?
      rddata.i = rsdata[0].u32 < (uint32_t)immsrc;
      break;
    } 
    case 4: {
      // RV32I: XORI
      // TODO: rddata.i = ?
      rddata.i = rsdata[0].i ^ immsrc;
      break;
    }
    case 5: {
      uint32_t shamt = immsrc & 0x1F;
			uint32_t shtyp = (immsrc >> 5) & 0x7F;
      if ((shtyp & 0x5F) != 0x0){
          break;
      }
      if (func7) {
        // RV32I: SRAI
        // TODO: rddata.i = ?
        rddata.i = rsdata[0].i32 >> (int32_t)shamt;
      } else {
        // RV32I: SRLI
        // TODO: rddata.i = ?
        rddata.i = rsdata[0].u >> shamt;
      }
      break;
    }
    case 6: {
      // RV32I: ORI
      // TODO: rddata.i = ?
      rddata.i = rsdata[0].i | immsrc;
      break;
    }
    case 7: {
      // RV32I: ANDI
      // TODO: rddata.i = ?
      rddata.i = rsdata[0].i & immsrc;
      break;
    }
    }
    rd_write = true;
    break;
  }
  case B_INST: {   
    trace->exe_type = ExeType::ALU;    
    trace->alu_type = AluType::BRANCH;    
    trace->used_regs.set(rsrc0);
    trace->used_regs.set(rsrc1);
    switch (func3) {
    case 0: {
      // RV32I: BEQ
      if (rsdata[0].i == rsdata[1].i) {
        next_pc = PC_ + immsrc;
      }
      break;
    }
    case 1: {
      // RV32I: BNE
      // TODO: next_pc = ?
      if (rsdata[0].i != rsdata[1].i){
        next_pc = PC_ + immsrc;
      }
      break;
    }
    case 4: {
      // RV32I: BLT
      // TODO: next_pc = ?
      if (rsdata[0].i32 < rsdata[1].i32){
        next_pc = PC_ + immsrc;
      }
      break;
    }
    case 5: {
      // RV32I: BGE
      // TODO: next_pc = ?
      if (rsdata[0].i32 >= rsdata[1].i32){
        next_pc = PC_ + immsrc;
      }
      break;
    }
    case 6: {
      // RV32I: BLTU
      // TODO: next_pc = ?
      if (rsdata[0].u32 < rsdata[1].u32){
        next_pc = PC_ + (uint32_t)immsrc;
      }
      break;
    }
    case 7: {
      // RV32I: BGEU
      // TODO: next_pc = ?
      if (rsdata[0].u >= rsdata[1].u){
        next_pc = PC_ + immsrc;
      }
      break;
    }
    default:
      std::abort();
    }
    break;
  }  
  case JAL_INST: {
    // RV32I: JAL
    trace->exe_type = ExeType::ALU;    
    trace->alu_type = AluType::BRANCH;
    rddata.i = PC_ + 4;
    next_pc = PC_ + immsrc;
    rd_write = true;
    break;
  }  
  case JALR_INST: {
    // RV32I: JALR
    trace->exe_type = ExeType::ALU;    
    trace->alu_type = AluType::BRANCH;
    trace->used_regs.set(rsrc0);
    // rddata.i = ?
    // TODO: next_pc = ?
    rddata.i = PC_ + 4;
    next_pc = ((rsdata[0].i32 + immsrc) >> 1) << 1;
    rd_write = true;
    break;
  }
  case L_INST: {
    trace->exe_type = ExeType::LSU;    
    trace->lsu_type = LsuType::LOAD;
    trace->used_regs.set(rsrc0);
    auto trace_data = std::make_shared<LsuTraceData>();
    trace->data = trace_data;
    uint32_t data_bytes = 1 << (func3 & 0x3);
    uint32_t data_width = 8 * data_bytes;
    uint64_t mem_addr;
    // TODO
    // mem_addr = ?
    mem_addr = rsdata[0].i + immsrc;
    uint64_t read_data = 0;
    this->dcache_read(&read_data, mem_addr, data_bytes);
    trace_data->mem_addrs = {mem_addr, data_bytes};
    switch (func3) {
    case 0: // RV32I: LB
    case 1: // RV32I: LH
      rddata.i = sext((Word)read_data, data_width);
      break;
    case 2: // RV32I: LW
      rddata.i = sext((Word)read_data, data_width);
      break;
    case 4: // RV32I: LBU
    case 5: // RV32I: LHU
      rddata.u64 = read_data;
      break;
    default:
      std::abort();      
    }
    rd_write = true;
    break;
  }
  case S_INST: {
    trace->exe_type = ExeType::LSU;    
    trace->lsu_type = LsuType::STORE;
    trace->used_regs.set(rsrc0);
    trace->used_regs.set(rsrc1);    
    auto trace_data = std::make_shared<LsuTraceData>();
    trace->data = trace_data;
    uint32_t data_bytes = 1 << (func3 & 0x3);
    uint64_t mem_addr;
    uint64_t write_data;
    // TODO
    // mem_addr = ?
    // write_data = ?
    mem_addr = rsdata[0].i + immsrc;
    write_data = rsdata[1].i32;
    trace_data->mem_addrs = {mem_addr, data_bytes};
    switch (func3) {
    case 0:
    case 1:
    case 2:
    case 3:
      this->dcache_write(&write_data, mem_addr, data_bytes);  
      break;
    default:
      std::abort();
    }
    break;
  }
  case SYS_INST: {
    uint32_t csr_addr = immsrc;
    uint32_t csr_value;
    if (func3 == 0) {
      trace->exe_type = ExeType::ALU;
      trace->alu_type = AluType::SYSCALL;
      switch (csr_addr) {
      case 0: 
        // RV32I: ECALL
        this->trigger_ecall();
        break;
      case 1: 
        // RV32I: EBREAK
        this->trigger_ebreak();
        break;
      case 0x002: // URET
      case 0x102: // SRET
      case 0x302: // MRET
        break;
      default:
        std::abort();
      }                
    } else {
      trace->exe_type = ExeType::CSR;
      csr_value = this->get_csr(csr_addr);
      switch (func3) {
      case 1: {
        // RV32I: CSRRW
        rddata.i = csr_value;
        this->set_csr(csr_addr, rsdata[0].i);      
        trace->used_regs.set(rsrc0);
        trace->csr_type = CSRType::CSRRW;
        rd_write = true;
        break;
      }
      case 2: {
        // RV32I: CSRRS
        rddata.i = csr_value;
        if (rsdata[0].i != 0) {
          this->set_csr(csr_addr, csr_value | rsdata[0].i);
        }
        trace->used_regs.set(rsrc0);
        trace->csr_type = CSRType::CSRRS;
        rd_write = true;
        break;
      }
      case 3: {
        // RV32I: CSRRC
        rddata.i = csr_value;
        if (rsdata[0].i != 0) {
          this->set_csr(csr_addr, csr_value & ~rsdata[0].i);
        }
        trace->used_regs.set(rsrc0);
        trace->csr_type = CSRType::CSRRC;
        rd_write = true;
        break;
      }
      case 5: {
        // RV32I: CSRRWI
        rddata.i = csr_value;
        this->set_csr(csr_addr, rsrc0);
        trace->csr_type = CSRType::CSRRW;    
        rd_write = true;
        break;
      }
      case 6: {
        // RV32I: CSRRSI;
        rddata.i = csr_value;
        if (rsrc0 != 0) {
          this->set_csr(csr_addr, csr_value | rsrc0);
        }
        trace->csr_type = CSRType::CSRRS;
        rd_write = true;
        break;
      }
      case 7: {
        // RV32I: CSRRCI
        rddata.i = csr_value;
        if (rsrc0 != 0) {
          this->set_csr(csr_addr, csr_value & ~rsrc0);
        }
        trace->csr_type = CSRType::CSRRC;
        rd_write = true;
        break;
      }
      default:
        break;
      }
    } 
    break;
  }   
  case FENCE: {
    // RV32I: FENCE
    trace->exe_type = ExeType::LSU;    
    trace->lsu_type = LsuType::FENCE;
    break;
  }
  default:
    std::abort();
  }

  if (rd_write) {
    trace->wb = true;
    auto type = instr.getRDType();    
    switch (type) {
    case RegType::Integer:      
      if (rdest) {
        DPH(2, "Dest Reg: " << type << std::dec << rdest << "={0x" << std::hex << rddata.i << "}" << std::endl);    
        reg_file_[rdest] = rddata.i;
        trace->used_regs[rdest] = 1;
      } else {
        // disable write to x0
        trace->wb = false;
      }
      break;
    default:
      std::abort();
      break;
    }
  }

  // advance program counter
  PC_ += 4;
  if (PC_ != next_pc) {
    DP(3, "*** Next PC=0x" << std::hex << next_pc << std::dec);
    PC_ = next_pc;
  }
}