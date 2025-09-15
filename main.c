#include"stm32f103x6.h"
#include<stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
void delay_ms(unsigned int ms)
{

	RCC->APB1ENR |=(1<<0); //Enable TIM2

	TIM2->PSC =7999; //TIM2 frequency= APB1 clock/ (PSC+1) ==1KHZ //TIM2 counts one every 1ms//APB1 clock =36MHZ default

	TIM2->ARR=(ms-1);//setting overflow count up to n-1 if n is the delay provided

	TIM2->CNT=0;//setting initial counter value

	TIM2->CR1 |=(1<<0);//Start the timer

	while(!(TIM2->SR & (1<<0))); //checking the 0th bit of status register if it is 1 or not

	TIM2->SR &= ~(1<<0); //clear the 0th bit of status register for future operation
}

void UART1_INIT(void)
{
	RCC->APB2ENR |= (1 << 2);  // Enable GPIOA
	RCC->APB2ENR |= (1 << 14); // Enable USART1

	GPIOA->CRH &= ~(0xF << 4);   // Clear bits for PA9
	GPIOA->CRH |= (0xB << 4);    // 0xB = 1011 => AF Push-Pull, 50 MHz

	GPIOA->CRH &= ~(0xF << 8);   // Clear bits for PA10
	GPIOA->CRH |= (0x4 << 8);    // 0x4 = 0100 => Input floating

	//USART1->BRR = (52 << 4) | (1);// Baud rate = 9600   //or//
	USART1->BRR = 8000000/9600;
	USART1->CR1 |= (1 << 13);           // UE: USART Enable
	USART1->CR1 |= (1 << 3);            // TE: Transmitter Enable
	USART1->CR1 |= (1 << 2);            // RE: Receiver Enable

}

void UART1_TxChar(char c) {
    while (!(USART1->SR & (1 << 7))); // Wait for TXE (Transmit data register empty)
    USART1->DR = c;
}

char UART1_RxChar(void) {
    while (!(USART1->SR & (1 << 5))); // Wait for RXNE (Receive data register not empty)
    return USART1->DR;
}

void UART1_TxStr(const char *s)
{
	while(*s)
	{
		UART1_TxChar(*s++);
	}
}


char *UART1_RxStr(char *s, int max_len)
{
    int i = 0;
    char c;
    while (1)
    {
        c = UART1_RxChar();

        // Handle Enter
        if (c == '\r' || c == '\n')
            break;

        // Handle Backspace (ASCII 8 or 127)
        if ((c == 8 || c == 127) && i > 0)
        {
            i--;                     // Remove last character
            UART1_TxStr("\b \b");    // Move cursor back, overwrite with space, move back again
            continue;
        }

        // Ignore backspace if no characters entered yet
        if ((c == 8 || c == 127) && i == 0)
            continue;

        // Normal character
        if (i < max_len - 1 && isprint(c))
        {
            s[i++] = c;
            UART1_TxChar(c); // Echo the character
        }
    }
    s[i] = '\0'; // Null-terminate the string
    return s;
}


/*void UART1_RxStr(char *s)
{
    char c;
    int i = 0;
    while (1)
    {
        c = UART1_RxChar();
        if (c == '\r') // Enter key
            break;
        s[i++] = c;
        UART1_TxChar(c); // Echo back
    }
    s[i] = '\0'; // Null terminate
    UART1_TxStr("\r\n"); // Go to next line
}
*/

// === Expression Parsing ===

const char *expr_ptr;

int parse_expression();


void skip_whitespace()
{
    while (*expr_ptr == ' ') expr_ptr++;
}

int parse_number()
{
    skip_whitespace();
    int num = 0;
    while (isdigit((unsigned char)*expr_ptr))
    {
        num = num * 10 + (*expr_ptr - '0');
        expr_ptr++;
    }
    return num;
}

int parse_factor()
{
    skip_whitespace();
    int result;

    if (*expr_ptr == '(')
    {
        expr_ptr++; // Skip '('
        result = parse_expression();
        if (*expr_ptr == ')')
            expr_ptr++; // Skip ')'
    }
    else
    {
        result = parse_number();
    }
    return result;
}

int parse_term()
{
    int result = parse_factor();
    skip_whitespace();
    while (*expr_ptr == '*' || *expr_ptr == '/')
    {
        char op = *expr_ptr++;
        int next = parse_factor();
        if (op == '*')
            result *= next;
        else if (op == '/')
            result /= next;
        skip_whitespace();
    }
    return result;
}

int parse_expression()
{
    int result = parse_term();
    skip_whitespace();
    while (*expr_ptr == '+' || *expr_ptr == '-')
    {
        char op = *expr_ptr++;
        int next = parse_term();
        if (op == '+')
            result += next;
        else if (op == '-')
            result -= next;
        skip_whitespace();
    }
    return result;
}

int evaluate_expression(const char *expr)
{
    expr_ptr = expr;
    return parse_expression();
}

void my_itoa(int val, char *str)
{
    sprintf(str, "%d", val);
}

int main(void)
{
    char input[100];
    char result_str[20];

    UART1_INIT();
    UART1_TxStr("\r\nUART Calculator Ready\r\n");

    while (1)
    {
        UART1_TxStr("\r\nEnter Expression: ");
        UART1_RxStr(input, sizeof(input));
       // UART1_RxStr(input);
        int result = evaluate_expression(input);
        my_itoa(result, result_str);
        UART1_TxStr("\r\nResult = ");
        UART1_TxStr(result_str);
        UART1_TxStr("\r\n");
    }
}

