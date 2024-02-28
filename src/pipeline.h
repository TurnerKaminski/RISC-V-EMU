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


#pragma once

#include <memory>
#include <iostream>
#include <util.h>
#include "types.h"
#include "debug.h"

namespace tinyrv {

class ITraceData {
public:
    using Ptr = std::shared_ptr<ITraceData>;
    ITraceData() {}
    virtual ~ITraceData() {}
};

struct LsuTraceData : public ITraceData {
  using Ptr = std::shared_ptr<LsuTraceData>;
  mem_addr_size_t mem_addrs;
};

struct pipeline_trace_t {
public:
  //--
  const uint64_t uuid;
  
  //--
  Word        PC;

  //--
  uint32_t    rdest;
  RegType     rdest_type;
  bool        wb;

  //--
  // Turner
  uint32_t    src1;
  uint32_t    src2;
  RegMask     used_regs;

  //- 
  ExeType     exe_type; 

  //--
  union {
    uint32_t unit_type;
    LsuType  lsu_type;
    AluType  alu_type;
    CSRType  csr_type;
  };

  ITraceData::Ptr data;

  pipeline_trace_t(uint64_t uuid) 
    : uuid(uuid)
    , PC(0)    
    , rdest(0)
    , rdest_type(RegType::None)
    , wb(false)
    // Turner
    , src1(0)
    , src2(0)
    , used_regs(0)
    , exe_type(ExeType::ALU)
    , unit_type(0)
    , data(nullptr)
  {}

  pipeline_trace_t(const pipeline_trace_t& rhs) 
    : uuid(rhs.uuid)
    , PC(rhs.PC)    
    , rdest(rhs.rdest)
    , rdest_type(rhs.rdest_type)
    , wb(rhs.wb)    
    // Turner
    , src1(rhs.src1)
    , src2(rhs.src2)
    , used_regs(rhs.used_regs)
    , exe_type(rhs.exe_type)
    , unit_type(rhs.unit_type)
    , data(rhs.data)
  {}
  
  ~pipeline_trace_t() {}
};

inline std::ostream &operator<<(std::ostream &os, const pipeline_trace_t& state) {
  os << "PC=0x" << std::hex << state.PC;
  os << ", wb=" << state.wb;
  if (state.wb) {
     os << ", rd=" << state.rdest_type << std::dec << state.rdest;
  }
  os << ", ex=" << state.exe_type;
  os << " (#" << std::dec << state.uuid << ")";
  return os;
}

///////////////////////////////////////////////////////////////////////////////

class PipelineLatch {
public:
  PipelineLatch() {}
  ~PipelineLatch() {}
  
  bool empty() const {
    return queue_.empty();
  }

  pipeline_trace_t* front() {
    return queue_.front();
  }

  pipeline_trace_t* back() {
    return queue_.back();
  }

  void push(pipeline_trace_t* value) {    
    queue_.push(value);
  }

  void pop() {
    queue_.pop();
  }

  void clear() {
    std::queue<pipeline_trace_t*> empty;
    std::swap(queue_, empty );
  }

protected:
  std::queue<pipeline_trace_t*> queue_;
};

}