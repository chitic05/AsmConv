#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>

#include <cstdint>
#include <bitset>
#include <string>
#include <unordered_map>

#ifndef MEMSIZE
    #define MEMSIZE 1048576 //1024*1024 = 1MB
#endif



namespace fs = std::filesystem;

uint32_t generateMask(uint8_t size){
        uint32_t mask = 0b0;
        for(uint8_t i = 0; i < size; i++){
            mask = (mask << 1) | 1;
        }
        return mask;
    }

uint8_t memory[1024*1024];

namespace Registers{
    int32_t eax=0x00000110, ebx, ecx, edx, esi, edi, esp=MEMSIZE, ebp;

    enum Reg{
        EAX, AX, AH, AL,
        EBX, BX, BH, BL,
        ECX, CX, CH, CL,
        EDX, DX, DH, DL,
        ESI,
        EDI,
        ESP,
        EBP,
        COUNT
    };

    struct RegDef{
        int32_t* base_register;
        uint8_t size;
        uint16_t offset;
    };

    RegDef regData[]={
        [EAX] = {&eax, 32, 0},
        [AX] = {&eax, 16, 0},
        [AH] = {&eax, 8, 8},
        [AL] = {&eax, 8, 0},

        [EBX] = {&ebx, 32, 0},
        [BX] = {&ebx, 16, 0},
        [BH] = {&ebx, 8, 8},
        [BL] = {&ebx, 8, 0},

        [ECX] = {&ecx, 32, 0},
        [CX] = {&ecx, 16, 0},
        [CH] = {&ecx, 8, 8},
        [CL] = {&ecx, 8, 0},

        [EDX] = {&edx, 32, 0},
        [DX] = {&edx, 16, 0},
        [DH] = {&edx, 8, 8},
        [DL] = {&edx, 8, 0},

        [ESI] = {&esi, 32, 0},
        [EDI] = {&edi, 32, 0},
        
        [ESP] = {&esp, 32, 0},
        [EBP] = {&ebp, 32, 0}
    };
    // Use to transform from text to the tag that we want
    std::unordered_map<std::string, Reg> stringToTag = {
        {"%eax", EAX}, {"%ax", AX}, {"%ah", AH}, {"%al", AL},
        {"%ebx", EBX}, {"%bx", BX}, {"%bh", BH}, {"%bl", BL},
        {"%ecx", ECX}, {"%cx", CX}, {"%ch", CH}, {"%cl", CL},
        {"%edx", EDX}, {"%dx", DX}, {"%dh", DH}, {"%dl", DL},
        {"%esi", ESI},
        {"%edi", EDI},
        {"%esp", ESP},
        {"%ebp", EBP}
    }; 
}

namespace Operands{
    enum class OperandType{
        REGISTER,
        IMMEDIATE,
        ADDRESS
    };

    struct Operand{
        OperandType type;

        uint8_t size;

        int32_t imm;
        uint32_t address; // for data labels(accessing memory)
        Registers::Reg regTag; 
          
    };

    int32_t readOperand(const Operand& op){
        switch(op.type){
            case OperandType::REGISTER:{
                Registers::RegDef registerData = Registers::regData[op.regTag];
                uint32_t mask = generateMask(registerData.size);
                return (*registerData.base_register>>registerData.offset) & mask;
                break;
            }
            case OperandType::ADDRESS:{
                uint32_t value = 0;
                for(uint8_t i = 0; i<op.size;i++){
                    value |= (uint32_t)memory[op.address + i] << (8 * (op.size - 1 - i));
                }

                if(op.size == 1) return (int32_t)(int8_t)value;
                else if(op.size == 2) return (int32_t)(int16_t)value; // now works for negative numbers
                return (int32_t) value;
                break;
            }

            case OperandType::IMMEDIATE:{
                return op.imm;
                break;
            }
            default:{
                throw std::runtime_error("Ceva eroare la readOperand");
                break;
            }
        }
        return 0;

    }
    //SA FAC WRITE IN MEMORIE
    void writeOperand(const Operand& op, int32_t value){
        switch(op.type){
            case OperandType::REGISTER:{
                Registers::RegDef registerData = Registers::regData[op.regTag];
                *registerData.base_register = value;
                break;
            }
            case OperandType::ADDRESS:{
                break;
            }
        }
    }
    
}
int main(int argc, char* argv[]){

    // if(!fs::exists("asmOut")) {
    //     fs::create_directory("asmOut");
    // }
    // if(argc > 1){
    //     for(size_t i = 1; i<argc; i++){
    //         std::string inputFile = "./asmFiles/";
    //         inputFile = inputFile + argv[i];
    //         std::ifstream in(inputFile);
    //         if(!in){
    //             std::cerr << "File " << argv[i] << " doesn't exist";
    //             continue;
    //         }

    //         std::string outputFile = "./asmOut/";
    //         outputFile = outputFile + argv[i];
    //         std::ofstream out(outputFile);
    //         if(!out){
    //             std::cerr << "Problems creating the output file( " << argv[i] << " )";
    //             continue;
    //         }

    //         std::string line;
    //         while(std::getline(in, line)){
    //             std::istringstream lineWords(line); // face un input string stream din linie 
    //             std::string word;
    //             while(lineWords >> word){
    //                 if(word == "mov" || word == "int"){
    //                     break;
    //                 }else out << word << " ";
    //             }
    //             out << '\n';
    //         }


    //         in.close();
    //         out.close();
    //     }        
    // }
    // else{
    //     std::cout << "PLEASE GIVE ME AT LEAST 1 FILE!";
    // }

    Operands::Operand eaxx = {
        .type=Operands::OperandType::REGISTER,
        .regTag = Registers::Reg::AX
    };
    std::cout << std::bitset<32>(Operands::readOperand(eaxx)) << std::endl;
    return 0;
}