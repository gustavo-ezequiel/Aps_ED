// salvar como filtros_anos.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE 4096
#define MAX_COLS 50

#define RESET   "\x1b[0m"
#define BOLD    "\x1b[1m"
#define CYAN    "\x1b[36m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define RED     "\x1b[31m"

typedef struct {
    char data[50];
    char municipio[200];
    char bioma[100];
    char precip[100];
} DadosBahia;

/* ---------- auxiliares ---------- */
void to_lowercase(char *str) {
    for (int i = 0; str[i]; i++) str[i] = (char)tolower((unsigned char)str[i]);
}

void remove_acentos(char *str) {
    char mapa_orig[] = "áàãâäÁÀÃÂÄéèêëÉÈÊËíìîïÍÌÎÏóòõôöÓÒÕÔÖúùûüÚÙÛÜçÇ";
    char mapa_subs[] = "aaaaaAAAAeeeeEEEEiiiiIIIIoooooOOOOOuuuuUUUUcC";
    for (int i = 0; str[i]; i++) {
        unsigned char c = (unsigned char)str[i];
        int trocou = 0;
        for (int j = 0; mapa_orig[j]; j++) {
            if (c == (unsigned char)mapa_orig[j]) { str[i] = mapa_subs[j]; trocou = 1; break;}
        }
        if (!trocou && c < 32) str[i] = ' ';
    }
}

void normalizar(char *str) {
    remove_acentos(str);
    to_lowercase(str);
}

int contains_ignore_case(const char *text, const char *pattern) {
    char *a = strdup(text);
    char *b = strdup(pattern);
    if (!a || !b) { free(a); free(b); return 0; }
    normalizar(a); normalizar(b);
    int found = strstr(a, b) != NULL;
    free(a); free(b);
    return found;
}

void print_line() {
    printf(CYAN "--------------------------------------------------------------------------------------" RESET "\n");
}

void interpretar_data(const char *data, int *ano, int *mes, int *dia) {
    *ano = *mes = *dia = 0;
    if (strchr(data, '-')) sscanf(data, "%d-%d-%d", ano, mes, dia); // YYYY-MM-DD
    else if (strchr(data, '/')) { sscanf(data, "%d/%d/%d", dia, mes, ano); if (*ano < 100) *ano += 2000; } // DD/MM/YYYY
}

int comparar_data(const void *a, const void *b) {
    const DadosBahia *r1 = a, *r2 = b;
    int a1,m1,d1,a2,m2,d2;
    interpretar_data(r1->data,&a1,&m1,&d1);
    interpretar_data(r2->data,&a2,&m2,&d2);
    if (m1 != m2) return (m1 - m2);
    if (d1 != d2) return (d1 - d2);
    return (a1 - a2);
}

/* processa um arquivo CSV e acrescenta registros que batem no filtro ao array dinâmico */
int process_file(const char *filename, int idx_data, int idx_bioma, int idx_municipio, int idx_precip,
                 int alvo, const char *filtro,
                 DadosBahia **arr_ptr, size_t *count_ptr, size_t *cap_ptr, int skip_header) {

    FILE *f = fopen(filename, "r");
    if (!f) {
        printf(RED "Aviso: nao foi possível abrir '%s' — pulando.\n" RESET, filename);
        return 0;
    }

    char line[MAX_LINE];
    if (skip_header) {
        if (!fgets(line, sizeof(line), f)) { fclose(f); return 0; }
    }

    while (fgets(line, sizeof(line), f)) {
        char copy[MAX_LINE];
        strncpy(copy, line, sizeof(copy)-1); copy[sizeof(copy)-1] = '\0';

        char *cols[MAX_COLS];
        int i = 0;
        char *p = strtok(copy, ",");
        while (p && i < MAX_COLS) { cols[i++] = p; p = strtok(NULL, ","); }

        if (i <= alvo) continue;

        char campo_norm[256];
        strncpy(campo_norm, cols[alvo], sizeof(campo_norm)-1); campo_norm[sizeof(campo_norm)-1] = '\0';
        normalizar(campo_norm);

        if (!contains_ignore_case(campo_norm, filtro)) continue;

        const char *data = (idx_data>=0 && idx_data<i) ? cols[idx_data] : "-";
        const char *muni = (idx_municipio>=0 && idx_municipio<i) ? cols[idx_municipio] : "-";
        const char *bioma = (idx_bioma>=0 && idx_bioma<i) ? cols[idx_bioma] : "-";
        const char *precip = (idx_precip>=0 && idx_precip<i) ? cols[idx_precip] : "-";

        /* remove acentos das cópias (em buffers temporários) */
        char data_clean[128], muni_clean[256], bioma_clean[128], precip_clean[128];
        strncpy(data_clean, data, sizeof(data_clean)-1); data_clean[sizeof(data_clean)-1] = '\0';
        strncpy(muni_clean, muni, sizeof(muni_clean)-1); muni_clean[sizeof(muni_clean)-1] = '\0';
        strncpy(bioma_clean, bioma, sizeof(bioma_clean)-1); bioma_clean[sizeof(bioma_clean)-1] = '\0';
        strncpy(precip_clean, precip, sizeof(precip_clean)-1); precip_clean[sizeof(precip_clean)-1] = '\0';
        remove_acentos(data_clean); remove_acentos(muni_clean); remove_acentos(bioma_clean); remove_acentos(precip_clean);

        /* garante espaço no array */
        if (*count_ptr >= *cap_ptr) {
            size_t novo = (*cap_ptr == 0) ? 512 : (*cap_ptr * 2);
            DadosBahia *tmp = realloc(*arr_ptr, novo * sizeof(DadosBahia));
            if (!tmp) { fclose(f); return -1; }
            *arr_ptr = tmp; *cap_ptr = novo;
        }

        DadosBahia *r = &((*arr_ptr)[*count_ptr]);
        strncpy(r->data, data_clean, sizeof(r->data)-1); r->data[sizeof(r->data)-1] = '\0';
        strncpy(r->municipio, muni_clean, sizeof(r->municipio)-1); r->municipio[sizeof(r->municipio)-1] = '\0';
        strncpy(r->bioma, bioma_clean, sizeof(r->bioma)-1); r->bioma[sizeof(r->bioma)-1] = '\0';
        strncpy(r->precip, precip_clean, sizeof(r->precip)-1); r->precip[sizeof(r->precip)-1] = '\0';

        (*count_ptr)++;
    }

    fclose(f);
    return 1;
}

/* ---------- main ---------- */
int main() {
    char escolherDados[32];
    printf(BOLD CYAN "Escolha o ano dos dados (digite 2023, 2024 ou ambos): " RESET);
    if (!fgets(escolherDados, sizeof(escolherDados), stdin)) return 1;
    escolherDados[strcspn(escolherDados, "\n")] = '\0';
    to_lowercase(escolherDados);

    int usar2023 = 0, usar2024 = 0;
    if (strstr(escolherDados, "2023")) usar2023 = 1;
    if (strstr(escolherDados, "2024")) usar2024 = 1;
    if (strstr(escolherDados, "amb")) { usar2023 = usar2024 = 1; }
    if (!usar2023 && !usar2024) {
        printf(RED "Opcao invalida. Saindo.\n" RESET); return 1;
    }

    char escolher[256], coluna[100];
    printf(YELLOW "De que forma deseja exibir os dados (bioma/municipio/data)? " RESET);
    if (!fgets(coluna, sizeof(coluna), stdin)) return 1;
    coluna[strcspn(coluna, "\n")] = '\0';
    normalizar(coluna);

    printf(YELLOW "Digite o valor que deseja filtrar: " RESET);
    if (!fgets(escolher, sizeof(escolher), stdin)) return 1;
    escolher[strcspn(escolher, "\n")] = '\0';
    normalizar(escolher);

    /* nomes dos arquivos (assumidos presentes no mesmo diretório) */
    const char *file2023 = "focos_br_ba_ref_2023.csv";
    const char *file2024 = "focos_br_ba_ref_2024.csv";

    /* Ler header do primeiro arquivo existente para detectar índices de colunas */
    FILE *f_head = NULL;
    if (usar2023) f_head = fopen(file2023, "r");
    if (!f_head && usar2024) f_head = fopen(file2024, "r");
    if (!f_head) { printf(RED "Erro: nenhum arquivo encontrado.\n" RESET); return 1; }

    char header[MAX_LINE];
    if (!fgets(header, sizeof(header), f_head)) { printf(RED "Arquivo vazio.\n" RESET); fclose(f_head); return 1; }
    fclose(f_head);

    char header_copy[MAX_LINE];
    strcpy(header_copy, header);
    remove_acentos(header_copy);

    printf("\n" YELLOW "Cabecalho detectado: %s\n" RESET, header_copy);

    int indice_bioma=-1, indice_data=-1, indice_municipio=-1, indice_precipitacao=-1;
    int idx = 0;
    char *tok = strtok(header_copy, ",");
    while (tok) {
        char tmp[256]; strncpy(tmp, tok, sizeof(tmp)-1); tmp[sizeof(tmp)-1]='\0';
        normalizar(tmp);
        if (strstr(tmp, "bioma")) indice_bioma = idx;
        if (strstr(tmp, "data")) indice_data = idx;
        if (strstr(tmp, "munic")) indice_municipio = idx;
        if (strstr(tmp, "precip") || strstr(tmp, "chuva") || strstr(tmp, "pluvi")) indice_precipitacao = idx;
        idx++; tok = strtok(NULL, ",");
    }

    int busca = -1;
    if (strcmp(coluna, "bioma") == 0) busca = indice_bioma;
    else if (strcmp(coluna, "data") == 0) busca = indice_data;
    else if (strcmp(coluna, "municipio") == 0) busca = indice_municipio;

    if (busca == -1) {
        printf(RED "Coluna '%s' nao encontrada no cabecalho.\n" RESET, coluna);
        return 1;
    }

    /* array dinâmico de registros */
    DadosBahia *DadosBahia = NULL;
    size_t contagem = 0, capacidade = 0;

    /* processa arquivos escolhidos (skip_header=1 para ignorar linha de cabeçalho) */
    if (usar2023) {
        int r = process_file(file2023, indice_data, indice_bioma, indice_municipio, indice_precipitacao, busca, escolher,
                             &DadosBahia, &contagem, &capacidade, 1);
        if (r < 0) { printf(RED "Erro de memoria ao processar %s\n" RESET, file2023); free(DadosBahia); return 1; }
    }
    if (usar2024) {
        int r = process_file(file2024, indice_data, indice_bioma, indice_municipio, indice_precipitacao, busca, escolher,
                             &DadosBahia, &contagem, &capacidade, 1);
        if (r < 0) { printf(RED "Erro de memoria ao processar %s\n" RESET, file2024); free(DadosBahia); return 1; }
    }

    if (contagem == 0) {
        print_line();
        printf(RED "Nenhum registro encontrado para '%s' = '%s'\n" RESET, coluna, escolher);
        print_line();
        free(DadosBahia);
        return 0;
    }

    /* ordenar por mês (01..12), depois dia, depois ano */
    qsort(DadosBahia, contagem, sizeof(DadosBahia), comparar_data);

    /* imprimir resultados */
    printf(BOLD GREEN "\n=== RESULTADOS ENCONTRADOS (ORDENADOS POR DATA) ===\n" RESET);
    print_line();
    printf(BOLD CYAN "%-15s | %-40s | %-20s | %-12s\n" RESET, "DATA", "MUNICIPIO", "BIOMA", "PRECIPITACAO");
    print_line();
    for (size_t i = 0; i < contagem; i++) {
        printf("%-15s | %-40s | %-20s | %-12s\n",
               DadosBahia[i].data, DadosBahia[i].municipio, DadosBahia[i].bioma, DadosBahia[i].precip);
    }
    print_line();
    printf(GREEN "Total de registros encontrados: %zu\n" RESET, contagem);
    print_line();

    free(DadosBahia);
    return 0;
}
