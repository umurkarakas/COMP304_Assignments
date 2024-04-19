#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define BLOCK_SIZE sizeof(struct block)

// variable to keep track of available memory
size_t available_memory = 0;

// struct to keep track of block size
struct block {
    size_t size;
};

// struct to keep track of free list
struct free_list {
    size_t size; // size of the block   
    struct free_list *next; // pointer to the next block
    struct free_list *prev; // pointer to the previous block, which is utilized for merging and splitting free blocks 
};

// head of free list
struct free_list *head = NULL;

// address of the end of the last block
void *block_end = NULL;

// function to align size
size_t size_align(size_t n) {
    size_t size_long = sizeof(long);
    return ((n + size_long - 1) / size_long) * size_long;
}

void *kumalloc(size_t size) {
    if(!size)   {
        return NULL;
    }
    size_t total_size = size_align(size+BLOCK_SIZE);
    void *block = NULL;
    struct free_list *temp = head;

    // find a block that is large enough in the free list
    while(temp) {
        if(temp->size >= total_size) {
            block = temp;
            break;
        } 
        temp = temp->next;
    }

    // if no block is found, allocate a new block from the heap using sbrk
    if(!block) {
        
        // frames are allocated in multiples of 8192 bytes
        size_t capacity = total_size > 8192 ? ((total_size / 8192) + 1) * 8192 : 8192;

        // if available memory is less than the capacity, allocate more memory
        if(available_memory <= 0) {
            // increase the heap size by capacity
            block = sbrk(capacity);
            // update the end of the last block if it is not NULL
            block_end = block_end ? block_end + total_size : block + total_size;
            // update available memory
            available_memory += capacity - total_size;
        }
        // if available memory is greater than the capacity, assign the block to the end of the last block in the last frame
        else if(total_size <= available_memory) {
            // assign the block to the end of the last block
            block = block_end;
            // update the end of the last block
            block_end += total_size;
            // update available memory
            available_memory -= total_size;
        }
        // memory is available but not enough, allocate more memory
        else {
            // increase the heap size by capacity
            sbrk(capacity);
            // assign the block to the end of the last block
            block = block_end;
            // update the end of the last block
            block_end += total_size;
            // update available memory
            available_memory += capacity - total_size;
        }

        // update the size of the block
        struct block *h = (struct block*) block;
        h->size = total_size;
    }
    // if a block is found in the free list, remove it from the free list and use it
    else {
        struct block *h = (struct block*) block;
        struct free_list *free_block = (struct free_list*) h;
        struct free_list *temp = head;
        // subseq is used for splitting the free block into two blocks, one for the memory allocation and the other for the remaining free memory
        struct free_list *subseq = (struct free_list*) ((char*) block + total_size);
        subseq->next = NULL;
        int size_diff = ((int) free_block->size - (int) total_size);
        subseq->size = size_diff;
        h->size = total_size;
        // if the block is the head of the free list, update the head
        if(head == free_block) {
            // if the size difference is greater than 0, split the block into two blocks
            if (size_diff > 0) {
                subseq->next = head->next;
                subseq->prev = head->prev;
                head = subseq;
            } else {
                head = head->next;
                if (head != NULL){
                    head->prev = NULL;
                }
            }
        }
        else {
            while(temp->next) {
                // find the block in the free list
                if(temp->next == free_block) {
                    // if the size difference is greater than 0, split the block into two blocks
                    if (size_diff > 0) {
                        subseq->next = temp->next->next;
                        temp->next->next->prev = subseq;
                        temp->next = subseq;
                        subseq->prev = temp;
                    }
                    // if the size difference is 0, remove the block from the free list
                    else {
                        temp->next->next->prev = temp;
                        temp->next = temp->next->next;
                    }
                    break;
                }
                if (temp->next == NULL) {
                    break;
                }
                temp = temp->next;
            }
        }
    }
    // return the block after the block size since the block size is stored in the first 8 bytes of the block
    return block + BLOCK_SIZE;
}

void *kucalloc(size_t nmemb, size_t size) {
    // if either nmemb or size is 0, return NULL
    if(!size || !nmemb) {
        return NULL;
    }
    // allocate memory for nmemb * size
    size_t total_size = size_align(nmemb * size);
    void *block = kumalloc(total_size);
    // set the allocated memory to 0
    memset(block, 0, total_size);
    return block;
}

void kufree(void *ptr) {
    // if ptr is NULL, return
    if(!ptr) {
        return;
    }
    // print is used for debugging
    int print = 0;
    // get the block from the pointer, subtract the block size from the pointer to get the block with block size
    struct block* block = (struct block*) (ptr - BLOCK_SIZE);
    struct free_list* free_block = (struct free_list*) block;
    int return_flag = 0;
    // if the block is the last block, free the block and decrease the heap size
    if ((block_end == (char*) block + block->size)) {
        if (print) {
            fprintf(stderr,"block_end address: %p\n", block_end);
            fprintf(stderr,"block address: %p\n", block);
            fprintf(stderr,"block end address: %p\n", (char*) block + block->size);
        }
        block_end = (char*) block;
        // decrease the heap size by the size of the block 
        sbrk(-(block->size));
        // return flag activated if the block is the last block, no need to add to the free list
        return_flag = 1;
    }
    struct free_list* temp = head;
    // update the heap if last blocks are previously freed
    while(temp){
        if (print) {
            fprintf(stderr,"temp address: %p\n", temp);
            fprintf(stderr,"temp end address: %p\n", (char*) temp + temp->size);
            fprintf(stderr,"block_end address: %p\n", block_end);
            fprintf(stderr,"**************************************************\n");
        }
        // if the block is the last block, free the block and decrease the heap size
        if ((char*) temp + temp->size == (char*) block_end) {
            // update the end of the last block
            block_end = (char*) temp;
            // decrease the heap size by the size of the block
            sbrk(-(temp->size));
            if (temp == head) {
                head = head->next;
            } else {
                temp->prev->next = temp->next;
            }
        }
        temp = temp->next;
    }
    if (return_flag) {
        return;
    }
    if(head == NULL) {
        free_block->next = NULL;
    } else {
        temp = head;
        // i is used for debugging
        int i = 0;
        while(temp) {
            i++;
            if (print) {
                printf("**************************************************\n");
                printf("iteration: %d\n", i);
            }
            // end1 and end2 are used for merging free blocks, end1 is the end of the current block, end2 is the end of the free block
            struct free_list *end1 = (struct free_list*) ((char*) temp + temp->size);
            struct free_list *end2 = (struct free_list*) ((char*) free_block + free_block->size);
            if (print) {
                printf("temp size: %ld\n", temp->size);
                printf("free_block size: %ld\n", free_block->size);
                printf("temp->next address: %p\n", temp->next);
                if (temp->next != NULL){
                    printf("temp->next end address: %p\n", (char*) temp->next + temp->next->size);
                }
                printf("temp->prev address: %p\n", temp->prev);
                printf("end1 address: %p\n", end1);
                printf("end2 address: %p\n", end2);
                printf("temp address: %p\n", temp);
                printf("free_block address: %p\n", free_block);
            }
            // if the free block is at the end of an already freed block, merge them
            if (end1 == free_block) {
                struct free_list* temp2 = head;
                // loop over all freed blocks to check freed block is in between two already freed blocks
                while(temp2) {
                    if(print){
                        printf("****************\n");
                        printf("temp address: %p\n", temp);
                        printf("temp2 address: %p\n", temp2);
                        printf("free_block address: %p\n", free_block);
                        printf("temp2 end address: %p\n", (char*) temp2 + temp2->size);
                        printf("end2 address: %p\n", end2);
                    }
                    // if the free block is in between two already freed blocks, merge those 3 blocks
                    if(temp2 == end2) {
                        // arrange size
                        temp->size = (size_t) ((int)temp->size + (int)free_block->size + (int)temp2->size);
                        // arrange pointers
                        if (temp2->next != temp) {
                            temp->next = temp2->next;
                        }    
                        if (temp2->next != NULL) {
                            temp2->next->prev = temp;
                        }
                        if (temp2 != head) {
                            temp2->prev->next = temp;
                        } else {
                            head = temp;
                        }
                        if (head == head->next) {
                            head->next = NULL;
                        }
                        return;
                    }
                    if (temp2->next == NULL) {
                        break;
                    }
                    temp2 = temp2->next;
                }
                
                if (print){
                    printf("previous size1: %ld\n", temp->size);
                    printf("added size1: %ld\n", free_block->size);
                    printf("new size1: %ld\n", temp->size + free_block->size);
                }
                // if no 3 blocks are merged, merge the 2 blocks
                temp->size = (size_t) ((int)free_block->size + (int)temp->size);
                return;
            }
            // if the free block ends at the beginning of an already freed block, merge them
            if(end2 == temp) {
                struct free_list* temp2 = head;
                // loop over all freed blocks to check freed block is in between two already freed blocks
                while(temp2) {
                    // end_temp is the end of the current block in the loop
                    struct free_list* end_temp = (struct free_list*) ((char*) temp2 + temp2->size);
                    if (print) {
                        printf("end temp address: %p\n", end_temp);
                    }
                    // if free block is in between two already freed blocks, merge those 3 blocks
                    if(free_block == end_temp) {
                        // arrange size
                        temp2->size = (size_t) ((int)temp2->size + (int)free_block->size + (int)temp->size);
                        // arrange pointers
                        if (temp->next != temp2) {
                            temp2->next = temp->next;
                        }
                        if (temp->next != NULL) {
                            temp->next->prev = temp2;
                        }

                        if (temp != head) {
                            temp->prev->next = temp2;
                        } else {
                            head = temp2;
                        }

                        if (head == head->next) {
                            head->next = NULL;
                        }
                        if (print) {
                            printf("-----------------------------------------------------\n");
                            printf("head address: %p\n", head);
                            printf("temp2 address: %p\n", temp2);
                            printf("temp address: %p\n", temp);
                            printf("-----------------------------------------------------\n");
                        }
                        return;
                    }
                    temp2 = temp2->next;
                }
                if (print) {
                    printf("previous size2: %ld\n", free_block->size);
                    printf("added size2: %ld\n", temp->size);
                    printf("temp address: %p\n", temp);
                    printf("temp->next address: %p\n", temp->next);
                    printf("temp->prev address: %p\n", temp->prev);
                }
                // if no 3 blocks are merged, merge the 2 blocks
                // arrange size and pointers
                free_block->size = (size_t) ((int)free_block->size + (int)temp->size);
                free_block->next = temp->next;
                free_block->prev = temp->prev;
                if (temp != head) {
                    temp->prev->next = free_block;
                } else {
                    head = free_block;
                }
                return;
            }
            temp = temp->next;
        }
        // if the free block is not merged with any other free block, add it to the free list
        free_block->next = head;
        head->prev = free_block;
    }
    head = free_block;
}

void *kurealloc(void *ptr, size_t size) {
    // if size is 0 and ptr is not NULL, free the block
    if(!size && ptr) {
        kufree(ptr);
        return NULL;
    }
    // if ptr is NULL, allocate memory
    if(!ptr) {
        return kumalloc(size);
    }
    // h is the block to be reallocated, subtract the block size from the pointer to get the block with block size
    struct block *h = (struct block*) (ptr - BLOCK_SIZE);
    // if the size of the block is greater than or equal to the size to be reallocated, return the pointer
    if(h->size >= size) {
        return ptr;
    }
    // allocate memory for the new size
    size_t total_size = size_align(size + BLOCK_SIZE);
    size_t prev_size = h->size;
    void *block = kumalloc(total_size);
    // copy the contents of the old block to the new block
    memcpy(block, h+1, prev_size);
    return block;
}


/*
 * Enable the code below to enable system allocator support for your allocator.
 * Doing so will make debugging much harder (e.g., using printf may result in
 * infinite loops).
 */
#if 1
void *malloc(size_t size) { return kumalloc(size); }
void *calloc(size_t nmemb, size_t size) { return kucalloc(nmemb, size); }
void *realloc(void *ptr, size_t size) { return kurealloc(ptr, size); }
void free(void *ptr) {  kufree(ptr); }
#endif