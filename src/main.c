/*******************************************************
This program was created by the CodeWizardAVR V4.07 
Automatic Program Generator
© Copyright 1998-2026 Pavel Haiduc, HP InfoTech S.R.L.
http://www.hpinfotech.ro

Project : codmorse
Version : 
Date    :  5/10/2026
Author  : 
Company : 
Comments: 


Chip type               : ATmega164
Program type            : Application
AVR Core Clock frequency: 10.000000 MHz
Memory model            : Small
External RAM size       : 0
Data Stack size         : 256
*******************************************************/

// I/O Registers definitions
#include <mega164.h>
#include <delay.h>

// Declare your global variables here
#define T_BASE 100      // Baza de timp T = 100ms 
#define MAX_BUFFER 20   // Capacitate buffer 
#define DELIMITATOR '#'

unsigned char Q=0;               // Starea curenta a automatului (FSM)
unsigned char buffer_tx[MAX_BUFFER]; 
int index_tx = 0;                  // Index pentru Input stack

// Variabile pentru receptie (Sampling)
unsigned char buffer_rx[MAX_BUFFER]; 
int index_rx = 0;
unsigned char temp_morse[6];    // Buffer pentru puncte/linii (max 5 pentru cifre)
int temp_index = 0;
unsigned char pauza_contor = 0; 
unsigned char mesaj_in_curs = 0; 

// --- Variabile pentru cazul special SOS ---
unsigned char SOS_activ = 0;     // 1 = Transmisie blocata, LED SOS clipeste 
unsigned char sos_step = 0;      // Monitorizarea secventei S-O-S
unsigned int timer_sos_10s = 0;  // 10 secunde de activare
unsigned int clipire_500ms = 0;  // Interval clipire LED 
unsigned char contor_20ms=0;

// --- FUNCTII AUXILIARE TRANSMISIE ---
void pulse_morse(unsigned char tip) {
    if (tip == '.') {
        PORTB.0 = 1; delay_ms(T_BASE); PORTB.0 = 0; // Punct = 1T 
    } else if (tip == '-') {
        PORTB.1 = 1; delay_ms(3 * T_BASE); PORTB.1 = 0; // Linie = 3T 
    }
    delay_ms(T_BASE); // Pauza intre simboluri = 1T 
}

void trimite_caracter(unsigned char c) {
    switch(c) {
        case 'A': pulse_morse('.'); pulse_morse('-'); break;
        case 'B': pulse_morse('-'); pulse_morse('.'); pulse_morse('.'); pulse_morse('.'); break;
        case 'C': pulse_morse('-'); pulse_morse('.'); pulse_morse('-'); pulse_morse('.'); break;
        case 'D': pulse_morse('-'); pulse_morse('.'); pulse_morse('.'); break;
        case 'E': pulse_morse('.'); break;
        case 'F': pulse_morse('.'); pulse_morse('.'); pulse_morse('-'); pulse_morse('.'); break;
        case 'G': pulse_morse('-'); pulse_morse('-'); pulse_morse('.'); break;
        case 'H': pulse_morse('.'); pulse_morse('.'); pulse_morse('.'); pulse_morse('.'); break;
        case 'I': pulse_morse('.'); pulse_morse('.'); break;
        case 'J': pulse_morse('.'); pulse_morse('-'); pulse_morse('-'); pulse_morse('-'); break;
        case 'K': pulse_morse('-'); pulse_morse('.'); pulse_morse('-'); break;
        case 'L': pulse_morse('.'); pulse_morse('-'); pulse_morse('.'); pulse_morse('.'); break;
        case 'M': pulse_morse('-'); pulse_morse('-'); break;
        case 'N': pulse_morse('-'); pulse_morse('.'); break;
        case 'O': pulse_morse('-'); pulse_morse('-'); pulse_morse('-'); break;
        case 'P': pulse_morse('.'); pulse_morse('-'); pulse_morse('-'); pulse_morse('.'); break;
        case 'Q': pulse_morse('-'); pulse_morse('-'); pulse_morse('.'); pulse_morse('-'); break;
        case 'R': pulse_morse('.'); pulse_morse('-'); pulse_morse('.'); break;
        case 'S': pulse_morse('.'); pulse_morse('.'); pulse_morse('.'); break;
        case 'T': pulse_morse('-'); break;
        case 'U': pulse_morse('.'); pulse_morse('.'); pulse_morse('-'); break;
        case 'V': pulse_morse('.'); pulse_morse('.'); pulse_morse('.'); pulse_morse('-'); break;
        case 'W': pulse_morse('.'); pulse_morse('-'); pulse_morse('-'); break;
        case 'X': pulse_morse('-'); pulse_morse('.'); pulse_morse('.'); pulse_morse('-'); break;
        case 'Y': pulse_morse('-'); pulse_morse('.'); pulse_morse('-'); pulse_morse('-'); break;
        case 'Z': pulse_morse('-'); pulse_morse('-'); pulse_morse('.'); pulse_morse('.'); break;
        
        case '1': pulse_morse('.'); pulse_morse('-'); pulse_morse('-'); pulse_morse('-'); pulse_morse('-'); break;
        case '2': pulse_morse('.'); pulse_morse('.'); pulse_morse('-'); pulse_morse('-'); pulse_morse('-'); break;
        case '3': pulse_morse('.'); pulse_morse('.'); pulse_morse('.'); pulse_morse('-'); pulse_morse('-'); break;
        case '4': pulse_morse('.'); pulse_morse('.'); pulse_morse('.'); pulse_morse('.'); pulse_morse('-'); break;
        case '5': pulse_morse('.'); pulse_morse('.'); pulse_morse('.'); pulse_morse('.'); pulse_morse('.'); break;
        case '6': pulse_morse('-'); pulse_morse('.'); pulse_morse('.'); pulse_morse('.'); pulse_morse('.'); break;
        case '7': pulse_morse('-'); pulse_morse('-'); pulse_morse('.'); pulse_morse('.'); pulse_morse('.'); break;
        case '8': pulse_morse('-'); pulse_morse('-'); pulse_morse('-'); pulse_morse('.'); pulse_morse('.'); break;
        case '9': pulse_morse('-'); pulse_morse('-'); pulse_morse('-'); pulse_morse('-'); pulse_morse('.'); break;
        case '0': pulse_morse('-'); pulse_morse('-'); pulse_morse('-'); pulse_morse('-'); pulse_morse('-'); break;
        
        case ' ': delay_ms(4 * T_BASE); break; // Pauza cuvant = 7T
    }
}

void executa_transmisie(void) {
    int i;
    PORTB.4 = 1; // LED TX ON 
    
    for (i = 0; i < index_tx; i++) {
        trimite_caracter(buffer_tx[i]);
        delay_ms(2 * T_BASE); // Pauza intre litere (total 3T) 
    }
    
    delay_ms(4 * T_BASE); // Asteptare minima 7T dupa mesaj 
    PORTB.4 = 0; 
    index_tx = 0; 
    Q = 0;        
}

unsigned char decodeaza_morse(void) {
    unsigned char rezultat = '?';

    if (temp_index == 1) {
        if (temp_morse[0] == '.') rezultat = 'E';
        else if (temp_morse[0] == '-') rezultat = 'T';
    } 
    else if (temp_index == 2) {
        if (temp_morse[0]=='.' && temp_morse[1]=='-') rezultat = 'A';
        else if (temp_morse[0]=='-' && temp_morse[1]=='.') rezultat = 'N';
        else if (temp_morse[0]=='.' && temp_morse[1]=='.') rezultat = 'I';
        else if (temp_morse[0]=='-' && temp_morse[1]=='-') rezultat = 'M';
    }
    else if (temp_index == 3) {
        if (temp_morse[0]=='-' && temp_morse[1]=='.' && temp_morse[2]=='.') rezultat = 'D';
        if (temp_morse[0]=='.' && temp_morse[1]=='-' && temp_morse[2]=='.') rezultat = 'R';
        if (temp_morse[0]=='.' && temp_morse[1]=='.' && temp_morse[2]=='.') rezultat = 'S';
        if (temp_morse[0]=='-' && temp_morse[1]=='-' && temp_morse[2]=='-') rezultat = 'O';
        if (temp_morse[0]=='-' && temp_morse[1]=='.' && temp_morse[2]=='-') rezultat = 'K';
        if (temp_morse[0]=='.' && temp_morse[1]=='-' && temp_morse[2]=='-') rezultat = 'W';
        if (temp_morse[0]=='-' && temp_morse[1]=='-' && temp_morse[2]=='.') rezultat = 'G';
        if (temp_morse[0]=='.' && temp_morse[1]=='.' && temp_morse[2]=='-') rezultat = 'U';
    }
    else if (temp_index == 4) {
        if (temp_morse[0]=='-' && temp_morse[1]=='.' && temp_morse[2]=='.' && temp_morse[3]=='.') rezultat = 'B';
        if (temp_morse[0]=='-' && temp_morse[1]=='.' && temp_morse[2]=='-' && temp_morse[3]=='.') rezultat = 'C';
        if (temp_morse[0]=='.' && temp_morse[1]=='.' && temp_morse[2]=='-' && temp_morse[3]=='.') rezultat = 'F';
        if (temp_morse[0]=='.' && temp_morse[1]=='.' && temp_morse[2]=='.' && temp_morse[3]=='.') rezultat = 'H';
        if (temp_morse[0]=='.' && temp_morse[1]=='-' && temp_morse[2]=='-' && temp_morse[3]=='-') rezultat = 'J';
        if (temp_morse[0]=='.' && temp_morse[1]=='-' && temp_morse[2]=='.' && temp_morse[3]=='.') rezultat = 'L';
        if (temp_morse[0]=='.' && temp_morse[1]=='-' && temp_morse[2]=='-' && temp_morse[3]=='.') rezultat = 'P';
        if (temp_morse[0]=='-' && temp_morse[1]=='-' && temp_morse[2]=='.' && temp_morse[3]=='-') rezultat = 'Q';
        if (temp_morse[0]=='.' && temp_morse[1]=='.' && temp_morse[2]=='.' && temp_morse[3]=='-') rezultat = 'V';
        if (temp_morse[0]=='-' && temp_morse[1]=='.' && temp_morse[2]=='.' && temp_morse[3]=='-') rezultat = 'X';
        if (temp_morse[0]=='-' && temp_morse[1]=='.' && temp_morse[2]=='-' && temp_morse[3]=='-') rezultat = 'Y';
        if (temp_morse[0]=='-' && temp_morse[1]=='-' && temp_morse[2]=='.' && temp_morse[3]=='.') rezultat = 'Z';
    }
    else if (temp_index == 5) {
        if (temp_morse[0]=='.' && temp_morse[1]=='-' && temp_morse[2]=='-' && temp_morse[3]=='-' && temp_morse[4]=='-') rezultat = '1';
        if (temp_morse[0]=='.' && temp_morse[1]=='.' && temp_morse[2]=='-' && temp_morse[3]=='-' && temp_morse[4]=='-') rezultat = '2';
        if (temp_morse[0]=='.' && temp_morse[1]=='.' && temp_morse[2]=='.' && temp_morse[3]=='-' && temp_morse[4]=='-') rezultat = '3';
        if (temp_morse[0]=='.' && temp_morse[1]=='.' && temp_morse[2]=='.' && temp_morse[3]=='.' && temp_morse[4]=='-') rezultat = '4';
        if (temp_morse[0]=='.' && temp_morse[1]=='.' && temp_morse[2]=='.' && temp_morse[3]=='.' && temp_morse[4]=='.') rezultat = '5';
        if (temp_morse[0]=='-' && temp_morse[1]=='.' && temp_morse[2]=='.' && temp_morse[3]=='.' && temp_morse[4]=='.') rezultat = '6';
        if (temp_morse[0]=='-' && temp_morse[1]=='-' && temp_morse[2]=='.' && temp_morse[3]=='.' && temp_morse[4]=='.') rezultat = '7';
        if (temp_morse[0]=='-' && temp_morse[1]=='-' && temp_morse[2]=='-' && temp_morse[3]=='.' && temp_morse[4]=='.') rezultat = '8';
        if (temp_morse[0]=='-' && temp_morse[1]=='-' && temp_morse[2]=='-' && temp_morse[3]=='-' && temp_morse[4]=='.') rezultat = '9';
        if (temp_morse[0]=='-' && temp_morse[1]=='-' && temp_morse[2]=='-' && temp_morse[3]=='-' && temp_morse[4]=='-') rezultat = '0';
    }

    // --- Detectie Secventiala SOS ---
    if (rezultat == 'S' && (sos_step == 0 || sos_step == 2)) sos_step++;
    else if (rezultat == 'O' && sos_step == 1) sos_step++;
    else sos_step = 0; 

    if (sos_step == 3) {
        SOS_activ = 1;          // Blocheaza transmisia 
        timer_sos_10s = 500;    // 10 secunde (500 unitati x 20ms) 
        sos_step = 0;
    }

    return rezultat;
}

// Timer 0 overflow interrupt service routine
interrupt [TIM0_OVF] void timer0_ovf_isr(void) {
    TCNT0 = 0x3C; // Baza de timp ~20ms 

    // A. Comportament Special SOS (10s Clipire / Blocare) 
    if (SOS_activ == 1) {
        if (timer_sos_10s > 0) {
            timer_sos_10s--;
            clipire_500ms++;
            
            // Perioada de clipire 500ms (250ms ON / 250ms OFF)
            if (clipire_500ms >= 12) {
                // Activam/Dezactivam ambele LED-uri simultan
                if (PORTB.0 == 0) {
                    PORTB.0 = 1; // LED Punct ON
                    PORTB.1 = 1; // LED Linie ON
                } else {
                    PORTB.0 = 0; // LED Punct OFF
                    PORTB.1 = 0; // LED Linie OFF
                }
                clipire_500ms = 0;                     
            }
        } else {
            SOS_activ = 0;   // Deblocare transmisie dupa 10 secunde
            PORTB.0 = 0;     // Asiguram stingerea LED-urilor
            PORTB.1 = 0;
        }
}

    // B. Receptie Morse (Sampling PINC) 
    if (PINC.0 == 0 || PINC.1 == 0) { // Semnal activ (SAU logic)
        if (mesaj_in_curs == 0) {
            buffer_rx[index_rx] = DELIMITATOR; // Start mesaj 
            index_rx = (index_rx + 1) % MAX_BUFFER; // Coada circulara 
            mesaj_in_curs = 1;
        }
        contor_20ms++;
        pauza_contor = 0; 
    } else { 
        if (contor_20ms > 0) {
            if (temp_index < 5) {
                if (contor_20ms >= 2 && contor_20ms <= 8) temp_morse[temp_index++] = '.'; // Punct 
                else if (contor_20ms >= 12 && contor_20ms <= 20) temp_morse[temp_index++] = '-'; // Linie 
            }
            contor_20ms = 0;
        }

        pauza_contor++;

        // Pauza intre litere (3T = ~300ms) 
        if (pauza_contor == 15 && temp_index > 0) {
            buffer_rx[index_rx] = decodeaza_morse(); 
            index_rx = (index_rx + 1) % MAX_BUFFER;
            temp_index = 0;
        }

        // Pauza intre cuvinte (7T = ~700ms) 
        if (pauza_contor == 35 && mesaj_in_curs == 1) {
            buffer_rx[index_rx] = ' '; 
            index_rx = (index_rx + 1) % MAX_BUFFER;
        }

        // Final de mesaj (Pauza lunga > 2 secunde) 
        if (pauza_contor > 100 && mesaj_in_curs == 1) {
            buffer_rx[index_rx] = DELIMITATOR; 
            index_rx = (index_rx + 1) % MAX_BUFFER;
            mesaj_in_curs = 0;
            pauza_contor = 0;
        }
    }
}
void main(void)
{
// Declare your local variables here

// Clock Oscillator division factor: 1
#pragma optsize-
CLKPR=(1<<CLKPCE);
CLKPR=(0<<CLKPCE) | (0<<CLKPS3) | (0<<CLKPS2) | (0<<CLKPS1) | (0<<CLKPS0);
#ifdef _OPTIMIZE_SIZE_
#pragma optsize+
#endif

// Input/Output Ports initialization
// Port A initialization
// Function: Bit7=In Bit6=In Bit5=In Bit4=In Bit3=In Bit2=In Bit1=In Bit0=In 
DDRA=(0<<DDA7) | (0<<DDA6) | (0<<DDA5) | (0<<DDA4) | (0<<DDA3) | (0<<DDA2) | (0<<DDA1) | (0<<DDA0);
// State: Bit7=T Bit6=T Bit5=T Bit4=T Bit3=T Bit2=P Bit1=P Bit0=P 
PORTA=(0<<PORTA7) | (0<<PORTA6) | (0<<PORTA5) | (0<<PORTA4) | (0<<PORTA3) | (1<<PORTA2) | (1<<PORTA1) | (1<<PORTA0);

// Port B initialization
// Function: Bit7=In Bit6=In Bit5=In Bit4=Out Bit3=Out Bit2=Out Bit1=Out Bit0=Out 
DDRB=(0<<DDB7) | (0<<DDB6) | (0<<DDB5) | (1<<DDB4) | (1<<DDB3) | (1<<DDB2) | (1<<DDB1) | (1<<DDB0);
// State: Bit7=T Bit6=T Bit5=T Bit4=0 Bit3=0 Bit2=0 Bit1=0 Bit0=0 
PORTB=(0<<PORTB7) | (0<<PORTB6) | (0<<PORTB5) | (0<<PORTB4) | (0<<PORTB3) | (0<<PORTB2) | (0<<PORTB1) | (0<<PORTB0);

// Port C initialization
// Function: Bit7=In Bit6=In Bit5=In Bit4=In Bit3=In Bit2=In Bit1=In Bit0=In 
DDRC=(0<<DDC7) | (0<<DDC6) | (0<<DDC5) | (0<<DDC4) | (0<<DDC3) | (0<<DDC2) | (0<<DDC1) | (0<<DDC0);
// State: Bit7=T Bit6=T Bit5=T Bit4=T Bit3=T Bit2=T Bit1=P Bit0=P 
PORTC=(0<<PORTC7) | (0<<PORTC6) | (0<<PORTC5) | (0<<PORTC4) | (0<<PORTC3) | (0<<PORTC2) | (1<<PORTC1) | (1<<PORTC0);

// Port D initialization
// Function: Bit7=In Bit6=In Bit5=In Bit4=In Bit3=In Bit2=In Bit1=In Bit0=In 
DDRD=(0<<DDD7) | (0<<DDD6) | (0<<DDD5) | (0<<DDD4) | (0<<DDD3) | (0<<DDD2) | (0<<DDD1) | (0<<DDD0);
// State: Bit7=P Bit6=P Bit5=P Bit4=P Bit3=P Bit2=P Bit1=P Bit0=P 
PORTD=(1<<PORTD7) | (1<<PORTD6) | (1<<PORTD5) | (1<<PORTD4) | (1<<PORTD3) | (1<<PORTD2) | (1<<PORTD1) | (1<<PORTD0);

// Timer/Counter 0 initialization
// Clock source: System Clock
// Clock value: 9.766 kHz
// Mode: Normal top=0xFF
// OC0A output: Disconnected
// OC0B output: Disconnected
// Timer Period: 20.07 ms
TCCR0A=(0<<COM0A1) | (0<<COM0A0) | (0<<COM0B1) | (0<<COM0B0) | (0<<WGM01) | (0<<WGM00);
TCCR0B=(0<<WGM02) | (1<<CS02) | (0<<CS01) | (1<<CS00);
TCNT0=0x3C;
OCR0A=0x00;
OCR0B=0x00;

// Timer/Counter 1 initialization
// Clock source: System Clock
// Clock value: Timer1 Stopped
// Mode: Normal top=0xFFFF
// OC1A output: Disconnected
// OC1B output: Disconnected
// Noise Canceler: Off
// Input Capture on Falling Edge
// Timer1 Overflow Interrupt: Off
// Input Capture Interrupt: Off
// Compare A Match Interrupt: Off
// Compare B Match Interrupt: Off
TCCR1A=(0<<COM1A1) | (0<<COM1A0) | (0<<COM1B1) | (0<<COM1B0) | (0<<WGM11) | (0<<WGM10);
TCCR1B=(0<<ICNC1) | (0<<ICES1) | (0<<WGM13) | (0<<WGM12) | (0<<CS12) | (0<<CS11) | (0<<CS10);
TCNT1H=0x00;
TCNT1L=0x00;
ICR1H=0x00;
ICR1L=0x00;
OCR1AH=0x00;
OCR1AL=0x00;
OCR1BH=0x00;
OCR1BL=0x00;

// Timer/Counter 2 initialization
// Clock source: System Clock
// Clock value: Timer2 Stopped
// Mode: Normal top=0xFF
// OC2A output: Disconnected
// OC2B output: Disconnected
ASSR=(0<<EXCLK) | (0<<AS2);
TCCR2A=(0<<COM2A1) | (0<<COM2A0) | (0<<COM2B1) | (0<<COM2B0) | (0<<WGM21) | (0<<WGM20);
TCCR2B=(0<<WGM22) | (0<<CS22) | (0<<CS21) | (0<<CS20);
TCNT2=0x00;
OCR2A=0x00;
OCR2B=0x00;

// Timer/Counter 0 Interrupt(s) initialization
TIMSK0=(0<<OCIE0B) | (0<<OCIE0A) | (1<<TOIE0);

// Timer/Counter 1 Interrupt(s) initialization
TIMSK1=(0<<ICIE1) | (0<<OCIE1B) | (0<<OCIE1A) | (0<<TOIE1);

// Timer/Counter 2 Interrupt(s) initialization
TIMSK2=(0<<OCIE2B) | (0<<OCIE2A) | (0<<TOIE2);

// External Interrupt(s) initialization
// INT0: Off
// INT1: Off
// INT2: Off
// Interrupt on any change on pins PCINT0-7: Off
// Interrupt on any change on pins PCINT8-15: Off
// Interrupt on any change on pins PCINT16-23: Off
// Interrupt on any change on pins PCINT24-31: Off
EICRA=(0<<ISC21) | (0<<ISC20) | (0<<ISC11) | (0<<ISC10) | (0<<ISC01) | (0<<ISC00);
EIMSK=(0<<INT2) | (0<<INT1) | (0<<INT0);
PCICR=(0<<PCIE3) | (0<<PCIE2) | (0<<PCIE1) | (0<<PCIE0);

// USART0 initialization
// USART0 disabled
UCSR0B=(0<<RXCIE0) | (0<<TXCIE0) | (0<<UDRIE0) | (0<<RXEN0) | (0<<TXEN0) | (0<<UCSZ02) | (0<<RXB80) | (0<<TXB80);

// USART1 initialization
// USART1 disabled
UCSR1B=(0<<RXCIE1) | (0<<TXCIE1) | (0<<UDRIE1) | (0<<RXEN1) | (0<<TXEN1) | (0<<UCSZ12) | (0<<RXB81) | (0<<TXB81);

// Analog Comparator initialization
// Analog Comparator: Off
// The Analog Comparator's positive input is
// connected to the AIN0 pin
// The Analog Comparator's negative input is
// connected to the AIN1 pin
ACSR=(1<<ACD) | (0<<ACBG) | (0<<ACO) | (0<<ACI) | (0<<ACIE) | (0<<ACIC) | (0<<ACIS1) | (0<<ACIS0);
ADCSRB=(0<<ACME);
// Digital input buffer on AIN0: On
// Digital input buffer on AIN1: On
DIDR1=(0<<AIN0D) | (0<<AIN1D);

// ADC initialization
// ADC disabled
ADCSRA=(0<<ADEN) | (0<<ADSC) | (0<<ADATE) | (0<<ADIF) | (0<<ADIE) | (0<<ADPS2) | (0<<ADPS1) | (0<<ADPS0);

// SPI initialization
// SPI disabled
SPCR=(0<<SPIE) | (0<<SPE) | (0<<DORD) | (0<<MSTR) | (0<<CPOL) | (0<<CPHA) | (0<<SPR1) | (0<<SPR0);

// TWI initialization
// TWI disabled
TWCR=(0<<TWEA) | (0<<TWSTA) | (0<<TWSTO) | (0<<TWEN) | (0<<TWIE);

// Globally enable interrupts
#asm("sei")

while (1)
      {
      switch(Q) {
            case 0: // STAREA REPAUS (IDLE) 
                PORTB.2 = 1; // LED IDLE ON
                if (PINA.0 == 0) { // Apasare MOD 
                    delay_ms(200); 
                    PORTB.2 = 0;
                    Q = 1; // Trecere în COMPUNERE
                }
                break;

            case 1: // --- STAREA COMPUNERE ---
                PORTB.3 = 1; // LED COMPUNERE ON
                
                if (PINA.1 == 0) { // Buton SAVE 
                    if (index_tx < MAX_BUFFER) {
                        buffer_tx[index_tx] = PIND & 0x7F; // Preluare ASCII 
                        index_tx++;
                    }
                    delay_ms(250);
                }
                
                if (PINA.2 == 0) { // Buton DELETE 
                    if (index_tx > 0) index_tx--; 
                    delay_ms(250); 
                }
                
                if (PIND.7 == 0 && SOS_activ == 0) { // Pornire Transmisie 
                    delay_ms(200);
                    PORTB.3 = 0;
                    Q = 2; 
                }
                
                if (PINA.0 == 0) { // Revenire manuala MOD
                    delay_ms(200);
                    PORTB.3 = 0;
                    Q = 0;
                }
                break;

            case 2: // --- STAREA TRANSMISIE ---
                executa_transmisie();
                break;
        }
      }
}
