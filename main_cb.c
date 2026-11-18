#include <stm32f411xe.h>
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>

// static volatile uint32_t ms_ticks = 0;
//
// void SysTick_Handler(void)
//{
//	ms_ticks++;
// }
//
// void delay_ms(uint32_t ms)
//{
//	uint32_t start_time = ms_ticks;
//	while((ms_ticks - start_time) < ms)
//	{
//
//	}
// }

typedef enum
{
    ST_OFF = 0,
    ST_ON,
    ST_BLINK,
    ST_COUNT // luon dat o cuoi de biet tong so trang thai bang 3
} state_t;

typedef enum
{
    EV_NONE = 0,
    EV_BUTTON,
    EV_TIMEOUT,
    EV_UART,
    EV_COUNT // biet duoc tong so trang thai bang 4
} event_t;

// ta dinh nghia trang thai an toan cho he thong la toan bo tat den
#define SAFE_STATE ST_OFF

typedef state_t (*state_fn_t)(event_t evnet);

state_t state_off_handler(event_t event);
state_t state_on_handler(event_t event);
state_t state_blink_handler(event_t event);

static const state_fn_t table[ST_COUNT] = {
    [ST_BLINK] = state_blink_handler,
    [ST_OFF] = state_off_handler,
    [ST_ON] = state_on_handler};

state_t state_off_handler(event_t event)
{
    switch (event)
    {
    case EV_BUTTON: // gap su kien nut nhan
        return ST_ON;
    default: // khong co su kien gi
        return ST_OFF;
    }
}

state_t state_off_handler_2(event_t event)
{
    if (event == EV_BUTTON)
    {
        led_set_state(ST_BLINK);
        return ST_BLINK;
    }
    return ST_OFF;
}

state_t state_blink_handler(event_t event)
{
    switch (event)
    {
    case EV_BUTTON:
        return ST_ON;
    case EV_TIMEOUT:
        return ST_BLINK;
    default:
        return ST_BLINK;
    }
}

state_t state_blink_handler_2(event_t event)
{
    if (event == EV_BUTTON)
    {
        led_set_state(ST_ON);
        return ST_ON;
    }
    else if (event == EV_TIMEOUT)
    {
        led_set_state(ST_BLINK);
        return ST_BLINK;
    }
    return ST_BLINK;
}

state_t state_on_handler(event_t event)
{
    switch (event)
    {
    case EV_BUTTON:
        return ST_OFF;
    default:
        return ST_ON;
    }
}

state_t state_off_handler_2(event_t event)
{
    if (event == EV_BUTTON)
    {
        led_set_state(ST_OFF);
        return ST_OFF;
    }
    return ST_ON;
}
// Luu su kien hien tai
static volatile state_t current_state = ST_OFF;

void fsm_dispatch(event_t event) // ham dieu phoi trang thai may
{
    if (current_state >= ST_COUNT)
    {
        current_state = SAFE_STATE;
        led_set_state(SAFE_STATE);
        return;
    }
    state_fn_t handler = table[current_state]; // lay ham tuong ung voi trang thai hien tai va thuc thi
    if (handler != NULL)
    {
        state_t next_state = handler(event);

        if (next_state < ST_COUNT)
        {
            current_state = next_state;
        }
        else
        {
            current_state = SAFE_STATE;
            led_set_state(SAFE_STATE);
        }
    }
}

// tao ra mot bien kuu su kien dang cho, chia re giua ISR va main , la cau noi giua ngat UART va ham chay chinh
static volatile event_t g_pending_event = EV_NONE;

/**
 * @brief ham callback duoc goi tu dong khi uart nhan duoc 1 byte
 * @param received_byte: Ki tu vua nhan duoc o cong uart
 */

void uart_rx_callback(uint8_t received_byte)
{
    // Kiem tra nhan dung ky tu 'b' hoac 'B'
    if (received_byte == 'b' || received_byte == 'B')
    {
        g_pending_event = EV_BUTTON;
    }
    else
    { // nhan duoc ki tu khac thi coi la su kien thong thuong
        g_pending_event = EV_UART;
    }
}
// ham nay chi ton vai chu ki lenh de thuc hien, dat tieu chuan "Keep it ISR-short"

void led_set_state(state_t state)
{
    switch (state)
    {
    case ST_OFF:
        // tat den
        break;
    case ST_BLINK:
        // blink
        break;
    case ST_ON:
        // bat den
        break;
    default:
        // tat den
        break;
    }
}
int main()
{
    // khoi tao dang ky phan cung
    // dang ky uart_rx_callback

    while (1)
    {
        // 1 kiem tra xem co su kien nao dang cho khong
        if (g_pending_event != EV_NONE)
        {
            event_t event_to_process = g_pending_event; // tam thoi luu bien vao cuc bo

            g_pending_event = EV_NONE; // xoa co su kien va de sang sang cho su kien tiep theo

            fsm_dispatch(event_to_process);
        } // cac cong viec khac
    }
}
