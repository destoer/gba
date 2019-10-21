#include "headers/arm_disass.h"
#include "headers/arm.h"
#include "headers/cpu.h"


const static char *shift_names[4] = 
{
    "lsl",
    "lsr",
    "asr",
    "ror"
};

void Disass::init(Mem *mem, Cpu *cpu)
{
    this->mem = mem;
    this->cpu = cpu;
    init_disass_table();
}

void Disass::init_disass_table()
{
    disass_opcode_table.resize(4096);

    for(int i = 0; i < 4096; i++)
    {
        // 101 << 9 (branch)
        if((i & 0xa00) == 0xa00)
        {
            disass_opcode_table[i] = disass_branch;
        }

        // arm data processing 00 at bit 27
        // more specific ones need to preceed this
        else if((i & 0x200) == 00)
        {
             // now we have to switch on its "opcode field"
            switch((i >> 5) & 0xf)
            {
                case 0xd: // mov
                {
                    disass_opcode_table[i] = disass_mov;
                    break;
                }

                default:
                {
                    disass_opcode_table[i] = disass_unknown;
                    break;
                }
            }
        }

        else 
        {
            disass_opcode_table[i] = disass_unknown;
        }        
    }
}

std::string Disass::disass_arm(uint32_t opcode,uint32_t pc)
{
    this->pc = pc; 
    uint32_t op = get_arm_opcode_bits(opcode);

    return std::invoke(disass_opcode_table[op],this,opcode);
}



std::string Disass::get_cond_suffix(int opcode)
{

    const int cond_bits = (opcode >> 28) & 0xf;
    const static std::string suf_array[16] =
    {
        "eq",
        "ne",
        "cs",
        "cc",
        "mi",
        "pl",
        "vs",
        "vc",
        "hi",
        "ls",
        "ge",
        "lt",
        "gt",
        "le",
        "", // AL
        "" // undefined
    };


    return std::string(suf_array[cond_bits]);
}





// all the below need to handle the encoding variants

std::string Disass::disass_mov(uint32_t opcode)
{
    // get the register and immediate
    std::string suffix = get_cond_suffix(opcode);
    if(is_set(opcode,20)) // if s bit is set
    {
        suffix += 's';
    }

    int rd = (opcode >> 12) & 0xf;

    // I bit set immediate and ror shift in increments of 2
    if(is_set(opcode,25))
    {
        uint32_t imm = get_arm_operand2_imm(opcode);
        return fmt::format("mov{} {}, #0x{:08x}",suffix,user_regs_names[rd],imm);
    }

    else // shift applied to register?
    {
        int shift_type = (opcode >> 5) & 3;
        int rm = opcode & 0xf;
        // shift type on register
        if(is_set(opcode,4))
        {
            int rs = (opcode >> 8) & 0xf;

            // eg movs r0, r1, asr r2
            return fmt::format("mov{} {},{}, {} {}",suffix,user_regs_names[rd],
                user_regs_names[rm],shift_names[shift_type],user_regs_names[rs]);
        }

        else // shift type on 5 bit immediate
        {
            int imm = (opcode >> 7) & 0x1f;
            // eg mov r0, r1, asr #0x2

            if(imm != 0)
            {
                return fmt::format("mov{} {}, {},{} #0x{:x}",suffix,user_regs_names[rd], 
                    user_regs_names[rm],shift_names[shift_type],imm);
            }

            else // just a register to register
            {
                return fmt::format("mov{} {}, {}",suffix,user_regs_names[rd], 
                    user_regs_names[rm]);                
            }

        }
    }

}

std::string Disass::disass_branch(uint32_t opcode)
{
    pc += 4;

    std::string output;


    // 24 bit offset is shifted left 2
    // and extended to a 32 bit int
    int32_t offset = (opcode & 0xffffff) << 2;

    pc += offset;


    std::string suffix = get_cond_suffix(opcode);


    // if the link bit is set this acts as a call instr
    if(is_set(opcode,L_BIT))
    {
        output = fmt::format("bal{} #0x{:08x}",suffix,pc);
    }


    else 
    {
        output = fmt::format("b{} #0x{:08x}",suffix,pc);
    }

    return output;
}

std::string Disass::disass_str(uint32_t opcode)
{
    std::string output;
    // register offset
    if(is_set(opcode,25))
    {
        puts("unhandled register offset!");
        exit(1);
    }

    else // immeditate
    {
        int imm = opcode & 0xfff;
        int rd = (opcode >> 12) & 0xf;
        int base = (opcode >> 16) & 0xf;
        output = fmt::format("str {},[{},#0x{:x}]",user_regs_names[rd],user_regs_names[base],imm);
    }
    return output;
}

std::string Disass::disass_ldr(uint32_t opcode)
{
    UNUSED(opcode); 
    puts("unhandled ldr");
    exit(1);
}


std::string Disass::disass_unknown(uint32_t opcode)
{
    uint32_t op = ((opcode >> 4) & 0xf) | ((opcode >> 16) & 0xff0);
    printf("[disass-arm]%08x:unknown opcode %08x:%08x\n",pc,opcode,op);
    cpu->print_regs();
    exit(1);   
}