#include <gpio.h>
#include <stm32.h>
#include <stdbool.h>
#include "keyboard.h"

#define SCAN_NOP_COUNT 10

// Populated by ScanKeyboard.
bool any_pressed;
bool two_pressed;
int row_pressed;
int col_pressed;

bool current_press_recorded = false;

// Software counter for fixing the number.
int tick_count = 0;
int ticks_to_fix_button = 100;  // 1s

// Is the counter on?
int counter_mode = 0;  // 1 means wait for fix

void ButtonClick(void);
void ScanKeyboard(void);

void KeyboardConfigure(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;


    // Ustaw stan niski na wyprowadzeniach kolumn.
    for (int i = 0; i < 4; ++i) {
        GPIOC->BSRRH = 1 << i;
    }
    // Skonfiguruj wyprowadzenia kolumn jako wyjscia przeciwsobne
    // bez rezystorow podciagajacych.
    for (int i = 0; i < 4; ++i) {
        GPIOoutConfigure(GPIOC,
            i,
            GPIO_OType_PP,
            GPIO_Low_Speed,
            GPIO_PuPd_NOPULL);
    }

    // Skonfiguruj wyprowadzenia wierszy jako wejscia
    // z rezystorami podciagajacymi, zglaszajace przerwanie przy
    // zboczu opadajacym.
    for (int i = 0; i < 4; ++i) {
        int pin = i + 6;
        GPIOinConfigure(GPIOC,
                pin,
                GPIO_PuPd_UP,
                EXTI_Mode_Interrupt,
                EXTI_Trigger_Falling);
    }

    // Wyzeruj znaczniki przerwan wierszy - przez wpisanie jedynek 
    // do odpowiedniego rejestru.
    for (int i = 0; i < 4; ++i) {
        int pin = i + 6;
        EXTI->PR = 1 << pin;
    }

    // First column.
    NVIC_SetPriority(EXTI9_5_IRQn, 0);
    NVIC_EnableIRQ(EXTI9_5_IRQn);

    // Set up counter for keyboard
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
    TIM3->CR1 = 0;
    TIM3->ARR = 10;
    TIM3->PSC = 15999;   // 1ms
    TIM3->EGR = TIM_EGR_UG;
    TIM3->DIER = TIM_DIER_UIE;
    TIM3->SR = ~TIM_SR_UIF;
    NVIC_EnableIRQ(TIM3_IRQn);
}

void HandleTick(void) {
    if (counter_mode == 1) {
        tick_count++;
        if (tick_count >= ticks_to_fix_button) {
            FixButton();
            counter_mode = 0;
            tick_count = 0;
        }
    }

    ButtonClick();
}

void TIM3_IRQHandler(void) {
    uint32_t it_status = TIM3->SR & TIM3->DIER;
    if (it_status & TIM_SR_UIF) {
        // wyzeruj znacznik przerwania TIMx
        TIM3->SR = ~TIM_SR_UIF;

        HandleTick();
    }
}


void ButtonClick(void) {
    // skanuj stan klawiatury:
    ScanKeyboard();

    if (!any_pressed) {
        current_press_recorded = false;

        // 1. zatrzymaj licznik.
        // TIM3->CR1 = 0;  // nope
        if (counter_mode == 0) {
            counter_mode = 1;
            tick_count = 0;
        }

        // 2. ustaw stan niski na liniach kolumn.
        for (int pin = 0; pin < 4; ++pin) {
            GPIOC->BSRRH = 1 << pin;
        }

        // 3. wyzeruj znaczniki przerwan wierszy.
        EXTI->PR = 0xf << 6;

        // 4. aktywuj przerwania wierszy w ukladzie exti.
        EXTI->IMR |= 0xf << 6;
        
    }

    else {
        if (current_press_recorded) return;

        // start over the counter
        tick_count = 0;

        current_press_recorded = true;

        if (two_pressed) {
            AmbiguousPress();
        } else if(any_pressed) {
            ButtonPressed(row_pressed, col_pressed);
        }
    }
}

// Row interrupt.
void EXTI9_5_IRQHandler(void) {

    // Dezaktywuj przerwania wierszy w ukladzie EXTI.
    EXTI->IMR &= ~(0xf << 6);
    
    // Wyzeruj znaczniki przerwan wierszy.
    EXTI->PR = 0xf << 6;
    
    // Ustaw stan wysoki na liniach kolumn.
    for (int pin = 0; pin < 4; ++pin) {
        GPIOC->BSRRL = 1 << pin;
    }

    // Wyzeruj rejestr CNT.
    TIM3->CNT = 0;

    // Uruchom licznik TIMx.
    TIM3->CR1 |= TIM_CR1_CEN;
}

void ScanKeyboard() {
    // Sets global vars.
    any_pressed = false;
    two_pressed = false;
    row_pressed = -1;
    col_pressed = -1;

    for (int col = 0; col < 4; ++col) {
        // ustaw stan niski na wyprowadzeniu tej kolumny
        GPIOC->BSRRH = 1 << col;

        // odczekaj pare taktow zegara, aby ustalic wynik
        for (int i = 0; i < SCAN_NOP_COUNT; ++i) {
            __NOP();
        }
        
        // I wczytaj stan wierszy
        int idr = GPIOC->IDR;

        // I ustaw stan wysoki na wyprowadzeniu tej kolumny
        GPIOC->BSRRL = 1 << col;

        // decyduj, który klawisz w tej kolumnie należy uznać za
        // wciśnięty
        
        for (int row = 0; row < 4; ++row) {
            int row_pin = row + 6;
            if (!(idr & (1 << row_pin))) {
                if (any_pressed) {
                    two_pressed = true;
                } else {
                    any_pressed = true;
                    row_pressed = row;
                    col_pressed = col;
                }
            }
        }
    }
}

