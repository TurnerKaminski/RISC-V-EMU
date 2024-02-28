# M151M Project #1 : Pipelined RISC-V CPU Simulator

**Description**:

In this project, you will design a RISC-V 5-stage pipelined processor using
C++. 

The ISA of this processor is the standard RV32-I base extension.

The provided source code contains an incomplete implementation of the CPU.
The processor implementation consists of two parts:
* The CPU emulator: decode and execute instructions to generate instruction trace.
* The CPU simulator: Use the instruction trace to perform cycle-level simulation.

You are responsible for completing the code of the emulator and simulator such that you can successfully execute the provided tests on the processor.

The project is divided into 3 parts:

## Part 1 : Decode 
In this part, you will complete the implementation of the emulator decode logic.
Refer to **TODO** entries inside decode.cpp for where you may make your changes

## Part 2 : Execute
In this part, you will complete the implementation of the emulator execute logic.
Refer to **TODO** entries inside execute.cpp for where you may make your changes

## Part 3 : Timing Simulation
In this part, you will complete the implementation of the simulator logic.
You need to resolve data and control hazards correctly inside the simulator such that the resulting timing is accurate.
* Resolving data hazard: You will simulate data forwarding support in the timing simulator by stalling the pipeline for the case(s) where data forwarding is not possible.
* Resolving control hazard: You will simply stall the pipeline when you encounter a control hazard.

Refer to **TODO** entries inside core.cpp for where you may make your changes. We also recommend modifying pipeline_state class to include states you need for controlling your pipeline.

## Testing your code
The provided Makefile contains a `test` command to execute all provided tests.

    $ make test

All tests are under the /tests/ folder.
You can execute an individual test by running:

    $ ./tinyrv -s tests/rv32ui-p-sub.hex

If a test succeeds, you will get "PASSED!" output message.

## Debugging your code
You need to build the project with DEBUG=```LEVEL``` where level varies from 0 to 5.
That will turn on the debug trace inside the code and show you what the processor is doing and some of its internal states. We also encourage looking at the test ``*.dump`` files to see the actual assembly code (e.g. rv32ui-p-sub.dump).

    $ make clean
    $ DEBUG=3 make
    $ ./tinyrv -s tests/rv32ui-p-sub.hex

## What to submit
**A zip file of your source code. 
When done with your changes, execute ```make submit``` to generate the submission.zip. Do not use another method for creating the zip file.

    $ make submit

Please submit the **submission.zip** file.

## Build instructions
* run ```make``` at the root of the project to build the simulator

        $ make

* A linux development environment is needed to build the project
* We recommend using a Ubuntu 18.04 or above distribution
* This C++ project requires C++ 11 to compile and it should come installed on Ubuntu 18.04 or above

## Guidelines
* Do not modify the following files: Makefile, main.cpp
* Do not modify the following directories: common
* If needed it is ok to add new files to the code or modify other parts of the codebase.
* Do not remove an existing file from the project.
* Do not change the Makefile, you can always add a new file as a header. Make sure to include the new file under the **src/** folder.

## Grading
* simulator compiles successfully: 2 pts
* simulator passes all tests: 8 pts
* simulator timing is correct: 5 pts
Based on the coverage of test suites, you will get partial scores.

***Please do not procrastinate.***  

## Understanding the codebase
The codebase contains the following important files:
- Makefile: your build makefile
- config.h: the processor configuration file
- debug.h: the application debugging layer
- main.cpp: implements the application's main() entry point where the command line is parsed and the processor class is instantiated. This is also where the simulation loop is executed.
- processor.cpp: implements the processor class which contains a single core.
- core.cpp: implements the CPU simulator pipeline.
- decode.cpp: implements the emulator's instruction decode
- execute.cpp: implements emulator's instruction execution
- instr.h: implements the emulator's decoded instruction class
- pipeline.h: implements the simulator pipeline state and pipeline latches
- type.h: implements processor data structures
- common/*: implements utility libraries

When the application execute, the ```Core::tick()``` function is invoked every cycle and that is where the 5 stages of the pipeline execute. The ```if_stage()``` invokes the emulator's ```Emulator::step()``` function to obtain the current pipeline trace to simulate. Internally, the emulator ```Emulator::step()``` function will invoke ```Emulator::decode()```, and then ```Emulator::execute()``` to construct the pipeline trace.

The default implementation of the simulator is to simply forward the instruction data (```pipeline_trace```) to next stage without any stall. The test program will PASSED, but the timing will be incorrect since this design ignores stalls caused by data and control hazards.

## Resources
* RISC-V ISA Specs: https://riscv.org/wp-content/uploads/2017/05/riscv-spec-v2.2.pdf
* Class materials.

## Q & A's
Q1: How do I stall the simulator pipeline?

A1: Stalling the pipeline simply means not sending its pipeline trace to the next stage (skipping a cycle). 

Q2: How do I flush or invalidate an instruction in a specific stage?

A2: You can add a valid bit to the ```pipeline_trace``` and use that to discard the trace when a stage is processing it, or even better you can simply remove the ```pipeline_trace``` from its ```pipeline_latch```.

Q3: How do I decode an instruction in ```Emulate::decode()```?

A3: For each of the ```TODO``` sections, you need to update the ```instr``` instance by setting its following members depending of the instruction type: addSrcReg(), addSrcReg(), setFunc3(), setImm(). Note that for "Opcode::I_INST:" only setImm() is needed as the other fields have been set already.

Q4: How do I decode an immediate in ```Emulate::decode()```?

A4: To decode an immediate value, you need to extract bits from the instruction code by applying the correct bit shifting ans masking.
You could use the following formula: instr[B:A] = (code >> A) & ((1 << (B-A+1))-1); e.g. instr[30:25] = (code >> 25) & ((1 << (30-25+1))-1) = (code >> 25) & 0x3F; instr[7] = instr[7:7] = (code >> 7) & ((1 << (7-7+1))-1) = (code >> 7) & 0x1;

Q5: How do I perform a signed or unsigned compare in ```Emulator::execute()```?

A5: The codebase defines reg_data_t as a union of multiple types that include a signed member (.i) and also an unsigned member (.u).

Q6: How do I use the ```dump``` files to debug my code?

A6: The program instructions are loaded to address 0x80000000, which is also the starting value of the program counter (PC).
You can open the dump file and search for "80000000 <_start>:" to see the first instruction that should be executed.
follows the debugging instructions above to verify the each instruction is executing correctly.

Q7: Which development environment can I use to work on the project.
A7: The project requires a Linux environment with GCC 4.8.1 minimum.
You could work on your PC and install Ubuntu via WSL 2.0 (https://learn.microsoft.com/en-us/windows/wsl/install).
You should also be able to use the UCLA SEASNET server to work on the project. 
We recommend using the Microsoft Visual Studio Code (VSCode) as IDE if possible. 
VSCode supports remote development via SSH (https://code.visualstudio.com/docs/remote/ssh) and best, you can also do interactive debugging of C++ code (https://code.visualstudio.com/docs/cpp/cpp-debug).