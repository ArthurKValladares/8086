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

#define DEBUG_PRINT false;

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

int get_mod_instruction_offset(ModEncoding mod_encoding) {
	if (mod_encoding == ModEncoding::RegisterMode || mod_encoding == ModEncoding::MemoryModeNoDisplacement) {
		return 2;
	}
	else if (mod_encoding == ModEncoding::MemoryMode8BitDisplacement) {
		return 3;
	}
	else {
		return 4;
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
					
					// TODO: cleanup all this left/right stuff
					if (std::holds_alternative<Reg>(rm)) {
						std::string_view left = reg_string(std::get<Reg>(rm));
						std::string_view right = reg_string(reg);
						print_mov_instruction(left, right);
					}
					else {
						std::string_view reg_field = reg_string(reg);

						std::string rm_field;
						std::string_view rm_sv = rm_string(std::get<RMNoDisp>(rm));
						switch (mod_encoding) {
							case ModEncoding::MemoryModeNoDisplacement: {
								rm_field = std::string("[") + std::string(rm_sv) + std::string("]");
								break;
							}
							case ModEncoding::MemoryMode8BitDisplacement: {
								uint8_t data = *((uint8_t*)(bytes + instruction_index + 2));
								if (data != 0) {
									rm_field = std::string("[") + std::string(rm_sv) + std::string(" + ") + std::to_string(data) + std::string("]");
								}
								else {
									rm_field = std::string("[") + std::string(rm_sv) + std::string("]");
								}
								break;
							}
							case ModEncoding::MemoryMode16BitDisplacement: {
								uint16_t data = *((uint16_t*)(bytes + instruction_index + 2));
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

						if (direction_to_register) {
							print_mov_instruction(reg_field, rm_field);
						}
						else {
							print_mov_instruction(rm_field, reg_field);
						}
					}					
					

					instruction_index += get_mod_instruction_offset(mod_encoding);

					break;
				}
				case InstructionID::MOV_ImmediateToRegister:
				{
					const bool is_word_instruction = (byte & 0b00001000) != 0;
					const Reg  reg = get_reg_from_bits(is_word_instruction, (uint8_t)(byte & 0b00000111));
					std::string_view reg_str = reg_string(reg);

					if (is_word_instruction) {
						uint16_t data = *((uint16_t*)(bytes + instruction_index + 1));

						print_mov_instruction(reg_str, std::to_string(data));

						instruction_index += 3;
					}
					else {
						uint8_t data = *(bytes + instruction_index + 1);

						print_mov_instruction(reg_str, std::to_string(data));

						instruction_index += 2;
					}

					break;
				}
				case InstructionID::MOV_ImmediateToRegisterMemory:
				{
					printf("TODO\n");
					const uint8_t next_byte = *(bytes + instruction_index + 1);

					const bool                        is_word_instruction = (byte & 0b00000001) != 0;
					const ModEncoding                 mod_encoding = get_mod_encoding(next_byte);
					const std::variant<Reg, RMNoDisp> rm = get_rm_value(is_word_instruction, mod_encoding, next_byte);


					instruction_index += get_mod_instruction_offset(mod_encoding);

					if (is_word_instruction) {
						instruction_index += 2;
					}
					else {
						instruction_index += 1;
					}
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