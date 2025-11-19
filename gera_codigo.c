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

// Helper: Carrega valor (constante, parametro ou variavel) para %eax
static void move_to_eax(unsigned char code[], int *idx, char var_type, int var_idx) {
    if (var_type == '$') {
        // mov $const, %eax
        emit_byte(code, idx, 0xB8);
        emit_int(code, idx, var_idx);
    } 
    else if (var_type == 'p') { 
        // mov %edi, %eax
        emit_byte(code, idx, 0x89);
        emit_byte(code, idx, 0xF8);
    } 
    else if (var_type == 'v') { 
        // mov -disp(%rbp), %eax
        int disp = (var_idx + 1) * -4;
        emit_byte(code, idx, 0x8B);
        emit_byte(code, idx, 0x45);
        emit_byte(code, idx, (unsigned char)disp);
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
            char c0;
            if (fscanf(f, "nd%c", &c0) != 1) {
               fprintf(stderr, "Erro: Falha ao ler 'end' na linha %d\n", line); exit(1);
            }
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
            
            move_to_eax(code, &idx, var0, idx0);
            emit_byte(code, &idx, 0xC9);         // leave
            emit_byte(code, &idx, 0xC3);         // ret
        }

        // 4. zret (retorno condicional)
        else if (c == 'z') {
            int idx0, idx1;
            char var0, var1;
            if (fscanf(f, "ret %c%d %c%d", &var0, &idx0, &var1, &idx1) != 4) {
                 fprintf(stderr, "Erro: Falha ao ler 'zret' na linha %d\n", line); exit(1);
            }
            
            // TODO: Implementaremos o ZRET aqui depois
        }

        // 5. v... (Atribuição ou Aritmética ou Call)
        else if (c == 'v') {
            int idx0; 
            char c0;
            
            // Lê "v0 = c..."
            if (fscanf(f, "%d = %c", &idx0, &c0) != 2) {
                 fprintf(stderr, "Erro: Formato invalido de atribuicao na linha %d\n", line); exit(1);
            }

            // --- CASO CALL ---
            if (c0 == 'c') { 
                int f_id, idx1;
                char var1;
                if (fscanf(f, "all %d %c%d", &f_id, &var1, &idx1) != 3) {
                    fprintf(stderr, "Erro: Falha ao ler 'call' na linha %d\n", line); exit(1);
                }
                // TODO: Implementaremos o CALL aqui depois
            }
            
            // --- CASO ARITMÉTICA / ATRIBUIÇÃO SIMPLES ---
            else { 
                int idx1, idx2;
                char var1 = c0, var2, op; 
                
                // O 'c0' já é o tipo do primeiro operando (p, $, v). Lemos o índice.
                if (fscanf(f, "%d", &idx1) != 1) {
                    fprintf(stderr, "Erro lendo indice do operando 1 na linha %d\n", line); exit(1);
                }

                // TRUQUE PARA IGNORAR ESPAÇOS ANTES DO OPERADOR
                // Precisamos ver se depois do numero vem +, -, * ou fim de linha
                char next_char;
                int peek;
                
                peek = fgetc(f);
                // Enquanto for espaço ou tab, continua lendo o próximo
                while (peek == ' ' || peek == '\t') {
                    peek = fgetc(f);
                }
                next_char = (char)peek;

                if (next_char == '+' || next_char == '-' || next_char == '*') {
                    // É ARITMÉTICA!
                    op = next_char;
                    // Lê o segundo operando
                    if (fscanf(f, " %c%d", &var2, &idx2) != 2) {
                         fprintf(stderr, "Erro lendo operando 2 na linha %d\n", line); exit(1);
                    }

                    // Gera código: Move Operando 1 -> EAX
                    move_to_eax(code, &idx, var1, idx1);

                    // Realiza a operação com Operando 2 sobre EAX
                    if (op == '+') {
                        if (var2 == '$') { 
                            emit_byte(code, &idx, 0x05); emit_int(code, &idx, idx2);
                        } else if (var2 == 'p') { 
                             emit_byte(code, &idx, 0x01); emit_byte(code, &idx, 0xF8);
                        } else if (var2 == 'v') { 
                             int disp = (idx2 + 1) * -4;
                             emit_byte(code, &idx, 0x03); emit_byte(code, &idx, 0x45); emit_byte(code, &idx, (unsigned char)disp);
                        }
                    }
                    else if (op == '-') {
                         if (var2 == '$') { 
                            emit_byte(code, &idx, 0x2D); emit_int(code, &idx, idx2);
                        } else if (var2 == 'p') { 
                             emit_byte(code, &idx, 0x29); emit_byte(code, &idx, 0xF8);
                        } else if (var2 == 'v') { 
                             int disp = (idx2 + 1) * -4;
                             emit_byte(code, &idx, 0x2B); emit_byte(code, &idx, 0x45); emit_byte(code, &idx, (unsigned char)disp);
                        }
                    }
                    else if (op == '*') {
                        if (var2 == '$') { 
                            emit_byte(code, &idx, 0x69); emit_byte(code, &idx, 0xC0); emit_int(code, &idx, idx2);
                        } else if (var2 == 'p') { 
                             emit_byte(code, &idx, 0x0F); emit_byte(code, &idx, 0xAF); emit_byte(code, &idx, 0xC7);
                        } else if (var2 == 'v') { 
                             int disp = (idx2 + 1) * -4;
                             emit_byte(code, &idx, 0x0F); emit_byte(code, &idx, 0xAF); emit_byte(code, &idx, 0x45); emit_byte(code, &idx, (unsigned char)disp);
                        }
                    }

                } else {
                    // É ATRIBUIÇÃO SIMPLES
                    // Devolve o caractere lido (ex: \n) para não estragar a leitura da próxima linha
                    ungetc(next_char, f);
                    
                    move_to_eax(code, &idx, var1, idx1);
                }

                // Salva EAX na variável de destino (local)
                int dest_disp = (idx0 + 1) * -4;
                emit_byte(code, &idx, 0x89);
                emit_byte(code, &idx, 0x45);
                emit_byte(code, &idx, (unsigned char)dest_disp);
            }
        }
        
        // 6. Conta linhas
        if (c == '\n') line++;
    }
    
    if (func_count > 0) {
        *entry = (funcp)func_addrs[func_count - 1];
    }
}