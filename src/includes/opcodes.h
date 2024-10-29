#ifndef CHILLYGB_OPCODES_H
#define CHILLYGB_OPCODES_H

void reset_apu_regs(cpu *c);

uint8_t jp(cpu *c, parameters *p);
uint8_t ld_r8_imm8(cpu *c, parameters *p);
uint8_t ld_imm16_a(cpu *c, parameters *p);
uint8_t ld_a_imm16(cpu *c, parameters *p);
uint8_t cp_a_imm8(cpu *c, parameters *p);
uint8_t jp_cond(cpu *c, parameters *p);
uint8_t jp_hl(cpu *c, parameters *p);
uint8_t ld_r16_imm16(cpu *c, parameters *p);
uint8_t ld_a_r16mem(cpu *c, parameters *p);
uint8_t ld_r16mem_a(cpu *c, parameters *p);
uint8_t inc_r16(cpu *c, parameters *p);
uint8_t dec_r16(cpu *c, parameters *p);
uint8_t ld_r8_r8(cpu *c, parameters *p);
uint8_t or_a_r8(cpu *c, parameters *p);

uint8_t nop(cpu *c, parameters *p);
uint8_t ld_imm16_sp(cpu *c, parameters *p);
uint8_t add_hl_r16(cpu *c, parameters *p);
uint8_t inc_r8(cpu *c, parameters *p);
uint8_t dec_r8(cpu *c, parameters *p);
uint8_t rlca(cpu *c, parameters *p);
uint8_t rrca(cpu *c, parameters *p);
uint8_t rla(cpu *c, parameters *p);
uint8_t rra(cpu *c, parameters *p);
uint8_t daa(cpu *c, parameters *p);
uint8_t cpl(cpu *c, parameters *p);
uint8_t scf(cpu *c, parameters *p);
uint8_t ccf(cpu *c, parameters *p);
uint8_t jr(cpu *c, parameters *p);
uint8_t jr_cond(cpu *c, parameters *p);
uint8_t stop(cpu *c, parameters *p);
uint8_t halt(cpu *c, parameters *p);

uint8_t add_a_r8(cpu *c, parameters *p);
uint8_t adc_a_r8(cpu *c, parameters *p);
uint8_t sub_a_r8(cpu *c, parameters *p);
uint8_t sbc_a_r8(cpu *c, parameters *p);
uint8_t and_a_r8(cpu *c, parameters *p);
uint8_t xor_a_r8(cpu *c, parameters *p);
uint8_t cp_a_r8(cpu *c, parameters *p);

uint8_t add_a_imm8(cpu *c, parameters *p);
uint8_t adc_a_imm8(cpu *c, parameters *p);
uint8_t sub_a_imm8(cpu *c, parameters *p);
uint8_t sbc_a_imm8(cpu *c, parameters *p);
uint8_t and_a_imm8(cpu *c, parameters *p);
uint8_t xor_a_imm8(cpu *c, parameters *p);
uint8_t or_a_imm8(cpu *c, parameters *p);
uint8_t ret_cond(cpu *c, parameters *p);
uint8_t ret(cpu *c, parameters *p);
uint8_t reti(cpu *c, parameters *p);
uint8_t call_cond(cpu *c, parameters *p);
uint8_t call(cpu *c, parameters *p);
uint8_t rst(cpu *c, parameters *p);
uint8_t pop(cpu *c, parameters *p);
uint8_t push(cpu *c, parameters *p);
uint8_t prefix(cpu *c, parameters *p);
uint8_t add_sp_imm8(cpu *c, parameters *p);
uint8_t ld_hl_sp_imm8(cpu *c, parameters *p);
uint8_t ld_sp_hl(cpu *c, parameters *p);
uint8_t di(cpu *c, parameters *p);
uint8_t ei(cpu *c, parameters *p);
uint8_t ldh_imm8_a(cpu *c, parameters *p);
uint8_t ldh_a_imm8(cpu *c, parameters *p);
uint8_t ldh_c_a(cpu *c, parameters *p);
uint8_t ldh_a_c(cpu *c, parameters *p);
uint8_t invalid(cpu *c, parameters *p);



#endif //CHILLYGB_OPCODES_H
