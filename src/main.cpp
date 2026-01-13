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
    #define MEMSIZE 1048576 //1024*1024 = 1MiB
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
    int32_t eax=0, ebx=0, ecx=0, edx=0, esi=0, edi=0, esp=MEMSIZE, ebp, eip;

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
        uint32_t address; 
        Registers::Reg regTag; 
        
        //2(reg) (reg,32,32)
        bool isIndirect = false;
        Registers::Reg baseReg;
        int32_t displacement = 0;
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
                uint32_t memAddr = op.address;
                
                if(op.isIndirect){
                    Registers::RegDef registerData = Registers::regData[op.baseReg];
                    memAddr = (uint32_t)(*registerData.base_register) + op.displacement;
                }
                
                uint32_t value = 0;
                for (uint8_t i = 0; i < op.size; i++) {
                    value |= static_cast<uint32_t>(Mem::memory[memAddr + i]) << (8 * i);
                }

                if(op.size == 1) return (int32_t)(int8_t)value;
                else if(op.size == 2) return (int32_t)(int16_t)value;
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
                uint32_t memAddr = op.address;
                
                if(op.isIndirect){
                    Registers::RegDef registerData = Registers::regData[op.baseReg];
                    memAddr = (uint32_t)(*registerData.base_register) + op.displacement;
                }
                
                uint32_t v = static_cast<uint32_t>(value);
                for(uint8_t i=0; i<op.size;i++){
                    uint8_t byte = static_cast<uint8_t>(v & 0xFF);
                    Mem::memory[memAddr+i] = byte;
                    v >>= 8;
                }
                break;
            }
        }
    }
    
}

Operands::Operand getOperandFromString(std::string str, uint8_t size);

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
    int8_t cmpState; 
    /*
    -2 -> less
    -1 -> less or equal
     0 -> equal
     1 -> greater or equal
     2 -> greater
    */
    std::vector<std::string> instructions;
    std::unordered_map<std::string, uint32_t> instr_labels;

    void add(std::istringstream& lineWords, std::ofstream& out, uint8_t size){
        std::string src, dest;
        Operands::Operand op_s, op_d;
        int32_t val_s, val_d;
        lineWords >> src >> dest;

        op_s = getOperandFromString(src, size);
        op_d = getOperandFromString(dest, size);

        val_s = Operands::readOperand(op_s);
        val_d = Operands::readOperand(op_d);

        int32_t sum = val_s + val_d;
        Operands::writeOperand(op_d, sum);
        if(size == 4)
            out << "mov" << " $" << sum << ", " << dest << '\n';
        else if(size == 2)
            out << "movw" << " $" << sum << ", " << dest << '\n';
        else if(size == 1)
            out << "movb" << " $" << sum << ", " << dest << '\n';
    }

    void sub(std::istringstream& lineWords, std::ofstream& out, uint8_t size){
        std::string src, dest;
        Operands::Operand op_s, op_d;
        int32_t val_s, val_d;
        lineWords >> src >> dest;

        op_s = getOperandFromString(src, size);
        op_d = getOperandFromString(dest, size);

        val_s = Operands::readOperand(op_s);
        val_d = Operands::readOperand(op_d);

        int32_t sub = val_d - val_s;
        Operands::writeOperand(op_d, sub);
        if(size == 4)
            out << "mov" << " $" << sub << ", " << dest << '\n';
        else if(size == 2)
            out << "movw" << " $" << sub << ", " << dest << '\n';
        else if(size == 1)
            out << "movb" << " $" << sub << ", " << dest << '\n';
    }
    void div(std::istringstream& lineWords, std::ofstream& out){
        std::string src, dest;
        Operands::Operand op_s, eax, edx;
        int32_t val_s;
        int64_t edx_eax;
        lineWords >> src;

        op_s = getOperandFromString(src, 4);
        val_s = Operands::readOperand(op_s);
        
        edx = {
            .type=Operands::OperandType::REGISTER,
            .regTag=Registers::EDX
        };
        eax = {
            .type=Operands::OperandType::REGISTER,
            .regTag=Registers::EAX
        };

        edx_eax = ((uint64_t)Operands::readOperand(edx)<<32) | (uint32_t)Operands::readOperand(eax);
        uint32_t rest, cat;
        cat = (uint32_t)(edx_eax / val_s);
        rest = (uint32_t)(edx_eax % val_s);
        Operands::writeOperand(eax, cat);
        Operands::writeOperand(edx, rest);
        
        out << "mov" << " $" << cat << ", " << "%eax" << '\n';
        out << "mov" << " $" << rest << ", " << "%edx" << '\n';
    }

    void mul(std::istringstream& lineWords, std::ofstream& out){
        std::string src;
        Operands::Operand op_s, eax, edx;
        int32_t val_s;
        int64_t result;
        lineWords >> src;

        op_s = getOperandFromString(src, 4);
        val_s = Operands::readOperand(op_s);
        
        edx = {
            .type=Operands::OperandType::REGISTER,
            .regTag=Registers::EDX
        };
        eax = {
            .type=Operands::OperandType::REGISTER,
            .regTag=Registers::EAX
        };

        int32_t eax_val = Operands::readOperand(eax);
        result = (int64_t)eax_val * val_s;
        
        uint32_t high = (uint32_t)(result >> 32);
        uint32_t low = (uint32_t)result;
        
        Operands::writeOperand(eax, low);
        Operands::writeOperand(edx, high);
        
        out << "mov" << " $" << low << ", " << "%eax" << '\n';
        out << "mov" << " $" << high << ", " << "%edx" << '\n';
    }

    void divw(std::istringstream& lineWords, std::ofstream& out){
        std::string src;
        Operands::Operand op_s, ax, dx;
        uint16_t val_s;
        uint32_t dx_ax;

        lineWords >> src;

        op_s = getOperandFromString(src, 2);
        val_s = (uint16_t)Operands::readOperand(op_s);

        ax = { .type = Operands::OperandType::REGISTER, .regTag = Registers::AX };
        dx = { .type = Operands::OperandType::REGISTER, .regTag = Registers::DX };

        dx_ax = ((uint32_t)Operands::readOperand(dx) << 16) | (uint32_t)Operands::readOperand(ax);

        uint16_t cat = dx_ax / val_s;
        uint16_t rest = dx_ax % val_s;

        Operands::writeOperand(ax, cat);
        Operands::writeOperand(dx, rest);

        out << "movw $" << cat << ", %ax\n";
        out << "movw $" << rest << ", %dx\n";
    }

    void mulw(std::istringstream& lineWords, std::ofstream& out){
        std::string src;
        Operands::Operand op_s, ax, dx;
        uint16_t val_s;
        uint32_t result;

        lineWords >> src;

        op_s = getOperandFromString(src, 2);
        val_s = (uint16_t)Operands::readOperand(op_s);

        ax = { .type = Operands::OperandType::REGISTER, .regTag = Registers::AX };
        dx = { .type = Operands::OperandType::REGISTER, .regTag = Registers::DX };

        uint16_t ax_val = (uint16_t)Operands::readOperand(ax);
        result = (uint32_t)ax_val * val_s;

        uint16_t high = (uint16_t)(result >> 16);
        uint16_t low = (uint16_t)result;

        Operands::writeOperand(ax, low);
        Operands::writeOperand(dx, high);

        out << "movw $" << low << ", %ax\n";
        out << "movw $" << high << ", %dx\n";
    }

    void divb(std::istringstream& lineWords, std::ofstream& out) {
        std::string src;
        Operands::Operand op_s, al, ah;
        uint8_t val_s;
        uint16_t ah_al;

        lineWords >> src;

        op_s = getOperandFromString(src, 1);
        val_s = (uint8_t)Operands::readOperand(op_s);

        al = { .type = Operands::OperandType::REGISTER, .regTag = Registers::AL };
        ah = { .type = Operands::OperandType::REGISTER, .regTag = Registers::AH };

        ah_al = ((uint16_t)Operands::readOperand(ah) << 8) | (uint8_t)Operands::readOperand(al);

        uint8_t cat = ah_al / val_s;
        uint8_t rest = ah_al % val_s;

        Operands::writeOperand(al, cat);
        Operands::writeOperand(ah, rest);

        out << "mob $" << (int)cat << ", %al\n";
        out << "mob $" << (int)rest << ", %ah\n";
    }

    void mulb(std::istringstream& lineWords, std::ofstream& out) {
        std::string src;
        Operands::Operand op_s, al, ah;
        uint8_t val_s;
        uint16_t result;

        lineWords >> src;

        op_s = getOperandFromString(src, 1);
        val_s = (uint8_t)Operands::readOperand(op_s);

        al = { .type = Operands::OperandType::REGISTER, .regTag = Registers::AL };
        ah = { .type = Operands::OperandType::REGISTER, .regTag = Registers::AH };

        uint8_t al_val = (uint8_t)Operands::readOperand(al);
        result = (uint16_t)al_val * val_s;

        uint8_t high = (uint8_t)(result >> 8);
        uint8_t low = (uint8_t)result;

        Operands::writeOperand(al, low);
        Operands::writeOperand(ah, high);

        out << "movb $" << (int)low << ", %al\n";
        out << "movb $" << (int)high << ", %ah\n";
    }
    void mov(std::istringstream& lineWords, std::ofstream& out, uint8_t size){
        std::string src, dest;
        Operands::Operand op_s, op_d;
        lineWords >> src >> dest;
        op_s = getOperandFromString(src, size);
        op_d = getOperandFromString(dest, size);
        Operands::writeOperand(op_d, Operands::readOperand(op_s));
    }



    void _or(std::istringstream& lineWords, std::ofstream& out, uint8_t size){
        std::string src, dest;
        Operands::Operand op_s, op_d;
        lineWords >> src >> dest;
        op_s = getOperandFromString(src, size);
        op_d = getOperandFromString(dest, size);
        auto val_s = Operands::readOperand(op_s);
        auto val_d = Operands::readOperand(op_d);
        val_d = val_d | val_s;
        Operands::writeOperand(op_d, val_d);
        if(size == 4)
            out << "mov" << " $" << val_d << ", " << dest << '\n';
        else if(size == 2)
            out << "movw" << " $" << val_d << ", " << dest << '\n';
        else if(size == 1)
            out << "movb" << " $" << val_d << ", " << dest << '\n';
    }
    void _xor(std::istringstream& lineWords, std::ofstream& out, uint8_t size){
        std::string src, dest;
        Operands::Operand op_s, op_d;
        lineWords >> src >> dest;
        op_s = getOperandFromString(src, size);
        op_d = getOperandFromString(dest, size);
        auto val_s = Operands::readOperand(op_s);
        auto val_d = Operands::readOperand(op_d);
        val_d = val_d ^ val_s;
        Operands::writeOperand(op_d, val_d);
        if(size == 4)
            out << "mov" << " $" << val_d << ", " << dest << '\n';
        else if(size == 2)
            out << "movw" << " $" << val_d << ", " << dest << '\n';
        else if(size == 1)
            out << "movb" << " $" << val_d << ", " << dest << '\n';
    }
    void _and(std::istringstream& lineWords, std::ofstream& out, uint8_t size){
        std::string src, dest;
        Operands::Operand op_s, op_d;
        lineWords >> src >> dest;
        op_s = getOperandFromString(src, size);
        op_d = getOperandFromString(dest, size);
        auto val_s = Operands::readOperand(op_s);
        auto val_d = Operands::readOperand(op_d);
        val_d = val_d & val_s;
        Operands::writeOperand(op_d, val_d);
        if(size == 4)
            out << "mov" << " $" << val_d << ", " << dest << '\n';
        else if(size == 2)
            out << "movw" << " $" << val_d << ", " << dest << '\n';
        else if(size == 1)
            out << "movb" << " $" << val_d << ", " << dest << '\n';

    }

    void inc(std::istringstream& lineWords, std::ofstream& out, uint8_t size){
        std::string dest;
        Operands::Operand op_d;
        lineWords >> dest;
        op_d = getOperandFromString(dest, size);
        auto val_d = Operands::readOperand(op_d);
        val_d = val_d + 1;
        Operands::writeOperand(op_d, val_d);
        if(size == 4)
            out << "mov" << " $" << val_d << ", " << dest << '\n';
        else if(size == 2)
            out << "movw" << " $" << val_d << ", " << dest << '\n';
        else if(size == 1)
            out << "movb" << " $" << val_d << ", " << dest << '\n';
    }
    void dec(std::istringstream& lineWords, std::ofstream& out, uint8_t size){
        std::string dest;
        Operands::Operand op_d;
        lineWords >> dest;
        op_d = getOperandFromString(dest, size);
        auto val_d = Operands::readOperand(op_d);
        val_d = val_d - 1;
        Operands::writeOperand(op_d, val_d);
        if(size == 4)
            out << "mov" << " $" << val_d << ", " << dest << '\n';
        else if(size == 2)
            out << "movw" << " $" << val_d << ", " << dest << '\n';
        else if(size == 1)
            out << "movb" << " $" << val_d << ", " << dest << '\n';
    }

    void shl(std::istringstream& lineWords, std::ofstream& out, uint8_t size){
        std::string src, dest;
        Operands::Operand op_s, op_d;
        lineWords >> src >> dest;
        op_s = getOperandFromString(src, size);
        op_d = getOperandFromString(dest, size);
        auto val_s = Operands::readOperand(op_s);
        auto val_d = Operands::readOperand(op_d);
        val_d = val_d << val_s;
        Operands::writeOperand(op_d, val_d);
        if(size == 4)
            out << "mov" << " $" << val_d << ", " << dest << '\n';
        else if(size == 2)
            out << "movw" << " $" << val_d << ", " << dest << '\n';
        else if(size == 1)
            out << "movb" << " $" << val_d << ", " << dest << '\n';
    }

    void shr(std::istringstream& lineWords, std::ofstream& out, uint8_t size){
        std::string src, dest;
        Operands::Operand op_s, op_d;
        lineWords >> src >> dest;
        op_s = getOperandFromString(src, size);
        op_d = getOperandFromString(dest, size);
        auto val_s = Operands::readOperand(op_s);
        auto val_d = Operands::readOperand(op_d);
        val_d = static_cast<int32_t>(static_cast<uint32_t>(val_d) >> val_s);
        Operands::writeOperand(op_d, val_d);
        if(size == 4)
            out << "mov" << " $" << val_d << ", " << dest << '\n';
        else if(size == 2)
            out << "movw" << " $" << val_d << ", " << dest << '\n';
        else if(size == 1)
            out << "movb" << " $" << val_d << ", " << dest << '\n';
    }

    void sar(std::istringstream& lineWords, std::ofstream& out, uint8_t size){
        std::string src, dest;
        Operands::Operand op_s, op_d;
        lineWords >> src >> dest;
        op_s = getOperandFromString(src, size);
        op_d = getOperandFromString(dest, size);
        auto val_s = Operands::readOperand(op_s);
        auto val_d = Operands::readOperand(op_d);
        val_d = val_d >> val_s;
        Operands::writeOperand(op_d, val_d);
        if(size == 4)
            out << "mov" << " $" << val_d << ", " << dest << '\n';
        else if(size == 2)
            out << "movw" << " $" << val_d << ", " << dest << '\n';
        else if(size == 1)
            out << "movb" << " $" << val_d << ", " << dest << '\n';
    }

void lea(std::istringstream& lineWords, std::ofstream& out, uint8_t size){
        std::string src, dest;
        Operands::Operand op_s, op_d;
        lineWords >> src >> dest;
        op_s = getOperandFromString(src, size);
        op_d = getOperandFromString(dest, size);
        // lea loads the address of the source into the destination
        if(op_s.type == Operands::OperandType::ADDRESS){
            Operands::writeOperand(op_d, op_s.address);
            if(size == 4)
                out << "mov" << " $" << op_s.address << ", " << dest << '\n';
            else if(size == 2)
                out << "movw" << " $" << op_s.address << ", " << dest << '\n';
            else if(size == 1)
                out << "movb" << " $" << op_s.address << ", " << dest << '\n';
        }
    }

    
    //mul
    //push
    //pop
    //cmp
    //call
    //jmp
    //loop
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
            std::string line, currentLabel;
            uint32_t instr_counter = 0;
            while(std::getline(in, line)){
                if(line != ""){
                    if(line == ".data"){
                        section = DATA;
                        out << line << '\n';
                        continue;
                    }
                    
                    if(line== ".text"){
                        section = TEXT;
                        out << line << '\n';
                        continue;
                    }
                        

                    if(section == DATA){
                        out << line << '\n';
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
                                    try {
                                        // Using base 0 lets stoul detect 0x for hex automatically
                                        v = static_cast<int32_t>(std::stoul(value, nullptr, 0));
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
                                                      
                    }    
                    if(section == TEXT){
                        std::string word;
                        std::istringstream lineWords(line);
                        lineWords >> word;
                        if(word == ".global"){
                            lineWords >> word;
                            currentLabel = word;
                        }else{
                            if(line.back()==':'){
                                line.pop_back();
                                Instr::instr_labels[line] = instr_counter;
                            }else{
                                Instr::instructions.push_back(line);
                                instr_counter++;
                            }
                        }
                        
                    }
                }
                
            }
            
            
            Registers::eip = Instr::instr_labels[currentLabel];
            while(Registers::eip< Instr::instructions.size()){
                std::string line = Instr::instructions[Registers::eip];
                std::string originalLine = line;
                std::replace(line.begin(), line.end(), ',', ' ');
                std::istringstream lineWords(line);
                std::string instruction;
                lineWords >> instruction;

                if(instruction == "mov" || instruction == "movl"){
                    out << originalLine;
                    Instr::mov(lineWords,out,4);
                }else if(instruction == "movw"){
                    out << originalLine;
                    Instr::mov(lineWords,out,2);
                }else if(instruction == "movb"){
                    out << originalLine;
                    Instr::mov(lineWords,out,1);
                }
                /*------------------------------*/
                else if(instruction == "add")
                    Instr::add(lineWords, out, 4);
                else if(instruction == "addl")
                    Instr::add(lineWords, out, 4);
                else if(instruction == "addw")
                    Instr::add(lineWords, out, 2);
                else if(instruction == "addb")
                    Instr::add(lineWords, out, 1);
                /*-------------------------------*/
                else if(instruction == "sub")
                    Instr::sub(lineWords, out, 4);
                else if(instruction == "subl")
                    Instr::sub(lineWords, out, 4);
                else if(instruction == "subw")
                    Instr::sub(lineWords, out, 2);
                else if(instruction == "subb")
                    Instr::sub(lineWords, out, 1);
                /*--------------------------------*/
                else if(instruction == "div" || instruction == "divl")
                    Instr::div(lineWords,out);
                else if(instruction == "divw")
                    Instr::divw(lineWords, out);
                else if(instruction == "divb")
                    Instr::divb(lineWords, out);
                /*---------------------------------*/
                else if(instruction == "mul" || instruction == "mull")
                    Instr::mul(lineWords,out);
                else if(instruction == "mulw")
                    Instr::mulw(lineWords, out);
                else if(instruction == "mulb")
                    Instr::mulb(lineWords, out);
                
                /*---------------------------------*/
                else if(instruction == "or")
                    Instr::_or(lineWords, out, 4);
                else if(instruction == "orl")
                    Instr::_or(lineWords, out, 4);
                else if(instruction == "orw")
                    Instr::_or(lineWords, out, 2);
                else if(instruction == "orb")
                    Instr::_or(lineWords, out, 1);
                /*---------------------------------*/
                else if(instruction == "xor")
                    Instr::_xor(lineWords, out, 4);
                else if(instruction == "xorl")
                    Instr::_xor(lineWords, out, 4);
                else if(instruction == "xorw")
                    Instr::_xor(lineWords, out, 2);
                else if(instruction == "xorb")
                    Instr::_xor(lineWords, out, 1);
                /*---------------------------------*/
                else if(instruction == "and")
                    Instr::_and(lineWords, out, 4);
                else if(instruction == "andl")
                    Instr::_and(lineWords, out, 4);
                else if(instruction == "andw")
                    Instr::_and(lineWords, out, 2);
                else if(instruction == "andb")
                    Instr::_and(lineWords, out, 1);
                /*---------------------------------*/
                else if(instruction == "inc")
                    Instr::inc(lineWords, out, 4);
                else if(instruction == "incl")
                    Instr::inc(lineWords, out, 4);
                else if(instruction == "incw")
                    Instr::inc(lineWords, out, 2);
                else if(instruction == "incb")
                    Instr::inc(lineWords, out, 1);
                /*---------------------------------*/
                else if(instruction == "dec")
                    Instr::dec(lineWords, out, 4);
                else if(instruction == "decl")
                    Instr::dec(lineWords, out, 4);
                else if(instruction == "decw")
                    Instr::dec(lineWords, out, 2);
                else if(instruction == "decb")
                    Instr::dec(lineWords, out, 1);
                /*---------------------------------*/
                else if(instruction == "lea")
                    Instr::lea(lineWords, out, 4);
                else if(instruction == "leal")
                    Instr::lea(lineWords, out, 4);
                else if(instruction == "leaw")
                    Instr::lea(lineWords, out, 2);
                else if(instruction == "leab")
                    Instr::lea(lineWords, out, 1);
                /*---------------------------------*/
                
                Registers::eip++;
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

Operands::Operand getOperandFromString(std::string str, uint8_t size){
    if(str[0] == '$'){
        std::string l =  str.substr(1, str.length()-1);
        if(Mem::labels.count(l)){
            return {.type=Operands::OperandType::IMMEDIATE, .imm=static_cast<int32_t>(Mem::labels[l].address)};
        }else{
            return {.type=Operands::OperandType::IMMEDIATE, .imm=static_cast<int32_t>(std::stoul(l, nullptr, 0))};
        }
    }else if(str[0] == '%'){
        return {.type=Operands::OperandType::REGISTER, .regTag=Registers::stringToTag[str]};
    }else if(str[0] == '('){
        
        size_t closePos = str.find(')');
        std::string regStr = str.substr(1, closePos - 1);
        return {.type=Operands::OperandType::ADDRESS,
                .size=size,
                .address=0,
                .isIndirect=true,
                .baseReg=Registers::stringToTag[regStr],
                .displacement=0};
    }else if(str.find('(') != std::string::npos){
        size_t openPos = str.find('(');
        std::string dispStr = str.substr(0, openPos);
        size_t closePos = str.find(')');
        std::string regStr = str.substr(openPos + 1, closePos - openPos - 1);
        
        int32_t displacement = 0;
        if(!dispStr.empty()){
            displacement = static_cast<int32_t>(std::stol(dispStr, nullptr, 0));
        }
        
        return {.type=Operands::OperandType::ADDRESS,
                .size=size,
                .address=0,
                .isIndirect=true,
                .baseReg=Registers::stringToTag[regStr],
                .displacement=displacement};
    }else{
        return {.type=Operands::OperandType::ADDRESS, 
                .size=size,
                .address=Mem::labels[str].address};
    }
    std::cerr << "Nu exista acest tip: " + str; 
}