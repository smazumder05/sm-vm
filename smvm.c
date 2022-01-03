/**
 * A Simple virtal machine implementation, It is designed to execute simple programs
 * and can execute simple programs in a platform independent manner. This vm is a process
 * virtual machine that are designed to execute programs in a platform indepemdent manner. E.g. JVM
 * It is designed to run the LC3 (Little Computer 3 educational programming language.)
 * LC3 is less complicated than x-86 assembly but has many similar features.
 *
 * The initial version does not have the following features
 * - interrupt processing
 * - priority levels
 * - process and state registers
 * - privledged mode
 * - user and supervisor stack
 * Only the basic hardware is virtualized and the machine will interact with the outside world
 * via stdio and stdout.
 *
 * basic functional flow is as follows
 * 1) Load the program in main memory
 * 2) Keep the current instruction to be executed in the RPC register. (Inc the program counter)
 * 3) Decode the instruction to fetch opcode (first 4 bits), based on the opcode, fetch the rest of
 *    the parameters
 * 4) Execute the method associated with the opcode.
 * 5) Finally increment the program counter
 */
#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<string.h>
#include<stdbool.h>

#define NOPS (16)  //Machine will have 16 instructions

//The main memory for the machine. It is fairly limited, it won't load/run programs
//longer than UINT16_MAX = 65535
uint16_t PC_START = 0x3000; //convention is to start loading programs from mem location 0x3000
uint16_t mem[UINT16_MAX] = {0}; //Max sise is unsigned 16 bits

//Two utility functions to read and write to main memory.
static inline uint16_t mr(uint16_t address) {return mem[address];}
static inline uint16_t mw(uint16_t address, uint16_t value) {mem[address] = value;}

//This machine will have 10 registers, 16 bits each.
//R0 is a general purpose register, will be used to read/write to stdin/stdout
//R1 to R7 will be other general purpose registers.
//RPC is the program counter.
//RCND is the conditional register, stores conditional flag  about the previous conditional operation
//From an implementation perspective, we will define an enum and access a register by an array reg[r3]
enum registers {R0 =0, R1, R2, R3, R4,R5,R6, R7, PC,CND, CNT};
uint16_t reg[CNT] = {0};

/* The INSTRUCTION SET
 *  Only 14 instructions, each instruction is encoded in a 16bit word as follows;
 *  OPCODE(4bits) and then depending on the instruction the parameters are encoded in the remaining
 *  12bits.
 *  Based on the opcode we identify the instruction and figure out how to decode/extract the relevant
 *  parameters.
 *  Because OpCodes are represented in 4 bits, the maximum number of instructions we can encode is 16 (2^4=16)
 *
 * Instruction 	OpCode Hex 	OpCode Bin 	C function 	Comments
 * ------------------------------------------------------------------------------
 *  br 	0x0 	0b0000 	 void br(uint16_t i) 	Conditional branch
 *  add 	0x1 	0b0001 	void and(uint16_t i) 	Used for addition.
 *   ld 	0x2 	0b0010 	void ld(uint16_t i) 	Load RPC + offset
 *   st 	0x3 	0b0011 	void st(uint16_t i) 	Store
 *  jsr 	0x4 	0b0100 	void jsr(uint16_t i) 	Jump to subroutine
 *  and 	0x5 	0b0101 	void and(uint16_t i) 	Bitwise logical AND
 *  ldr 	0x6 	0b0110 	void ldr(uint16_t i) 	Load Base+Offset
 *  str 	0x7 	0b0111 	void str(uint16_t i) 	Store base + offset
 *  rti 	0x8 	0b1000 	void rti(uint16_t i) 	Return from interrupt (not implemented)
 *  not 	0x9 	0b1001 	void not(uint16_t i) 	Bitwise complement
 *  ldi 	0xA 	0b1010 	void ldi(uint16_t i) 	Load indirect
 *  sti 	0xB 	0b1011 	void sti(uint16_t i) 	Store indirect
 *  jmp 	0xC 	0b1100 	void jmp(uint16_t i) 	Jump/Return to subroutine
 *       	0xD 	0b1101 	  	Unused OpCode
 *  lea 	0xE 	0b1110 	void lea(uint16_t i) 	Load effective address
 *  trap 	0xF 	0b1111 	void trap(uint16_t i) 	System trap/call/
 */
#define OPC(i) ((i)>>12)

//Save all possible instructions and its associated functions  in an array. The index will represent the actual opcode
//and the value will be a pointer to  the corresponding function.
//op_ex_f op_ex[NOPS] = {
  //  br, add, ld, st, jsr, and, ldr, str, rti, not, ldi, sti, jmp, res, lea, trap
//};

/**
Instruction 	OpCode Hex 	OpCode Bin 	C function 	Comments
------------------------------------------------------------------------------
 br 	0x0 	0b0000 	 void br(uint16_t i) 	Conditional branch
 add 	0x1 	0b0001 	void and(uint16_t i) 	Used for addition.
 ld 	0x2 	0b0010 	void ld(uint16_t i) 	Load RPC + offset
 st 	0x3 	0b0011 	void st(uint16_t i) 	Store
 jsr 	0x4 	0b0100 	void jsr(uint16_t i) 	Jump to subroutine
 and 	0x5 	0b0101 	void and(uint16_t i) 	Bitwise logical AND
 ldr 	0x6 	0b0110 	void ldr(uint16_t i) 	Load Base+Offset
 str 	0x7 	0b0111 	void str(uint16_t i) 	Store base + offset
 rti 	0x8 	0b1000 	void rti(uint16_t i) 	Return from interrupt (not implemented)
 not 	0x9 	0b1001 	void not(uint16_t i) 	Bitwise complement
 ldi 	0xA 	0b1010 	void ldi(uint16_t i) 	Load indirect
 sti 	0xB 	0b1011 	void sti(uint16_t i) 	Store indirect
 jmp 	0xC 	0b1100 	void jmp(uint16_t i) 	Jump/Return to subroutine
     	0xD 	0b1101 	  	Unused OpCode
 lea 	0xE 	0b1110 	void lea(uint16_t i) 	Load effective address
 trap 	0xF 	0b1111 	void trap(uint16_t i) 	System trap/call/
 */
