#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>

#include <cstdint>
#include <bitset>
#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>

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

namespace Mem{
    uint8_t memory[MEMSIZE]; //start -> end memoria principala, end->start stiva
    uint32_t memoryPeak=0;
    struct Label{
        uint8_t size;
        uint32_t address;
    };
    std::unordered_map<std::string, Label> labels;
}

namespace Registers{
    int32_t eax, ebx, ecx, edx, esi, edi, esp=MEMSIZE, ebp;

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
                for (uint8_t i = 0; i < op.size; i++) {
                    value |= static_cast<uint32_t>(Mem::memory[op.address + i]) << (8 * i);
                }

                if(op.size == 1) return (int32_t)(int8_t)value;
                else if(op.size == 2) return (int32_t)(int16_t)value; // fixed negative number problems
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
    void writeOperand(const Operand& op, int32_t value){
        switch(op.type){
            case OperandType::REGISTER:{
                Registers::RegDef registerData = Registers::regData[op.regTag];
                uint32_t mask = generateMask(registerData.size) << registerData.offset;

                *registerData.base_register =
                    (*registerData.base_register & ~mask) |
                    ((static_cast<uint32_t>(value) << registerData.offset) & mask);
                break;
            }
            case OperandType::ADDRESS:{
                uint32_t v = static_cast<uint32_t>(value);
                for(uint8_t i=0; i<op.size;i++){
                    uint8_t byte = static_cast<uint8_t>(v & 0xFF);
                    Mem::memory[op.address+i] = byte;
                    v >>= 8;
                }
                break;
            }
        }
    }
    
}

namespace Instr{
    enum class Type{
        MOV,
        ADD,
        SUB,
        MUL,
        DIV,
        JMP,
        CALL
    };

    struct Instruction{
        Type type;
        
        Operands::Operand src;
        Operands::Operand dest;
    };

    std::vector<Instruction> instructions;

}

int main(int argc, char* argv[]){

    if(!fs::exists("asmOut")) {
        fs::create_directory("asmOut");
    }
    if(argc > 1){
        for(size_t i = 1; i<argc; i++){
            std::string inputFile = "./asmFiles/";
            inputFile = inputFile + argv[i];
            std::ifstream in(inputFile);
            if(!in){
                std::cerr << "File " << argv[i] << " doesn't exist";
                continue;
            }

            std::string outputFile = "./asmOut/";
            outputFile = outputFile + argv[i];
            std::ofstream out(outputFile);
            if(!out){
                std::cerr << "Problems creating the output file( " << argv[i] << " )";
                continue;
            }
            
            enum Sections{
                DATA,
                TEXT
            };
            Sections section;
            std::string line;
            while(std::getline(in, line)){
                if(line != ""){
                    if(line == ".data"){
                        section = DATA;
                        continue;
                    }
                    
                    if(line== ".text"){
                        section = TEXT;
                        continue;
                    }
                        

                    if(section == DATA){
                        std::replace(line.begin(), line.end(), ',', ' ');
                        std::istringstream lineWords(line); // face un input string stream din linie 

                        std::string labelName, type, value;
                        uint8_t size;
                        uint32_t address;
                        

                        address = Mem::memoryPeak;

                        lineWords >> labelName;
                        if (labelName.back() == ':') {
                            labelName.pop_back();
                        }
                        lineWords >> type;
                        if(type == ".byte" || type == ".ascii" || type == ".asciz")
                            size = 1;
                        else if(type == ".word")
                            size = 2;
                        else if(type == ".long" || type == ".space")
                            size = 4;

                        Mem::labels[labelName] = {size, address};
                    
                        std::cout << labelName << ":";
                        std::cout << Mem::labels[labelName].address << '\n';
                        lineWords >> value;
                        if (value.front() == '"') {
                            // It's a string (e.g., .ascii "Hello")
                            // Note: operator >> only gets one word. For strings with spaces, 
                            // you might need std::getline(lineWords, value, '"') instead.
                            std::string temp;
                            while(lineWords >> temp){
                                value += " "+temp;
                            }
                            value = value.substr(1, value.length()-2);
                            int cont = 0;
                            for (char c : value) {
                                    Operands::Operand op = {
                                    .type = Operands::OperandType::ADDRESS,
                                    .size = 1,
                                    .address = address + cont
                                };
                                Operands::writeOperand(op, static_cast<int32_t>(c));
                                cont++;
                            }
                            Mem::memoryPeak += cont;
                            if(type == ".asciz"){
                                Mem::memory[Mem::memoryPeak] = '\n';
                                Mem::memoryPeak++;
                            }
                            // for(int i = address; i<Mem::memoryPeak;i++)
                            //     std::cout << Mem::memory[i];
                        }else{
                            uint32_t v=0;
                            uint32_t counter = 0;
                            do{
                                if (value.front() == '\'') {
                                    v = static_cast<int32_t>(value[1]);
                                } 
                                else {
                                    // It's likely a number (Decimal or Hex)
                                    try {
                                        // Using base 0 lets stoul detect 0x for hex automatically
                                        v = std::stoul(value, nullptr, 0);
                                    } catch (...) {
                                        std::cerr << "Error: Could not parse value: " << value << std::endl;
                                        continue;
                                    }
                                }
                                Operands::Operand op = {
                                    .type = Operands::OperandType::ADDRESS,
                                    .size = size,
                                    .address = address + counter*size
                                };
                                Operands::writeOperand(op, v);
                                counter++;
                                Mem::memoryPeak += size;
                            }while(lineWords >> value);
                        }
                            
                                // Operands::Operand current = {
                                //     .type = Operands::OperandType::ADDRESS,
                                //     .size = size,
                                //     .address = address + counter*size
                                // };
                                // Operands::writeOperand(current, v);
                                // counter++;
                        
                                                
                    }    
                }
                
            }
            for(uint i = Mem::labels["ch"].address; i<Mem::memoryPeak;i+=Mem::labels["ch"].size){
                int32_t v = Operands::readOperand({.type=Operands::OperandType::ADDRESS, .size=1,.address=i});
                std::cout << v;
            }
            in.close();
            out.close();
        }        
    }
    else{
        std::cout << "PLEASE GIVE ME AT LEAST 1 FILE!";
    }

    // Operands::Operand eaxx = {
    //     .type=Operands::OperandType::ADDRESS,
    //     .size=4,
    //     .address=0
    // };

   
    // Operands::writeOperand(eaxx, 0x0101);
    //  eaxx={
    //     .type=Operands::OperandType::ADDRESS,
    //     .size=4,
    //     .address=1
    // };
    // int32_t value = Operands::readOperand(eaxx);
    // std::cout << std::bitset<32>(value) << std::endl;
    return 0;
}