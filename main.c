/*
PROPOSTA DE IMPLEMENTAÇÃO DE UM CONTROLADOR FUZZY PARA RASPBERRY PI PICO W: APLICAÇÃO EM UM SISTEMA TÉRMICO
Capacitação Profissional em Sistemas Embarcados - EmbarcaTech - IFMA - PROJETO FINAL
Autor: Marcos Gabriel Barros Louzeiro
E-mail: marcoslouzeiro3@gmail.com
Data: 09/02/2025
*/

// Importação das bibliotecas necessárias a para a execução do algoritmo
#include <stdio.h>
#include "pico/stdlib.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "inc/ssd1306.h"
#include "hardware/i2c.h"

// Definição dos pinos de comunicação I2C
const uint I2C_SDA = 14;
const uint I2C_SCL = 15;

// Definição dos pinos dos LEDs RGB
#define RED 13
#define BLUE 12
#define GREEN 11

// Definição de constantes
#define size 1001                // Tamanho dos vetores
#define AMOSTRAGEM 4;           // Tempo de Amostragem
int cont1;                      // Variável de controle para ciclos 

// Variáveis do sistema de inferência fuzzy
float set_point;                   // Valor desejado de Temperatura definido pelo usuário   
float erro_temperatura;           // Armazena a diferença entre a temperatura desejada e a temperatura atual
float variacao_erro_temperatura; // Taxa de variação do erro 
float temp_atual;               // Temperatura Atual
float temp_anterior;           // Temperatura Anterior
float erro_temp;              // Variável auxiliar que armazena o valor do erro de temperatura
float var_erro_temp;         // Variável auxiliar que armazena o valor da variação do erro

// Declaração das funções de pertinência para o erro de temperatura
float erro_alto_negativo;
float erro_baixo_negativo;
float erro_zero;
float erro_baixo_positivo;
float erro_alto_positivo;

// Declaração das funções de pertinência para a variação do erro de temperatura
float variacao_erro_negativa;
float variacao_erro_zero;
float variacao_erro_positiva;

// Declaração das variáveis que armazenam o resultado da implicação das regras SE-ENTÃO
float regra1, regra2, regra3, regra4, regra5, regra6, regra7, regra8, regra9, regra10;
float regra11, regra12, regra13, regra14, regra15;

// Variáveis para defuzzificação
float numerador;
float denominador;
float defuzzificada;

// Declaração dos vetores para gerar as saídas e o vetor de agregação
float r_x[size];
float r_mb[size];
float r_b[size];
float r_a[size];
float r_ma[size];
float agreg[size];
double vi;

// Parâmetros da modelagem ARX da planta térmcia
float a1 = 1.4614;
float a2 = -0.4798;
float b1 = 0.0216;

/* Função para fuzzificação da entrada 1 do sistema que corresponde ao erro de temperatura
 São utilizadas em sua maioria funções trapezoidais e triangulares pela simplicidade de implementação */
void fuzzification_erro()
{
    if (erro_temperatura >= -70 && erro_temperatura <= -40) 
    {
        erro_alto_negativo = 1;
        erro_baixo_negativo = 0;
        erro_zero = 0;
        erro_baixo_positivo = 0;
        erro_alto_positivo = 0;
    }
    if (erro_temperatura >= -40 && erro_temperatura <= -20)
    {
        erro_alto_negativo = (-20 - erro_temperatura) / 20;
        erro_baixo_negativo = (erro_temperatura + 40) / 20;
        erro_zero = 0;
        erro_baixo_positivo = 0;
        erro_alto_positivo = 0;
    }
    if (erro_temperatura >= -20 && erro_temperatura <= 0)
    {
        erro_alto_negativo = 0;
        erro_baixo_negativo = (-erro_temperatura) / 20;
        erro_zero = (erro_temperatura + 20) / 20;
        erro_baixo_positivo = 0;
        erro_alto_positivo = 0;
    }
    if (erro_temperatura >= 0 && erro_temperatura <= 20)
    {
        erro_alto_negativo = 0;
        erro_baixo_negativo = 0;
        erro_zero = (20 - erro_temperatura) / 20;
        erro_baixo_positivo = (erro_temperatura) / 20;
        erro_alto_positivo = 0;
    }
    if (erro_temperatura >= 20 && erro_temperatura <= 40)
    {
        erro_alto_negativo = 0;
        erro_baixo_negativo = 0;
        erro_zero = 0;
        erro_baixo_positivo = (40 - erro_temperatura) / 20;
        erro_alto_positivo = (erro_temperatura - 20) / 20;
    }
    if (erro_temperatura >= 40 && erro_temperatura <= 70)
    {
        erro_alto_negativo = 0;
        erro_baixo_negativo = 0;
        erro_zero = 0;
        erro_baixo_positivo = 0;
        erro_alto_positivo = 1;
    }
}

/* Função para fuzzificação da variação da entrada 2 do sistema que é a variação do erro de temperatura*/
void fuzzification_var_erro()
{
    if (variacao_erro_temperatura >= -17.5 && variacao_erro_temperatura <= -5)
    {
        variacao_erro_negativa = 1;
        variacao_erro_zero = 0;
        variacao_erro_positiva = 0;
    }
    if (variacao_erro_temperatura >= -5 && variacao_erro_temperatura <= 0)
    {
        variacao_erro_negativa = (-variacao_erro_temperatura) / 5;
        variacao_erro_zero = (variacao_erro_negativa + 5) / 5;
        variacao_erro_positiva = 0;
    }
    if (variacao_erro_temperatura >= 0 && variacao_erro_temperatura <= 5)
    {
        variacao_erro_negativa = 0;
        variacao_erro_zero = (5 - variacao_erro_temperatura) / 5;
        variacao_erro_positiva = (variacao_erro_temperatura) / 5;
    }
    if (variacao_erro_temperatura >= 5 && variacao_erro_temperatura <= 17.5)
    {
        variacao_erro_negativa = 0;
        variacao_erro_zero = 0;
        variacao_erro_positiva = 1;
    }
}

/*Função para realizar o processo de implicação de Mínimo, AND ou regra E
A finalidade dessa função é selecionar o menor valor entre as pertinencias das duas entradas
Este valor fica armazenado na variável correspondente da regra*/
void implication()
{

    regra1 = fmin(erro_alto_negativo, variacao_erro_negativa);
    regra2 = fmin(erro_baixo_negativo, variacao_erro_negativa);
    regra3 = fmin(erro_zero, variacao_erro_negativa);
    regra4 = fmin(erro_baixo_positivo, variacao_erro_negativa);
    regra5 = fmin(erro_alto_positivo, variacao_erro_negativa);

    regra6 = fmin(erro_alto_negativo, variacao_erro_zero);
    regra7 = fmin(erro_baixo_negativo, variacao_erro_zero);
    regra8 = fmin(erro_zero, variacao_erro_zero);
    regra9 = fmin(erro_baixo_positivo, variacao_erro_zero);
    regra10 = fmin(erro_alto_positivo, variacao_erro_zero);

    regra11 = fmin(erro_alto_negativo, variacao_erro_positiva);
    regra12 = fmin(erro_baixo_negativo, variacao_erro_positiva);
    regra13 = fmin(erro_zero, variacao_erro_positiva);
    regra14 = fmin(erro_baixo_positivo, variacao_erro_positiva);
    regra15 = fmin(erro_alto_positivo, variacao_erro_positiva);
}

/* Função para gerar o gráfico das pertinências da saída*/
void saida()
{
    cont1 = 0;
    vi = 0.0;
    while (cont1 <= 1000)
    {
        r_x[cont1] = vi;
        vi = vi + 0.01;
        cont1 = cont1 + 1;
    }

    for (int k = 0; k < size; k = k + 1)
    {
        if (r_x[k] >= 0 && r_x[k] <= 1)
        {
            r_mb[k] = 1;
            r_b[k] = 0;
            r_a[k] = 0;
            r_ma[k] = 0;
        }

        if (r_x[k] >= 1 && r_x[k] <= 3)
        {
            r_mb[k] = 1;
            r_b[k] = 0;
            r_a[k] = 0;
            r_ma[k] = 0;
        }
        if (r_x[k] >= 3 && r_x[k] <= 5)
        {
            r_mb[k] = 0;
            r_b[k] = 1;
            r_a[k] = 0;
            r_ma[k] = 0;
        }
        if (r_x[k] >= 5 && r_x[k] <= 7)
        {
            r_mb[k] = 0;
            r_b[k] = (7 - r_x[k]) / 2;
            r_a[k] = (r_x[k] - 5) / 2;
            r_ma[k] = 0;
        }
        if (r_x[k] >= 7 && r_x[k] <= 9)
        {
            r_mb[k] = 0;
            r_b[k] = 0;
            r_a[k] = (9 - r_x[k]) / 2;
            r_ma[k] = (r_x[k] - 7) / 2;
        }
        if (r_x[k] >= 9 && r_x[k] <= 10)
        {
            r_mb[k] = 0;
            r_b[k] = 0;
            r_a[k] = 0;
            r_ma[k] = 1;
        }
    }
}

/* Função construída para realizar a etapa de agregação
   Cada laço ele soma a contribuição das regras no vetor de agregação
   Gerando a resposta não crisp*/
void agregacao()
{
    for (int i = 0; i < size; i++)
    {
        agreg[i] = 0.0;
        agreg[i] = fmax(agreg[i], fmin(regra1, r_mb[i]));
    }
    for (int i = 0; i < size; i++)
    {
        agreg[i] = fmax(agreg[i], fmin(regra2, r_mb[i]));
    }
    for (int i = 0; i < size; i++)
    {
        agreg[i] = fmax(agreg[i], fmin(regra3, r_mb[i]));
    }
    for (int i = 0; i < size; i++)
    {
        agreg[i] = fmax(agreg[i], fmin(regra4, r_a[i]));
    }
    for (int i = 0; i < size; i++)
    {
        agreg[i] = fmax(agreg[i], fmin(regra5, r_ma[i]));
    }
    for (int i = 0; i < size; i++)
    {
        agreg[i] = fmax(agreg[i], fmin(regra6, r_mb[i]));
    }
    for (int i = 0; i < size; i++)
    {
        agreg[i] = fmax(agreg[i], fmin(regra7, r_b[i]));
    }
    for (int i = 0; i < size; i++)
    {
        agreg[i] = fmax(agreg[i], fmin(regra8, r_mb[i]));
    }
    for (int i = 0; i < size; i++)
    {
        agreg[i] = fmax(agreg[i], fmin(regra9, r_a[i])); 
    }
    for (int i = 0; i < size; i++)
    {
        agreg[i] = fmax(agreg[i], fmin(regra10, r_ma[i]));
    }
    for (int i = 0; i < size; i++)
    {
        agreg[i] = fmax(agreg[i], fmin(regra11, r_mb[i]));
    }
    for (int i = 0; i < size; i++)
    {
        agreg[i] = fmax(agreg[i], fmin(regra12, r_b[i]));
    }
    for (int i = 0; i < size; i++)
    {
        agreg[i] = fmax(agreg[i], fmin(regra13, r_mb[i]));
    }
    for (int i = 0; i < size; i++)
    {
        agreg[i] = fmax(agreg[i], fmin(regra14, r_a[i]));
    }
    for (int i = 0; i < size; i++)
    {
        agreg[i] = fmax(agreg[i], fmin(regra15, r_ma[i]));
    }
}

/*  Função definida para realizar o processo de defuzzificação
    A metodologia empregada é o método das Centroides (CoG)*/
void defuzzificacao()
{
    numerador = 0;
    denominador = 0;

    for (int i = 0; i < size; i++)
    {
        numerador = (numerador + (agreg[i] * r_x[i]));
        denominador = (denominador + agreg[i]);
    }

    defuzzificada = numerador / denominador;
}

/*  Função que agrega todos as outras funções definidas
    Aqui são utilizadas as variáveis auxiliares definidas anteriormente
    E retorna como o resultado da defuzzificação*/
float controlador()
{
    erro_temperatura = erro_temp;
    variacao_erro_temperatura = var_erro_temp;

    fuzzification_erro();
    fuzzification_var_erro();
    implication();
    saida();
    agregacao();
    defuzzificacao();

    return defuzzificada;
}

/* Função Principal*/
int main()
{   /*Inicialização e chamada das funções para serem executadas*/
    stdio_init_all();
    fuzzification_erro(erro_temperatura);
    fuzzification_var_erro(variacao_erro_temperatura);
    implication(erro_alto_negativo, erro_baixo_negativo, erro_zero, erro_baixo_positivo, erro_alto_positivo, variacao_erro_negativa, variacao_erro_zero, variacao_erro_positiva);
    saida(cont1, r_x, vi);
    agregacao(regra1, regra2, regra3, regra4, regra5, regra6, regra7, regra8, regra9, regra10, regra11, regra12, regra13, regra14, regra15, r_mb, r_b, r_a, r_ma);
    defuzzificacao(numerador, denominador, cont1, agreg);

    /*Inicialização dos LEDs RGB*/
    gpio_init(RED);
    gpio_init(BLUE);
    gpio_init(GREEN);

    /*Definição dos pinos dos LEDs como saída*/
    gpio_set_dir(RED, GPIO_OUT);
    gpio_set_dir(BLUE, GPIO_OUT);
    gpio_set_dir(GREEN, GPIO_OUT);

    cont1 = 0;
    vi = 0.0;
    set_point = 0;
    sleep_ms(10000); /* O usuário deve aguardar 10 segundos após o ínicio da execução do algoritmo e digitar o valor de temperatura desejado*/
    printf("Digite o valor desejado de temperatura: ");
    scanf("%f", &set_point);

    /*Condições Iniciais do modelo ARX da planta térmica*/
    float y_prev1 = 30.0f; /*A temperatura Inicia em 30°C*/
    float y_prev2 = 32.0f;
    float u_prev = 0.0f;

    /*Inicialização do I2C e outros parâmetros importantes */
    i2c_init(i2c1, ssd1306_i2c_clock * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    /* Inicialização do display OLED */
    ssd1306_init();

    while (true)
    {
        /* Cálculo das entradas do Sistema de Inferência Fuzzy */
        erro_temp = set_point - y_prev1;
        var_erro_temp = (y_prev1 - y_prev2) / AMOSTRAGEM;

        /* Normalização do parâmetro u da planta */
        float u = controlador(erro_temp, var_erro_temp);
        float un = u / 8.0f;

        if (erro_temp < 0)
        {
            un = 0.0f;
        }

        /* Normalização dos parâmetros y da planta */
        float yn1 = (y_prev1 - 0.0) / (250.0f - 0.0f); /* Conversão do valor para se encaixar na faixa de temperatura de operação*/
        float yn2 = y_prev2 / 250.0f;
        float yn = a1 * yn1 + a2 * yn2 + b1 * un; /* Equação do modelo ARX */
        float y = yn * 250.0f;

        /* Atualização dos parâmetros da planta*/
        y_prev2 = y_prev1;
        y_prev1 = y;
        u_prev = u;

        printf("Set Point = %f, Temperatura Atual = %f, Erro = %f, Sinal de Controle = %f \n", set_point, y_prev1, erro_temp, defuzzificada); /*Exibe no Serial Monitor as grandezas */

        /* Laço de acionamento dos LEDs mediante o valor do erro de temperatura */
        if (erro_temp >= 0 && erro_temp <= 5)
        {
            gpio_put(GREEN, true);
            gpio_put(BLUE,false);
            gpio_put(RED,false);
        }
        if(erro_temp >= 6 && erro_temp <= 39){
            gpio_put(RED,false);
            gpio_put(BLUE,true);
            gpio_put(RED, false);
        }
        if(erro_temp >= 40 && erro_temp <= 70){
            gpio_put(RED,true);
            gpio_put(BLUE,false);
            gpio_put(GREEN,false);
        }

        // Preparar área de renderização para o display (ssd1306_width pixels por ssd1306_n_pages páginas)
        struct render_area frame_area = {
            start_column : 0,
            end_column : ssd1306_width - 1,
            start_page : 0,
            end_page : ssd1306_n_pages - 1
        };

        calculate_render_area_buffer_length(&frame_area);

        // zera o display inteiro
        uint8_t ssd[ssd1306_buffer_length];
        memset(ssd, 0, ssd1306_buffer_length);
        render_on_display(ssd, &frame_area);

        /*Conversão das variáveis para Strings e serem exibidas no display */
        char erro_temp_str[20]; /* Buffer para armazenar o valor como uma string para aparecer no display*/
        char set_point_str[20];
        char y_prev_str[20];
        char defuzzificada_str[20];
        sprintf(erro_temp_str, "E: %.2f ", erro_temp); /*Erro de Temperatura*/ 
        sprintf(set_point_str, "SP: %.2f", set_point); /*Temperatura Desejada*/
        sprintf(y_prev_str, "TA: %.2f", y_prev1); /*Temperatura Atual*/
        sprintf(defuzzificada_str, "C %.2f", defuzzificada); /*Sinal de Controle*/

    restart:
        /*Vetor de Strings contendo as informações a serem exibidas*/
        char *text[] = {
            "Erro:",
            erro_temp_str,
            "T.Desejada:",
            set_point_str,
            "T.Atual:",
            y_prev_str // Aqui usamos o texto formatado com o valor de erro_temp
        };
            /*Desenha os valores formatos no display OLED em posições específicas*/
            ssd1306_draw_string(ssd, 5, 0, set_point_str); /*Temperatura Desejada na primeira Linha*/
            ssd1306_draw_string(ssd, 5, 16, y_prev_str); /* Temperatura atual na segunda linha*/
            ssd1306_draw_string(ssd, 5, 32, erro_temp_str); /* Erro de Temperatura na terceira Linha*/
            ssd1306_draw_string(ssd, 5, 48, defuzzificada_str); /* Sinal de Controle na quarta linha*/
            

        /*Atualiza o display com as informações desenhadas*/ 
        render_on_display(ssd, &frame_area);
        /*Aguarda 1 segundo antes de atualizar a tela novamente*/
        sleep_ms(1000);
    }
}
