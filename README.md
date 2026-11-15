# Generic Ring Buffer (Circular Buffer) in C


## 1. Key API Details

### 1.1. Initialization API (`Init`)

The system supports two initialization methods: dynamic memory allocation or using pre-allocated static memory.

#### a. `RingBuffer_InitDynamic`
Initializes the Ring Buffer by automatically allocating memory on the Heap via `malloc`.

```c
RB_Status_t RingBuffer_InitDynamic(RingBuffer_t *rb, size_t capacity, size_t size_element);
```

* **Parameters:**
  * `rb`: Pointer to the `RingBuffer_t` object to be initialized.
  * `capacity`: Maximum number of elements the buffer can hold. **Must be a power of two** (e.g., 4, 8, 16, 32,...).
  * `size_element`: Size in bytes of each element (typically using the `sizeof(...)` operator).
* **Underlying Operations:**
  * Validates input parameters (`rb != NULL`, `capacity` is a power of two, checks against overflow).
  * Allocates a memory block of size `capacity * size_element`.
  * Sets the bitmask `mask = capacity - 1` to optimize circular index wrapping.
  * Resets `head = 0`, `tail = 0`, `size = 0`.
---

#### b. `RingBuffer_InitStatic`
Initializes the Ring Buffer using pre-allocated static buffer memory provided by the user.

```c
RB_Status_t RingBuffer_InitStatic(RingBuffer_t *rb, void *buffer, size_t capacity, size_t size_element);
```

* **Parameters:**
  * `rb`: Pointer to the `RingBuffer_t` object.
  * `buffer`: Pointer to the pre-allocated array / memory region.
  * `capacity`: Maximum number of elements (must be a power of two).
  * `size_element`: Size of each element (`sizeof(...)`).
* **Underlying Operations:**
  * Assigns the memory buffer pointer `buffer` to `rb->buffer`.
  * Sets `mask = capacity - 1` and resets state (`head = 0`, `tail = 0`, `size = 0`).
---

### 2.2. Empty Check API (`IsEmpty`)

```c
bool RingBuffer_IsEmpty(const RingBuffer_t *rb);
```

* **Purpose:** Checks whether the Ring Buffer currently contains any elements.
* **Parameters:**
  * `rb`: Pointer to the Ring Buffer to check (declared as `const` to ensure data immutability).
* **Underlying Operations:**
  * Validation: If `rb` is invalid (`NULL`, uninitialized), the function returns `true` to prevent unauthorized read operations.
  * Compares current element count: `rb->size == 0`.
* **Return Value:**
  * `true`: Buffer is empty (contains no elements).
  * `false`: Buffer contains at least one element.

---

### 2.3. Full Check API (`IsFull`)

```c
bool RingBuffer_IsFull(const RingBuffer_t *rb);
```

* **Purpose:** Checks whether the Ring Buffer has reached its maximum allowed capacity.
* **Parameters:**
  * `rb`: Pointer to the Ring Buffer to check.
* **Underlying Operations:**
  * Validation: If `rb` is invalid, the function returns `false`.
  * Compares current element count against capacity: `rb->size == rb->capacity`.
* **Return Value:**
  * `true`: Buffer is full; cannot push more elements without overwriting or popping first.
  * `false`: Buffer still has available space.

---

### 2.4. Push Element API (`Push`)

```c
RB_Status_t RingBuffer_Push(RingBuffer_t *rb, const void *data);
```

* **Purpose:** Pushes (writes) a data element into the next available position (`tail`) in the buffer.
* **Parameters:**
  * `rb`: Pointer to the Ring Buffer.
  * `data`: Pointer to the memory buffer holding the data to push.
* **Underlying Operations:**
  1. Checks that pointers `rb` and `data` are not `NULL`.
  2. Checks whether the buffer is full (`RingBuffer_IsFull`).
  3. Calculates the destination address:
     $$\text{dst} = (\text{uint8\_t *})\text{rb->buffer} + (\text{rb->tail} \times \text{rb->size\_element})$$
  4. Uses `memcpy` to copy `size_element` bytes from `data` to `dst`.
  5. Advances the write pointer `tail` circularly using the bitmask formula:
     $$\text{tail}_{\text{new}} = (\text{tail} + 1) \ \& \ \text{rb->mask}$$
  6. Increments the element counter: `rb->size++`.

---

### 2.5. Pop Element API (`Pop`)

```c
RB_Status_t RingBuffer_Pop(RingBuffer_t *rb, void *data);
```

* **Purpose:** Retrieves (reads) an element according to FIFO (First-In, First-Out) order from the read position (`head`) and removes that element from the buffer.
* **Parameters:**
  * `rb`: Pointer to the Ring Buffer.
  * `data`: Pointer to the memory buffer where retrieved data will be stored.
* **Underlying Operations:**
  1. Checks that pointers `rb` and `data` are not `NULL`.
  2. Checks whether the buffer is empty (`RingBuffer_IsEmpty`).
  3. Calculates the source address:
     $$\text{src} = (\text{uint8\_t *})\text{rb->buffer} + (\text{rb->head} \times \text{rb->size\_element})$$
  4. Uses `memcpy` to copy `size_element` bytes from `src` to `data`.
  5. Advances the read pointer `head` circularly using the bitmask formula:
     $$\text{head}_{\text{new}} = (\text{head} + 1) \ \& \ \text{rb->mask}$$
  6. Decrements the element counter: `rb->size--`.

---
