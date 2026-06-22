#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>

#define MAX_RESTR 100

#define OP_MeI 0  // <=
#define OP_MaI 1  // >=
#define OP_Men 2  // <
#define OP_Ma  3  // >
#define OP_Ig  4  // ==
#define OP_Dif 5  // !=

typedef struct { 
    int coef;   // coeficiente 'a' de x
    int termo;  // termo independente 'b' somado/subtraído do lado esquerdo
    int op;     // operador
    int rhs;    // lado direito da restrição 'c'
} Restricao;

typedef struct {
    Restricao restricoes[MAX_RESTR];
    int num_restricoes;
} LIA;

int ler_operador(const char* token) {
    if (strcmp(token, "<=") == 0) return OP_MeI;
    if (strcmp(token, ">=") == 0) return OP_MaI;
    if (strcmp(token, "<")  == 0) return OP_Men;
    if (strcmp(token, ">")  == 0) return OP_Ma;
    if (strcmp(token, "==") == 0) return OP_Ig;
    if (strcmp(token, "!=") == 0) return OP_Dif;
    printf("Operador invalido: %s\n", token);
    exit(1);
}

void ler_arquivo_lia(const char* nome_arquivo, LIA* problema) {
    FILE* arquivo = fopen(nome_arquivo, "r");
    if (!arquivo) { printf("Erro ao abrir arquivo.\n"); exit(1); }

    char linha[256];
    int idx = 0;

    while (fgets(linha, sizeof(linha), arquivo)) {
        if (linha[0] == 'c' || linha[0] == '\n' || linha[0] == '\r') continue;
        if (linha[0] == 'p') continue;

        int coef = 1, termo = 0, rhs = 0;
        char sinal = '+';
        char op_str[3];

        if (sscanf(linha, "%dx %c %d %2s %d", &coef, &sinal, &termo, op_str, &rhs) == 5) {
            if (sinal == '-') termo = -termo;
        }
        else if (linha[0] == 'x') {
            coef = 1;
            if (sscanf(linha, "x %c %d %2s %d", &sinal, &termo, op_str, &rhs) == 5) {
                if (sinal == '-') termo = -termo;
            } else if (sscanf(linha, "x %2s %d", op_str, &rhs) == 2) {
                termo = 0;
            }
        }
        else if (sscanf(linha, "%dx %2s %d", &coef, op_str, &rhs) == 3) {
            termo = 0;
        } else {
            continue;
        }

        problema->restricoes[idx].coef = coef;
        problema->restricoes[idx].termo = termo;
        problema->restricoes[idx].op = ler_operador(op_str);
        problema->restricoes[idx].rhs = rhs;
        idx++;
    }

    problema->num_restricoes = idx;
    fclose(arquivo);
}

void normalizar_restricao(Restricao r, int* min, int* max) {
    *min = INT_MIN;
    *max = INT_MAX;

    if (r.coef == 0) {
        printf("Coeficiente 0 invalido.\n");
        exit(1);
    }

    int alvo = r.rhs - r.termo;

    switch (r.op) {         
        case OP_MeI:     
            if (r.coef > 0) *max = (int) floor((double) alvo / r.coef);
            else            *min = (int) ceil((double) alvo / r.coef);
            break;
        case OP_MaI:
            if (r.coef > 0) *min = (int) ceil((double) alvo / r.coef);
            else            *max = (int) floor((double) alvo / r.coef);
            break;
        case OP_Men:
            if (r.coef > 0) *max = (int) floor((double) (alvo - 1) / r.coef);
            else            *min = (int) ceil((double) (alvo - 1) / r.coef);
            break;
        case OP_Ma:
            if (r.coef > 0) *min = (int) ceil((double) (alvo + 1) / r.coef);
            else            *max = (int) floor((double) (alvo + 1) / r.coef);
            break;
        case OP_Ig:
            if (alvo % r.coef != 0) { *min = 1; *max = 0; return; }
            *min = *max = alvo / r.coef;
            break;
        case OP_Dif:
            *min = INT_MIN;
            *max = INT_MAX;
            break;
    }
}

int resolver_lia(LIA* problema, int* min_final, int* max_final) {
    int min_atual = INT_MIN, max_atual = INT_MAX;

    for (int i = 0; i < problema->num_restricoes; i++) {
        int min, max;
        normalizar_restricao(problema->restricoes[i], &min, &max);

        if (min > min_atual) min_atual = min;
        if (max < max_atual) max_atual = max;

        if (min_atual > max_atual) { 
            *min_final = min_atual;
            *max_final = max_atual;
            return 0; // UNSAT
        }
    }

    *min_final = min_atual;
    *max_final = max_atual;
    return (min_atual <= max_atual);
}

int verificar_solucao(LIA* problema, int x) {
    for (int i = 0; i < problema->num_restricoes; i++) {
        Restricao r = problema->restricoes[i];
        int valor = (r.coef * x) + r.termo;
        switch (r.op) {
            case OP_MeI: if (!(valor <= r.rhs)) return 0; break;
            case OP_MaI: if (!(valor >= r.rhs)) return 0; break;
            case OP_Men: if (!(valor <  r.rhs)) return 0; break;
            case OP_Ma:  if (!(valor >  r.rhs)) return 0; break;
            case OP_Ig:  if (!(valor == r.rhs)) return 0; break;
            case OP_Dif: if (!(valor != r.rhs)) return 0; break;
        }
    }
    return 1;
}

int main() {
    LIA problema = {0};
    char nome_arq[100];

    printf("Digite o nome do arquivo (.lia): ");
    fgets(nome_arq, sizeof(nome_arq), stdin);
    nome_arq[strcspn(nome_arq, "\n")] = 0;

    ler_arquivo_lia(nome_arq, &problema);

    int min, max;
    int status = resolver_lia(&problema, &min, &max);

    if (!status) {
        printf("\nUNSAT!\n");
        return 0;
    }

    printf("\nSAT!\n");
    
   
    printf("Intervalo de busca logico: x in [");
    if (min == INT_MIN) printf("-inf, "); else printf("%d, ", min);
    if (max == INT_MAX) printf("inf]\n"); else printf("%d]\n", max);

   // para nao ter um for extremamente grande
    int iter_min = (min == INT_MIN) ? -50 : min;
    int iter_max = (max == INT_MAX) ? 50 : max;

    if (min == INT_MIN || max == INT_MAX) {
        printf("Aviso: Como o intervalo possui extremidades infinitas, listando solucoes no escopo parcial [%d, %d]:\n\n", iter_min, iter_max);
    } else {
        printf("\n");
    }

    int alguma_valida = 0;
    
    for (int x = iter_min; x <= iter_max; x++) {
        if (verificar_solucao(&problema, x)) {
            printf("x = %d  -> valida\n", x);
            alguma_valida = 1;
        }
    }
    
    if (!alguma_valida) {
        printf("Nenhuma solucao inteira valida encontrada neste escopo.\n");
    }

    return 0;
}
