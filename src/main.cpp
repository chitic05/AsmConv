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

namespace Mem{
    uint8_t memory[MEMSIZE]; //start -> end memoria principala, end->start stiva
    uint32_t memoryPeak=0;
    struct Label{
        uint8_t size;
        uint32_t address;
    };
    std::unordered_map<std::string, Label> labels;
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

    enum State{
        L, //less
        LE, // less or equal
        E, // equal
        GE, //greater or equal
        G ,// greater
        A, //above
        AE, //above or equal
        Z, //zero
    };
    uint8_t flags[] = {0, 0, 0, 0, 0, 0, 0, 0};
    void resetFlags(){
        for(uint8_t i = 0; i<8; i++)
            flags[i] = 0;
    }
    std::vector<std::string> instructions;
    
    std::unordered_map<std::string, uint32_t> instr_labels;
    
    std::string currentLabel;
    
    void resetAll(){
        // Reset all instruction state
        flags[L] = 0;
        flags[LE] = 0;
        flags[E] = 0;
        flags[GE] = 0;
        flags[G] = 0;
        flags[A] = 0;
        flags[AE] = 0;
        flags[Z] = 0;
        instructions.clear();
        instr_labels.clear();
        currentLabel = "";
        
        // Reset registers
        Registers::eax = 0;
        Registers::ebx = 0;
        Registers::ecx = 0;
        Registers::edx = 0;
        Registers::esi = 0;
        Registers::edi = 0;
        Registers::esp = MEMSIZE;
        Registers::ebp = 0;
        Registers::eip = 0;
        
        // Reset memory
        std::fill(std::begin(Mem::memory), std::end(Mem::memory), 0);
        Mem::memoryPeak = 0;
        Mem::labels.clear();
    }

    void add(std::string src, std::string dest, std::ofstream& out, uint8_t size){
        Operands::Operand op_s, op_d;
        int32_t val_s, val_d;
        resetFlags();

        op_s = getOperandFromString(src, size);
        op_d = getOperandFromString(dest, size);

        val_s = Operands::readOperand(op_s);
        val_d = Operands::readOperand(op_d);

        int32_t sum = val_s + val_d;
        Operands::writeOperand(op_d, sum);
        
        if(size == 4) out << "movl $" << sum << ", " << dest << '\n';
        else if(size == 2) out << "movw $" << sum << ", " << dest << '\n';
        else if(size == 1) out << "movb $" << sum << ", " << dest << '\n';
    }

    void sub(std::string src, std::string dest, std::ofstream& out, uint8_t size){
        Operands::Operand op_s, op_d;
        int32_t val_s, val_d;
        resetFlags();

        op_s = getOperandFromString(src, size);
        op_d = getOperandFromString(dest, size);

        val_s = Operands::readOperand(op_s);
        val_d = Operands::readOperand(op_d);

        int32_t sub = val_d - val_s;
        Operands::writeOperand(op_d, sub);
        
        if(op_d.type == Operands::OperandType::ADDRESS){
            if(size == 4) out << "subl " << src << ", " << dest << '\n';
            else if(size == 2) out << "subw " << src << ", " << dest << '\n';
            else if(size == 1) out << "subb " << src << ", " << dest << '\n';
        } else {
            if(size == 4) out << "movl $" << sub << ", " << dest << '\n';
            else if(size == 2) out << "movw $" << sub << ", " << dest << '\n';
            else if(size == 1) out << "movb $" << sub << ", " << dest << '\n';
        }
    }
    void div(std::string src, std::ofstream& out){
        Operands::Operand op_s, eax, edx;
        int32_t val_s;
        int64_t edx_eax;
        resetFlags();

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
        
        out << "movl" << " $" << cat << ", " << "%eax" << '\n';
        out << "movl" << " $" << rest << ", " << "%edx" << '\n';
    }

    void mul(std::string src, std::ofstream& out){
        Operands::Operand op_s, eax, edx;
        int32_t val_s;
        int64_t result;
        resetFlags();

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
        
        out << "movl" << " $" << low << ", " << "%eax" << '\n';
        out << "movl" << " $" << high << ", " << "%edx" << '\n';
    }

    void divw(std::string src, std::ofstream& out){
        Operands::Operand op_s, ax, dx;
        uint16_t val_s;
        uint32_t dx_ax;
        resetFlags();

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

    void mulw(std::string src, std::ofstream& out){
        Operands::Operand op_s, ax, dx;
        uint16_t val_s;
        uint32_t result;
        resetFlags();

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

    void divb(std::string src, std::ofstream& out) {
        Operands::Operand op_s, al, ah;
        uint8_t val_s;
        uint16_t ah_al;
        resetFlags();

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

    void mulb(std::string src, std::ofstream& out) {
        Operands::Operand op_s, al, ah;
        uint8_t val_s;
        uint16_t result;
        resetFlags();

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
    void mov(std::string src, std::string dest, std::ofstream& out, uint8_t size){
        Operands::Operand op_s, op_d;
        op_s = getOperandFromString(src, size);
        op_d = getOperandFromString(dest, size);
        auto val = Operands::readOperand(op_s);
        Operands::writeOperand(op_d, val);
        
        // Check if source is a label reference like $v, $label
        bool isLabelRef = false;
        if(src.length() > 1 && src[0] == '$'){
            std::string labelPart = src.substr(1);
            isLabelRef = (Mem::labels.count(labelPart) > 0);
        }
        
        if(op_s.type == Operands::OperandType::ADDRESS || 
           op_d.type == Operands::OperandType::ADDRESS ||
           isLabelRef){
            if(size == 4)
                out << "movl " << src << ", " << dest << '\n';
            else if(size == 2)
                out << "movw " << src << ", " << dest << '\n';
            else if(size == 1)
                out << "movb " << src << ", " << dest << '\n';
        } else {
            // Daca src e registru sau o valoare instanta sa scrie cu valoarea simulata
            if(size == 4)
                out << "movl" << " $" << val << ", " << dest << '\n';
            else if(size == 2)
                out << "movw" << " $" << val << ", " << dest << '\n';
            else if(size == 1)
                out << "movb" << " $" << val << ", " << dest << '\n';
        }
    }



    void _or(std::string src, std::string dest, std::ofstream& out, uint8_t size){
        Operands::Operand op_s, op_d;
        op_s = getOperandFromString(src, size);
        op_d = getOperandFromString(dest, size);
        resetFlags();
        auto val_s = Operands::readOperand(op_s);
        auto val_d = Operands::readOperand(op_d);
        val_d = val_d | val_s;
        Operands::writeOperand(op_d, val_d);
        
        if(op_d.type == Operands::OperandType::ADDRESS){
            if(size == 4) out << "orl " << src << ", " << dest << '\n';
            else if(size == 2) out << "orw " << src << ", " << dest << '\n';
            else if(size == 1) out << "orb " << src << ", " << dest << '\n';
        } else {
            if(size == 4) out << "movl $" << val_d << ", " << dest << '\n';
            else if(size == 2) out << "movw $" << val_d << ", " << dest << '\n';
            else if(size == 1) out << "movb $" << val_d << ", " << dest << '\n';
        }
    }
    void _xor(std::string src, std::string dest, std::ofstream& out, uint8_t size){
        Operands::Operand op_s, op_d;
        op_s = getOperandFromString(src, size);
        op_d = getOperandFromString(dest, size);
        resetFlags();
        auto val_s = Operands::readOperand(op_s);
        auto val_d = Operands::readOperand(op_d);
        val_d = val_d ^ val_s;
        Operands::writeOperand(op_d, val_d);
        
        if(op_d.type == Operands::OperandType::ADDRESS){
            if(size == 4) out << "xorl " << src << ", " << dest << '\n';
            else if(size == 2) out << "xorw " << src << ", " << dest << '\n';
            else if(size == 1) out << "xorb " << src << ", " << dest << '\n';
        } else {
            if(size == 4) out << "movl $" << val_d << ", " << dest << '\n';
            else if(size == 2) out << "movw $" << val_d << ", " << dest << '\n';
            else if(size == 1) out << "movb $" << val_d << ", " << dest << '\n';
        }
    }
    void _and(std::string src, std::string dest, std::ofstream& out, uint8_t size){
        Operands::Operand op_s, op_d;
        op_s = getOperandFromString(src, size);
        op_d = getOperandFromString(dest, size);
        resetFlags();
        auto val_s = Operands::readOperand(op_s);
        auto val_d = Operands::readOperand(op_d);
        val_d = val_d & val_s;
        Operands::writeOperand(op_d, val_d);
        
        if(op_d.type == Operands::OperandType::ADDRESS){
            if(size == 4) out << "andl " << src << ", " << dest << '\n';
            else if(size == 2) out << "andw " << src << ", " << dest << '\n';
            else if(size == 1) out << "andb " << src << ", " << dest << '\n';
        } else {
            if(size == 4) out << "movl $" << val_d << ", " << dest << '\n';
            else if(size == 2) out << "movw $" << val_d << ", " << dest << '\n';
            else if(size == 1) out << "movb $" << val_d << ", " << dest << '\n';
        }

    }

    void inc(std::string dest, std::ofstream& out, uint8_t size){
        Operands::Operand op_d;
        op_d = getOperandFromString(dest, size);
        resetFlags();
        auto val_d = Operands::readOperand(op_d);
        val_d = val_d + 1;
        Operands::writeOperand(op_d, val_d);
        
        if(val_d == 0){
            flags[E] = 1;
            flags[Z] = 1;
        }
        
        if(size == 4) out << "movl $" << val_d << ", " << dest << '\n';
        else if(size == 2) out << "movw $" << val_d << ", " << dest << '\n';
        else if(size == 1) out << "movb $" << val_d << ", " << dest << '\n';
    }
    void dec(std::string dest, std::ofstream& out, uint8_t size){
        Operands::Operand op_d;
        op_d = getOperandFromString(dest, size);
        resetFlags();
        auto val_d = Operands::readOperand(op_d);
        val_d = val_d - 1;
        Operands::writeOperand(op_d, val_d);
        
        if(val_d == 0){
            flags[E] = 1;
            flags[Z] = 1;
        }
        
        if(size == 4) out << "movl $" << val_d << ", " << dest << '\n';
        else if(size == 2) out << "movw $" << val_d << ", " << dest << '\n';
        else if(size == 1) out << "movb $" << val_d << ", " << dest << '\n';
    }

    void shl(std::string src, std::string dest, std::ofstream& out, uint8_t size){
        Operands::Operand op_s, op_d;
        op_s = getOperandFromString(src, size);
        op_d = getOperandFromString(dest, size);
        resetFlags();
        auto val_s = Operands::readOperand(op_s);
        auto val_d = Operands::readOperand(op_d);
        val_d = val_d << val_s;
        Operands::writeOperand(op_d, val_d);
        
        if(op_d.type == Operands::OperandType::ADDRESS){
            if(size == 4) out << "shll " << src << ", " << dest << '\n';
            else if(size == 2) out << "shlw " << src << ", " << dest << '\n';
            else if(size == 1) out << "shlb " << src << ", " << dest << '\n';
        } else {
            if(size == 4) out << "movl $" << val_d << ", " << dest << '\n';
            else if(size == 2) out << "movw $" << val_d << ", " << dest << '\n';
            else if(size == 1) out << "movb $" << val_d << ", " << dest << '\n';
        }
    }

    void shr(std::string src, std::string dest, std::ofstream& out, uint8_t size){
        Operands::Operand op_s, op_d;
        op_s = getOperandFromString(src, size);
        op_d = getOperandFromString(dest, size);
        resetFlags();
        auto val_s = Operands::readOperand(op_s);
        auto val_d = Operands::readOperand(op_d);
        val_d = static_cast<int32_t>(static_cast<uint32_t>(val_d) >> val_s);
        Operands::writeOperand(op_d, val_d);
        
        if(op_d.type == Operands::OperandType::ADDRESS){
            if(size == 4) out << "shrl " << src << ", " << dest << '\n';
            else if(size == 2) out << "shrw " << src << ", " << dest << '\n';
            else if(size == 1) out << "shrb " << src << ", " << dest << '\n';
        } else {
            if(size == 4) out << "movl $" << val_d << ", " << dest << '\n';
            else if(size == 2) out << "movw $" << val_d << ", " << dest << '\n';
            else if(size == 1) out << "movb $" << val_d << ", " << dest << '\n';
        }
    }

    void sar(std::string src, std::string dest, std::ofstream& out, uint8_t size){
        Operands::Operand op_s, op_d;
        op_s = getOperandFromString(src, size);
        op_d = getOperandFromString(dest, size);
        resetFlags();
        auto val_s = Operands::readOperand(op_s);
        auto val_d = Operands::readOperand(op_d);
        val_d = val_d >> val_s;
        Operands::writeOperand(op_d, val_d);
        
        if(op_d.type == Operands::OperandType::ADDRESS){
            if(size == 4) out << "sarl " << src << ", " << dest << '\n';
            else if(size == 2) out << "sarw " << src << ", " << dest << '\n';
            else if(size == 1) out << "sarb " << src << ", " << dest << '\n';
        } else {
            if(size == 4) out << "movl $" << val_d << ", " << dest << '\n';
            else if(size == 2) out << "movw $" << val_d << ", " << dest << '\n';
            else if(size == 1) out << "movb $" << val_d << ", " << dest << '\n';
        }
    }

    void lea(std::string src, std::string dest, std::ofstream& out, uint8_t size){
        Operands::Operand op_s, op_d;
        op_s = getOperandFromString(src, size);
        op_d = getOperandFromString(dest, size);
        resetFlags();
        if(op_s.type == Operands::OperandType::ADDRESS){
            Operands::writeOperand(op_d, op_s.address);
            if(size == 4)
                out << "movl $" << src << ", " << dest << '\n';
            else if(size == 2)
                out << "movw $" << src << ", " << dest << '\n';
            else if(size == 1)
                out << "movb $" << src << ", " << dest << '\n';
        }
    }

    void push(std::string src, std::ofstream& out, uint8_t size){
        Operands::Operand op_s;
        op_s = getOperandFromString(src, size);
        auto val_s = Operands::readOperand(op_s);
        Registers::esp -= 4;
        Operands::Operand stack ={
            .type=Operands::OperandType::ADDRESS,
            .size=4,
            .address=(uint32_t)Registers::esp
        };
        Operands::writeOperand(stack, val_s);
        
        if(size == 4){
            out << "pushl " << src << "\n";
        }
        else if(size == 2){
            out << "pushw " << src << "\n";
        }
        else if(size == 1){
            out << "pushb " << src << "\n";
        }
    }

    void pop(std::string dest, std::ofstream& out, uint8_t size){
        Operands::Operand op_d;
        op_d = getOperandFromString(dest, size);
        Operands::Operand stack ={
            .type=Operands::OperandType::ADDRESS,
            .size=4,
            .address=(uint32_t)Registers::esp
        };
        auto val_d = Operands::readOperand(stack);
        Operands::writeOperand(op_d, val_d);
        Registers::esp += 4;
        
        if(size == 4){
            out << "popl " << dest << "\n";
        }
        else if(size == 2){
            out << "popw " << dest << "\n";
        }
        else if(size == 1){
            out << "popb " << dest << "\n";
        }
    }

    void test(std::string src, std::string dest, std::ofstream& out, uint8_t size){
        Operands::Operand op_s, op_d;
        op_s = getOperandFromString(src, size);
        op_d = getOperandFromString(dest, size);
        resetFlags();

        auto val_s = Operands::readOperand(op_s);
        auto val_d = Operands::readOperand(op_d);
        auto result = val_s & val_d;

        if(result == 0){
            flags[E] = 1;
            flags[Z] = 1;
        }
    }

    void cmp(std::string src, std::string dest, std::ofstream& out, uint8_t size){
        Operands::Operand op_s, op_d;
        op_s = getOperandFromString(src, size);
        op_d = getOperandFromString(dest, size);
        resetFlags();
        
        auto val_s = Operands::readOperand(op_s);
        auto val_d = Operands::readOperand(op_d);
        if(val_d <= val_s)
            flags[LE] = 1;
        if(val_d >= val_s)
            flags[GE] = 1;
        if(val_d == val_s){
            flags[E] = 1;
            flags[Z] = 1;
        }
            
        if(val_d < val_s)
            flags[L] = 1;
        if(val_d > val_s)
            flags[G] = 1;
        if(static_cast<uint32_t>(val_d) > static_cast<uint32_t>(val_s))
            flags[A] = 1;
        if(static_cast<uint32_t>(val_d) >= static_cast<uint32_t>(val_s))
            flags[AE] = 1;
    }
    void jmp(std::string targetLabel, std::ofstream& out){
        Registers::eip = Instr::instr_labels[targetLabel];
        currentLabel = targetLabel;
    }

    void loop(std::string targetLabel, std::ofstream& out){
        Registers::ecx--;
        if(Registers::ecx != 0){
            Registers::eip = Instr::instr_labels[targetLabel];
            Instr::currentLabel = targetLabel;
        } else {
            // When loop doesn't jump, increment eip normally
            Registers::eip++;
        }
    }
    void call(std::string targetLabel, std::ofstream& out){
        if(instr_labels.count(targetLabel) == 0){
            out << "call " << targetLabel << '\n';
            Registers::eip++;  // For external calls, increment eip manually
            return;
        }
        
        uint32_t returnAddr = static_cast<uint32_t>(Registers::eip + 1);
        Registers::esp -= 4;
        Operands::Operand stackSlot = {
            .type=Operands::OperandType::ADDRESS,
            .size=4,
            .address=(uint32_t)Registers::esp
        };
        Operands::writeOperand(stackSlot, static_cast<int32_t>(returnAddr));

        Registers::eip = Instr::instr_labels[targetLabel];
        Instr::currentLabel = targetLabel;
    }
    void ret(std::ofstream& out){
        Operands::Operand stackSlot = {
            .type=Operands::OperandType::ADDRESS,
            .size=4,
            .address=(uint32_t)Registers::esp
        };
        uint32_t returnAddr = static_cast<uint32_t>(Operands::readOperand(stackSlot));
        Registers::esp += 4;

        if(returnAddr < Instr::instructions.size()){
            Registers::eip = static_cast<int32_t>(returnAddr) - 1;
        } else {
            Registers::eip = Instr::instructions.size();
        }
    }
}

int main(int argc, char* argv[]){

    if(!fs::exists("asmOut")) {
        fs::create_directory("asmOut");
    }

    if(argc > 1){
        for(size_t i = 1; i<argc; i++){
            // Reset cand citim un fisier nou
            Instr::resetAll();
            
            std::string inputFile = "./asmFiles/";
            inputFile = inputFile + argv[i];
            std::ifstream in(inputFile);
            if(!in){
                std::cerr << "File " << argv[i] << " doesn't exist!\n";
                continue;
            }
            std::cout << argv[i] << ": " << '\n';

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
            uint32_t instr_counter = 0;
            while(std::getline(in, line)){
                // Remove comments (starting with # or ;)
                size_t commentPos = line.find_first_of("#;");
                if(commentPos != std::string::npos){
                    line = line.substr(0, commentPos);
                }
                // Trim trailing whitespace
                line.erase(line.find_last_not_of(" \t\r\n") + 1);
                
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
                    
                    if(line.find(".extern") == 0){
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
                        else if(type == ".long")
                            size = 4;
                        else if(type == ".space")
                            size = 1;

                        Mem::labels[labelName] = {size, address};
                        
                        if(type == ".space"){
                            lineWords >> value;
                            uint32_t numBytes = static_cast<uint32_t>(std::stoul(value, nullptr, 0));
                            for(uint32_t i = 0; i < numBytes; i++){
                                Mem::memory[address + i] = 0;
                            }
                            Mem::memoryPeak += numBytes;
                            continue; 
                        }
                        
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
                            Instr::currentLabel = word;
                            out << line << '\n';
                        }else{
                            if(line.back()==':'){
                                Instr::instructions.push_back(line+'\n');
                                line.pop_back();
                                Instr::instr_labels[line] = instr_counter;
                                instr_counter++;
                            }else{
                                Instr::instructions.push_back(line+'\n');
                                instr_counter++;
                            }
                        }
                        
                    }
                }
                
            }
            
            out << Instr::currentLabel+":" << '\n';
            Registers::eip = Instr::instr_labels[Instr::currentLabel];
            while(Registers::eip< Instr::instructions.size()){

                std::string line = Instr::instructions[Registers::eip];
                std::string originalLine = line;
            
                // If line contains %esp, output it as-is
                if(line.find("%esp") != std::string::npos){
                    out << line;
                    Registers::eip++;
                    continue;
                }
            
                if(!line.empty() && line.back()=='\n') line.pop_back();
                if(!line.empty() && line.back()==':'){
                    line.pop_back();
                    Instr::currentLabel = line;
                    Registers::eip++;
                    continue;
                }
                
                std::istringstream instructionExtractor(line);
                std::string instruction;
                instructionExtractor >> instruction;
                
                // Extract operands from original line (before comma replacement)
                std::string src, dest;
                size_t instrEnd = line.find(instruction) + instruction.length();
                std::string operandsStr = line.substr(instrEnd);
                
                // Remove leading spaces
                operandsStr.erase(0, operandsStr.find_first_not_of(" \t"));
                
                // Find the comma that separates operands (not inside parentheses)
                size_t lastComma = std::string::npos;
                int parenDepth = 0;
                for(size_t i = operandsStr.length(); i-- > 0; ){
                    if(operandsStr[i] == ')') parenDepth++;
                    else if(operandsStr[i] == '(') parenDepth--;
                    else if(operandsStr[i] == ',' && parenDepth == 0){
                        lastComma = i;
                        break;
                    }
                }
                
                if(lastComma != std::string::npos){
                    src = operandsStr.substr(0, lastComma);
                    dest = operandsStr.substr(lastComma + 1);
                } else {
                    // Single operand instruction or two operands without comma
                    size_t spacePos = operandsStr.find_first_of(" \t");
                    if(spacePos != std::string::npos){
                        src = operandsStr.substr(0, spacePos);
                        dest = operandsStr.substr(spacePos);
                        dest.erase(0, dest.find_first_not_of(" \t"));
                    } else {
                        src = operandsStr;
                    }
                }
                
                // Trim whitespace from operands
                src.erase(src.find_last_not_of(" \t") + 1);
                src.erase(0, src.find_first_not_of(" \t"));
                dest.erase(dest.find_last_not_of(" \t") + 1);
                dest.erase(0, dest.find_first_not_of(" \t"));

                if(instruction == "mov" || instruction == "movl"){
                    Instr::mov(src, dest, out, 4);
                }else if(instruction == "movw"){
                    Instr::mov(src, dest, out, 2);
                }else if(instruction == "movb"){
                    Instr::mov(src, dest, out, 1);
                }
                /*------------------------------*/
                else if(instruction == "add")
                    Instr::add(src, dest, out, 4);
                else if(instruction == "addl")
                    Instr::add(src, dest, out, 4);
                else if(instruction == "addw")
                    Instr::add(src, dest, out, 2);
                else if(instruction == "addb")
                    Instr::add(src, dest, out, 1);
                /*-------------------------------*/
                else if(instruction == "sub")
                    Instr::sub(src, dest, out, 4);
                else if(instruction == "subl")
                    Instr::sub(src, dest, out, 4);
                else if(instruction == "subw")
                    Instr::sub(src, dest, out, 2);
                else if(instruction == "subb")
                    Instr::sub(src, dest, out, 1);
                /*--------------------------------*/
                else if(instruction == "div" || instruction == "divl")
                    Instr::div(src, out);
                else if(instruction == "divw")
                    Instr::divw(src, out);
                else if(instruction == "divb")
                    Instr::divb(src, out);
                /*---------------------------------*/
                else if(instruction == "mul" || instruction == "mull")
                    Instr::mul(src, out);
                else if(instruction == "mulw")
                    Instr::mulw(src, out);
                else if(instruction == "mulb")
                    Instr::mulb(src, out);
                
                /*---------------------------------*/
                else if(instruction == "or")
                    Instr::_or(src, dest, out, 4);
                else if(instruction == "orl")
                    Instr::_or(src, dest, out, 4);
                else if(instruction == "orw")
                    Instr::_or(src, dest, out, 2);
                else if(instruction == "orb")
                    Instr::_or(src, dest, out, 1);
                /*---------------------------------*/
                else if(instruction == "xor")
                    Instr::_xor(src, dest, out, 4);
                else if(instruction == "xorl")
                    Instr::_xor(src, dest, out, 4);
                else if(instruction == "xorw")
                    Instr::_xor(src, dest, out, 2);
                else if(instruction == "xorb")
                    Instr::_xor(src, dest, out, 1);
                /*---------------------------------*/
                else if(instruction == "and")
                    Instr::_and(src, dest, out, 4);
                else if(instruction == "andl")
                    Instr::_and(src, dest, out, 4);
                else if(instruction == "andw")
                    Instr::_and(src, dest, out, 2);
                else if(instruction == "andb")
                    Instr::_and(src, dest, out, 1);
                /*---------------------------------*/
                else if(instruction == "inc")
                    Instr::inc(src, out, 4);
                else if(instruction == "incl")
                    Instr::inc(src, out, 4);
                else if(instruction == "incw")
                    Instr::inc(src, out, 2);
                else if(instruction == "incb")
                    Instr::inc(src, out, 1);
                /*---------------------------------*/
                else if(instruction == "dec")
                    Instr::dec(src, out, 4);
                else if(instruction == "decl")
                    Instr::dec(src, out, 4);
                else if(instruction == "decw")
                    Instr::dec(src, out, 2);
                else if(instruction == "decb")
                    Instr::dec(src, out, 1);
                /*---------------------------------*/
                else if(instruction == "lea")
                    Instr::lea(src, dest, out, 4);
                else if(instruction == "leal")
                    Instr::lea(src, dest, out, 4);
                else if(instruction == "leaw")
                    Instr::lea(src, dest, out, 2);
                else if(instruction == "leab")
                    Instr::lea(src, dest, out, 1);
                /*---------------------------------*/
                else if(instruction == "push")
                    Instr::push(src, out, 4);
                else if(instruction == "pushl")
                    Instr::push(src, out, 4);
                else if(instruction == "pushw")
                    Instr::push(src, out, 2);
                else if(instruction == "pushb")
                    Instr::push(src, out, 1);
                /*---------------------------------*/
                else if(instruction == "pop")
                    Instr::pop(src, out, 4);
                else if(instruction == "popl")
                    Instr::pop(src, out, 4);
                else if(instruction == "popw")
                    Instr::pop(src, out, 2);
                else if(instruction == "popb")
                    Instr::pop(src, out, 1);
                /*---------------------------------*/
                else if(instruction == "test")
                    Instr::test(src, dest, out, 4);
                else if(instruction == "testl")
                    Instr::test(src, dest, out, 4);
                else if(instruction == "testw")
                    Instr::test(src, dest, out, 2);
                else if(instruction == "testb")
                    Instr::test(src, dest, out, 1);
                /*---------------------------------*/
                else if(instruction == "cmp")
                    Instr::cmp(src, dest, out, 4);
                else if(instruction == "cmpl")
                    Instr::cmp(src, dest, out, 4);
                else if(instruction == "cmpw")
                    Instr::cmp(src, dest, out, 2);
                else if(instruction == "cmpb")
                    Instr::cmp(src, dest, out, 1);
                /*---------------------------------*/       
                else if(instruction == "jl"){
                    if(Instr::flags[Instr::L] == 1){
                        Instr::jmp(src, out);
                        continue;
                    }
                }
                else if(instruction == "jle"){
                    if(Instr::flags[Instr::LE] == 1){
                        Instr::jmp(src, out);
                        continue;
                    }
                }
                else if(instruction == "je"){
                    if(Instr::flags[Instr::E] == 1){
                        Instr::jmp(src, out);
                        continue;
                    }
                }
                else if(instruction == "jge"){
                    if(Instr::flags[Instr::GE] == 1){
                        Instr::jmp(src, out);
                        continue;
                    }
                }
                else if(instruction == "jg"){
                    if(Instr::flags[Instr::G] == 1){
                        Instr::jmp(src, out);
                        continue;
                    }
                }
                else if(instruction == "ja"){
                    if(Instr::flags[Instr::A] == 1){
                        Instr::jmp(src, out);
                        continue;
                    }
                }
                else if(instruction == "jae"){
                    if(Instr::flags[Instr::AE] == 1){
                        Instr::jmp(src, out);
                        continue;
                    }
                }
                else if(instruction == "jne"){
                    if(Instr::flags[Instr::E] == 0){
                        Instr::jmp(src, out);
                        continue;
                    }
                }
                else if(instruction == "jz"){
                    if(Instr::flags[Instr::Z] == 1){
                        Instr::jmp(src, out);
                        continue;
                    }
                }
                else if(instruction == "jnz"){
                    if(Instr::flags[Instr::Z] == 0){
                        Instr::jmp(src, out);
                        continue;
                    }
                }
                else if(instruction == "jmp"){
                    Instr::jmp(src, out);
                    continue;
                }
                else if(instruction == "loop"){
                    Instr::loop(src, out);
                    continue;  
                }
                else if(instruction == "call"){
                    Instr::call(src, out);
                    continue;
                }
                else if(instruction == "ret"){
                    Instr::ret(out);
                    continue;
                }
                /*---------------------------------*/
                else if(instruction == "sar")
                    Instr::sar(src, dest, out, 4);
                else if(instruction == "sarl")
                    Instr::sar(src, dest, out, 4);
                else if(instruction == "sarw")
                    Instr::sar(src, dest, out, 2);
                else if(instruction == "sarb")
                    Instr::sar(src, dest, out, 1);
                /*---------------------------------*/
                else if(instruction == "shr")
                    Instr::shr(src, dest, out, 4);
                else if(instruction == "shrl")
                    Instr::shr(src, dest, out, 4);
                else if(instruction == "shrw")
                    Instr::shr(src, dest, out, 2);
                else if(instruction == "shrb")
                    Instr::shr(src, dest, out, 1);
                /*---------------------------------*/
                else if(instruction == "shl")
                    Instr::shl(src, dest, out, 4);
                else if(instruction == "shll")
                    Instr::shl(src, dest, out, 4);
                else if(instruction == "shlw")
                    Instr::shl(src, dest, out, 2);
                else if(instruction == "shlb")
                    Instr::shl(src, dest, out, 1);
                else if(instruction == "int")
                    out << Instr::instructions[Registers::eip];
                else std::cerr << instruction + " not known";
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
            // Handle binary literals with 0b prefix
            int32_t value;
            if(l.length() > 2 && l[0] == '0' && (l[1] == 'b' || l[1] == 'B')){
                value = static_cast<int32_t>(std::stoul(l.substr(2), nullptr, 2));
            } else {
                value = static_cast<int32_t>(std::stoul(l, nullptr, 0));
            }
            return {.type=Operands::OperandType::IMMEDIATE, .imm=value};
        }
    }else if(str[0] == '%'){
        return {.type=Operands::OperandType::REGISTER, .regTag=Registers::stringToTag[str]};
    }else if(str[0] == '('){
        
        size_t closePos = str.find(')');
        std::string innerStr = str.substr(1, closePos - 1);
        
        // Replace commas with spaces for proper tokenization
        std::replace(innerStr.begin(), innerStr.end(), ',', ' ');
        
        // Check for indexed addressing
        std::istringstream iss(innerStr);
        std::string baseStr, indexStr, scaleStr;
        iss >> baseStr;
        
        if(iss >> indexStr){
            // Has index register and possibly scale (indexed addressing)
            int32_t scale = 1;
            if(iss >> scaleStr){
                scale = static_cast<int32_t>(std::stol(scaleStr, nullptr, 0));
            }
            
            Registers::RegDef baseData = Registers::regData[Registers::stringToTag[baseStr]];
            Registers::RegDef indexData = Registers::regData[Registers::stringToTag[indexStr]];
            uint32_t addr = (uint32_t)(*baseData.base_register) + 
                           (uint32_t)(*indexData.base_register) * scale;
            
            return {.type=Operands::OperandType::ADDRESS,
                    .size=size,
                    .address=addr};
        }else{
            // Simple indirect addressing (%eax)
            Registers::RegDef baseData = Registers::regData[Registers::stringToTag[baseStr]];
            uint32_t addr = (uint32_t)(*baseData.base_register);
            
            return {.type=Operands::OperandType::ADDRESS,
                    .size=size,
                    .address=addr};
        }
    }else if(str.find('(') != std::string::npos){
        size_t openPos = str.find('(');
        std::string dispStr = str.substr(0, openPos);
        size_t closePos = str.find(')');
        std::string innerStr = str.substr(openPos + 1, closePos - openPos - 1);
        
        int32_t displacement = 0;
        if(!dispStr.empty()){
            displacement = static_cast<int32_t>(std::stol(dispStr, nullptr, 0));
        }
        
        // Replace commas with spaces for proper tokenization
        std::replace(innerStr.begin(), innerStr.end(), ',', ' ');
        
        // Check for indexed addressing
        std::istringstream iss(innerStr);
        std::string baseStr, indexStr, scaleStr;
        iss >> baseStr;
        
        if(iss >> indexStr){
            // Has index register and possibly scale (indexed addressing)
            int32_t scale = 1;
            if(iss >> scaleStr){
                scale = static_cast<int32_t>(std::stol(scaleStr, nullptr, 0));
            }
            
            Registers::RegDef baseData = Registers::regData[Registers::stringToTag[baseStr]];
            Registers::RegDef indexData = Registers::regData[Registers::stringToTag[indexStr]];
            uint32_t addr = displacement + 
                           (uint32_t)(*baseData.base_register) + 
                           (uint32_t)(*indexData.base_register) * scale;
            
            return {.type=Operands::OperandType::ADDRESS,
                    .size=size,
                    .address=addr};
        }else{
            // Simple indirect addressing with displacement
            Registers::RegDef baseData = Registers::regData[Registers::stringToTag[baseStr]];
            uint32_t addr = displacement + (uint32_t)(*baseData.base_register);
            
            return {.type=Operands::OperandType::ADDRESS,
                    .size=size,
                    .address=addr};
        }
    }else{
        return {.type=Operands::OperandType::ADDRESS, 
                .size=size,
                .address=Mem::labels[str].address};
    }

    std::cerr << "Nu exista acest tip: " + str; 
}