/**
*****************************************************************************
**
**  File        : main.c
**
**  Abstract    : main function.
**
**  Functions   : main
**
**  Environment : Atollic TrueSTUDIO/STM32
**                STMicroelectronics STM32Lxx Standard Peripherals Library
**
**  Distribution: The file is distributed “as is,” without any warranty
**                of any kind.
**
**  (c)Copyright Atollic AB.
**  You may use this file as-is or modify it according to the needs of your
**  project. This file may only be built (assembled or compiled and linked)
**  using the Atollic TrueSTUDIO(R) product. The use of this file together
**  with other tools than Atollic TrueSTUDIO(R) is not permitted.
**
*****************************************************************************
*/

/* Includes */
#include <stddef.h>
#include "stm32l1xx.h"
#include <math.h>
#include "spi.h"
#include "ssd1306.h"
#include "ili9163.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "adc.h"

#define SIZE_OF_WORLD 13
#define NUMBER_OF_ELEMENTS SIZE_OF_WORLD * SIZE_OF_WORLD
#define SIZE_OF_SNAKE 10
#define DM 10 //DISPLAY MULTIPLIER

/*
Struktura Snake reprezentuje jednu cast hada, finalny had je pole tychto struktur.
Ci je dany prvok pola uz realny had alebo je to este nenaplnena struktura, ktora
sa naplni az ked had zozerie jedlo, rozhoduje premenna isUsed. Teda, ak ma had uz
tri gulicky, prve tri struktury maju isUsed 1
*/

typedef struct{
    int x;
    int y;
    int isUsed;
}Snake;


char get_adc_char(){
	if (adc_value >1800 && adc_value < 2200){
		return 'w';
	}
	else if(adc_value >850 && adc_value < 1200){
		return 'd';
	}
	else if(adc_value >50 && adc_value < 400){
		return 'a';
	}
	else if(adc_value >2400 && adc_value < 2800){
		return 's';
	}
}
void initWorld(char map[SIZE_OF_WORLD][SIZE_OF_WORLD]) {
/*
Funkcia initWorld memsetne celu maticu na medzery, nasledne ju preiteruje a nastavi
okraje matice na X, teda spravime si ohradu
*/
    int row, col;

    memset(map, ' ',NUMBER_OF_ELEMENTS);

    for (row=0; row < SIZE_OF_WORLD; row++) {
        for(col = 0; col < SIZE_OF_WORLD; col++) {
            if(col == 0 || col == SIZE_OF_WORLD-1) {
                map[row][col] = 'X';
            }
            if(row == 0 || row == SIZE_OF_WORLD-1){
                map[row][col] = 'X';
            }
        }
    }
}

void printWorld(char map[SIZE_OF_WORLD][SIZE_OF_WORLD]) {
/*
Vyprinti aktualny stav matice
*/

    int row, col;

    for (row=0; row < SIZE_OF_WORLD; row++) {
        for(col = 0; col < SIZE_OF_WORLD; col++) {
             //printf("%c", map[row][col]);
        	lcdPutCh(map[row][col], col*DM, row*DM, 31, 0);
        }

        //printf("\n");
    }

}


void spawnFood(char map[SIZE_OF_WORLD][SIZE_OF_WORLD]) {
/*
Vygeneruje nahodnu poziciu v ramci matice a da tam jedlo. Funkcia generuje jedlo,
pokial sa nespawne do neobsadenej pozicie (kontroluje,ci sa nespawne do hada alebo steny)
*/
    //srand(time(NULL));
    int x,y;

    do {
        x = rand() % SIZE_OF_WORLD;
        y = rand() % SIZE_OF_WORLD;
    } while (map[x][y] != ' ');

    map[x][y] = 'H';
}

void initSnake(char map[SIZE_OF_WORLD][SIZE_OF_WORLD], Snake snake[SIZE_OF_SNAKE]) {
/*
Funkcia vytvori "hlavu" hada, vsetky ostatne struktury v poli nastavi na nula. Hlavu hada
umiestni do stredu mapy
*/
    int i;

    snake[0].isUsed = 1;
    snake[0].x = (int)floor(SIZE_OF_WORLD/2);
    snake[0].y = (int)floor(SIZE_OF_WORLD/2);
    for(i = 1; i < SIZE_OF_SNAKE; i++) {
        snake[i].isUsed = 0;
    }

    map[snake[0].x][snake[0].y] = 'O';
}

void moveSnake(char map[SIZE_OF_WORLD][SIZE_OF_WORLD], Snake snake[SIZE_OF_SNAKE]) {
/*
Funkcia, ktora hybe hadom, pokial sa hybe v ramci cistej matice (ak pred nim nie je
akakolvek prekazka ci jedlo). Preiterujem sa od hlavy az po chvost a kazdej casti hada
nastavim suradnice tej predchadzajucej,teda kazda cast sa "pohne" tam, kde bola predosla
*/
    int i;
    Snake oldSnake[SIZE_OF_SNAKE];

    for(i = 0; i < SIZE_OF_SNAKE; i++) {
        oldSnake[i] = snake[i];
    }

    for(i = 1; i < SIZE_OF_SNAKE; i++) {
        if(snake[i].isUsed == 1) {
            snake[i] = oldSnake[i-1];
        }
    }
}

void growthSnake(char map[SIZE_OF_WORLD][SIZE_OF_WORLD], Snake snake[SIZE_OF_SNAKE], int* score) {
/*
Funkcia, ktorou sa zvacsuje had. Funguje podobne ako moveSnake, len nepohnem chvostom, len hlavou.
Teda, akoby som pridal jednu gulicku dopredu, na poziciu jedla.
*/
    int i;
    Snake oldSnake[SIZE_OF_SNAKE];

    for(i = 0; i < SIZE_OF_SNAKE; i++) {
        oldSnake[i] = snake[i];
    }

    for(i = 1; i < *score + 1; i++) {
            snake[i] = oldSnake[i-1];
    }

}

void clearMapFromSnake(char map[SIZE_OF_WORLD][SIZE_OF_WORLD]) {
/*
Funkcia potrebna pri pohybe hada, najskor z matice hada odstranime, potom preratame,kam sa
ma had posunut a nasledne ho tam pridame. V tejto funkcii ho len odstranime
*/
    int row, col;

    for (row=0; row < SIZE_OF_WORLD; row++) {
        for(col = 0; col < SIZE_OF_WORLD; col++) {
            if(map[row][col] == 'O') {
             map[row][col] = ' ';
            }
        }
    }
}

void addSnakeToMap(char map[SIZE_OF_WORLD][SIZE_OF_WORLD], Snake snake[SIZE_OF_SNAKE]) {
/*
Po vyratani novych suradnic pridame hada tam, kde ma byt
*/
    int i;

    for (i = 0; i < SIZE_OF_SNAKE; i++) {
        if(snake[i].isUsed == 1) {
            map[snake[i].x][snake[i].y] = 'O';
        }
    }
}

int collision(char map[SIZE_OF_WORLD][SIZE_OF_WORLD], Snake snake[SIZE_OF_SNAKE],int x, int y, int* score) {
/*
Funkcia deteguje koliziu. Podmienky su zoradene tak, ako predpokladame ich pravdepodobnostny vyskyt a teda
zjednodusujeme zlozistost. Funkcia vracia taku hodnotu,aka udalost nastala, teda 1 ak je mapa volna, 2 ak
sme nieco zjedli a -1 ak sme narazili. Je moznost doprogramovat narazenie sameho do seba, momentalne je to
vyriesene tak, ze do seba proste nemozem ist
*/
    if(map[x][y] == ' ') {
        return 1;
    }
    if(map[x][y] == 'H') {
        return 2;
    }
    if(map[x][y] == 'X' ) {
        return -1;
    }
    /*
    TODO kolizia s hadom..
    if (map[x] == snake[score].x && map[y] == snake[score].y) {
        return -2;
    }
    */
}

int move(char direction, char map[SIZE_OF_WORLD][SIZE_OF_WORLD], Snake snake[SIZE_OF_SNAKE], int* score) {
    int decision = 0;

    switch(direction) {
        case 'a':    // key up
            decision = collision(map, snake, snake[0].x, snake[0].y - 1, score);
            if (decision < 0) {
                return 1;
            } else if (decision == 1) {
                moveSnake(map,snake);
                snake[0].y--;
            } else if (decision == 2) {
                (*score)++;
                growthSnake(map, snake, score);
                snake[0].y--;
                spawnFood(map);
            }
            break;
        case 'd':    // key down
            decision = collision(map, snake, snake[0].x, snake[0].y + 1, score);
            if (decision < 0) {
                return 1;
            } else if (decision == 1) {
                moveSnake(map,snake);
                snake[0].y++;
            } else if (decision == 2){
                (*score)++;
                growthSnake(map, snake, score);
                snake[0].y++;
                spawnFood(map);
            }
            break;
        case 's':    // key right
            decision = collision(map, snake, snake[0].x + 1, snake[0].y, score);
            if (decision < 0) {
                return 1;
            } else if (decision == 1) {
                moveSnake(map,snake);
                snake[0].x++;
            } else if (decision == 2){
                (*score)++;
                growthSnake(map, snake, score);
                snake[0].x++;
                spawnFood(map);
            }
            break;
        case 'w':    // key left
            decision = collision(map, snake, snake[0].x - 1, snake[0].y, score);
            if (decision < 0) {
                return 1;
            } else if (decision == 1) {
                moveSnake(map,snake);
                snake[0].x--;
            } else if (decision == 2){
                (*score)++;
                growthSnake(map, snake, score);
                snake[0].x--;
                spawnFood(map);
            }
            break;
        case 'o':
            return 1;
            break;
    }

    return 0;
}


void buttoninit(){
	  GPIO_InitTypeDef struktura1;
	 RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOC, ENABLE);
		     struktura1.GPIO_Mode = GPIO_Mode_IN ;
		     struktura1.GPIO_OType = GPIO_OType_PP;
		     struktura1.GPIO_PuPd = GPIO_PuPd_NOPULL;
		     struktura1.GPIO_Pin = GPIO_Pin_13;
		     GPIO_Init(GPIOC, &struktura1);
}


int main(void)
{
  adc_init();
  initSPI2();
  initCD_Pin();
  initCS_Pin();
  initRES_Pin();
  buttoninit();

  lcdInitialise(LCD_ORIENTATION0);
 /* lcdClearDisplay(decodeRgbValue(0, 0, 0));*/

  int button = 0;
  int endFlag = 0;
  int *score = (int*)malloc(sizeof(int));
  *score = 0;
  int isFood = 1;
  char direction = 0;
  char map[SIZE_OF_WORLD][SIZE_OF_WORLD];
  Snake snake[SIZE_OF_SNAKE];

  lcdClearDisplay(decodeRgbValue(0, 0, 0));
  initWorld(map);
  initSnake(map,snake);
  spawnFood(map);
  map[3][6] = 'H';

  printWorld(map);




  do{
         if(!isFood) {
             spawnFood(map);
             isFood = 1;
         }


         direction = get_adc_char();

         button = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_13);

         /*if(!button)
        	 direction = 'w';*/


         endFlag = move(direction, map, snake, score);

         //system("@cls||clear");
         clearMapFromSnake(map);
         addSnakeToMap(map,snake);
         printWorld(map);

         //while ( getchar() != '\n' );
         //fflush(stdin);
         direction = 0;


         if(*score == SIZE_OF_SNAKE -1) {
             printf("VYHRAL SI!!!\n");
             return 0;
         }

  	     Delay(3000);

     }while(endFlag != 1);

     printf("Koniec hry. Tvoje score je : %d\n", *score);


/*
  while (1){
	   Delay(3000);
  }
  */
  return 0;
}

