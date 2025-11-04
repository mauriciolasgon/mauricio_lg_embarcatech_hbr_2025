#include <stdio.h>
#include "lib/Unity/src/unity.h"



void setUp(void){}
void tearDown(void){}

float adc_to_celsius(uint16_t adc_val){
    float ADC_voltage = adc_val*(3.3f/((1<<12)-1)); // (valor adc*3.3v)/4096
    return (27 - (ADC_voltage - 0.706f) /0.001721f); // formula apresentada no datasheet
}

void test_adc_to_celsius(){
    uint16_t raw_adc= (uint16_t)(0.706f/(3.3f/((1<<12)-1)));

    float temp= adc_to_celsius(raw_adc);

    TEST_ASSERT_FLOAT_WITHIN(0.1,27.0f,temp);
}

int main()
{
    
    printf("Iniciando testes\n");
    UNITY_BEGIN();
    RUN_TEST(test_adc_to_celsius);
    UNITY_END();
    printf("Teste finalizado\n");
    
    return 0;
    
}
