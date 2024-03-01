#include <stdio.h>
#include <stdlib.h>

#include <stdint.h>

typedef uint8_t u8;

struct Instruction {
	u8 opp;
	u8 d;
	u8 w;
	u8 s;
	u8 mod;
	u8 regrm[2];
};

struct Register {
	u8 bits;
	char w_table[2][3];
};

struct Op {
	u8 bits;
	char str[4];
};

Op opps[] = {
	{0b100010, "mov"},
	{0b000000, "add"},
	{0b000100, "adc"},
	{0b001010, "sub"},
	{0b000110, "sbb"},
	{0b001110, "cmp"},
};

Op functions_ops[] = {
	{0b100010, register_register}
};

struct EffectiveAddress {
	u8 bits;
	char table[8];
};

Register registers[] = {
	{0b000, {"al", "ax"}},
	{0b001, {"cl", "cx"}},
	{0b010, {"dl", "dx"}},
	{0b011, {"bl", "bx"}},
	{0b100, {"ah", "sp"}},
	{0b101, {"ch", "bp"}},
	{0b110, {"dh", "si"}},
	{0b111, {"bh", "di"}}
};

enum {
	reg_idx = 0,
	rm_idx = 1,
};

char effective_addresses[][8] = {
	"bx + si", //0b000
	"bx + di", //0b001
	"bp + si", //0b010
	"bp + di", //0b011
	"si",      //0b100
	"di",      //0b101
	"bp",      //0b110
	"bx"      //0b111
};

void imm_register_memory(char *buffer_read, int *idx) {
	inst.s = (buffer_read[0] & 0b00000010) >> 1;
	inst.w = (buffer_read[0] & 0b00000001) >> 0;
	inst.mod = (buffer_read[1] & 0b11000000) >> 6;
	inst.opp = (buffer_read[1] & 0b00111000) >> 3;
	inst.regrm[rm_idx] = (buffer_read[1] & 0b00000111) >> 0;
	
	Op opps_imm[] = {
		{0b000, "add"},
		{0b010, "adc"},
		{0b101, "sub"},
		{0b011, "sbb"},
		{0b111, "cmp"},
	};

	printf("%s ", opps_imm[opcode]);
	//calculate displacement
	int displacement = 0;
	int data = 0 ;
	if (inst.mod == 0b00) {
		data = buffer_read[2];
		if (inst.w && inst.s == 0) {
			data &= 0b11111111;
			data |= buffer_read[3] << 8;
		}
	}
	if (inst.mod == 0b01) {
		displacement = buffer_read[2];
		data = buffer_read[3];
		if (inst.w && inst.s == 0) {
			data &= 0b11111111;
			data |= buffer_read[4] << 8;
		}
		idx += inst.mod;
	}
	else if (inst.mod == 0b10) {
		displacement = buffer_read[2];
		displacement &= 0b11111111;
		displacement |= buffer_read[3] << 8;
		data = buffer_read[4];
		if (inst.w && inst.s == 0) {
			data &= 0b11111111;
			data |= buffer_read[5] << 8;
		}
		idx += inst.mod;
	}

	//Now print
	if (inst.w) {
		printf("word ");
	}
	else {
		printf("byte ");
	}
	if (inst.mod == 0b00) {
		printf("[%s],%d\n", effective_addresses[rm_idx], data);
	}
	else if (inst.mod == 0b01 || inst.mod & 0b10) {
		printf("mov [%s + %d],%d\n", effective_addresses[rm_idx], displacement, data);
	}
	idx += 3 + inst.w;
}

void register_register(u8 opcode, char *buffer_read, int *idx) {
	Instruction inst;
	inst.d = (buffer_read[0] & 0b00000010) >> 1;
	inst.w = (buffer_read[0] & 0b00000001) >> 0;
	inst.mod = (buffer_read[1] & 0b11000000) >> 6;
	inst.regrm[reg_idx] = (buffer_read[1] & 0b00111000) >> 3;
	inst.regrm[rm_idx] = (buffer_read[1] & 0b00000111) >> 0;

	printf("%s ", opps[opcode]);
	if (inst.mod == 0b11) {
		u8 regrm1 = inst.regrm[!inst.d];
		u8 regrm2 = inst.regrm[inst.d];
		printf("%s %s,%s\n",
			opps[opcode],
			registers[regrm1].w_table[inst.w],
			registers[regrm2].w_table[inst.w]);
			idx += 2;
	}
	else if (inst.mod == 0b00) {
		if (inst.regrm[rm_idx] == 0b110) {
			int displacement = buffer_read[2];
			displacement &= 0b11111111;
			displacement |= buffer_read[3] << 8;

			if (inst.d) {
				printf("%s,[%d]\n", registers[inst.regrm[reg_idx]].w_table[inst.w], displacement);
			}
			else {
				printf("[%d],%s\n", displacement, registers[inst.regrm[reg_idx]].w_table[inst.w]);
			}
			idx += 4;
		}
		else {
			if (inst.d) {
				printf("%s,[%s]\n", registers[inst.regrm[reg_idx]].w_table[inst.w],
					effective_addresses[inst.regrm[rm_idx]]);
			}
			else {
				printf("[%s],%s\n",
					effective_addresses[inst.regrm[rm_idx]], registers[inst.regrm[reg_idx]].w_table[inst.w]);
			}
			idx += 2;
		}
	}
	else if (inst.mod == 0b01) {
		int displacement = buffer_read[2];
		if (inst.d) {
			printf("%s,[%s + %d]\n", registers[inst.regrm[reg_idx]].w_table[inst.w],
					effective_addresses[inst.regrm[rm_idx]],
					displacement);
		}
		else {
			printf("[%s + %d],%s\n",
					effective_addresses[inst.regrm[rm_idx]],
					displacement, registers[inst.regrm[reg_idx]].w_table[inst.w]);
		}
		idx += 3;
	}
	else if (inst.mod == 0b10) {
		int displacement = buffer_read[2];
		displacement &= 0b11111111;
		displacement |= buffer_read[3] << 8;
		if (inst.d) {
			printf("%s,[%s + %d]\n", registers[inst.regrm[reg_idx]].w_table[inst.w],
					effective_addresses[inst.regrm[rm_idx]],
					displacement);
		}
		else {
			printf("[%s + %d],%s\n",
					effective_addresses[inst.regrm[rm_idx]],
					displacement, registers[inst.regrm[reg_idx]].w_table[inst.w]);
		}
		idx += 4;
	}
}


int main(int argc, char* argv[]) {
	if (argc != 2) {
		printf("./a.out <path_file>\n");
		return -1;
	}
	FILE* f = fopen(argv[1], "r");
	if (!f) {
		printf("Error: Cannot open file\n");
		return -1;
	}
	fseek(f, 0, SEEK_END);
	long int length = ftell(f);
	fseek(f, 0, SEEK_SET);
	char* buf = (char*)malloc(length);
	fread(buf, length, 1, f);
	fclose(f);
	printf("bits 16\n\n");

	for (int idx = 0; idx < length;) {
		Instruction inst;
		char* buffer_read = &buf[idx];

		//Registers mov,add,sub
		u8 opcode = ((buffer_read[0] & 0b11111100) >> 2);
		if (0b100010 == opcode ||
		    0b000000 == opcode ||
			0b000100 == opcode ||
			0b001010 == opcode ||
			0b000110 == opcode ||
			0b001110 == opcode) {
			printf("register to register\n");
			register_register(opcode, buffer_read, &idx);
			continue;
		}

		if (0b1011 == ((buffer_read[0] & 0b11110000) >> 4)) {
			//inmediate to register 
			printf("immediate to register\n");
			inst.w = (buffer_read[0] & 0b00001000) >> 3;
			inst.regrm[reg_idx] = (buffer_read[0] & 0b00000111) >> 0;

			int16_t data = buffer_read[1];
			if (inst.w) { //data is 16
				data = data & 0b11111111;
				data |= buffer_read[2] << 8;
				idx += 1;
			}

			printf("mov ");
			u8 regrm = inst.regrm[reg_idx];
			printf("%s,%d\n", registers[regrm].w_table[inst.w], data);
			idx += 2;
			continue;
		}

		if (0b1100011 == ((buffer_read[0] & 0b11111110) >> 1)) {
			//inmediate to register/memory in mov case
			printf("immediate to register/memory\n");
			inst.w = (buffer_read[0] & 0b00000001) >> 0;
			inst.mod = (buffer_read[1] & 0b11000000) >> 6;
			inst.regrm[rm_idx] = (buffer_read[1] & 0b00000111) >> 0;

			//calculate displacement
			int displacement = 0;
			int data = 0 ;
			if (inst.mod == 0b00) {
				data = buffer_read[2];
				if (inst.w) {
					data &= 0b11111111;
					data |= buffer_read[3] << 8;
				}
			}
			if (inst.mod == 0b01) {
				displacement = buffer_read[2];
				data = buffer_read[3];
				if (inst.w) {
					data &= 0b11111111;
					data |= buffer_read[4] << 8;
				}
				idx += inst.mod;
			}
			else if (inst.mod == 0b10) {
				displacement = buffer_read[2];
				displacement &= 0b11111111;
				displacement |= buffer_read[3] << 8;
				data = buffer_read[4];
				if (inst.w) {
					data &= 0b11111111;
					data |= buffer_read[5] << 8;
				}
				idx += inst.mod;
			}

			//Now print
			if (inst.mod == 0b00) {
				printf("mov [%s],", effective_addresses[rm_idx]);
			}
			else if (inst.mod == 0b01 || inst.mod & 0b10) {
				printf("mov [%s + %d],", effective_addresses[rm_idx], displacement);
			}

			if (inst.w) {
				printf("word %d\n", data);
			}
			else {
				printf("byte %d\n", data);
			}
			idx += 3 + inst.w;
			continue;
		}

		if (0b100000 == opcode) {
			//similar case but for add,sub and similars
			printf("immediate to register/memory\n");
			imm_register_memory(opcode, buffer_read, &idx);
			continue;
		}

		if (((buffer_read[0] & 0b11111100) >> 2) == 0b101000) {
			printf("memory acummulator\n");
			inst.d = (buffer_read[0] & 0b00000010) >> 1;
			inst.w = (buffer_read[0] & 0b00000001);
			int16_t address = buffer_read[1];
			if (inst.w)
				address &= 0b11111111;
				address |= buffer_read[2] << 8;

			if (inst.d) {
				printf("mov [%d],ax\n", address);
			}
			else {
				printf("mov ax,[%d]\n", address);
			}

			idx += 2 + inst.w;
		}
	}
	return 0;
}
