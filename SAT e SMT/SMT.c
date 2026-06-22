#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>

#define MAX_RESTR 100

#define OP_MeI 0  // <=
#define OP_MaI 1  // >=
#define OP_Men 2  // <
#define OP_Ma 3  // >
#define OP_Ig 4  // ==

typedef struct {
    int coef;   // coeficiente de x
    int op;     // operador (OP_MI, OP_MaI, ...)
    int rhs;    // lado direito da restrição
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
    printf("Operador invalido: %s\n", token);
    exit(1);
}

void ler_arquivo_lia(const char* nome_arquivo, LIA* problema) {
    FILE* arquivo = fopen(nome_arquivo, "r");
    if (!arquivo) { printf("Erro ao abrir arquivo.\n"); exit(1); }

    char linha[256];
    int idx = 0;

    while (fgets(linha, sizeof(linha), arquivo)) {
        if (linha[0] == 'c') continue; // comentario, ignora

        if (linha[0] == 'p') {
            // linha "p lia N" -> so usamos para validar, num_restricoes real
            // sera contado pelas linhas lidas abaixo
            continue;
        }

        char op_str[3];
        int coef, rhs;

        // le no formato: coef op rhs   (ex: "2 <= 8")
        if (sscanf(linha, "%d %2s %d", &coef, op_str, &rhs) == 3) {
            problema->restricoes[idx].coef = coef;
            problema->restricoes[idx].op   = ler_operador(op_str);
            problema->restricoes[idx].rhs  = rhs;
            idx++;
        }
    }

    problema->num_restricoes = idx;
    fclose(arquivo);
}

// ========== NORMALIZACAO: transforma cada restricao em um intervalo [min, max] ==========
//
// Para "coef * x OP rhs", isolamos x dividindo por coef.
// Se coef for negativo, o sentido da desigualdade INVERTE (regra de aritmetica).
// Usamos divisao inteira com cuidado (igual fariamos para garantir x inteiro).

void normalizar_restricao(Restricao r, int* min, int* max) {
    *min = INT_MIN;
    *max = INT_MAX;

    if (r.coef == 0) {
        printf("Coeficiente 0 invalido.\n");
        exit(1);
    }

    // valor limite (real) seria rhs / coef; tratamos os casos de
    // arredondamento para manter o dominio dos INTEIROS correto.
    switch (r.op) {
        case OP_MeI: // coef*x <= rhs
            if (r.coef > 0) *max = (int) floor((double) r.rhs / r.coef);
            else            *min = (int) ceil((double) r.rhs / r.coef);
            break;
        case OP_MaI: // coef*x >= rhs
            if (r.coef > 0) *min = (int) ceil((double) r.rhs / r.coef);
            else            *max = (int) floor((double) r.rhs / r.coef);
            break;
        case OP_Men: // coef*x < rhs  ->  coef*x <= rhs-1 (em inteiros)
            if (r.coef > 0) *max = (int) floor((double) (r.rhs - 1) / r.coef);
            else            *min = (int) ceil((double) (r.rhs - 1) / r.coef);
            break;
        case OP_Ma: // coef*x > rhs  ->  coef*x >= rhs+1
            if (r.coef > 0) *min = (int) ceil((double) (r.rhs + 1) / r.coef);
            else            *max = (int) floor((double) (r.rhs + 1) / r.coef);
            break;
        case OP_Ig: // coef*x == rhs  -> x = rhs/coef, so se divisao for exata
            if (r.rhs % r.coef != 0) { *min = 1; *max = 0; return; }// intervalo vazio
            *min = *max = r.rhs / r.coef;
            break;
    }
}

// ========== INTERSECAO (papel do Theory Solver) ==========
// Igual ao verificar_formula() do SAT, mas em vez de percorrer uma
// lista encadeada de literais, percorremos o vetor de restricoes
// fazendo a interseccao acumulada dos intervalos.

int resolver_lia(LIA* problema, int* min_final, int* max_final) {
    int min_atual = INT_MIN, max_atual = INT_MAX;

    for (int i = 0; i < problema->num_restricoes; i++) {
        int min, max;
        normalizar_restricao(problema->restricoes[i], &min, &max);

        // interseccao: pega o maior dos minimos e o menor dos maximos
        if (min > min_atual) min_atual = min;
        if (max < max_atual) max_atual = max;

        // poda antecipada: se o intervalo ja esta vazio, UNSAT
        if (min_atual > max_atual) {
            *min_final = min_atual;
            *max_final = max_atual;
            return 0; // UNSAT
        }
    }

    *min_final = min_atual;
    *max_final = max_atual;
    return (min_atual <= max_atual); // SAT se sobrou pelo menos um inteiro
}

// ========== VERIFICACAO FINAL (igual ao "Check Solution" do slide) ==========
// Reconfirma cada restricao original com o x escolhido, igual o slide faz
// substituindo x=3 e x=4 nas formulas originais.

int verificar_solucao(LIA* problema, int x) {
    for (int i = 0; i < problema->num_restricoes; i++) {
        Restricao r = problema->restricoes[i];
        int valor = r.coef * x;
        switch (r.op) {
            case OP_MeI: if (!(valor <= r.rhs)) return 0; break;
            case OP_MaI: if (!(valor >= r.rhs)) return 0; break;
            case OP_Men: if (!(valor <  r.rhs)) return 0; break;
            case OP_Ma: if (!(valor >  r.rhs)) return 0; break;
            case OP_Ig: if (!(valor == r.rhs)) return 0; break;
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
    printf("Intervalo final apos interseccao: x in [%d, %d]\n\n", min, max);

    // Enumera e confirma cada solucao inteira do intervalo, como no slide
    int alguma_valida = 0;
    for (int x = min; x <= max; x++) {
        if (verificar_solucao(&problema, x)) {
            printf("x = %d  -> valida\n", x);
            alguma_valida = 1;
        }
    }

    if (!alguma_valida) {
        printf("Nenhuma solucao inteira valida encontrada (UNSAT na pratica).\n");
    }

    return 0;
}
