/* Rafaela Bessa 2420043 3WA */
/* Lis de Almeida  Matricula 3WA */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gera_codigo.h"

// --- Funções Auxiliares ---

static void emit_int(unsigned char code[], int *idx, int val) {
    *(int *)(code + *idx) = val;
    *idx += 4;
}

static void emit_byte(unsigned char code[], int *idx, unsigned char byte) {
    code[*idx] = byte;
    *idx += 1;
}

// Helper: Carrega valor (constante, parametro ou variavel) para %eax ou %edi
// reg_dest: 0 para %eax, 1 para %edi
static void move_to_reg(unsigned char code[], int *idx, char var_type, int var_idx, int reg_dest) {
    if (reg_dest == 0) { // Destino: %eax
        if (var_type == '$') {
            emit_byte(code, idx, 0xB8); emit_int(code, idx, var_idx); // mov $const, %eax
        } else if (var_type == 'p') {
            emit_byte(code, idx, 0x89); emit_byte(code, idx, 0xF8);   // mov %edi, %eax
        } else if (var_type == 'v') {
            int disp = (var_idx + 1) * -4;
            emit_byte(code, idx, 0x8B); emit_byte(code, idx, 0x45); emit_byte(code, idx, (unsigned char)disp); // mov -disp(%rbp), %eax
        }
    } else { // Destino: %edi (usado para passar parametro no call)
        if (var_type == '$') {
            emit_byte(code, idx, 0xBF); emit_int(code, idx, var_idx); // mov $const, %edi
        } else if (var_type == 'p') {
            // mov %edi, %edi (nada a fazer, já está lá)
        } else if (var_type == 'v') {
            int disp = (var_idx + 1) * -4;
            emit_byte(code, idx, 0x8B); emit_byte(code, idx, 0x7D); emit_byte(code, idx, (unsigned char)disp); // mov -disp(%rbp), %edi
        }
    }
}

// --- Função Principal ---

void gera_codigo(FILE *f, unsigned char code[], funcp *entry) {
    int c;
    int idx = 0; 
    unsigned long func_addrs[50]; 
    int func_count = 0;
    int line = 1; 

    while ((c = fgetc(f)) != EOF) {

        // 1. function
        if (c == 'f') {
            char c0;
            if (fscanf(f, "unction%c", &c0) != 1) {
                fprintf(stderr, "Erro: Falha ao ler 'function' na linha %d\n", line); exit(1);
            }
            
            func_addrs[func_count] = (unsigned long)&code[idx];
            *entry = (funcp)&code[idx]; 
            func_count++;

            // PRÓLOGO
            emit_byte(code, &idx, 0x55);       // push %rbp
            emit_byte(code, &idx, 0x48);       // mov %rsp, %rbp
            emit_byte(code, &idx, 0x89);
            emit_byte(code, &idx, 0xE5);
            emit_byte(code, &idx, 0x48);       // sub $32, %rsp
            emit_byte(code, &idx, 0x83);
            emit_byte(code, &idx, 0xEC);
            emit_byte(code, &idx, 0x20);
        }

        // 2. end
        else if (c == 'e') {
            char c1 = fgetc(f);
            char c2 = fgetc(f);
            if (c1 != 'n' || c2 != 'd') {
               fprintf(stderr, "Erro: Falha ao ler 'end' na linha %d\n", line); exit(1);
            }
            // Consome eventual \n mas não obriga
            int c3 = fgetc(f);
            if (c3 != '\n' && c3 != EOF) ungetc(c3, f);

            // EPÍLOGO
            emit_byte(code, &idx, 0xC9); // leave
            emit_byte(code, &idx, 0xC3); // ret
        }

        // 3. ret (retorno incondicional)
        else if (c == 'r') {
            int idx0;
            char var0;
            if (fscanf(f, "et %c%d", &var0, &idx0) != 2) {
                 fprintf(stderr, "Erro: Falha ao ler 'ret' na linha %d\n", line); exit(1);
            }
            
            move_to_reg(code, &idx, var0, idx0, 0); // Move para EAX
            emit_byte(code, &idx, 0xC9);            // leave
            emit_byte(code, &idx, 0xC3);            // ret
        }

        // 4. zret (retorno condicional)
        else if (c == 'z') {
            int idx0, idx1;
            char var0, var1;
            if (fscanf(f, "ret %c%d %c%d", &var0, &idx0, &var1, &idx1) != 4) {
                 fprintf(stderr, "Erro: Falha ao ler 'zret' na linha %d\n", line); exit(1);
            }
            
            move_to_reg(code, &idx, var0, idx0, 0); // Condição em EAX

            // cmp $0, %eax -> 83 F8 00
            emit_byte(code, &idx, 0x83); emit_byte(code, &idx, 0xF8); emit_byte(code, &idx, 0x00);

            // jne <offset> -> 75 <byte>
            emit_byte(code, &idx, 0x75);
            int pos_do_pulo = idx; 
            emit_byte(code, &idx, 0x00); // Placeholder

            // Código de retorno
            move_to_reg(code, &idx, var1, idx1, 0); // Valor de retorno em EAX
            emit_byte(code, &idx, 0xC9);            // leave
            emit_byte(code, &idx, 0xC3);            // ret

            // Corrige o pulo
            code[pos_do_pulo] = idx - (pos_do_pulo + 1);
        }

        // 5. v... (Atribuição, Aritmética ou Call)
        else if (c == 'v') {
            int idx0; 
            char c0;
            
            if (fscanf(f, "%d = %c", &idx0, &c0) != 2) {
                 fprintf(stderr, "Erro: Formato invalido na linha %d\n", line); exit(1);
            }

            // --- CASO CALL ---
            if (c0 == 'c') { 
                int f_id, idx1;
                char var1;
                if (fscanf(f, "all %d %c%d", &f_id, &var1, &idx1) != 3) {
                    fprintf(stderr, "Erro: Falha ao ler 'call' na linha %d\n", line); exit(1);
                }
                
                // 1. Preparar argumento em %edi
                move_to_reg(code, &idx, var1, idx1, 1); // 1 = destino EDI

                // 2. Calcular o offset relativo para o call
                // Fórmula: Destino - (Endereço_Instrução_Call + 5)
                // Endereço_Instrução_Call é &code[idx]
                // Mas precisamos fazer a conta com inteiros
                
                long call_instr_addr = (unsigned long)&code[idx];
                long target_addr = func_addrs[f_id];
                int offset = (int)(target_addr - (call_instr_addr + 5));

                // 3. Emitir call (E8 + 4 bytes offset)
                emit_byte(code, &idx, 0xE8);
                emit_int(code, &idx, offset);

                // O resultado do call já está em %eax, pronto para ser salvo
            }
            
            // --- CASO ARITMÉTICA / ATRIBUIÇÃO SIMPLES ---
            else { 
                int idx1, idx2;
                char var1 = c0, var2, op; 
                
                if (fscanf(f, "%d", &idx1) != 1) {
                    fprintf(stderr, "Erro lendo indice na linha %d\n", line); exit(1);
                }

                char next_char;
                int peek = fgetc(f);
                while (peek == ' ' || peek == '\t') peek = fgetc(f);
                next_char = (char)peek;

                if (next_char == '+' || next_char == '-' || next_char == '*') {
                    op = next_char;
                    if (fscanf(f, " %c%d", &var2, &idx2) != 2) {
                         fprintf(stderr, "Erro lendo operando 2 na linha %d\n", line); exit(1);
                    }

                    move_to_reg(code, &idx, var1, idx1, 0); // Move para EAX

                    if (op == '+') {
                        if (var2 == '$') { emit_byte(code, &idx, 0x05); emit_int(code, &idx, idx2); }
                        else if (var2 == 'p') { emit_byte(code, &idx, 0x01); emit_byte(code, &idx, 0xF8); }
                        else if (var2 == 'v') { int d = (idx2+1)*-4; emit_byte(code, &idx, 0x03); emit_byte(code, &idx, 0x45); emit_byte(code, &idx, (unsigned char)d); }
                    }
                    else if (op == '-') {
                         if (var2 == '$') { emit_byte(code, &idx, 0x2D); emit_int(code, &idx, idx2); }
                        else if (var2 == 'p') { emit_byte(code, &idx, 0x29); emit_byte(code, &idx, 0xF8); }
                        else if (var2 == 'v') { int d = (idx2+1)*-4; emit_byte(code, &idx, 0x2B); emit_byte(code, &idx, 0x45); emit_byte(code, &idx, (unsigned char)d); }
                    }
                    else if (op == '*') {
                        if (var2 == '$') { emit_byte(code, &idx, 0x69); emit_byte(code, &idx, 0xC0); emit_int(code, &idx, idx2); }
                        else if (var2 == 'p') { emit_byte(code, &idx, 0x0F); emit_byte(code, &idx, 0xAF); emit_byte(code, &idx, 0xC7); }
                        else if (var2 == 'v') { int d = (idx2+1)*-4; emit_byte(code, &idx, 0x0F); emit_byte(code, &idx, 0xAF); emit_byte(code, &idx, 0x45); emit_byte(code, &idx, (unsigned char)d); }
                    }
                } else {
                    ungetc(next_char, f);
                    move_to_reg(code, &idx, var1, idx1, 0);
                }
            }

            // SALVA EAX NA VARIÁVEL DE DESTINO
            int dest_disp = (idx0 + 1) * -4;
            emit_byte(code, &idx, 0x89);
            emit_byte(code, &idx, 0x45);
            emit_byte(code, &idx, (unsigned char)dest_disp);
        }
        
        if (c == '\n') line++;
    }
    
    if (func_count > 0) {
        *entry = (funcp)func_addrs[func_count - 1];
    }
}