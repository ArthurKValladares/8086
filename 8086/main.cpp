#include <stdio.h>
#include <cstdint>
#include <optional>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <bitset>
#include <cassert>
#include <variant>
#include <string_view>
#include <string>
#include <utility>

#define DEBUG_PRINT true;

void print_byte(uint8_t byte) {
	const std::bitset<8> bitset(byte);
	std::cout << bitset;
}

void print_mov_instruction(std::string_view left, std::string_view right) {
	std::cout << "mov, " << left << ", " << right << std::endl;
}

enum class ModEncoding {
	MemoryModeNoDisplacement,
	MemoryMode8BitDisplacement,
	MemoryMode16BitDisplacement,
	RegisterMode,
};

ModEncoding get_mod_encoding(uint8_t byte) {
	const uint8_t mod_bits = (uint8_t)(byte >> 6);
	if (mod_bits == 0b00000000) {
		return ModEncoding::MemoryModeNoDisplacement;
	}
	else if (mod_bits == 0b00000001) {
		return ModEncoding::MemoryMode8BitDisplacement;
	}
	else if (mod_bits == 0b00000010) {
		return ModEncoding::MemoryMode16BitDisplacement;
	}
	else {
		assert(mod_bits == 0b00000011);
		return ModEncoding::RegisterMode;
	}
}

enum class Reg {
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

std::string_view reg_string(Reg reg) {
	switch (reg)
	{
	case Reg::AL:
		return "al";
	case Reg::CL:
		return "cl";
	case Reg::DL:
		return "dl";
	case Reg::BL:
		return "bl";
	case Reg::AH:
		return "ah";
	case Reg::CH:
		return "ch";
	case Reg::DH:
		return "dh";
	case Reg::BH:
		return "bh";
	case Reg::AX:
		return "ax";
	case Reg::CX:
		return "cx";
	case Reg::DX:
		return "dx";
	case Reg::BX:
		return "bx";
	case Reg::SP:
		return "sp";
	case Reg::BP:
		return "bp";
	case Reg::SI:
		return "si";
	case Reg::DI:
		return "di";
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

enum class RMNoDisp {
	BX_Plus_SI,
	BX_Plus_DI,
	BP_Plus_SI,
	BP_Plus_DI,
	SI,
	DI,
	BP,
	BX,
};

std::string_view rm_string(RMNoDisp rm) {
	switch (rm) {
	case RMNoDisp::BX_Plus_SI:
		return "bx + si";
	case RMNoDisp::BX_Plus_DI:
		return "bx + di";
	case RMNoDisp::BP_Plus_SI:
		return "bp + si";
	case RMNoDisp::BP_Plus_DI:
		return "bp + di";
	case RMNoDisp::SI:
		return "si";
	case RMNoDisp::DI:
		return "di";
	case RMNoDisp::BP:
		return "bp";
	case RMNoDisp::BX:
		return "bx";
	}
}

uint8_t get_8_bit_disp(const uint8_t* bytes, int instruction_index) {
	return *((uint8_t*)(bytes + instruction_index + 2));
}

uint16_t get_16_bit_disp(const uint8_t* bytes, int instruction_index) {
	return *((uint16_t*)(bytes + instruction_index + 2));
}

std::string rm_sring_with_disp(RMNoDisp rm, ModEncoding mod_encoding, const uint8_t* bytes, int instruction_index) {
	std::string rm_field;
	const std::string_view rm_sv = rm_string(rm);
	switch (mod_encoding) {
	case ModEncoding::MemoryModeNoDisplacement: {
		rm_field = std::string("[") + std::string(rm_sv) + std::string("]");
		break;
	}
	case ModEncoding::MemoryMode8BitDisplacement: {
		uint8_t data = get_8_bit_disp(bytes, instruction_index);
		if (data != 0) {
			rm_field = std::string("[") + std::string(rm_sv) + std::string(" + ") + std::to_string(data) + std::string("]");
		}
		else {
			rm_field = std::string("[") + std::string(rm_sv) + std::string("]");
		}
		break;
	}
	case ModEncoding::MemoryMode16BitDisplacement: {
		uint16_t data = get_16_bit_disp(bytes, instruction_index);
		if (data != 0) {
			rm_field = std::string("[") + std::string(rm_sv) + std::string(" + ") + std::to_string(data) + std::string("]");
		}
		else {
			rm_field = std::string("[") + std::string(rm_sv) + std::string("]");
		}
		break;
	}
	case ModEncoding::RegisterMode: {
		break;
	}
	}
	return rm_field;
}

std::variant<Reg, RMNoDisp> get_rm_value(bool is_word_instruction, ModEncoding mod_encoding, uint8_t byte) {
	const uint8_t rm_bits = (uint8_t)(byte & 0b00000111);
	if (mod_encoding == ModEncoding::RegisterMode) {
		return get_reg_from_bits(is_word_instruction, rm_bits);
	}
	else {
		if (rm_bits == 0b00000000) {
			return RMNoDisp::BX_Plus_SI;
		} else if (rm_bits == 0b00000001) {
			return RMNoDisp::BX_Plus_DI;
		} else if (rm_bits == 0b00000010) {
			return RMNoDisp::BP_Plus_SI;
		} else if (rm_bits == 0b00000011) {
			return RMNoDisp::BP_Plus_DI;
		} else if (rm_bits == 0b00000100) {
			return RMNoDisp::SI;
		} else if (rm_bits == 0b00000101) {
			return RMNoDisp::DI;
		} else if (rm_bits == 0b00000110) {
			return RMNoDisp::BP;
		} else{
			assert(rm_bits == 0b00000111);
			return RMNoDisp::BX;
		}
	}
}

int get_mod_instruction_offset(ModEncoding mod_encoding, std::optional<RMNoDisp> rm) {
	if (mod_encoding == ModEncoding::MemoryModeNoDisplacement) {
		if (rm.has_value() && *rm == RMNoDisp::BP) {
			return 4;
		}
		else {
			return 2;
		}
	}
	else if (mod_encoding == ModEncoding::RegisterMode) {
		return 2;
	}
	else if (mod_encoding == ModEncoding::MemoryMode8BitDisplacement) {
		return 3;
	}
	else {
		return 4;
	}
}

bool is_direct_addrress(ModEncoding mod_encoding, RMNoDisp rm) {
	return rm == RMNoDisp::BP && mod_encoding == ModEncoding::MemoryModeNoDisplacement;
}

enum class InstructionID {
	MOV_RegisterMemoryToFromRegister,
	MOV_ImmediateToRegisterMemory,
	MOV_ImmediateToRegister,
	MOV_MemoryToAccumulator,
	MOV_AccumulatorToMemory,
	MOV_RegisterMemoryToSegmentRegister,
	MOV_SegmentRegistertoRegisterMemory,
};

std::string_view get_id_string(InstructionID id) {
	switch (id) {
	case InstructionID::MOV_RegisterMemoryToFromRegister:
		return "RegisterMemoryToFromRegister";
	case InstructionID::MOV_ImmediateToRegisterMemory:
		return "ImmediateToRegisterMemory";
	case InstructionID::MOV_ImmediateToRegister:
		return "ImmediateToRegister";
	case InstructionID::MOV_MemoryToAccumulator:
		return "MemoryToAccumulator";
	case InstructionID::MOV_AccumulatorToMemory:
		return "AccumulatorToMemory";
	case InstructionID::MOV_RegisterMemoryToSegmentRegister:
		return "RegisterMemoryToSegmentRegister";
	case InstructionID::MOV_SegmentRegistertoRegisterMemory:
		return "SegmentRegistertoRegisterMemory";
	}
}

std::optional<InstructionID> get_instruction_id(uint8_t byte) {
	if ((byte & 0b11111100) == 0b10001000) {
		return std::optional{ InstructionID::MOV_RegisterMemoryToFromRegister };
	}
	else if ((byte & 0b11111110) == 0b11000110) {
		return std::optional{ InstructionID::MOV_ImmediateToRegisterMemory };
	}
	else if ((byte & 0b11110000) == 0b10110000) {
		return std::optional{ InstructionID::MOV_ImmediateToRegister };
	}
	else if ((byte & 0b11111110) == 0b10100000) {
		return std::optional{ InstructionID::MOV_MemoryToAccumulator };
	}
	else if ((byte & 0b11111110) == 0b10100010) {
		return std::optional{ InstructionID::MOV_AccumulatorToMemory };
	}
	else if (byte == 0b10001110) {
		return std::optional{ InstructionID::MOV_RegisterMemoryToSegmentRegister };
	}
	else if (byte == 0b10001100) {
		return std::optional{ InstructionID::MOV_SegmentRegistertoRegisterMemory };
	}
	else {
		return std::nullopt;
	}
}

void process_instructions(const uint8_t* bytes, uint64_t count) {
	int instruction_index = 0;
	while (instruction_index < count) {
		const uint8_t byte = *(bytes + instruction_index);
		const std::optional<InstructionID> opt_id = get_instruction_id(byte);
		if (opt_id.has_value() ) {
			const InstructionID id = *opt_id;

#if DEBUG_PRINT
			std::cout << get_id_string(id) << std::endl;
#endif

			switch (id)
			{
				case InstructionID::MOV_RegisterMemoryToFromRegister:
				{
					const uint8_t next_byte = *(bytes + instruction_index + 1);

					const bool                        is_word_instruction = (byte & 0b00000001) != 0;
					const bool                        direction_to_register = (byte & 0b00000010) != 0;
					const ModEncoding                 mod_encoding = get_mod_encoding(next_byte);
					const Reg                         reg = get_reg_from_bits(is_word_instruction, (uint8_t)((next_byte >> 3) & 0b00000111));
					const std::variant<Reg, RMNoDisp> rm = get_rm_value(is_word_instruction, mod_encoding, next_byte);
					
					int mod_offset;
					if (std::holds_alternative<Reg>(rm)) {
						const std::string_view rm_reg = reg_string(std::get<Reg>(rm));
						const std::string_view reg_str = reg_string(reg);
						print_mov_instruction(rm_reg, reg_str);

						mod_offset = get_mod_instruction_offset(mod_encoding, std::nullopt);
					}
					else {
						const std::string_view reg_field = reg_string(reg);
						const RMNoDisp rm_no_disp = std::get<RMNoDisp>(rm);
						std::string rm_field;
						if (is_direct_addrress(mod_encoding, rm_no_disp)) {
							uint16_t data = get_16_bit_disp(bytes, instruction_index);
							rm_field = std::to_string(data);
						}
						else {
							rm_field = rm_sring_with_disp(rm_no_disp, mod_encoding, bytes, instruction_index);
						}

						if (direction_to_register) {
							print_mov_instruction(reg_field, rm_field);
						}
						else {
							print_mov_instruction(rm_field, reg_field);
						}

						mod_offset = get_mod_instruction_offset(mod_encoding, std::optional<RMNoDisp>{ rm_no_disp });
					}					
					

					instruction_index += mod_offset;

					break;
				}
				case InstructionID::MOV_ImmediateToRegister:
				{
					const bool is_word_instruction = (byte & 0b00001000) != 0;
					const Reg  reg = get_reg_from_bits(is_word_instruction, (uint8_t)(byte & 0b00000111));
					const std::string_view reg_str = reg_string(reg);

					if (is_word_instruction) {
						const uint16_t data = *((uint16_t*)(bytes + instruction_index + 1));

						print_mov_instruction(reg_str, std::to_string(data));

						instruction_index += 3;
					}
					else {
						const uint8_t data = *(bytes + instruction_index + 1);

						print_mov_instruction(reg_str, std::to_string(data));

						instruction_index += 2;
					}

					break;
				}
				case InstructionID::MOV_ImmediateToRegisterMemory:
				{
					const uint8_t next_byte = *(bytes + instruction_index + 1);

					const bool                        is_word_instruction = (byte & 0b00000001) != 0;
					const ModEncoding                 mod_encoding = get_mod_encoding(next_byte);
					const std::variant<Reg, RMNoDisp> rm = get_rm_value(is_word_instruction, mod_encoding, next_byte);

					std::string rm_string;
					int mod_offset;
					if (std::holds_alternative<Reg>(rm)) {
						const std::string_view rm_reg = reg_string(std::get<Reg>(rm));

						rm_string = rm_reg;
						mod_offset = get_mod_instruction_offset(mod_encoding, std::nullopt);
					}
					else {						
						const RMNoDisp rm_no_disp = std::get<RMNoDisp>(rm);
						const std::string rm_field = rm_sring_with_disp(rm_no_disp, mod_encoding, bytes, instruction_index);

						rm_string = rm_field;
						mod_offset = get_mod_instruction_offset(mod_encoding, std::optional<RMNoDisp>{ rm_no_disp });
					}

					if (is_word_instruction) {
						const uint16_t data = *((uint16_t*)(bytes + instruction_index + mod_offset));
						const std::string data_field = "word " + std::to_string(data);
						print_mov_instruction(rm_string, data_field);

						instruction_index += 2;
					}
					else {
						const uint8_t data = *(bytes + instruction_index + mod_offset);
						const std::string data_field = "byte " + std::to_string(data);
						print_mov_instruction(rm_string, data_field);

						instruction_index += 1;
					}
					instruction_index += mod_offset;

					break;
				}
				case InstructionID::MOV_MemoryToAccumulator:
				{
					printf("TODO\n");
					

					instruction_index += 3;
					break;
				}
				case InstructionID::MOV_AccumulatorToMemory:
				{
					printf("TODO\n");


					instruction_index += 3;
					break;
				}
				default:
				{
					printf("UNSUPPORTED\n");
					break;
				}
			}
		}
		else {
			printf("Invalid instruction: ");
			print_byte(byte);
			printf("\n");
			instruction_index += 2;
		}
	}
}

int main() {
	std::ifstream in_file;
	in_file.open(".\\data\\listing_0040_challenge_movs");
	if (in_file) {
		// File size
		in_file.seekg(0, std::ios::end);
		const size_t num_bytes = in_file.tellg();
		in_file.seekg(0, std::ios::beg);
		const size_t length = num_bytes;

		// Read to buffer
		uint8_t* bytes = new uint8_t[length];
		in_file.read((char*) bytes, num_bytes);

		process_instructions(bytes, length);

		// Cleanup
		delete[] bytes;
	}
	else {
		printf("Could not read input file");
	}

	return 0;
}