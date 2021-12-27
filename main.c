/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved 
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/
#include "project.h"

/****Variables para el control general del Sistema****/
typedef struct Variables{                                                        
    uint16 ms:10;                                                              
    uint16 seg:3;
    uint16 contador:5;
}Variables;
Variables variable;


/****Variables para el control del ADC****/
typedef struct Medidas{           
    uint32 acumulado_Vref:20; 
    uint32 acumulado_voltaje_sensor_crudo:20;
    uint32 acumulado_temperatura:20;   
    
    uint32 Vref:20;
    uint32 ADC:12;
    uint32 voltaje_sensor_crudo:20;
    uint32 temperatura:12;                                                    
}medidas;
medidas medida;
 

/**** Funciones del Sistema****/
void sensar(void);
void imprimir(void);

CY_ISR_PROTO(cronometro);

int main(void)
{
    CyGlobalIntEnable; /* Enable global interrupts. */

    isr_cronometro_StartEx(cronometro);                                         // Inicializacion de interrupciones
    
    Seguidor_1_Start();                                                         // Inicializacion de componentes
    Seguidor_2_Start();
    Amplificador_Sumador_Start();
    
    Cronometro_Start();
    AMux_Start();
    ADC_Start();
    ADC_SetResolution(12);
    LCD_Start();

    LCD_ClearDisplay();
    
    for(;;)
    {
        if (variable.ms%25 == 0) sensar();                                      // Sensa 40 veces por segundo
        if (variable.ms%500 == 0) imprimir();                                   // Imprime 2 veces por segundo
    }
}

void imprimir(void){                                                            // Interfaz en el LCD
    LCD_ClearDisplay();
    
    LCD_Position(0,0);                                                          // Temperatura medida
    LCD_PrintString("Temp: ");
    LCD_PrintNumber(medida.temperatura/100);
    LCD_PutChar('.');
    LCD_PrintNumber(medida.temperatura/10%10);
    LCD_PrintNumber(medida.temperatura%10);
    LCD_Position(0,11);
    LCD_PutChar(0xDF);
    LCD_PutChar('C');
    
    LCD_Position(1,0);                                                          // Lectura del ADC
    LCD_PrintString("ADC: ");
    LCD_PrintNumber(medida.ADC);
    
    LCD_Position(2,0);                                                          // Resolución del ADC en función de los bits usados
    LCD_PrintString("n:12   Resol: 0.02");
    //LCD_PrintString("n:10   Resol: 0.09");
    //LCD_PrintString("n:8   Resol: 0.36");
    
    LCD_Position(3,0);                                                          // Voltaje de referencia:
    //LCD_PrintString("Vref+: 4.6");                                            // - para 8 y 10 bits
    
    LCD_PrintString("Vref+: ");                                                 
    LCD_PrintNumber(medida.Vref/1000);                                          // - Rutina de Vref+ para n=12 bits
    LCD_PutChar('.');
    LCD_PrintNumber(medida.Vref/100%10);
    LCD_PrintNumber(medida.Vref/10%10);
    LCD_PrintNumber(medida.Vref%10);
}

void sensar(void){
    variable.contador++;                                                        // Contador hasta 20
    
    AMux_Select(0);                                                             // Mide Vref+
    ADC_StartConvert();
    ADC_IsEndConversion(ADC_WAIT_FOR_RESULT);
    medida.acumulado_Vref += ADC_GetResult16();
    
    AMux_Select(1);                                                             // Mide Vsensor antes de acondicionarse
    ADC_StartConvert();
    ADC_IsEndConversion(ADC_WAIT_FOR_RESULT);
    medida.acumulado_voltaje_sensor_crudo += ADC_GetResult16();
    
    AMux_Select(2);                                                             // Mide Vsensor acondicionado
    ADC_StartConvert();
    ADC_IsEndConversion(ADC_WAIT_FOR_RESULT);
    medida.acumulado_temperatura += ADC_GetResult16();
    
    if (variable.contador == 20){                                               
        variable.contador = 0;
        
        medida.acumulado_Vref /= 20;                                            // Promedios de las variables cada 20 muestras
        medida.acumulado_voltaje_sensor_crudo /= 20;
        medida.acumulado_temperatura /= 20;
        
        medida.Vref = (uint32)1000*4095*1.024/medida.acumulado_Vref;            // Ecuaciones de Visualización
        medida.voltaje_sensor_crudo = medida.acumulado_voltaje_sensor_crudo;
        medida.ADC = medida.acumulado_temperatura;
        medida.temperatura = (uint32) 100*(medida.ADC-911)*40/1781;    // Para n = 12 bits
        //medida.temperatura = (uint32) 100*(medida.ADC-227)*40/445;   // Para n = 10 bits
        //medida.temperatura = (uint32) 10*(medida.ADC-56)*40/111;     // Para n = 8 bits
        
        medida.acumulado_Vref = 0;                                              // Reset de las variables acumuladas
        medida.acumulado_voltaje_sensor_crudo = 0;
        medida.acumulado_temperatura = 0;
    }
}

CY_ISR(cronometro){                                                             // Cronometro de muestreo y promedio
    variable.ms++;
    if (variable.ms == 1000) {
        variable.ms = 0;
        variable.seg++;
    }
    
}
/* [] END OF FILE */
