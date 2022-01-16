/* References:
 1- https://github.com/andrestc/linux-prog/blob/master/ch7/malloc.c
 2- https://www.geeksforgeeks.org/data-structures/linked-list/doubly-linked-list/
 I have looked at these sources to get a better understanding of the task. */
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

#include "memory_management.h"

#define memory_block(pointer) ((void*)((unsigned long) pointer + sizeof(block)))
#define block_header(pointer) ((void*)((unsigned long) pointer - sizeof(block)))

typedef struct block block;
struct block {
  int block_size;
  block *next_block;
  block *prev_block;
  bool available;
};

block *head = NULL;

// Split an existing block into 2 blocks.
void split_block(block *old_block, size_t size) {
  void *start_address;
  block *result_block;

  start_address = memory_block(old_block);
  result_block = (block*)((unsigned long) start_address + size);

  // Subtract sizes and update the original block.
  result_block->block_size = (old_block->block_size) - (size + sizeof(block));
  old_block->block_size = size;
  old_block->available = false;

  /* Insert the new resultant block from the split in the list after the block
  that was splited and set it up. */
  result_block->available = true;
  result_block->next_block = old_block->next_block;
  old_block->next_block = result_block;
  result_block->prev_block = old_block;
  if (result_block->next_block != NULL)
    result_block->next_block->prev_block = result_block;
}

/* Looks for continuous/adjacent free blocks and marges them together and if the
last block of memory at the program break is free, it returns it to the OS. */
void merge() {
  block *pointer = head;

  while(pointer->next_block != NULL) {
    if ((pointer->available == true) && (pointer->next_block->available == true)) {
      // Sort out the pointers to remove block from the list first.
      block *to_be_deleted = pointer->next_block;
      if (to_be_deleted->next_block != NULL)
        to_be_deleted->next_block->prev_block = to_be_deleted->prev_block;

      if (to_be_deleted->prev_block != NULL)
        to_be_deleted->prev_block->next_block = to_be_deleted->next_block;

      // Add the sizes now
      pointer->block_size = pointer->block_size + sizeof(block) + to_be_deleted->block_size;
    }
    else
      pointer = pointer->next_block;
  }

  // If the last block ends on the program break and it is free, return it to the OS.
  // pointer is already pointing at the last block after the loop finishes.
  if (pointer->available == true) {

    /* If there are other blocks, make the new last block point to NULL.
    If there is only one block make head point to NULL again. */
    if (pointer->prev_block != NULL)
      pointer->prev_block->next_block = NULL;
    else
      head = NULL;

    if (brk(pointer) != 0)
      printf("brk error. \n");
  }
}

void * _malloc(size_t size) {
  void *address_ptr;
  block *new_block, *checking_ptr;
  int padding;

  // Padding, all memory requested will be a multiple of 8
  if ( (size % 8) != 0 ) {
    padding = 8 - (size % 8);
    size = size + padding;
  }

  /* Loop through doubly list of blocks to find a free block of equal size, or if it is
  not of equal size split it to the requested size and mark the rest as free */
  checking_ptr = head;
  while(checking_ptr) {
    // If it is exactly equal
    if ((checking_ptr->block_size == size) && (checking_ptr->available == true)) {
      checking_ptr->available = false;
      return memory_block(checking_ptr);
    }
    else if ((checking_ptr->block_size >= size + sizeof(block)) && (checking_ptr->available == true)) {
      // Split an existing block and make excess memory free to use.
      split_block(checking_ptr, size);
      return memory_block(checking_ptr);
    }
    else {
      checking_ptr = checking_ptr->next_block;
    }
  }

  // No free existing blocks so we call sbrk
  new_block = sbrk(size + sizeof(block));
  address_ptr = memory_block(new_block);

  // If the list is non-existant, create the first block and assign head.
  if (head == NULL) {
    head = new_block;
    new_block->block_size = size;
    new_block->next_block = NULL;
    new_block->prev_block = NULL;
    new_block->available = false;
  }
  else { // append to the end of the list
    new_block->next_block = NULL;
    new_block->block_size = size;
    new_block->available = false;

    block *last_block = head;
    while(last_block->next_block != NULL)
      last_block = last_block->next_block;

    last_block->next_block = new_block;
    new_block->prev_block = last_block;
  }

  return address_ptr;
}

void _free(void * ptr) {
  if (ptr == NULL)
    return;
  else {
    // Set the given block to be free and available for use.
    block *block_header = block_header(ptr);
    block_header->available = true;

    merge();
  }
}
