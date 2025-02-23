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

const uint I2C_SDA = 14;
const uint I2C_SCL = 15;

#define RED 13
#define BLUE 12
#define GREEN 11

// Definição das constantes utilizadas no projeto
#define size 1001
#define AMOSTRAGEM 4;
//
int cont1;

// Definição das variáveis de entrada do sistema de inferência fuzzy Mamdani
float set_point;
float erro_temperatura;          // Variável de erro de temperatura definida como sendo E[n] = T[s] - T[y] Com amostragem discreta, sendo "T[s]" a temperatura de set point ou temperatura desejada e "T[y]"" é a temperatura tual
float variacao_erro_temperatura; // Variável da variação do erro definida como: VE = T[y] - T[y-1]/T. Sendo T[y-1] a temperatura no estado anterior e "T" é o tempo de amostragem
float temp_atual;
float temp_anterior;
float erro_temp;
float var_erro_temp;
// Definição das funções de pertinência da variável do erro de temperatura
float erro_alto_negativo;
float erro_baixo_negativo;
float erro_zero;
float erro_baixo_positivo;
float erro_alto_positivo;
// Definição das funções de pertinência da variável variação do erro de temperatura
float variacao_erro_negativa;
float variacao_erro_zero;
float variacao_erro_positiva;
// Definição das variáveis que armazenam o resultado da interação das pertinências do erro e da variação do erro
float regra1;
float regra2;
float regra3;
float regra4;
float regra5;
float regra6;
float regra7;
float regra8;
float regra9;
float regra10;
float regra11;
float regra12;
float regra13;
float regra14;
float regra15;
// Definição das variáveis necessárias para o processo de defuzzificação
float numerador;
float denominador;
float defuzzificada;
// Definição dos vetores
float r_x[size];
float r_mb[size];
float r_b[size];
float r_a[size];
float r_ma[size];
float agreg[size];
double vi;

// Definição dos parâmetros para modelagem da planta de temperatura
float a1 = 1.4614;
float a2 = -0.4798;
float b1 = 0.0216;

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
        //    printf("Entrou aqui");
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
        agreg[i] = fmax(agreg[i], fmin(regra9, r_a[i])); // ainda sem alterar
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

int main()
{
    stdio_init_all();
    fuzzification_erro(erro_temperatura);
    fuzzification_var_erro(variacao_erro_temperatura);
    implication(erro_alto_negativo, erro_baixo_negativo, erro_zero, erro_baixo_positivo, erro_alto_positivo, variacao_erro_negativa, variacao_erro_zero, variacao_erro_positiva);
    saida(cont1, r_x, vi);
    agregacao(regra1, regra2, regra3, regra4, regra5, regra6, regra7, regra8, regra9, regra10, regra11, regra12, regra13, regra14, regra15, r_mb, r_b, r_a, r_ma);
    defuzzificacao(numerador, denominador, cont1, agreg);

    gpio_init(RED);
    gpio_init(BLUE);
    gpio_init(GREEN);

    gpio_set_dir(RED, GPIO_OUT);
    gpio_set_dir(BLUE, GPIO_OUT);
    gpio_set_dir(GREEN, GPIO_OUT);

    cont1 = 0;
    vi = 0.0;
    set_point = 0;
    sleep_ms(10000);
    printf("Digite o valor desejado de temperatura: ");
    scanf("%f", &set_point);
    float y_prev1 = 30.0f;
    float y_prev2 = 32.0f;
    float u_prev = 0.0f;

    // Inicialização do i2c
    i2c_init(i2c1, ssd1306_i2c_clock * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Processo de inicialização completo do OLED SSD1306
    ssd1306_init();

    while (true)
    {

        erro_temp = set_point - y_prev1;
        var_erro_temp = (y_prev1 - y_prev2) / AMOSTRAGEM;

        float u = controlador(erro_temp, var_erro_temp);
        float un = u / 8.0f;

        if (erro_temp < 0)
        {
            un = 0.0f;
        }

        float yn1 = (y_prev1 - 0.0) / (250.0f - 0.0f);
        float yn2 = y_prev2 / 250.0f;
        float yn = a1 * yn1 + a2 * yn2 + b1 * un;
        float y = yn * 250.0f;

        y_prev2 = y_prev1;
        y_prev1 = y;
        u_prev = u;

        // printf("O valor de y prévio é = %f\n", y_prev1);
        printf("Erro = %f\n", erro_temp);
        // printf("Variação do Erro = %f\n", var_erro_temp);
        // printf("Defuzzificação é %f \n",defuzzificada);

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

        // Converte o erro de temperatura para uma string
        char erro_temp_str[20]; // Buffer para armazenar o valor como string
        char set_point_str[20];
        char y_prev_str[20];
        sprintf(erro_temp_str, "E: %.2f ", erro_temp); // Formata o valor de erro_temp para string com 2 casas decimais
        sprintf(set_point_str, "SP: %.2f", set_point);
        sprintf(y_prev_str, "TA: %.2f", y_prev1);

    restart:

        char *text[] = {
            "Erro:",
            erro_temp_str,
            "T.Desejada:",
            set_point_str,
            "T.Atual:",
            y_prev_str // Aqui usamos o texto formatado com o valor de erro_temp
        };

        int p = 0;
        for (uint i = 0; i < count_of(text); i++)
        {
            ssd1306_draw_string(ssd, 5, p, text[i]);
            p += 8;
        }

        // Renderiza o conteúdo na tela
        render_on_display(ssd, &frame_area);

        sleep_ms(1000);
    }
}
