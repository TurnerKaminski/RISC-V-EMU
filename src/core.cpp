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
#include <iomanip>
#include <string.h>
#include <assert.h>
#include <util.h>
#include "types.h"
#include "core.h"
#include "debug.h"
#include "processor_impl.h"

using namespace tinyrv;

int counter = 0;
bool LD_flag = false;
bool stall_flag = false;
uint32_t LD_dest = 0;

Core::Core(const SimContext& ctx, uint32_t core_id, ProcessorImpl* processor)
    : SimObject(ctx, "core")
    , core_id_(core_id)
    , processor_(processor)
    , emulator_(this)
{
  this->reset();
}

Core::~Core() {}

void Core::reset() { 
  emulator_.clear();
  if_id_latch_.clear();
  id_ex_latch_.clear();
  ex_mem_latch_.clear();
  mem_wb_latch_.clear();

  fetch_stalled_ = false;
  issued_instrs_ = 0;
  perf_stats_ = PerfStats();
}

void Core::tick() {
  this->wb_stage();
  this->mem_stage();
  this->ex_stage();
  this->id_stage();
  this->if_stage();

  ++perf_stats_.cycles;
  DPN(2, std::flush);  
}

void Core::if_stage() {
  if (stall_flag)
  {
    std::cout<<"fetch-stall\n";
    stall_flag = false;
    return;
  }
  if (counter > 0){
    counter -= 1;
    fetch_stalled_ = true;
  }
  else{
    fetch_stalled_ = false;
  }
  if (fetch_stalled_){
    std::cout<<"fetch-stalled\n";
    return;
  }
  auto trace = emulator_.step();

  DT(3, "pipeline-fetch: " << *trace);

  // TODO:

  // Stall three cycles if there is a branch type execution
  if (trace->alu_type == AluType::BRANCH && trace->exe_type != ExeType::CSR)
  {
    counter = 3;
  }
  // move instruction to next stage
  if_id_latch_.push(trace);

  ++issued_instrs_;
}

void Core::id_stage() {

  if (if_id_latch_.empty())
    return;

  auto trace = if_id_latch_.front();
  
  DT(3, "pipeline-decode: " << *trace);

  // TODO:

  // Stall 1 cycle for RAW hazard

  // RAW handling
  // Check the src reg if flag is true
  if (LD_flag == true)
  {
    // only check 1 cycle
    LD_flag = false;
    // RAW hazard
    if (LD_dest == trace->src1 || LD_dest == trace->src2)
    {
      stall_flag = true;
      std::cout<<"decode-stall\n";
      return;
    }
  }
  // If the curr trace is a LD, set a flag and store the dest reg
  if (trace->lsu_type == LsuType::LOAD && trace->exe_type == ExeType::LSU){
    LD_flag = true;
    LD_dest = trace->rdest;
  }
  // move instruction to next stage
  id_ex_latch_.push(trace);

  if_id_latch_.pop();
}

void Core::ex_stage() {   
  if (id_ex_latch_.empty())
    return;

  auto trace = id_ex_latch_.front();
  
  DT(3, "pipeline-execute: " << *trace);

  // TODO:

  // move instruction to next stage
  ex_mem_latch_.push(trace);

  id_ex_latch_.pop();
}

void Core::mem_stage() {
  if (ex_mem_latch_.empty())
    return;

  auto trace = ex_mem_latch_.front();
  
  DT(3, "pipeline-memory: " << *trace);

  // TODO:

  // move instruction to next stage
  mem_wb_latch_.push(trace);

  ex_mem_latch_.pop();
}

void Core::wb_stage() {
  if (mem_wb_latch_.empty())
    return;

  auto trace = mem_wb_latch_.front();
  
  DT(3, "pipeline-writeback: " << *trace);

  // TODO:

  assert(perf_stats_.instrs <= issued_instrs_);
  ++perf_stats_.instrs;

  mem_wb_latch_.pop();

  // delete the trace
  delete trace;
}

bool Core::check_exit(Word* exitcode, bool riscv_test) const {
  return emulator_.check_exit(exitcode, riscv_test);
}

bool Core::running() const {
  return (perf_stats_.instrs != issued_instrs_);
}

void Core::attach_ram(RAM* ram) {
  emulator_.attach_ram(ram);
}

void Core::showStats() {
  std::cout << std::dec << "PERF: instrs=" << perf_stats_.instrs << ", cycles=" << perf_stats_.cycles << std::endl;
}