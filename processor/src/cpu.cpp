#include "cpu.hpp"

#include <iostream>
#include <cstring>
#include <iomanip>

#include "constants.hpp"
#include "debug.hpp"

template<typename LHS, typename RHS>
static constants::cmp::flag calculate_cmp_flag(LHS lhs, RHS rhs) {
    using namespace constants;
    uint32_t flag = 0;
    if (lhs < rhs) flag |= static_cast<uint32_t>(cmp::lt);
    else if (lhs > rhs) flag |= static_cast<uint32_t>(cmp::gt);
    if (lhs == rhs) flag |= static_cast<uint32_t>(cmp::eq);
    if (rhs == 0) flag |= static_cast<uint32_t>(cmp::z);

    return static_cast<cmp::flag>(flag);
}

void processor::CPU::raise_error(constants::error::code code, uint64_t val) {
    flag_reset(constants::flag::is_running);
    regs[constants::registers::flag] |= (int(code) & constants::error::mask) << constants::error::offset;
    regs[constants::registers::ret] = val;
}

uint64_t processor::CPU::raise_error(constants::error::code code, uint64_t val, uint64_t ret) {
    raise_error(code, val);
    return ret;
}

// fetch `<reg> <reg> <value>`, return if OK. Index into instruction `HEADER_SIZE + offset`
// pass `is_double` to `get_arg` of value
bool processor::CPU::fetch_reg_reg_val(uint64_t inst, constants::registers::reg &reg1, constants::registers::reg &reg2,
                                       uint64_t &value, uint8_t offset, bool is_double) {
    using namespace constants::inst;

    reg1 = get_arg_reg(inst, header_size + offset);
    if (!is_running()) return false;

    reg2 = get_arg_reg(inst, header_size + offset + reg_size);
    if (!is_running()) return false;

    value = get_arg_value(inst, header_size + offset + 2 * reg_size, is_double);
    return is_running();
}

void processor::CPU::test_is_zero(constants::registers::reg reg) {
    if (regs[reg] == 0) {
        flag_set(constants::flag::zero);
    } else {
        flag_reset(constants::flag::zero);
    }

    if (debug.zflag)
        std::cout << DEBUG_STR ANSI_CYAN " zero flag" ANSI_RESET ": register $" << constants::registers::to_string(reg) << " -> " <<
                  (flag_test(constants::flag::zero) ? ANSI_GREEN "set" : ANSI_RED "reset") << ANSI_RESET << std::endl;
}

// load <reg> <value> -- load value into register
void processor::CPU::exec_load(uint64_t inst) {
    // fetch register, check if in bounds
    constants::registers::reg reg = get_arg_reg(inst, constants::inst::header_size);
    if (!check_register(reg)) {
        raise_error(constants::error::reg, reg, 0);
        return;
    }

    // fetch and resolve value, check if OK
    uint64_t value = get_arg_value(inst, constants::inst::header_size + constants::inst::reg_size, false);
    if (!is_running()) return;

    // assign value to register
    regs[reg] = value;

    if (debug.cpu)
        std::cout << DEBUG_STR " load: load value 0x" << std::hex << value << std::dec << " into register $"
                  << constants::registers::to_string(reg) << std::endl;
    test_is_zero(reg);
}

// loadu <reg> <value> -- load value into upper register
void processor::CPU::exec_load_upper(uint64_t inst) {
    // fetch register, check if in bounds
    constants::registers::reg reg = get_arg_reg(inst, constants::inst::header_size);
    if (!check_register(reg)) return raise_error(constants::error::reg, reg);

    // fetch and resolve value, check if OK
    uint64_t value = get_arg_value(inst, constants::inst::header_size + constants::inst::reg_size, false);
    if (!is_running()) return;

    // store value in register's upper 32 bits
    ((uint32_t *) &regs[reg])[1] = *(uint32_t *) &value;

    if (debug.cpu)
        std::cout << DEBUG_STR " loadu: load value 0x" << std::hex << value << std::dec << " into register $"
                  << constants::registers::to_string(reg) << "'s upper half" << std::endl;
    test_is_zero(reg);
}

// store <addr> <reg> -- store value from register in memory
void processor::CPU::exec_store(uint64_t inst) {
    // fetch register, check if in bounds
    constants::registers::reg reg = get_arg_reg(inst, constants::inst::header_size);
    if (!check_register(reg)) return raise_error(constants::error::reg, reg);

    // fetch address, check if OK
    uint32_t addr = get_arg_addr(inst, constants::inst::header_size + constants::inst::reg_size);
    if (!is_running()) return;

    // store in memory at address
    data_bus.store(addr, sizeof(uint64_t), regs[reg]);

    if (debug.cpu)
        std::cout << DEBUG_STR " store: copy register $" << constants::registers::to_string(reg) << " (0x" << std::hex << regs[reg]
                  << ") to address 0x" << addr << std::dec << std::endl;
    test_is_zero(reg);
}

// cmp <reg> <value> -- compare register with value
// e.g., `reg < value`
void processor::CPU::exec_compare(uint64_t inst) {
    using namespace constants;

    // fetch data type bits
    auto datatype = static_cast<inst::datatype::dt>((inst >> inst::header_size) & 0x7);

    // fetch register, check if OK
    constants::registers::reg reg = get_arg_reg(inst, inst::header_size + inst::datatype::size);
    if (!check_register(reg)) return raise_error(error::reg, reg);

    // fetch and resolve value, check if OK
    uint64_t value = get_arg_value(inst, inst::header_size + inst::datatype::size + inst::reg_size,
                                   datatype == inst::datatype::dbl);
    if (!is_running()) return;

    // deduce comparison flag depending on datatype
    cmp::flag flag;

    switch (datatype) {
        case inst::datatype::u64: {
            uint64_t lhs = regs[reg];
            flag = calculate_cmp_flag(lhs, value);
        }
            break;
        case inst::datatype::u32: {
            auto *lhs = (uint32_t *) &regs[reg], *rhs = (uint32_t *) &value;
            flag = calculate_cmp_flag(*lhs, *rhs);
        }
            break;
        case inst::datatype::s64: {
            auto *lhs = (int64_t *) &regs[reg], *rhs = (int64_t *) &value;
            flag = calculate_cmp_flag(*lhs, *rhs);
        }
            break;
        case inst::datatype::s32: {
            auto *lhs = (int32_t *) &regs[reg], *rhs = (int32_t *) &value;
            flag = calculate_cmp_flag(*lhs, *rhs);
        }
            break;
        case inst::datatype::flt: {
            auto *lhs = (float *) &regs[reg], *rhs = (float *) &value;
            flag = calculate_cmp_flag(*lhs, *rhs);
        }
            break;
        case inst::datatype::dbl: {
            auto *lhs = (double *) &regs[reg], *rhs = (double *) &value;
            flag = calculate_cmp_flag(*lhs, *rhs);
        }
            break;
        default:
            if (debug.errs)
                os << ANSI_RED "unknown data type indicator: 0x" << std::hex << datatype << std::dec << std::endl
                   << ANSI_RESET;
            raise_error(error::datatype, datatype);
    }

    // update flag bits in register
    regs[registers::flag] = (regs[registers::flag] & ~0xf) | (int(flag) & 0xf);

    if (debug.cpu) {
        std::cout << DEBUG_STR " cmp: datatype=" << inst::datatype::to_string(datatype) << " (0x" << datatype << std::endl
                  << DEBUG_STR " cmp: register $" << registers::to_string(reg) << " (0x" << regs[reg] << ") vs 0x" << value
                  << " = " ANSI_CYAN << cmp::to_string(flag) << std::endl << std::dec << ANSI_RESET;
    }
}

// not <reg> <reg>
void processor::CPU::exec_not(uint64_t inst) {
    // fetch register, check if OK
    auto reg_dst = get_arg_reg(inst, constants::inst::header_size);
    if (!check_register(reg_dst)) return raise_error(constants::error::reg, reg_dst);

    auto reg_src = get_arg_reg(inst, constants::inst::header_size + constants::inst::reg_size);
    if (!check_register(reg_src)) return raise_error(constants::error::reg, reg_src);

    // inverse source register, update flag
    regs[reg_dst] = ~regs[reg_src];

    if (debug.cpu)
        std::cout << DEBUG_STR " not: " << constants::registers::to_string(reg_dst) << " = ~0x" << std::hex << regs[reg_src]
                  << " = 0x" << regs[reg_dst] << std::dec << std::endl;
    test_is_zero(reg_dst);
}

// and <reg> <reg> <value>
void processor::CPU::exec_and(uint64_t inst) {
    constants::registers::reg reg_src, reg_dst;
    uint64_t value;

    if (!fetch_reg_reg_val(inst, reg_src, reg_dst, value, 0, false)) return;

    regs[reg_dst] = regs[reg_src] & value;

    if (debug.cpu)
        std::cout << DEBUG_STR " and: " << constants::registers::to_string(reg_src) << " (0x" << std::hex << regs[reg_src]
                  << ") & 0x" << value << " = 0x" << regs[reg_dst] << std::dec << std::endl;
    test_is_zero(reg_dst);
}

// or <reg> <reg> <value>
void processor::CPU::exec_or(uint64_t inst) {
    constants::registers::reg reg_src, reg_dst;
    uint64_t value;

    if (!fetch_reg_reg_val(inst, reg_src, reg_dst, value, 0, false)) return;

    regs[reg_dst] = regs[reg_src] | value;

    if (debug.cpu)
        std::cout << DEBUG_STR " or: " << constants::registers::to_string(reg_src) << " (0x" << std::hex << regs[reg_src] << ") | 0x"
                  << value << " = 0x" << regs[reg_dst] << std::dec << std::endl;
    test_is_zero(reg_dst);
}

// xor <reg> <reg> <value>
void processor::CPU::exec_xor(uint64_t inst) {
    constants::registers::reg reg_src, reg_dst;
    uint64_t value;

    if (!fetch_reg_reg_val(inst, reg_src, reg_dst, value, 0, false)) return;

    regs[reg_dst] = regs[reg_src] ^ value;
    if (debug.cpu)
        std::cout << DEBUG_STR " xor: " << constants::registers::to_string(reg_src) << " (0x" << std::hex << regs[reg_src]
                  << ") ^ 0x" << value << " = 0x" << regs[reg_dst] << std::dec << std::endl;
    test_is_zero(reg_dst);
}

// shr <reg> <reg> <value>
void processor::CPU::exec_shift_left(uint64_t inst) {
    constants::registers::reg reg_src, reg_dst;
    uint64_t value;

    if (!fetch_reg_reg_val(inst, reg_src, reg_dst, value, 0, false)) return;

    regs[reg_dst] = regs[reg_src] << value;
    if (debug.cpu)
        std::cout << DEBUG_STR " shl: 0x" << std::hex << regs[reg_src] << std::dec << " << " << value << " = 0x"
                  << std::hex << regs[reg_dst] << std::dec << std::endl;
    test_is_zero(reg_dst);
}

// shr <reg> <reg> <value>
void processor::CPU::exec_shift_right(uint64_t inst) {
    constants::registers::reg reg_src, reg_dst;
    uint64_t value;

    if (!fetch_reg_reg_val(inst, reg_src, reg_dst, value, 0, false)) return;

    regs[reg_dst] = regs[reg_src] >> value;
    if (debug.cpu)
        std::cout << DEBUG_STR " shr: 0x" << std::hex << regs[reg_src] << std::dec << " >> " << value << " = 0x"
                  << std::hex << regs[reg_dst] << std::dec << std::endl;
    test_is_zero(reg_dst);
}

// macro for arithmetic operation
#define ARITH_OPERATION(OPERATOR, INJECT) \
  auto datatype = static_cast<constants::inst::datatype::dt>((inst >> constants::inst::header_size) & 0x7);\
  constants::registers::reg reg_src, reg_dst;\
  uint64_t value, result;\
  if (!fetch_reg_reg_val(inst, reg_src, reg_dst, value, constants::inst::datatype::size, datatype == constants::inst::datatype::dbl))\
    return;\
  if (debug.cpu) std::cout << DEBUG_STR " arithmetic operation (on type " << constants::inst::datatype::to_string(datatype) << "): ";\
  switch (datatype) {\
    case constants::inst::datatype::u64: {\
      auto *rhs = (int32_t *) &value;\
      auto res = regs[reg_src] OPERATOR *rhs;                  \
      result = res;                                    \
      if (debug.cpu) std::cout << regs[reg_src] << " " #OPERATOR " " << *rhs << " = " << res << std::endl;\
      break;\
    }\
    case constants::inst::datatype::u32: {\
      auto *lhs = (uint32_t *) &regs[reg_src];\
      auto *rhs = (int32_t *) &value;\
      auto res = *lhs OPERATOR *rhs;      \
      result = res;                                    \
      if (debug.cpu) std::cout << *lhs << " " #OPERATOR " " << *rhs << " = " << res << std::endl;\
    }\
    break;\
    case constants::inst::datatype::s64: {\
      auto *lhs = (int64_t *) &regs[reg_src];\
      auto *rhs = (int32_t *) &value;\
      int64_t res = *lhs OPERATOR *rhs;\
      result = *(uint64_t *) &res;\
      if (debug.cpu) std::cout << *lhs << " " #OPERATOR " " << *rhs << " = " << res << std::endl;\
    }\
    break;\
    case constants::inst::datatype::s32: {\
      auto *lhs = (int32_t *) &regs[reg_src], *rhs = (int32_t *) &value, res = *lhs + *rhs;\
      result = *(uint64_t *) &res;\
      if (debug.cpu) std::cout << *lhs << " " #OPERATOR " " << *rhs << " = " << res << std::endl;\
    }\
    break;\
    case constants::inst::datatype::flt: {\
      auto *lhs = (float *) &regs[reg_src], *rhs = (float *) &value, res = *lhs OPERATOR *rhs;\
      result = *(uint64_t *) &res;\
      if (debug.cpu) std::cout << *lhs << " " #OPERATOR " " << *rhs << " = " << res << std::endl;\
    }\
    break;\
    case constants::inst::datatype::dbl: {\
      auto *lhs = (double *) &regs[reg_src], *rhs = (double *) &value, res = *lhs OPERATOR *rhs;\
      result = *(uint64_t *) &res;\
      if (debug.cpu) std::cout << *lhs << " " #OPERATOR " " << *rhs << " = " << res << std::endl;\
    }\
    break;\
    default:\
      if (debug.errs) std::cerr << ANSI_RED "unknown data type indicator: 0x" << std::hex << datatype << std::dec << std::endl;\
      return raise_error(constants::error::datatype, datatype);\
  }\
  INJECT\
  regs[reg_dst] = result;\
  test_is_zero(reg_dst);

// add <reg> <reg> <value>
void processor::CPU::exec_add(uint64_t inst) {
    ARITH_OPERATION(+,)
}

// sub <reg> <reg> <value>
void processor::CPU::exec_sub(uint64_t inst) {
    ARITH_OPERATION(-,)
}

// mul <reg> <reg> <value>
void processor::CPU::exec_mul(uint64_t inst) {
    ARITH_OPERATION(*,)
}

// div <reg> <reg> <value>
void processor::CPU::exec_div(uint64_t inst) {
    ARITH_OPERATION(/,)
}

// mod <reg> <value> <value>
void processor::CPU::exec_mod(uint64_t inst) {
    constants::registers::reg reg_dst, reg_src;
    uint64_t value;

    if (!fetch_reg_reg_val(inst, reg_dst, reg_src, value, 0, false))
        return;

    auto *lhs = (int64_t *) &regs[reg_src];
    auto *rhs = (int32_t *) &value;
    int64_t result = *lhs % *rhs;
    regs[reg_dst] = result;

    if (debug.cpu) std::cout << DEBUG_STR " mod: " << *lhs << " mod " << *rhs << " = " << result << std::endl;
    test_is_zero(reg_dst);
}

// syscall <value>
void processor::CPU::exec_syscall(uint64_t inst) {
    using namespace constants;

    // fetch and resolve value, check if OK
    uint64_t value = get_arg_value(inst, inst::header_size, false);
    if (!is_running()) return;

    if (debug.cpu) std::cout << DEBUG_STR " syscall: invoke operation " << value << " (";

    switch (static_cast<syscall>(value)) {
        case syscall::print_hex:
            if (debug.cpu) std::cout << "print_hex)" << std::endl;
            os << "0x" << std::hex << regs[registers::r1] << std::dec;
            break;
        case syscall::print_int:
            if (debug.cpu) std::cout << "print_int)" << std::endl;
            os << *(int *) &regs[registers::r1];
            break;
        case syscall::print_float:
            if (debug.cpu) std::cout << "print_float)" << std::endl;
            os << *(float *) &regs[registers::r1];
            break;
        case syscall::print_double:
            if (debug.cpu) std::cout << "print_double)" << std::endl;
            os << *(double *) &regs[registers::r1];
            break;
        case syscall::print_char:
            if (debug.cpu) std::cout << "print_char)" << std::endl;
            os << *(char *) &regs[registers::r1];
            break;
        case syscall::print_string: {
            if (debug.cpu) std::cout << "print_string)" << std::endl;
            uint32_t addr = regs[registers::r1];
            if (!check_memory(addr)) return raise_error(error::segfault, addr);
            os << (const char *) (data_bus.mem.data() + addr);
            break;
        }
        case syscall::read_int: {
            if (debug.cpu) std::cout << "read_int)" << std::endl;
            int n;
            is >> n;
            regs[registers::ret] = n;
            break;
        }
        case syscall::read_float: {
            if (debug.cpu) std::cout << "read_float)" << std::endl;
            float n;
            is >> n;
            regs[registers::ret] = *(uint32_t *) &n;
            break;
        }
        case syscall::read_double: {
            if (debug.cpu) std::cout << "read_double)" << std::endl;
            double n;
            is >> n;
            regs[registers::ret] = *(uint64_t *) &n;
            break;
        }
        case syscall::read_char: {
            if (debug.cpu) std::cout << "read_char)" << std::endl;
            char n;
            is >> n;
            regs[registers::ret] = *(uint8_t *) &n;
            break;
        }
        case syscall::read_string: {
            if (debug.cpu) std::cout << "read_string)" << std::endl;
            uint32_t addr = regs[registers::r1], length = regs[registers::r1 + 1];
            if (!check_memory(addr)) return raise_error(error::segfault, addr);
            is.read((char *) (data_bus.mem.data() + addr), length);
            break;
        }
        case syscall::exit:
            if (debug.cpu) std::cout << "exit)" << std::endl;
            halt();
            break;
        case syscall::print_regs:
            if (debug.cpu) std::cout << "print_regs)" << std::endl;
            print_registers();
            break;
        case syscall::print_mem: {
            if (debug.cpu) std::cout << "print_mem)" << std::endl;
            uint64_t addr = regs[registers::r1], size = regs[registers::r1 + 1];
            if (!check_memory(addr)) return raise_error(error::segfault, addr);
            if (!check_memory(addr + size - 1)) return raise_error(error::segfault, addr + size - 1);

            os << "Mem(0x" << std::hex << addr << ":0x" << addr + size - 1 << " = { ";
            for (uint32_t i = 0; i < size; i++)
                os << "0x" << std::setw(2) << std::setfill('0') << (int) data_bus.mem[addr + i] << " ";
            os << std::dec << "}";
            break;
        }
        case syscall::print_stack:
            if (debug.cpu) os << "print_stack)" << std::endl;
            print_stack();
            break;
        default:
            if (debug.cpu) std::cout << "unknown)" << std::endl;
            if (debug.errs)
                std::cerr << ANSI_RED "invocation of unknown syscall operation (" << value << ")" << std::endl;
            raise_error(error::syscall, value);
    }
}

template<typename T>
void processor::CPU::push(T val) {
    regs[constants::registers::sp] -= sizeof(val);
    data_bus.store(regs[constants::registers::sp], sizeof(val), val);
}

// push <value>
void processor::CPU::exec_push(uint64_t inst) {
    // fetch and resolve value, check if OK
    uint64_t value = get_arg_value(inst, constants::inst::header_size, false);
    if (!is_running()) return;

    uint32_t data = *(uint32_t *) &value;
    if (debug.cpu)
        std::cout << DEBUG_STR " push: value 0x" << std::hex << data << " to $sp = 0x" << regs[constants::registers::sp]
                  << std::dec << std::endl;

    push(data);
}

// jal <reg> <value>
void processor::CPU::exec_jal(uint64_t inst) {
    using namespace constants;

    registers::reg reg = get_arg_reg(inst, inst::header_size);
    if (!is_running()) return;

    uint64_t value = get_arg_value(inst, inst::header_size + inst::reg_size, false);
    if (!is_running()) return;

    if (debug.cpu)
        std::cout << DEBUG_STR " jal: cache $ip (0x" << std::hex << regs[registers::ip] << ") in "
                  << constants::registers::to_string(reg) << "; jump to 0x" << value << std::dec << std::endl;

    // cache + jump
    regs[reg] = regs[registers::ip];
    regs[registers::ip] = value;
}

template<typename T>
static uint64_t cast_value(constants::inst::datatype::dt dt, T src) {
    using namespace constants::inst::datatype;
    switch (dt) {
        case u32: {
            uint32_t tmp = src;
            return *(uint64_t *) &tmp;
        }
        case u64: {
            uint64_t tmp = src;
            return tmp;
        }
        case s32: {
            int32_t tmp = src;
            return *(uint64_t *) &tmp;
        }
        case s64: {
            int64_t tmp = src;
            return *(uint64_t *) &tmp;
        }
        case flt: {
            float tmp = src;
            return *(uint64_t *) &tmp;
        }
        case dbl: {
            double tmp = src;
            return *(uint64_t *) &tmp;
        }
        default:
            return 0;
    }
}

// cvt(d1)2(d2) reg reg
void processor::CPU::exec_convert(uint64_t inst) {
    using namespace constants::inst;

    // extra datatypes to convert from/to
    uint8_t pos = header_size;
    auto d1 = static_cast<datatype::dt>((inst >> pos) & 0x7);
    auto d2 = static_cast<datatype::dt>((inst >> (pos += datatype::size)) & 0x7);

    // extra source and destination registers
    constants::registers::reg reg_dst = get_arg_reg(inst, pos += datatype::size);
    constants::registers::reg reg_src = get_arg_reg(inst, pos += reg_size);

    uint64_t value = regs[reg_src];
    switch (d1) {
        case datatype::u32:
            value = cast_value(d2, *(uint32_t *) &value);
            break;
        case datatype::u64:
            value = cast_value(d2, value);
            break;
        case datatype::s32:
            value = cast_value(d2, *(int32_t *) &value);
            break;
        case datatype::s64:
            value = cast_value(d2, *(int64_t *) &value);
            break;
        case datatype::flt:
            value = cast_value(d2, *(float *) &value);
            break;
        case datatype::dbl:
            value = cast_value(d2, *(double *) &value);
            break;
        default:;
    }

    if (debug.cpu)
        std::cout << DEBUG_STR " cvt: convert from " << constants::inst::datatype::to_string(d1) << " in reg "
                  << constants::registers::to_string(reg_src) << " to " << constants::inst::datatype::to_string(d2) << " in reg "
                  << constants::registers::to_string(reg_dst) << std::endl;
    regs[reg_dst] = value;
    test_is_zero(reg_dst);
}

constants::registers::reg processor::CPU::_arg_reg(uint32_t data) {
    return static_cast<constants::registers::reg>(data);
}

uint32_t processor::CPU::_arg_addr(uint32_t data) {
    if (!check_memory(data)) return raise_error(constants::error::segfault, data, 0);
    return data;
}

constants::registers::reg processor::CPU::get_arg_reg(uint64_t inst, uint8_t pos) {
    return static_cast<constants::registers::reg>(inst >> pos);
}

uint32_t processor::CPU::_arg_reg_indirect(uint32_t data) {
    uint8_t reg = data & 0xff;
    if (!check_register(reg)) return raise_error(constants::error::reg, reg, 0);

    // recover 24-bit offset, add sign if needed
    uint32_t offset = (data >> 8) & 0xffffff;
    if (offset & 0x800000) offset |= 0xFF000000;

    data = regs[reg] + *(int32_t *) &offset;
    if (!check_memory(data)) return raise_error(constants::error::segfault, data, 0);

    return data;
}

uint64_t processor::CPU::get_arg_value(uint64_t word, uint8_t pos, bool cast_imm_double) {
    using namespace constants::inst;

    // switch on indicator bits, extract data after
    auto indicator = static_cast<arg>((word >> pos) & 0x3);
    uint64_t data = word >> (pos + 2);

    switch (indicator) {
        case arg::imm:
            if (cast_imm_double) {
                double d = *(float *) &data;
                return *(uint64_t *) &d;
            }

            return data;
        case arg::mem:
            return data_bus.load(_arg_addr(data), sizeof(uint64_t));
        case arg::reg:
            return regs[_arg_reg(data)];
        case arg::reg_indirect:
            return data_bus.load(_arg_reg_indirect(data), sizeof(uint64_t));
        default:
            return 0;
    }
}

uint64_t processor::CPU::get_arg_addr(uint64_t word, uint8_t pos) {
    using namespace constants::inst;

    // switch on indicator bits, extract data after
    uint8_t indicator = (word >> pos) & 0x1;
    uint64_t data = word >> (pos + 1);

    switch (static_cast<arg>(0x2 | indicator)) {
        case arg::mem:
            return _arg_addr(data);
        case arg::reg_indirect:
            return _arg_reg_indirect(data);
        default:
            return 0;
    }
}

processor::CPU::CPU(std::ostream &os, std::istream &is, const Debug &debug) : os(os), is(is), debug(debug) {
    using namespace constants;
    addr_interrupt_handler = default_interrupt_handler;

    // clear and configure key registers
    memset(regs.data(), 0, sizeof(regs));
    regs[registers::imr] = 0xffffffffffffffff;
    regs[registers::sp] = dram::size;
    regs[registers::fp] = regs[registers::fp];

    // clear memory
    data_bus.mem.clear();
}

uint64_t processor::CPU::fetch() {
    uint64_t ip = regs[constants::registers::ip];

    if (!check_memory(ip)) return raise_error(constants::error::segfault, ip, 0);
    return data_bus.load(ip, sizeof(uint64_t));
}

void processor::CPU::execute(uint64_t inst) {
    using namespace constants;

    // extract the opcode
    auto opcode = static_cast<inst::op>(inst & inst::op_mask);

    // halt on NOP?
    if (halt_on_nop && opcode == inst::_nop) {
        halt();
        return;
    }

    // extract conditional test bits
    auto test_bits = static_cast<cmp::flag>((inst >> inst::cmp_offset) & inst::cmp_mask);

    // test?
    if (test_bits != cmp::na) {
        // extract cmp bits from both the instruction and the flag register
        auto flag_bits = static_cast<cmp::flag>(regs[registers::flag] & cmp_bits);

        if (debug.conditionals) std::cout << DEBUG_STR ANSI_CYAN " conditional" ANSI_RESET ": " << constants::cmp::to_string(test_bits) << " -> ";

        // special case for [N]Z test, otherwise compare directly
        if (flag_test(int(test_bits), flag::zero)) {
            bool zero_flag = flag_test(flag::zero);

            if ((test_bits == cmp::nz && zero_flag) || (test_bits == cmp::z && !zero_flag)) {
                if (debug.conditionals)
                    std::cout << ANSI_RED "fail" ANSI_RESET " ($flag: 0x" << std::hex << int(flag_bits) << std::dec
                              << ")" << std::endl;
                return;
            }
        } else if (test_bits != flag_bits) {
            if (debug.conditionals)
                std::cout << ANSI_RED "fail" ANSI_RESET " ($flag: 0x" << std::hex << int(flag_bits) << std::dec << ")"
                          << std::endl;
            return;
        }

        if (debug.conditionals) std::cout << ANSI_GREEN "pass" ANSI_RESET << std::endl;
    }

    switch (opcode) {
        case inst::_load:
            return exec_load(inst);
        case inst::_load_upper:
            return exec_load_upper(inst);
        case inst::_store:
            return exec_store(inst);
        case inst::_compare:
            return exec_compare(inst);
        case inst::_convert:
            return exec_convert(inst);
        case inst::_not:
            return exec_not(inst);
        case inst::_and:
            return exec_and(inst);
        case inst::_or:
            return exec_or(inst);
        case inst::_xor:
            return exec_xor(inst);
        case inst::_shl:
            return exec_shift_left(inst);
        case inst::_shr:
            return exec_shift_right(inst);
        case inst::_add:
            return exec_add(inst);
        case inst::_sub:
            return exec_sub(inst);
        case inst::_mul:
            return exec_mul(inst);
        case inst::_div:
            return exec_div(inst);
        case inst::_mod:
            return exec_mod(inst);
        case inst::_jal:
            return exec_jal(inst);
        case inst::_push: // deprecated
            return exec_push(inst);
        case inst::_syscall:
            return exec_syscall(inst);
        default:
            if (debug.errs)
                std::cerr << ANSI_RED "unknown opcode " << std::hex << opcode << " (in instruction 0x" << inst
                          << std::dec << ")" << std::endl;
            raise_error(error::opcode, opcode);
    }
}

bool processor::CPU::is_interrupt() const {
    // do not allow interrupt stacking
    // TODO is this (^) guard required?
    return !flag_test(constants::flag::in_interrupt)
           && (regs[constants::registers::isr] & regs[constants::registers::imr]);
}

void processor::CPU::handle_interrupt() {
    using namespace constants::registers;

    // save $ip to $iip
    regs[iip] = regs[ip];

    // disable future interrupts
    flag_set(constants::flag::in_interrupt);

    // jump to the interrupt handler
    regs[ip] = addr_interrupt_handler;

    if (debug.cpu)
        std::cout << DEBUG_STR ANSI_CYAN " interrupt! " ANSI_RESET
                     "$isr=0x" << std::hex << regs[isr] << ", $imr=0x" << regs[imr] << ", %iip=0x" << regs[iip]
                  << std::dec << std::endl;
}

void processor::CPU::step(int step) {
    // check if we are in an interrupt
    if (is_interrupt()) {
        handle_interrupt();
    }

    if (debug.cpu) {
        std::cout << DEBUG_STR ANSI_VIOLET;
        if (step < 0) std::cout << " step";
        else std::cout << " cycle #" << step;
        std::cout << ANSI_RESET ": $ip=0x" << std::hex << regs[constants::registers::ip] << ", inst=";
    }

    // fetch next instruction, return if halted
    uint64_t inst = fetch();
    if (!is_running()) return;

    if (debug.cpu) std::cout << "0x" << inst << std::dec << std::endl;

    // increment $ip
    regs[constants::registers::ip] += sizeof(inst);

    // finally, execute the instruction
    execute(inst);
}

void processor::CPU::step_cycle() {
    // set running bit, clear error flag
    flag_set(constants::flag::is_running);
    flag_reset(constants::flag::error);

    for (int cnt = 0; is_running(); cnt++) {
        step(cnt);
    }
}

static const std::array<std::string, 11> register_strings = {
        "$ip",
        "$rip",
        "$sp",
        "$fp",
        "$flag",
        "$isr",
        "$imr",
        "$iip",
        "$ret",
        "$k1",
        "$k2"
};

void processor::CPU::print_registers() const {
    using namespace constants::registers;
    uint8_t i;
    os << std::hex;

    // special registers
    for (i = 0; i < r1; i++) {
        os << std::left << std::setw(8) << register_strings[i] << " = 0x" << regs[i] << std::endl;
    }

    // general registers
    for (i = 0; i < count - r1; i++) {
        os << "$r" << i + 1 << "      = 0x" << regs[r1 + i] << std::endl;
    }

    os << std::dec;
}

void processor::CPU::print_stack() const {
    uint64_t addr = regs[constants::registers::sp], size = dram::size - addr;
    os << "STACK: top = 0x" << std::hex << dram::size - 1 << " -> bottom = 0x" << addr << " = $sp + 1 (" << std::dec
       << size << " bytes)" << std::endl;

    uint32_t i = 0, j;
    while (i < size) {
        for (j = 0; j < 8 && i < size; i++, j++)
            os << "0x" << std::hex << std::setw(2) << std::setfill('0') << (int) data_bus.mem[addr + i] << " ";
        os << std::endl;
    }
}

void processor::CPU::print_error(bool prefix) const {
    using namespace constants;
    uint8_t error = get_error();

    if (error == error::ok)
        return;

    if (prefix)
        os << ERROR_STR " ";

    switch (error) {
        case error::opcode:
            os << "E-OPCODE: invalid opcode 0x" << std::hex << regs[registers::ret] << " (at $ip=" << std::dec
               << regs[registers::ip] << ")" << std::endl;
            break;
        case error::segfault:
            os << "E-SEGSEGV: segfault on access of 0x" << std::hex << regs[registers::ret] << std::dec << std::endl;
            break;
        case error::reg:
            os << "E-REG: segfault on access of register at +0x" << std::hex << regs[registers::ret] << std::dec
               << std::endl;
            break;
        case error::syscall:
            os << "E-SYSCALL: syscall call with unknown command 0x" << std::hex << regs[registers::ret] << std::dec
               << std::endl;
            break;
        case error::datatype:
            os << "E-DATATYPE: invalid datatype specifier 0x" << std::hex << regs[registers::ret] << std::dec
               << " (at $ip=0x" << regs[registers::ip] << ")" << std::endl;
            break;
        default:
            os << "E-UNKNOWN: unknown error, $ret=0x" << std::hex << regs[registers::ret] << std::dec << std::endl;
    }
}
