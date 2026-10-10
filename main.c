#include <stdio.h>
#include <unistd.h>

// Header block structure attached to every allocated/free memory chunk[cite: 1]
typedef struct block_meta {
    size_t size;              // Size of the usable memory block[cite: 1]
    int is_free;              // 1 if free, 0 if allocated[cite: 1]
    struct block_meta *next;  // Pointer to the next block in the free list[cite: 1]
} block_meta;

#define META_SIZE sizeof(block_meta)

// Head of the linked list tracking memory blocks
void *head = NULL;

// Searches the linked list for a free block of sufficient size (First-Fit Search)[cite: 1]
block_meta *find_free_block(block_meta **last, size_t size) {
    block_meta *current = (block_meta *)head;
    while (current && !(current->is_free && current->size >= size)) {
        *last = current;
        current = current->next;
    }
    return current;
}

// Requests new memory from the kernel using sbrk()[cite: 1]
block_meta *request_space(block_meta *last, size_t size) {
    block_meta *block = (block_meta *)sbrk(0);
    void *request = sbrk(size + META_SIZE);
    
    if (request == (void *)-1) {
        return NULL; // sbrk failed
    }

    if (last) { // Link the new block to the end of the list
        last->next = block;
    }

    block->size = size;
    block->is_free = 0;
    block->next = NULL;
    return block;
}

// Custom memory allocation function[cite: 1]
void *my_malloc(size_t size) {
    if (size <= 0) {
        return NULL;
    }

    block_meta *block;

    if (!head) { // First allocation call
        block = request_space(NULL, size);
        if (!block) {
            return NULL;
        }
        head = block;
    } else {
        block_meta *last = (block_meta *)head;
        block = find_free_block(&last, size);
        
        if (!block) { // No free block found; request new heap space[cite: 1]
            block = request_space(last, size);
            if (!block) {
                return NULL;
            }
        } else { // Suitable free block found; reuse it[cite: 1]
            block->is_free = 0;
        }
    }

    // Return pointer to usable memory space (just after the block metadata)
    return (void *)(block + 1);
}

// Custom memory deallocation function[cite: 1]
void my_free(void *ptr) {
    if (!ptr) {
        return;
    }

    // Move back from payload pointer to access metadata header
    block_meta *block_ptr = (block_meta *)ptr - 1;
    block_ptr->is_free = 1; // Mark block as free for reuse[cite: 1]
}

// ==========================================
// Test Cases from PPT Presentation[cite: 1]
// ==========================================

int main() {
    printf("--- Test 1: Memory Allocation ---\n");
    void *p1 = my_malloc(100);
    printf("p1 = %p\n\n", p1);

    printf("--- Test 2: Memory Reuse ---\n");
    my_free(p1);
    void *p2 = my_malloc(50);
    printf("p1 = %p\n", p1);
    printf("p2 = %p\n", p2);
    if (p1 == p2) {
        printf("Success: Freed block was reused!\n\n");
    }

    printf("--- Test 3: Multiple Blocks ---\n");
    void *p3 = my_malloc(100);
    void *p4 = my_malloc(200);
    void *p5 = my_malloc(300);
    printf("p3 = %p\n", p3);
    printf("p4 = %p\n", p4);
    printf("p5 = %p\n", p5);

    return 0;
}
