    #include "stm32f411xe.h"
    #include <stdint.h>
    #include <stdbool.h>
    #include <stddef.h>

    /*
    =====================================================================
    ====
     * PHẦN 1: ĐỊNH NGHĨA STATE, EVENT VÀ FUNCTION POINTER
     *
     ====================================================================
     ===== */
    typedef enum {
        ST_OFF = 0,
        ST_BLINK,
        ST_ON,
        ST_COUNT // ST_COUNT = 3: Dùng để xác định kích thước mảng và
        kiểm tra Out of range
    } state_t;

    typedef enum {
        EV_NONE = 0,
        EV_BUTTON,  // Kích hoạt khi nhận ký tự 'b'/'B' từ UART
        EV_UART,    // Kích hoạt khi nhận ký tự khác qua UART
        EV_TIMEOUT  // Kích hoạt khi hết thời gian chờ (sau 3 giây)
    } event_t;

    // Kiểu con trỏ hàm State: Nhận vào event, xử lý và trả về state tiếp
    theo
    typedef state_t (*state_fn_t)(event_t event);

    // Kiểu con trỏ hàm Callback cho UART RX
    typedef void (*uart_rx_callback_t)(uint8_t byte);

    /*
    =====================================================================
    ====
     * PHẦN 2: BỘ ĐỆM HÀNG ĐỢI SỰ KIỆN (EVENT QUEUE) ĐẢM BẢO "ISR-SHORT"
     *
     ====================================================================
     ===== */
    #define EVENT_QUEUE_SIZE 8
    static volatile event_t s_event_queue[EVENT_QUEUE_SIZE];
    static volatile uint8_t s_queue_head = 0;
    static volatile uint8_t s_queue_tail = 0;

    // Đẩy event vào hàng đợi (Được gọi cực nhanh từ ISR)
    static inline void event_queue_post(event_t evt) {
        uint8_t next = (s_queue_head + 1) % EVENT_QUEUE_SIZE;
        if (next != s_queue_tail) { // Tránh tràn hàng đợi
            s_event_queue[s_queue_head] = evt;
            s_queue_head = next;
        }
    }

    // Lấy event ra khỏi hàng đợi (Được gọi trong Main Loop)
    static inline bool event_queue_get(event_t *evt) {
        if (s_queue_head == s_queue_tail) {
            return false; // Hàng đợi rỗng
        }
        *evt = s_event_queue[s_queue_tail];
        s_queue_tail = (s_queue_tail + 1) % EVENT_QUEUE_SIZE;
        return true;
    }

    /*
    =====================================================================
    ====
     * PHẦN 3: ĐIỀU KHIỂN PHẦN CỨNG BARE-METAL (GPIO, UART, SYSTICK)
     *
     ====================================================================
     ===== */
    static volatile uint32_t g_ticks = 0;
    static uart_rx_callback_t g_uart_rx_callback = NULL;

    void SysTick_Handler(void) {
        g_ticks++;
    }

    // Gửi 1 chuỗi string qua UART1
    void UART1_SendString(const char *str) {
        while (*str) {
            while (!(USART1->SR & USART_SR_TXE)); // Chờ bộ đệm truyền
            trống
            USART1->DR = (uint8_t)(*str++);
        }
    }

    // Khởi tạo LED PC13 (Active-Low)
    void LED_Init(void) {
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
        GPIOC->MODER &= ~(3U << (13 * 2));
        GPIOC->MODER |=  (1U << (13 * 2)); // PC13 Output
        GPIOC->BSRR = (1U << 13);          // Ban đầu tắt LED (Active-low
        -> ghi 1)
    }

    // Khởi tạo USART1: PA9 (TX), PA10 (RX) - Baud 115200 (Clock 16MHz)
    void UART1_Init(void) {
        // 1. Cấp clock cho GPIOA và USART1
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
        RCC->APB2ENR |= RCC_APB2ENR_USART1EN;

        // 2. Chân PA9, PA10 sang chế độ Alternate Function (10 nhị phân)
        GPIOA->MODER &= ~((3U << (9 * 2)) | (3U << (10 * 2)));
        GPIOA->MODER |=  ((2U << (9 * 2)) | (2U << (10 * 2)));

        // 3. Chọn AF7 (USART1) trong thanh ghi AFR[1] (cho chân 8 đến
        15)
        GPIOA->AFR[1] &= ~((0xFU << ((9 - 8) * 4)) | (0xFU << ((10 - 8) *
        4)));
        GPIOA->AFR[1] |=  ((7U  << ((9 - 8) * 4)) | (7U  << ((10 - 8) *
        4)));

        // 4. Cài Baudrate 115200: 16MHz / (16 * 115200) = 8.6875 ->
        Mantissa=8, Fraction=11 (0x8B)
        USART1->BRR = (8 << 4) | 11;

        // 5. Bật Transmitter, Receiver và Bật ngắt RXNEIE
        USART1->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE |
        USART_CR1_RXNEIE;

        // 6. Cho phép ngắt USART1 trên NVIC Cortex-M4
        NVIC_SetPriority(USART1_IRQn, 2);
        NVIC_EnableIRQ(USART1_IRQn);
    }

    // Đăng ký Callback cho UART RX (Yêu cầu 3)
    void uart_register_rx_callback(uart_rx_callback_t cb) {
        g_uart_rx_callback = cb;
    }

    // Trình phục vụ ngắt USART1 ISR (Keep it ISR-short)
    void USART1_IRQHandler(void) {
        if (USART1->SR & USART_SR_RXNE) {
            uint8_t rx_data = (uint8_t)(USART1->DR & 0xFF); // Đọc DR tự
            động xóa cờ RXNE
            if (g_uart_rx_callback != NULL) {
                g_uart_rx_callback(rx_data); // Gọi callback đã đăng ký
            }
        }
    }

    // Hàm Callback thực tế: Biến byte 'b' thành EV_BUTTON (Cực ngắn -
    tốn ~3 chu kỳ CPU)
    void App_UART_RxCallback(uint8_t byte) {
        if (byte == 'b' || byte == 'B') {
            event_queue_post(EV_BUTTON); // Ký tự 'b' -> Sự kiện Button
        } else {
            event_queue_post(EV_UART);   // Các ký tự khác -> Sự kiện
            UART
        }
    }

    /*
    =====================================================================
    ====
     * PHẦN 4: KHAI BÁO CÁC HÀM XỬ LÝ TRẠNG THÁI (STATE HANDLERS)
     * Vòng lặp trạng thái: OFF -> BLINK -> ON -> OFF
     *
     ====================================================================
     ===== */

    // Trạng thái 1: TẮT LED (ST_OFF)
    state_t state_off_handler(event_t event) {
        // LED TẮT (Ghi 1 vào PC13)
        GPIOC->BSRR = (1U << 13);

        if (event == EV_BUTTON || event == EV_TIMEOUT) {
            UART1_SendString("\r\n[State Transition]: OFF -> BLINK\r\n");
            return ST_BLINK;
        }
        return ST_OFF; // Giữ nguyên trạng thái nếu không có sự kiện
        chuyển đổi
    }

    // Trạng thái 2: NHẤP NHÁY LED (ST_BLINK)
    state_t state_blink_handler(event_t event) {
        // Nhấp nháy LED theo nhịp 200ms
        if ((g_ticks / 200) % 2 == 0) {
            GPIOC->BSRR = (1U << (13 + 16)); // BẬT LED
        } else {
            GPIOC->BSRR = (1U << 13);        // TẮT LED
        }

        if (event == EV_BUTTON || event == EV_TIMEOUT) {
            UART1_SendString("\r\n[State Transition]: BLINK -> ON\r\n");
            return ST_ON;
        }
        return ST_BLINK;
    }

    // Trạng thái 3: BẬT LED (ST_ON)
    state_t state_on_handler(event_t event) {
        // LED BẬT (Ghi 0 vào PC13)
        GPIOC->BSRR = (1U << (13 + 16));

        if (event == EV_BUTTON || event == EV_TIMEOUT) {
            UART1_SendString("\r\n[State Transition]: ON -> OFF\r\n");
            return ST_OFF;
        }
        return ST_ON;
    }

    /*
    =====================================================================
    ====
     * PHẦN 5: BẢNG STATE MACHINE & CƠ CHẾ AN TOÀN DEFENSIVE PROGRAMMING
     *
     ====================================================================
     ===== */

    // Yêu cầu 2: static const state_fn_t table[ST_COUNT] with designated
    initialization
    static const state_fn_t state_table[ST_COUNT] = {
        [ST_OFF]   = state_off_handler,
        [ST_BLINK] = state_blink_handler,
        [ST_ON]    = state_on_handler,
    };

    static state_t s_current_state = ST_OFF;

    // Yêu cầu 4: Out of range -> safe state, never call a NULL
    void fsm_dispatch(event_t event) {
        // 1. Kiểm tra tràn mảng: Nếu state bị lỗi vượt ngưỡng -> ép về
        safe state (ST_OFF)
        if (s_current_state >= ST_COUNT) {
            s_current_state = ST_OFF;
        }

        // 2. Lấy con trỏ hàm xử lý từ bảng
        state_fn_t handler = state_table[s_current_state];

        // 3. Kiểm tra con trỏ NULL: Tuyệt đối không bao giờ gọi nếu con
        trỏ là NULL!
        if (handler != NULL) {
            s_current_state = handler(event);
        } else {
            // Fallback an toàn nếu có lỗi
            s_current_state = ST_OFF;
        }
    }

    /*
    =====================================================================
    ====
     * PHẦN 6: HÀM MAIN VÀ BỘ TẠO SỰ KIỆN TIMEOUT
     *
     ====================================================================
     ===== */
    int main(void) {
        // Khởi tạo phần cứng
        SysTick_Config(16000000 / 1000); // SysTick 1ms
        LED_Init();
        UART1_Init();

        // Đăng ký callback cho UART
        uart_register_rx_callback(App_UART_RxCallback);

        UART1_SendString("\r\n=== FSM LED DEMO READY ===\r\n");
        UART1_SendString("Nhan 'b' de chuyen State hoac doi 3 giay
        (Timeout)\r\n");

        uint32_t last_state_change_time = g_ticks;
        state_t last_recorded_state = s_current_state;

        while (1) {
            event_t evt = EV_NONE;

            // 1. Kiểm tra sự kiện TIMEOUT: Nếu ở 1 state quá 3000ms (3
            giây)
            if (g_ticks - last_state_change_time >= 3000) {
                evt = EV_TIMEOUT;
                last_state_change_time = g_ticks;
            }

            // 2. Lấy sự kiện từ hàng đợi (Ưu tiên sự kiện do UART bấm
            'b' sinh ra)
            if (event_queue_get(&evt)) {
                last_state_change_time = g_ticks; // Reset timer khi có
                người bấm
            }

            // 3. Thực hiện State Machine
            fsm_dispatch(evt);

            // Nếu trạng thái vừa đổi, reset lại mốc thời gian timeout
            if (s_current_state != last_recorded_state) {
                last_recorded_state = s_current_state;
                last_state_change_time = g_ticks;
            }
        }
    }