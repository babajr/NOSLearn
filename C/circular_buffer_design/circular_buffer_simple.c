/*
 * LeetCode 622: Design Circular Queue (C)
 *
 * Approach: fixed-size array + head index + element count.
 *   - Tracking `size` (instead of a tail pointer or a wasted slot) makes
 *     isEmpty / isFull trivial and avoids the classic off-by-one bugs.
 *   - tail position is derived: (head + size) % capacity
 *
 * All operations are O(1) time, O(k) space.
 * For Concurrent Model: refer https://book-of-gehn.github.io/articles/2020/03/22/Lock-Free-Queue-Part-I.html
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct
{
    int *data;
    int head;     /* index of the front element */
    int size;     /* number of elements currently stored */
    int capacity; /* max elements (k) */
} MyCircularQueue;

MyCircularQueue *myCircularQueueCreate(int k)
{
    MyCircularQueue *q = malloc(sizeof(MyCircularQueue));
    if (!q)
        return NULL;
    q->data = malloc(sizeof(int) * k);
    if (!q->data)
    {
        free(q);
        return NULL;
    }
    q->head = 0;
    q->size = 0;
    q->capacity = k;
    return q;
}

bool myCircularQueueIsEmpty(MyCircularQueue *q)
{
    return q->size == 0;
}

bool myCircularQueueIsFull(MyCircularQueue *q)
{
    return q->size == q->capacity;
}

/* Insert at the rear. Returns false if the queue is full. */
bool myCircularQueueEnQueue(MyCircularQueue *q, int value)
{
    if (myCircularQueueIsFull(q))
        return false;
    int tail = (q->head + q->size) % q->capacity;
    q->data[tail] = value;
    q->size++;
    return true;
}

/* Remove from the front. Returns false if the queue is empty. */
bool myCircularQueueDeQueue(MyCircularQueue *q)
{
    if (myCircularQueueIsEmpty(q))
        return false;
    q->head = (q->head + 1) % q->capacity;
    q->size--;
    return true;
}

/* Front element, or -1 if empty. */
int myCircularQueueFront(MyCircularQueue *q)
{
    return myCircularQueueIsEmpty(q) ? -1 : q->data[q->head];
}

/* Rear element, or -1 if empty. */
int myCircularQueueRear(MyCircularQueue *q)
{
    if (myCircularQueueIsEmpty(q))
        return -1;
    return q->data[(q->head + q->size - 1) % q->capacity];
}

void myCircularQueueFree(MyCircularQueue *q)
{
    if (!q)
        return;
    free(q->data);
    free(q);
}

/* ------------------------- Driver code ------------------------- */

static void check(const char *op, int got, int expected)
{
    printf("%-18s -> %-5d (expected %-2d) %s\n",
           op, got, expected, got == expected ? "OK" : "FAIL");
}

int main(void)
{
    /* LeetCode's sample: ["MyCircularQueue","enQueue","enQueue","enQueue",
       "enQueue","Rear","isFull","deQueue","enQueue","Rear"] with k = 3 */
    MyCircularQueue *q = myCircularQueueCreate(3);

    check("enQueue(1)", myCircularQueueEnQueue(q, 1), 1);
    check("enQueue(2)", myCircularQueueEnQueue(q, 2), 1);
    check("enQueue(3)", myCircularQueueEnQueue(q, 3), 1);
    check("enQueue(4)", myCircularQueueEnQueue(q, 4), 0); /* full */
    check("Rear()", myCircularQueueRear(q), 3);
    check("isFull()", myCircularQueueIsFull(q), 1);
    check("deQueue()", myCircularQueueDeQueue(q), 1);
    check("enQueue(4)", myCircularQueueEnQueue(q, 4), 1); /* wraps around */
    check("Rear()", myCircularQueueRear(q), 4);
    check("Front()", myCircularQueueFront(q), 2);

    /* Edge cases: drain the queue, then probe the empty state */
    myCircularQueueDeQueue(q);
    myCircularQueueDeQueue(q);
    myCircularQueueDeQueue(q);
    check("isEmpty()", myCircularQueueIsEmpty(q), 1);
    check("deQueue() empty", myCircularQueueDeQueue(q), 0);
    check("Front() empty", myCircularQueueFront(q), -1);
    check("Rear() empty", myCircularQueueRear(q), -1);

    myCircularQueueFree(q);
    return 0;
}
