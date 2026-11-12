
#include <stdio.h>
#include <stdint.h>

#include "ring_buffer.h"

#define INT_CAPACITY 8U
#define SENSOR_CAPACITY 4U

/* Custom data type */
typedef struct
{
    float temperature;
    float humidity;
    uint32_t timestamp;

} SensorData_t;

/* ============================================
 * Test 1: Dynamic Generic Ring Buffer
 * ============================================ */

static void Test_DynamicRingBuffer(void)
{
    printf("\n========== DYNAMIC RING BUFFER ==========\n");

    /* Zero initialize Ring Buffer object */
    RingBuffer_t rb = {0};

    /* Initialize with dynamic allocation */
    if (RingBuffer_InitDynamic(
            &rb,
            INT_CAPACITY,
            sizeof(int)) != RB_OK)
    {
        printf("Dynamic initialization failed!\n");
        return;
    }

    printf("Capacity: %zu\n", RingBuffer_Capacity(&rb));

    /* Push data */
    for (int i = 1; i <= 8; i++)
    {
        if (RingBuffer_Push(&rb, &i) == RB_OK)
        {
            printf("Push: %d\n", i);
        }
    }

    printf("Is Full: %s\n",
           RingBuffer_IsFull(&rb) ? "YES" : "NO");

    /* Try pushing when full */
    int extra = 100;

    if (RingBuffer_Push(&rb, &extra) == RB_ERROR_FULL)
    {
        printf("Buffer is full!\n");
    }

    /* Pop 3 elements */
    int value;
    printf("\nPop 3 elements:\n");

    for (int i = 0; i < 3; i++)
    {
        if (RingBuffer_Pop(&rb, &value) == RB_OK)
        {
            printf("Pop: %d\n", value);
        }
    }

    /* Push again to test wrap-around */
    printf("\nPush after Pop:\n");

    for (int i = 9; i <= 11; i++)
    {
        if (RingBuffer_Push(&rb, &i) == RB_OK)
        {
            printf("Push: %d\n", i);
        }
    }


    /* Pop all remaining elements */
    printf("\nRemaining data:\n");

    while (RingBuffer_Pop(&rb, &value) == RB_OK)
    {
        printf("%d ", value);
    }

    printf("\n");

    printf("Is Empty: %s\n",
           RingBuffer_IsEmpty(&rb) ? "YES" : "NO");

    /* Release dynamic memory */
    RingBuffer_Destroy(&rb);
}

/* ============================================
 * Test 2: Static Generic Ring Buffer
 * ============================================ */

static void Test_StaticRingBuffer(void)
{
    printf("\n========== STATIC RING BUFFER ==========\n");

    RingBuffer_t rb = {0};

    /* Static memory allocation */
    static SensorData_t storage[SENSOR_CAPACITY];

    /* Initialize */
    if (RingBuffer_InitStatic(
            &rb,
            storage,
            SENSOR_CAPACITY,
            sizeof(SensorData_t)) != RB_OK)
    {
        printf("Static initialization failed!\n");
        return;
    }

    /* Create sensor data */
    SensorData_t sensor1 = {
        .temperature = 30.5f,
        .humidity = 70.0f,
        .timestamp = 1000U};

    SensorData_t sensor2 = {
        .temperature = 31.2f,
        .humidity = 72.5f,
        .timestamp = 2000U};

    SensorData_t sensor3 = {
        .temperature = 32.0f,
        .humidity = 75.0f,
        .timestamp = 3000U};

    /* Push */
    RingBuffer_Push(&rb, &sensor1);
    RingBuffer_Push(&rb, &sensor2);
    RingBuffer_Push(&rb, &sensor3);

    printf("Current size: %zu\n",
           RingBuffer_Size(&rb));

    /* Pop */
    SensorData_t result;

    while (RingBuffer_Pop(&rb, &result) == RB_OK)
    {
        printf(
            "Temperature: %.2f | "
            "Humidity: %.2f | "
            "Timestamp: %u\n",
            (double)result.temperature,
            (double)result.humidity,
            (unsigned int)result.timestamp);
    }

    /* No malloc was used, so Destroy is not needed
       to release the static storage. */

    RingBuffer_Destroy(&rb);
}

/* ============================================
 * Main
 * ============================================ */

int main(void)
{
    Test_DynamicRingBuffer();

    Test_StaticRingBuffer();

    return 0;
}
