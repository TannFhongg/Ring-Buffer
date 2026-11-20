#include <stm32f411xe.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef enum
{
    ST_OFF = 0,
    ST_ON,
    ST_BLINK,
    ST_COUNT
} state_t;

typedef enum
{
    EV_NONE = 0,
    EV_BUTTON,
    EV_UART,
    EV_TIMEOUT
} event_t;

typedef state_t (*state_fn_t)(event_t event);

typedef void (*uart_rx_cb_t)(uint8_t byte);

// ---------------------------------------
#define EVENT_QUEUE_SIZE 8
static volatile event_t s_event_queue[EVENT_QUEUE_SIZE];
static volatile uint8_t s_write_in = 0U;
static volatile uint8_t s_read_out = 0U;

static inline void event_queue_post(event_t event)
{
    uint8_t next = (s_write_in + 1) & (EVENT_QUEUE_SIZE - 1U);
    if (next != s_read_out)
    {
        s_event_queue[s_write_in] = event;
        s_write_in = next;
    }
}

static inline bool event_queue_get(event_t *event)
{
    if (s_write_in == s_read_out)
    {
        return false;
    }
    *event = s_event_queue[s_read_out];
    s_read_out = (s_read_out + 1) & (EVENT_QUEUE_SIZE - 1U);
    return true;
}

// -------------------------------
static volatile uint32_t g_ticks = 0U;
static uart_rx_cb_t g_uart_rx_cb = NULL;

void SysTick_Handler(void)
{
    g_ticks++;
}

void UART1_SendString(const char *str)
{
    if (str == NULL)
        return;
    while (*str)
    {
        while (!(USART1->SR & USART_SR_TXE))
            ; // cho bo dem trong

        USART1->DR = (uint8_t)(*str++);
    }
}

#define LED_13 13
#define PA9 9
#define PA10 10
void LED_Init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;

    GPIOC->MODER &= ~(3U << (LED_13 * 2));
    GPIOC->MODER |= (1U << (LED_13 * 2));

    GPIOC->BSRR = (1U << LED_13);
}

void UART1_Init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;

    GPIOA->MODER &= ~((3U << (PA9 * 2) | (3U << (PA10 * 2))));
    GPIOA->MODER |= (2 << (PA9 * 2)) | (2 << (PA10 * 2));

    GPIOA->AFR[1] &= ~((0xFU << ((PA9 - 8) * 4)) | (0xFU << ((PA10 - 8) * 4)));
    GPIOA->AFR[1] |= (7U << ((PA9 - 8) * 4) | (7U << ((PA10 - 8) * 4)));

    USART1->BRR = (8 << 4) | 11;

    USART1->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE; // thanh ghi dieu khien
    // USART1 -> CR1 = (1U << 13) | (1U << 3) | (1U << 2) | (1U << 5);

    NVIC->ISER[1] = (1 << 5); // pos 5 (37 mod 32)
    // NVIC_EnableIRQ(USART1_IRQn);

    NVIC->IP[37] = (2 << 4); // dat muc uu tien la 2
                             // NVIC_SetPriority(USART1_IRQn,2);
}

void SYSCFG_Init()

{
    RCC->APB2ENR |= (1 << 14);        // RCC_APB2ENR_SYSCFGEN;
    SYSCFG->EXTICR[0] &= ~(0xF << 0); // SYSCFG->EXTICR[0] = SYSCFG_EXTICR1

    EXTI->IMR |= (1 << 0);

    EXTI->RTSR &= ~(1 << 0);
    EXTI->FTSR |= (1 << 0);

    NVIC->ISER[0] = (1 << 6);
    NVIC->IP[6] = (3 << 4);
}
void UserButton_Init()
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    GPIOA->MODER &= ~(3 << (0 * 2));

    GPIOA->PUPDR &= ~(3 << (0 * 2));
    GPIOA->PUPDR |= (1 << (0 * 2));
}
void uart_register_rx_cb(uart_rx_cb_t cb)
{
    g_uart_rx_cb = cb;
}

void USART1_IRQHandler(void)
{
    if (USART1->SR & USART_SR_RXNE)
    {
        uint8_t rx_data = (uint8_t)(USART1->DR & 0xFF); // doc xong xoa co
        if (g_uart_rx_cb != NULL)
        {
            g_uart_rx_cb(rx_data);
        }
    }
}

void EXTI0_IRQHandler(void)
{
    if (EXTI->PR & (1 << 0))
    {
        EXTI->PR = (1 << 0);

        static uint32_t last_press_time = 0;
        if (g_ticks - last_press_time > 200)
        {
            last_press_time = g_ticks;

            event_queue_post(EV_BUTTON);
        }
    }
}
// ham call back thuc te
void App_UART_RX_cb(uint8_t byte)
{
    if (byte == 'b' || byte == 'B')
    {
        event_queue_post(EV_BUTTON);
    }
    else
    {
        event_queue_post(EV_UART);
    }
}

// -------------------------------------------------------------

state_t state_off_handler(event_t event)
{
    GPIOC->BSRR = (1U << 13);

    if (event == EV_BUTTON || event == EV_TIMEOUT)
    {
        UART1_SendString("\r\n[State Transition]: OFF -> BLINK\r\n");
        return ST_BLINK;
    }

    return ST_OFF;
}

state_t state_blink_handler(event_t event)
{
    if ((g_ticks / 200) % 2 == 0)
    {
        GPIOC->BSRR = (1U << (16 + LED_13));
    }
    else
    {
        GPIOC->BSRR = (1U << LED_13);
    }

    if (event == EV_BUTTON || event == EV_TIMEOUT)
    {
        UART1_SendString("\r\n[State Transition:] BLINK -> ON\r\n");
        return ST_ON;
    }
    return ST_BLINK;
}

state_t state_on_handler(event_t event)
{
    GPIOC->BSRR = (1U << (LED_13 + 16));

    if (event == EV_BUTTON || event == EV_TIMEOUT)
    {
        UART1_SendString("\r\n[State Transition]:ON -> OFF\r\n");
        return ST_OFF;
    }
    return ST_ON;
}

// ------------------------------------------

static const state_fn_t state_table[ST_COUNT] = {
    [ST_OFF] = state_off_handler,
    [ST_ON] = state_on_handler,
    [ST_BLINK] = state_blink_handler};

static state_t current_state = ST_OFF;

void fsm_dispatch(event_t event)
{
    if (current_state >= ST_COUNT)
    {
        current_state = ST_OFF;
    }

    state_fn_t handler = state_table[current_state];

    if (handler != NULL)
    {
        state_t next_state = handler(event);

        if (next_state < ST_COUNT)
        {
            current_state = next_state;
        }
        else
        {
            current_state = ST_OFF;
        }
    }
    else
    {
        current_state = ST_OFF;
    }
}

int main(void)
{
    SysTick_Config(16000000 / 1000U); // Systick 1ms

    LED_Init();
    UART1_Init();
    UserButton_Init();
    SYSCFG_Init();
    uart_register_rx_cb(App_UART_RX_cb);

    UART1_SendString("\r\n=== FSM LED DEMO READY ===\r\n");
    UART1_SendString("Nhan 'b' de chuyen State hoac doi 3 giay (Timeout)\r\n");

    uint32_t last_state_change_time = g_ticks;
    state_t last_record_state = current_state;

    while (1)
    {
        event_t event = EV_NONE;

        if (g_ticks - last_state_change_time >= 3000)
        {
            event = EV_TIMEOUT;
            last_state_change_time = g_ticks;
        }

        if (event_queue_get(&event))
        {
            last_state_change_time = g_ticks;
        }

        fsm_dispatch(event);

        if (current_state != last_record_state)
        {
            last_record_state = current_state;
            last_state_change_time = g_ticks;
        }
    }
}
