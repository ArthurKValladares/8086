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

void print_instruction(Instruction instruction) {
	const std::bitset<8> higher(instruction.bits >> 8);
	const std::bitset<8> lower(instruction.bits & 0x00ff);
	std::cout << higher << " "  << lower;
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
		if (opt_id.has_value()) {
			const InstructionID id = *opt_id;
			switch (id)
			{
			case InstructionID::MOV:
				printf("mov, ");
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