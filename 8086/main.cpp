#include <stdio.h>
#include <cstdint>
#include <optional>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <bitset>

struct Instruction {
	uint16_t bits;
};

bool word_instruction(Instruction instruction) {
	return (instruction.bits & 0b00000001) != 0;
}

void print_byte(uint8_t byte) {
	const std::bitset<8> bitset(byte);
	std::cout << bitset;
}

void print_instruction(Instruction instruction) {
	const std::bitset<8> higher(instruction.bits >> 8);
	const std::bitset<8> lower(instruction.bits & 0x00ff);
	std::cout << higher << " "  << lower;
}

enum ModEncoding {
	MemoryModeNoDisplacement,
	MemoryMode8BitDisplacement,
	MemoryMode16BitDisplacement,
	RegisterMode,
};

ModEncoding get_mod_encoding(Instruction instruction) {
	const uint8_t op_bits = (uint8_t)((instruction.bits >> 8) >> 6);
	if (op_bits == 0b00000000) {
		return ModEncoding::MemoryModeNoDisplacement;
	}
	else if (op_bits == 0b00000001) {
		return ModEncoding::MemoryMode8BitDisplacement;
	}
	else if (op_bits == 0b00000010) {
		return ModEncoding::MemoryMode16BitDisplacement;
	}
	else {
		return ModEncoding::RegisterMode;
	}
}

enum Reg {
	AL,
	CL,
	DL,
	BL,
	AH,
	CH,
	DH,
	BH,
	AX,
	CX,
	DX,
	BX,
	SP,
	BP,
	SI,
	DI,
};

// TODO: return string later
void print_reg(Reg reg) {
	switch (reg)
	{
	case Reg::AL:
		printf("al");
		break;
	case Reg::CL:
		printf("cl");
		break;
	case Reg::DL:
		printf("dl");
		break;
	case Reg::BL:
		printf("bl");
		break;
	case Reg::AH:
		printf("ah");
		break;
	case Reg::CH:
		printf("ch");
		break;
	case Reg::DH:
		printf("dh");
		break;
	case Reg::BH:
		printf("bh");
		break;
	case Reg::AX:
		printf("ax");
		break;
	case Reg::CX:
		printf("cx");
		break;
	case Reg::DX:
		printf("dx");
		break;
	case Reg::BX:
		printf("bx");
		break;
	case Reg::SP:
		printf("sp");
		break;
	case Reg::BP:
		printf("bp");
		break;
	case Reg::SI:
		printf("si");
		break;
	case Reg::DI:
		printf("di");
		break;
	default:
		break;
	}
}

Reg get_reg_from_bits(bool is_word_instruction, uint8_t reg_bits) {

	if (reg_bits == 0b00000000) {
		if (!is_word_instruction) {
			return Reg::AL;
		}
		else {
			return Reg::AX;
		}
	}
	else if (reg_bits == 0b00000001) {
		if (!is_word_instruction) {
			return Reg::CL;
		}
		else {
			return Reg::CX;
		}
	}
	else if (reg_bits == 0b00000010) {
		if (!is_word_instruction) {
			return Reg::DL;
		}
		else {
			return Reg::DX;
		}
	}
	else if (reg_bits == 0b00000011) {
		if (!is_word_instruction) {
			return Reg::BL;
		}
		else {
			return Reg::BX;
		}
	}
	else if (reg_bits == 0b00000100) {
		if (!is_word_instruction) {
			return Reg::AH;
		}
		else {
			return Reg::SP;
		}
	}
	else if (reg_bits == 0b00000101) {
		if (!is_word_instruction) {
			return Reg::CH;
		}
		else {
			return Reg::BP;
		}
	}
	else if (reg_bits == 0b00000110) {
		if (!is_word_instruction) {
			return Reg::DH;
		}
		else {
			return Reg::SI;
		}
	}
	else {
		if (!is_word_instruction) {
			return Reg::BH;
		}
		else {
			return Reg::DI;
		}
	}
}

Reg get_reg(Instruction instruction) {
	const bool is_word_instruction = word_instruction(instruction);
	const uint8_t reg_bits = (uint8_t)((instruction.bits >> 8) >> 3 & 0b00000111);
	return get_reg_from_bits(is_word_instruction, reg_bits);
}

Reg get_rm_reg(Instruction instruction) {
	const bool is_word_instruction = word_instruction(instruction);
	const uint8_t reg_bits = (uint8_t)(instruction.bits >> 8 & 0b00000111);
	return get_reg_from_bits(is_word_instruction, reg_bits);
}

enum InstructionID {
	MOV
};

std::optional<InstructionID> get_instruction_id(Instruction instruction) {
	const uint8_t op_bits = (uint8_t) (instruction.bits & 0b11111100);
	if (op_bits == 0b10001000) {
		return std::optional{ InstructionID::MOV };
	}
	else {
		return std::nullopt;
	}
}

void process_instructions(const Instruction* instructions, uint64_t count) {
	for (int i = 0; i < count; ++i) {
		const Instruction instruction = *(instructions + i);
		const std::optional<InstructionID> opt_id = get_instruction_id(instruction);
		if (opt_id.has_value() ) {
			const ModEncoding   mod_ecoding = get_mod_encoding(instruction);
			const InstructionID id = *opt_id;
			const Reg           reg = get_reg(instruction);
			// TODO: we are unconditionally gettting the rm reg here, but in a real emulator we would need to check the R/M encoding
			// and proceed differently according to the encoding
			const Reg           rm_reg = get_rm_reg(instruction);
			switch (id)
			{
			case InstructionID::MOV:
				printf("mov, ");
				print_reg(rm_reg);
				printf(", ");
				print_reg(reg);
				printf("\n");
				break;
			default:
				break;
			}
		}
		else {
			printf("Invalid instruction: ");
			print_instruction(instruction);
			printf("\n");
		}
	}
}

void main() {
	std::ifstream in_file;
	// TODO: relative path
	in_file.open("C:\\Users\\Arthur\\Documents\\projects\\8086\\8086\\data\\listing_0037_single_register_mov.txt");
	if (in_file) {
		// File size
		in_file.seekg(0, std::ios::end);
		const size_t num_bytes = in_file.tellg();
		in_file.seekg(0, std::ios::beg);
		const size_t length = num_bytes / 2;

		// Read to buffer
		const Instruction* instructions = new Instruction[length];
		in_file.read((char*)instructions, num_bytes);

		process_instructions(instructions, length);

		// Cleanup
		delete[] instructions;
	}
	else {
		printf("Could not read input file");
	}
}