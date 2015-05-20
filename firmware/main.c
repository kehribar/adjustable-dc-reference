/*-----------------------------------------------------------------------------
/
/
/----------------------------------------------------------------------------*/
#include <stdint.h>
#include <stdlib.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/eeprom.h>
/*---------------------------------------------------------------------------*/
#include "suart.h"
#include "xitoa.h"
#include "digital.h"
/*---------------------------------------------------------------------------*/
char buffer[64];
uint16_t* chA_addr = (uint16_t*)10;
uint16_t* chB_addr = (uint16_t*)20;
/*---------------------------------------------------------------------------*/
void ltc1448_update(uint16_t chA, uint16_t chB)
{
    uint8_t i;
    uint16_t tmp;

    /* Select Chip */
    digitalWrite(B,0,LOW);

    /* Channel A */
    tmp = chA;
    for(i=0;i<12;i++)
    {
        if(tmp & (1<<11))
        {
            digitalWrite(B,1,HIGH);
        }
        else
        {
            digitalWrite(B,1,LOW);
        }

        digitalWrite(B,2,HIGH);
        digitalWrite(B,2,LOW);

        tmp = tmp << 1;
    }

    /* Channel B */
    tmp = chB;
    for(i=0;i<12;i++)
    {
        if(tmp & (1<<11))
        {
            digitalWrite(B,1,HIGH);
        }
        else
        {
            digitalWrite(B,1,LOW);
        }

        digitalWrite(B,2,HIGH);
        digitalWrite(B,2,LOW);

        tmp = tmp << 1;
    }

    /* Deselect Chip */
    digitalWrite(B,0,HIGH);
}
/*---------------------------------------------------------------------------*/
void readLine(char* lineBuf,uint8_t len)
{
    uint8_t c, i;
    len--; i=0;

    xmit('>');  
    for (;;) 
    {
        c = rcvr();
        if (i == len) break;
        if (c == '\r' || c == '\n') break;
        if (c >= ' '){
            lineBuf[i++] = c;
            xmit(c);
        }
    }
    lineBuf[i]=0;
    xmit('\r'); 
    xmit('\n');
}
/*---------------------------------------------------------------------------*/
uint8_t custom_atoi(char** ptr, uint16_t* res)
{    
    uint16_t val = 0;
    uint8_t hasSomething = 0;

    while((**ptr >= '0')&&(**ptr <= '9'))
    {
        val = (val * 10) + (**ptr - '0');
        *ptr += 1;
        hasSomething = 1;
    }

    *res = val;
    return hasSomething;
}
/*---------------------------------------------------------------------------*/
int main()
{
    char* ptr;
    long* value;
    uint16_t chA;
    uint16_t chB;        

    /* DAC pinout */
    pinMode(B,0,OUTPUT);
    pinMode(B,1,OUTPUT);
    pinMode(B,2,OUTPUT);

    /* Serial connection pinout */
    pinMode(B,3,OUTPUT);
    pinMode(B,4,INPUT);    
    
    /* Disable as much as possible */
    PRR = 0x0F;
    ACSR = (1<< ACD);

    /* Init xprintf library */
    xfunc_out = xmit;

    /* Initial values from EEPROM memory */
    chA = eeprom_read_word(chA_addr);
    chB = eeprom_read_word(chB_addr);
    ltc1448_update(chA,chB);

    xprintf(PSTR("> 'Hello World' from DC volt/current source!\r\n"));

    while(1)
    {
        /* Clear the buffer ... */
        memset(buffer,0x00,sizeof(buffer));

        /* Get the message ... */
        readLine(buffer,sizeof(buffer));        
        
        /* Analyze the message. */
        switch(buffer[0])
        {
            /*---------------------------------------------------------------*/
            /* Update the DAC command. */
            case 'a':
            {
                ptr = buffer;                
                
                *ptr++; /* skip initial command char */                
                *ptr++; /* skip the separator char */

                if(custom_atoi(&ptr,&chA))
                {
                    *ptr++; /* skip the separator char */

                    if(custom_atoi(&ptr,&chB))
                    {
                        ltc1448_update(chA,chB);
                        xprintf(PSTR("> Channel A: %d Channel B: %d\r\n"),chA,chB);
                    }
                    else
                    {
                        xprintf(PSTR("> Formatting error!\r\n"));
                    }
                    
                }
                else
                {
                    xprintf(PSTR("> Formatting error!\r\n"));
                }

                break;
            }
            /*---------------------------------------------------------------*/
            /* Store the latest dac values to EEPROM */
            case 'b':
            {
                eeprom_write_word(chA_addr,chA);
                eeprom_write_word(chB_addr,chB);
                xprintf(PSTR("> Values are stored to EEPROM\r\n"));
                break;
            }
            /*---------------------------------------------------------------*/
            /* Unknown command ... */
            default:
            {
                xprintf(PSTR("> What?\r\n"));
                break;
            }
            /*---------------------------------------------------------------*/
        }        
    }

    return 0;
}
/*---------------------------------------------------------------------------*/
