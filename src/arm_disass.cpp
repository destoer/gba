#include "headers/disass.h"
#include "headers/arm.h"
#include "headers/cpu.h"




void Disass::init_arm_disass_table()
{
    disass_arm_table.resize(4096);

    for(int i = 0; i < 4096; i++)
    {
        switch(i >> 10) // bits 27 and 26 of opcode
        {
            case 0b00:
            {

                 int op = (i >> 5) & 0xf;

                //ARM.3: Branch and Exchange
                // bx
                if(i == 0b000100100001) // <--- start here
                {
                    disass_arm_table[i] = disass_arm_branch_and_exchange;
                }


                // msr and mrs
                // ARM.6: PSR Transfer
                // bit 24-23 must be 10 for this instr
                // bit 20 must also be zero
                // check it ocupies the unused space for
                //TST,TEQ,CMP,CMN with a S of zero
                else if(op >= 0b1000 && op <= 0b1011)
                {
                    disass_arm_table[i] = disass_arm_psr;
                }

                //  ARM.5: Data Processing 00 at bit 27
                else
                {
                    disass_arm_table[i] = disass_arm_data_processing;
                }
                break;
            }

            case 0b01:
            {
                //ARM.9: Single Data Transfer
                if(true) // assume for now
                {
                    disass_arm_table[i] = disass_arm_single_data_transfer;
                }

                else 
                {
                    disass_arm_table[i] = disass_arm_unknown;
                }
                break;
            }

            case 0b10:
            {

                // 101 (ARM.4: Branch and Branch with Link)
                if(is_set(i,9)) // if bit 25 set
                {
                    disass_arm_table[i] = disass_arm_branch;
                }

                else 
                {
                    disass_arm_table[i] = disass_arm_unknown;
                }

                break;
            }

            case 0b11:
            {
                disass_arm_table[i] = disass_arm_unknown;
                break;
            }
        }        
    }
}

std::string Disass::disass_arm(uint32_t opcode,uint32_t pc)
{
    this->pc = pc; 
    uint32_t op = get_arm_opcode_bits(opcode);

    return std::invoke(disass_arm_table[op],this,opcode);
}



std::string Disass::disass_arm_get_cond_suffix(int opcode)
{

    const int cond_bits = (opcode >> 28) & 0xf;
    return std::string(suf_array[cond_bits]);
}






// get a shift type diassembeld from an opcode
std::string Disass::disass_arm_get_shift_string(uint32_t opcode)
{
    int shift_type = (opcode >> 5) & 3;
    int rm = opcode & 0xf;
    // shift type on register
    if(is_set(opcode,4))
    {
        int rs = (opcode >> 8) & 0xf;

        //r1, asr r2
        return fmt::format("{},{} {}",user_regs_names[rm],shift_names[shift_type],user_regs_names[rs]);
    }

    else // shift type on 5 bit immediate
    {
        int imm = (opcode >> 7) & 0x1f;
        // egr1, asr #0x2

        if(imm != 0)
        {
            return fmt::format("{},{} #0x{:x}",user_regs_names[rm],shift_names[shift_type],imm);
        }

        else // just a register to register
        {          
            return std::string(user_regs_names[rm]); 
        }            
    } 
}


std::string Disass::disass_arm_branch_and_exchange(uint32_t opcode)
{
    std::string suffix = disass_arm_get_cond_suffix(opcode);

    int rn = opcode & 0xf;

    return fmt::format("bx{} {}",suffix,user_regs_names[rn]);
}

// psr transfer 
std::string Disass::disass_arm_psr(uint32_t opcode)
{
    // determine wether we are doing msr or mrs
    // and what kind we are doing
    std::string output;

    bool is_msr = is_set(opcode,21); // 21 set msr else mrs

    std::string suffix = disass_arm_get_cond_suffix(opcode);

    bool spsr = is_set(opcode,22);

    std::string sr;

    if(spsr)
    {
        int mode = cpu->get_mode();

        if(mode == USER || mode == SYSTEM)
        {
            puts("undefined banked psr register");
            return "";
        }

        sr = status_banked_names[mode];
    }

    else 
    {
        sr = "cpsr";
    }


    if(is_msr) //msr (unsure this handles the different write operations eg flag only)
    {
        int rm = opcode & 0xf;
        // msr to psr
        if(is_set(opcode,15)) 
        {
            output = fmt::format("msr{} {}, {}",suffix,sr,user_regs_names[rm]); 
        }

        // msr to psr (flag bits only)
        else
        {
            if(is_set(opcode,25))
            {
                output = fmt::format("msr{} {}_flg, {}",suffix,sr,get_arm_operand2_imm(opcode));
            }

            else
            {
                output = fmt::format("msr{} {}_flg, {}",suffix,sr,user_regs_names[rm]);
            } 
        }
    }

    else // mrs
    {
        int rd = (opcode >> 12) & 0xf;
        output = fmt::format("mrs{} {}, {}",suffix,user_regs_names[rd],sr); 
    }
    return output;
}

std::string Disass::disass_arm_data_processing(uint32_t opcode)
{
    // get the register and immediate
    std::string suffix = disass_arm_get_cond_suffix(opcode);
    if(is_set(opcode,20)) // if s bit is set
    {
        suffix += 's';
    }

    int rd = (opcode >> 12) & 0xf;

    int rn = (opcode >> 16) & 0xf;

    // if i bit its an immediate else its a shifted reg or value
    std::string operand2 = is_set(opcode,25)? fmt::format("#0x{:08x}",get_arm_operand2_imm(opcode)) : disass_arm_get_shift_string(opcode); 

    // what instr is it
    int op = (opcode >> 21) & 0xf;
    switch((opcode >> 21) & 0xf)
    {

        case 0x4: // add
        {
            return fmt::format("add{} {},{},{}",suffix,user_regs_names[rd],user_regs_names[rn],operand2);
            break;
        }

        case 0xd: // mov
        {
            return fmt::format("mov{} {},{}",suffix,user_regs_names[rd],operand2);
            break;
        }

        default:
        {
            printf("unknown data processing diass %08x\n",op);
            exit(1);
        }
    }
}

std::string Disass::disass_arm_branch(uint32_t opcode)
{
    pc += 4;

    std::string output;


    // 24 bit offset is shifted left 2
    // and extended to a 32 bit int
    int32_t offset = (opcode & 0xffffff) << 2;

    pc += offset;


    std::string suffix = disass_arm_get_cond_suffix(opcode);


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

// str, ldr

// when u get back to this handle post indexed and the register offset
std::string Disass::disass_arm_single_data_transfer(uint32_t opcode)
{
    
    std::string output;

    // get the instr (load / store bit)
    std::string instr = is_set(opcode,20)? "ldr" : "str";
    std::string suffix = disass_arm_get_cond_suffix(opcode);


    suffix += is_set(opcode,22) ? "b" : ""; // is byte str

    int rd = (opcode >> 12) & 0xf;
    int base = (opcode >> 16) & 0xf;


    std::string imm;

    // register offset
    if(is_set(opcode,25))
    {
        imm = disass_arm_get_shift_string(opcode);
    }

    else // immeditate
    {
        // 12 bit immediate
        int v = opcode & 0xfff;
        imm = fmt::format("#0x{:x}",v);
    }

    if(is_set(opcode,24)) // pre indexed
    {
        // write back enabled?
        std::string w = is_set(opcode,21) ? "!" : ""; 
        output = fmt::format("{}{} {},[{},{}]{}",instr,suffix,
            user_regs_names[rd],user_regs_names[base],imm,w);
    }

    else // post indexed
    {
        // if post indexed add a t if writeback set
        // this forces user mode in a system context
        suffix += is_set(opcode,21) ? "t" : "";
        output = fmt::format("{}{} {},[{}],{}",instr,suffix,user_regs_names[rd],
            user_regs_names[base],imm);
    }

    return output;
}


std::string Disass::disass_arm_unknown(uint32_t opcode)
{
    uint32_t op = ((opcode >> 4) & 0xf) | ((opcode >> 16) & 0xff0);
    printf("[disass-arm]%08x:unknown opcode %08x:%08x\n",pc,opcode,op);
    cpu->print_regs();
    exit(1);   
}