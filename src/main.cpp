#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>

#include <cstdint>
#include <bitset>
#include <string>
#include <unordered_map>

namespace fs = std::filesystem;

namespace Registers{
    int32_t eax, ebx, ecx, edx, esi, edi, esp, ebp;

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

    uint32_t generateMask(uint8_t size, uint8_t offset){
        uint32_t mask = 0b0;
        for(uint8_t i = 0; i < size; i++){
            mask = (mask << 1) | 1;
        }
        mask = mask << offset;
        return mask;
    }

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

    int32_t getRegValue(std::string regName){
        Reg regTag;
        if(stringToTag.find(regName) != stringToTag.end())
            regTag = stringToTag[regName];
        else{
            throw std::runtime_error("Register " + regName + " doesn't exist!");
        }            
        RegDef registerData = regData[regTag];
        uint32_t mask = generateMask(registerData.size, registerData.offset);
        return *registerData.base_register & mask;
    }
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

            std::string line;
            while(std::getline(in, line)){
                std::istringstream lineWords(line); // face un input string stream din linie 
                std::string word;
                while(lineWords >> word){
                    if(word == "mov" || word == "int"){
                        break;
                    }else out << word << " ";
                }
                out << '\n';
            }


            in.close();
            out.close();
        }        
    }
    else{
        std::cout << "PLEASE GIVE ME AT LEAST 1 FILE!";
    }

    return 0;
}