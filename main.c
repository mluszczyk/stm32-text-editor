#include <gpio.h>
#include <stm32.h>
#include "lcd.h"
#include <stdbool.h>

#define SCAN_NOP_COUNT 10

bool any_pressed;
bool two_pressed;
int row_pressed;
int col_pressed;

bool roundabout_state = false;

char* layout[4][4] = {
    {"1", "abc2", "def3", ""},     
    {"ghi4", "jkl5", "mno6", ""},
    {"prs7", "tuv8", "wxy9", ""},
    {"*", " 0", "#", ""},
};

struct {
    int row;
    int col;
} clear_button = {2, 3};

struct {
    int row;
    int col;
} backspace_button = {1, 3};

struct {
    int row;
    int col;
} current_roundabout_button = {-1, -1};

int current_roundabout_position = 0;

int tick_count = 0;
int ticks_to_fix_button = 100;  // 1s

int counter_mode = 0;  // 1 means wait for fix

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
        // wciśnięty i TODO: czy jest to przytrzymanie
        
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

        // I umieść informację o wciśniętym klawiszu w kolejce zdarzeń do
        // obsłużenia
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

bool current_press_recorded = false;

void button_repeat(void) {
    current_roundabout_position++;
    if (!layout[current_roundabout_button.row]
               [current_roundabout_button.col]
               [current_roundabout_position]) {
        current_roundabout_position = 0;
    }
    char current_char = layout[current_roundabout_button.row]
               [current_roundabout_button.col]
               [current_roundabout_position];
    LCDbackspace();
    LCDputcharWrap(current_char);
}


void pressed(int row, int col) {
    if (clear_button.row == row && clear_button.col == col) {
        LCDclear();
        LCDputcharWrap('_');
    } else if(backspace_button.row == row && backspace_button.col == col) {
        if (current_roundabout_button.row == -1) {
            LCDbackspace();
        }
        LCDbackspace();
        LCDputcharWrap('_');
    } else {
        if (current_roundabout_button.row == row &&
            current_roundabout_button.col == col) {
            button_repeat();
        } else {
            if (current_roundabout_button.row == -1)
                LCDbackspace();  // erase cursor
            current_roundabout_button.row = -1;
            current_roundabout_button.col = -1;  // call fix button?
            current_roundabout_position = 0;
            char* choice = layout[row][col];
            if (*choice && choice[1]) {
                current_roundabout_button.row = row;
                current_roundabout_button.col = col;
                LCDputcharWrap(*choice);
                // cursor will be added at fix

            } else if (*choice) {
                LCDputcharWrap(*choice);
                LCDputcharWrap('_');
            }
        }
    }
}

void button_click(void) {
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
            LCDputcharWrap('A');  // ambiguous
        } else if(any_pressed) {
            pressed(row_pressed, col_pressed);
        }
    }
}

void fix_button(void) {
    if (current_roundabout_button.row != -1) {
        LCDputcharWrap('_');
    }
    current_roundabout_button.row = -1;
    current_roundabout_button.col = -1;
    counter_mode = 0;
    tick_count = 0;
    //TIM3->CR1 = 0; // stop timer.
}

void handle_tick(void) {
    if (counter_mode == 1) {
        tick_count++;
        if (tick_count >= ticks_to_fix_button) {
            fix_button();
        }
    }

    button_click();
}

void TIM3_IRQHandler(void) {
    uint32_t it_status = TIM3->SR & TIM3->DIER;
    if (it_status & TIM_SR_UIF) {
        // wyzeruj znacznik przerwania TIMx
        TIM3->SR = ~TIM_SR_UIF;

        handle_tick();
    }
}

void setUpKeyboard() {
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

int main() {
    LCDconfigure();
    LCDclear();
    LCDputcharWrap('_');

    setUpKeyboard();


    for (int i = 0;; ++i) {
        __NOP();
    }
}
