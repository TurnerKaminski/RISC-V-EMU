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
#include <string>
#include <stdlib.h>
#include <string.h>
#include <iomanip>
#include <vector>
#include <unordered_map>
#include <util.h>
#include "debug.h"
#include "types.h"
#include "emulator.h"
#include "instr.h"

using namespace tinyrv;

namespace tinyrv {

static const std::unordered_map<Opcode, InstType> sc_instTable = {
  {Opcode::R_INST,     InstType::R_TYPE},
  {Opcode::L_INST,     InstType::I_TYPE},
  {Opcode::I_INST,     InstType::I_TYPE},
  {Opcode::S_INST,     InstType::S_TYPE},
  {Opcode::B_INST,     InstType::B_TYPE},
  {Opcode::LUI_INST,   InstType::U_TYPE},
  {Opcode::AUIPC_INST, InstType::U_TYPE},
  {Opcode::JAL_INST,   InstType::J_TYPE},
  {Opcode::JALR_INST,  InstType::I_TYPE},
  {Opcode::SYS_INST,   InstType::I_TYPE},
  {Opcode::FENCE,      InstType::I_TYPE},
};

enum Constants {
  width_opcode= 7,
  width_reg   = 5,
  width_func3 = 3,
  width_func7 = 7,
  width_mop   = 3,
  width_i_imm = 12,
  width_j_imm = 20,
  width_v_imm = 11,
  width_aq    = 1,
  width_rl    = 1,

  shift_opcode= 0,
  shift_rd    = width_opcode,
  shift_func3 = shift_rd + width_reg,
  shift_rs1   = shift_func3 + width_func3,
  shift_rs2   = shift_rs1 + width_reg,
  shift_func2 = shift_rs2 + width_reg,
  shift_func7 = shift_rs2 + width_reg,

  mask_opcode = (1 << width_opcode) - 1,  
  mask_reg    = (1 << width_reg)   - 1,
  mask_func3  = (1 << width_func3) - 1,
  mask_func7  = (1 << width_func7) - 1,
  mask_i_imm  = (1 << width_i_imm) - 1,
  mask_j_imm  = (1 << width_j_imm) - 1,
  mask_v_imm  = (1 << width_v_imm) - 1,
};

static const char* op_string(const Instr &instr) {
  auto opcode = instr.getOpcode();
  auto func3  = instr.getFunc3();
  auto func7  = instr.getFunc7();
  auto imm    = instr.getImm();

  switch (opcode) {
  case Opcode::LUI_INST:   return "LUI";
  case Opcode::AUIPC_INST: return "AUIPC";
  case Opcode::R_INST:
    if (func7 & 0x1) {
      switch (func3) {
      case 0: return "MUL";
      case 1: return "MULH";
      case 2: return "MULHSU";
      case 3: return "MULHU";
      case 4: return "DIV";
      case 5: return "DIVU";
      case 6: return "REM";
      case 7: return "REMU";
      default:
        std::abort();
      }
    } else {
      switch (func3) {
      case 0: return func7 ? "SUB" : "ADD";
      case 1: return "SLL";
      case 2: return "SLT";
      case 3: return "SLTU";
      case 4: return "XOR";
      case 5: return (func7 & 0x20) ? "SRA" : "SRL";
      case 6: return "OR";
      case 7: return "AND";
      default:
        std::abort();
      }
    }
  case Opcode::I_INST:
    switch (func3) {
    case 0: return "ADDI";
    case 1: return "SLLI";
    case 2: return "SLTI";
    case 3: return "SLTIU";
    case 4: return "XORI";
    case 5: return (func7 & 0x20) ? "SRAI" : "SRLI";
    case 6: return "ORI";
    case 7: return "ANDI";
    default:
      std::abort();
    }  
  case Opcode::B_INST:
    switch (func3) {
    case 0: return "BEQ";
    case 1: return "BNE";
    case 4: return "BLT";
    case 5: return "BGE";
    case 6: return "BLTU";
    case 7: return "BGEU";
    default:
      std::abort();
    }
  case Opcode::JAL_INST:   return "JAL";
  case Opcode::JALR_INST:  return "JALR";
  case Opcode::L_INST:
    switch (func3) {
    case 0: return "LB";
    case 1: return "LH";
    case 2: return "LW";
    case 3: return "LD";
    case 4: return "LBU";
    case 5: return "LHU";
    case 6: return "LWU";
    default:
      std::abort();
    }
  case Opcode::S_INST:
    switch (func3) {
    case 0: return "SB";
    case 1: return "SH";
    case 2: return "SW";
    case 3: return "SD";
    default:
      std::abort();
    }
  case Opcode::SYS_INST: 
    switch (func3) {
    case 0:
      switch (imm) {
      case 0x000: return "ECALL";
      case 0x001: return "EBREAK";
      case 0x002: return "URET";
      case 0x102: return "SRET";
      case 0x302: return "MRET";
      default:
        std::abort();      
      }
    case 1: return "CSRRW";
    case 2: return "CSRRS";
    case 3: return "CSRRC";
    case 5: return "CSRRWI";
    case 6: return "CSRRSI";
    case 7: return "CSRRCI";
    default:
      std::abort();
    }
  case Opcode::FENCE: return "FENCE";
  default:
    std::abort();
  }
}

std::ostream &operator<<(std::ostream &os, const Instr &instr) {  
  auto opcode = instr.getOpcode();
  auto func3  = instr.getFunc3();

  os << op_string(instr);
  
  int sep = 0;
  if (instr.getRDType() != RegType::None) {
    if (sep++ != 0) { os << ", "; } else { os << " "; }
    os << instr.getRDType() << std::dec << instr.getRDest();
  }
  for (uint32_t i = 0; i < instr.getNRSrc(); ++i) {    
    if (instr.getRSType(i) == RegType::None)
      continue;
    if (sep++ != 0) { os << ", "; } else { os << " "; }
    os << instr.getRSType(i) << std::dec << instr.getRSrc(i);
  }
  if (instr.hasImm()) {
    if (sep++ != 0) { os << ", "; } else { os << " "; }
    os << "0x" << std::hex << instr.getImm();
  }
  if (opcode == Opcode::SYS_INST && func3 >= 5) {
    if (sep++ != 0) { os << ", "; } else { os << " "; }
    os << "0x" << std::hex << instr.getRSrc(0);
  }
  return os;
}

}

std::shared_ptr<Instr> Emulator::decode(uint32_t code) const {  
  auto instr = std::make_shared<Instr>();
  auto op = Opcode((code >> shift_opcode) & mask_opcode);
  instr->setOpcode(op);

  auto func3 = (code >> shift_func3) & mask_func3;
  auto func7 = (code >> shift_func7) & mask_func7;

  auto rd  = (code >> shift_rd)  & mask_reg;
  auto rs1 = (code >> shift_rs1) & mask_reg;
  auto rs2 = (code >> shift_rs2) & mask_reg;

  auto op_it = sc_instTable.find(op);
  if (op_it == sc_instTable.end()) {
    std::cout << std::hex << "Error: invalid opcode: 0x" << op << std::endl;
    return nullptr;
  }

  auto iType = op_it->second;

  switch (iType) {
  case InstType::R_TYPE:
    switch (op) {
    default:
      instr->setDestReg(rd, RegType::Integer);
      instr->addSrcReg(rs1, RegType::Integer);
      instr->addSrcReg(rs2, RegType::Integer);
      break;
    }
    instr->setFunc3(func3);
    instr->setFunc7(func7);
    break;

  case InstType::I_TYPE: {
    instr->setDestReg(rd, RegType::Integer);
    instr->addSrcReg(rs1, RegType::Integer);    
    instr->setFunc3(func3);
    instr->setFunc7(func7);
    switch (op) {
    case Opcode::SYS_INST:
      if (func3 != 0) {
        // RV32I: CSR
        if (func3 >= 5) {
          // rs1 holds zimm
          instr->setSrcReg(0, rs1, RegType::None);
        }        
      } else {        
        instr->setDestReg(rd, RegType::None);
        instr->setSrcReg(0, rs1, RegType::None);
      }      
      // uint12
      instr->setImm(code >> shift_rs2);
      break;    
    case Opcode::FENCE:
      // uint12
      instr->setImm(code >> shift_rs2);
      instr->setDestReg(rd, RegType::None);
      instr->setSrcReg(0, rs1, RegType::None);
      break;
    case Opcode::I_INST:
      // TODO:
      instr->setImm((int32_t)code >> 20);
      break;
    default:
      // int12
      auto imm = code >> shift_rs2;
      instr->setImm(sext(imm, width_i_imm));
      break;
    }
  } break;

  case InstType::S_TYPE: {    
    // TODO:
    instr->setDestReg(rd, RegType::None);
    instr->addSrcReg(rs1, RegType::Integer);
    instr->addSrcReg(rs2, RegType::Integer);
    instr->setFunc3(func3);
    auto imm = ((code >> 7) & 0x1F) | ((((int32_t)code) >> 25) << 5);
    instr->setImm(imm);
  } break;

  case InstType::B_TYPE: {
    // TODO:
    instr->addSrcReg(rs1, RegType::Integer);
    instr->addSrcReg(rs2, RegType::Integer);
    instr->setFunc3(func3);
    instr->setImm((((code >> 8) & 0xF) | (((code >> 25) & 0x3F) << 4) | (((code >> 7) & 0x1) << 10) | ((((int32_t)code) >> 31)) << 11) << 1);
    
  } break;

  case InstType::U_TYPE: {
    // TODO:
    instr->setDestReg(rd, RegType::Integer);
    instr->setFunc3(func3);
    instr->setFunc7(func7);
    instr->setImm((code & 0xFFFFF000) >> 12); 
  } break;

  case InstType::J_TYPE: {
    // TODO:
    instr->setDestReg(rd, RegType::Integer);
    instr->setImm((((code >> 21) & 0x3FF) | (((code >> 20) & 0x1) << 10) | (((code >> 12) & 0xFF) << 11) | ((((int32_t)code) >> 31)) << 19) << 1);
  } break;   

  default:
    std::abort();
  }

  return instr;
}