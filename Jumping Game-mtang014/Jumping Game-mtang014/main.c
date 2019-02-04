/*
 * Jumping Game-mtang014.c
 *
 * Created: 3/1/2018 3:03:09 PM
 * Author : Michael(mtang014@ucr.edu)
 * Section: 023
 * I acknowledge all content contained herein, excluding template or example  
 * code, is my own original work. 
 */ 

#include <avr/io.h>
#include "timer.h"
#include "io.c"
#include "bit.h"

unsigned char *top = "       ##  ###           ####        #####         ##      ####        ";
unsigned char *bot = "   ##           ######         ####          ####      ##              ";
unsigned char textSize = 68;
unsigned char tmpA, player_position, game_pause, game_over, pauseHS, score;
//top
int k = 0;
int j = 1;
//bot
int l = 0;
int m = 17;

unsigned long int findGCD(unsigned long int a, unsigned long int b)
{
	unsigned long int c;
	while(1){
		c = a%b;
		if(c==0){return b;}
		a = b;
		b = c;
	}
	return 0;
}

typedef struct _task {
	signed char state;
	unsigned long int period;
	unsigned long int elapsedTime;
	int (*TaskFct) (int);
} task;

enum states1 {SM_start1, move} state1;
enum states2 {SM_start2, display} state2;
//enum states3 {SM_start3, display} state3;	

void set_PWM(double frequency) {
	static double current_frequency; // Keeps track of the currently set frequency
	// Will only update the registers when the frequency changes, otherwise allows
	// music to play uninterrupted.
	if (frequency != current_frequency) {
		if (!frequency) { TCCR3B &= 0x08; } //stops timer/counter
		else { TCCR3B |= 0x03; } // resumes/continues timer/counter
		
		// prevents OCR3A from overflowing, using prescaler 64
		// 0.954 is smallest frequency that will not result in overflow
		if (frequency < 0.954) { OCR3A = 0xFFFF; }
		
		// prevents OCR0A from underflowing, using prescaler 64					// 31250 is largest frequency that will not result in underflow
		else if (frequency > 31250) { OCR3A = 0x0000; }
		
		// set OCR3A based on desired frequency
		else { OCR3A = (short)(8000000 / (128 * frequency)) - 1; }

		TCNT3 = 0; // resets counter
		current_frequency = frequency; // Updates the current frequency
	}
}

void PWM_on() {
	TCCR3A = (1 << COM3A0);
	// COM3A0: Toggle PB3 on compare match between counter and OCR0A
	TCCR3B = (1 << WGM32) | (1 << CS31) | (1 << CS30);
	// WGM02: When counter (TCNT0) matches OCR0A, reset counter
	// CS01 & CS30: Set a prescaler of 64
	set_PWM(0);
}

void PWM_off() {
	TCCR3A = 0x00;
	TCCR3B = 0x00;
}

int SM_Player(int state){
	
	switch (state){
		case SM_start1:
		PWM_off();
		score = 0;
		//Screen flashes to give that cool effect
		//Cannot press start. You must hold start button if you are committed to a match
		    LCD_DisplayString(1,"Gravity Byte    Hold Start     ");
			if(GetBit(tmpA, 2)){
				PWM_on();
				set_PWM(493.88);
				state = move;
				break;
			}
			state = SM_start1;
		break;
		
		case move:
		PWM_off();
		if(GetBit(tmpA, 3) && (player_position > 17)){
		    PWM_on();
			set_PWM(440.00);
			player_position -= 16;
			
		}
		if(GetBit(tmpA, 4) && (player_position < 17)){
		    PWM_on();
			set_PWM(440.00);
			player_position += 16;
	
		}
		if(GetBit(tmpA, 5) && !pauseHS){
			pauseHS = 1;
			if(game_over == 1){
				game_over = 0;
				break;
			}
			else{
				game_pause = ~game_pause;
				}
		}
		state = move;
		break;
		
		default:
		state = SM_start1;
		break;
	}
	LCD_Cursor(player_position);
	return state;
}

int SM_LCD(int state){
	switch (state) {
		case SM_start2:
		if(game_over == 1){
			state = SM_start2;
			break;
		}
		state = display;
		break;
		
		case display:
		state = display;
		break;
		
		default:
		state = SM_start2;
		break;
	}
	
	switch (state) {
		case SM_start2:
		break;
		
		case display:
		PWM_off();
		//top
		pauseHS = 0;
		if(game_pause == 0){
			for (j = 1; j <= 16; j++) {
				LCD_Cursor(j);
				//add check here
				if(((*(top + k + j - 1)) == '#') && j == player_position){
					PWM_on();
					set_PWM(523.25);
					LCD_ClearScreen();
					k = l = 0;
					LCD_DisplayString(1,"Oops! Score:");
					LCD_Cursor(13);
					LCD_WriteData(score + '0');
					score = 1;
					game_over = 1;
					state = SM_start2;
					break;
				}
				LCD_WriteData(*(top + k + j - 1));
			}
			if ( k > (textSize - 16)) {
				k = 0;
			}
			else{
				k++;
				score++;
			}
			
			//bot
			for (m = 17; m <= 32; m++) {
				LCD_Cursor(m);
				//add check here
				if(((*(bot + l + (m - 16) - 1)) == '#') && m == player_position){
					//lose game
					PWM_on();
					set_PWM(523.25);
					LCD_ClearScreen();
					k = l = 0;
					LCD_DisplayString(1,"Game Over!      Your Score:");
					LCD_Cursor(28);
					LCD_WriteData(score + '0');
					score = 1;
					game_over = 1;
					if(GetBit(tmpA, 2)){
						return 0;
						break;
					}
					else{
						state = SM_start2;
						}
					//state = display;
					break;
				}
				LCD_WriteData(*(bot + l + (m - 16) - 1));
			}
			if ( l > (textSize - 16)) {
				l = 0;
			}
			else {
				l++;
				score++;
			}
		}
		break;
		
		default: break;
	}
	return state;
}

int main(void)
{
	DDRA = 0x03; PORTA = 0xFC; //buttons
	DDRB = 0xFF; PORTB = 0x00; //buzzer output
	DDRC = 0xFF; PORTC = 0x00; //lcd data
	DDRD = 0xFF; PORTD = 0x00; //lcd control
	//var initialization
	game_pause = game_over = 0;
	player_position = 2; //2 so that cursor moves up and down
	unsigned long int player_calc = 10;
	unsigned long int writeLCD_calc = 300;
	
	unsigned long int tempGCD = 1;
	tempGCD = findGCD(player_calc, writeLCD_calc);
	
	unsigned long int GCD;
	GCD = tempGCD;
	
	unsigned long int player_period = player_calc / GCD;
	unsigned long int writeLCD_period = writeLCD_calc / GCD;
	
	static task player, writeLCD;
	task *tasks[] = { &player, &writeLCD};
	const unsigned short numTasks = sizeof(tasks)/sizeof(task*);
	
	//PWM_on();
	
	player.state = SM_start1;
	player.period = player_period;
	player.elapsedTime = player_period;
	player.TaskFct = &SM_Player;
	
	writeLCD.state = SM_start2;
	writeLCD.period = writeLCD_period;
	writeLCD.elapsedTime = writeLCD_period;
	writeLCD.TaskFct = &SM_LCD;
	
	TimerSet(GCD);
	TimerOn();
	LCD_init();

	unsigned short i = 0;
	while(1)
	{
		tmpA = ~PINA;
		for (i = 0; i < numTasks; i++) {
			if (tasks[i]->elapsedTime == tasks[i]->period){
				tasks[i]->state = tasks[i]->TaskFct(tasks[i]->state);
				tasks[i]->elapsedTime = 0;
			}
			tasks[i]->elapsedTime += 1; //Adjusts game speed. Cursor cannot be seen if any faster.
		}
		while(!TimerFlag);
		TimerFlag = 0;
	}
	return 0;
}
