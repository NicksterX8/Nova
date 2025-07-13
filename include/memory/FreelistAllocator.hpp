#pragma once

#include "memory/Allocator.hpp"

// unfinished

template<typename AllocatorT>
struct FreelistAllocator {
    struct Node {
        Node* next;
        size_t size;
    };

    Node* head;
    EMPTY_BASE_OPTIMIZE AllocatorT allocator;

    void allocateNode(size_t size) {
        void* memory = allocator.allocate(size + sizeof(Node), alignof(std::max_align_t));
        Node* node = (Node*)memory;
        char* ptr = (char*)memory + sizeof(Node);

        *node = {
            .next = head,
            .size = size
        };

        head = node;
    }

    // return one of the nodes if it has the given size, if no nodes have that size, return null
    void* tryAllocate(size_t size) {
        Node* node = head;
        Node* last = head;
        while (node) {
            if (node->size == size) {
                char* ptr = (char*)node;
                // remove node from linked list
                last->next = node->next; // move last ptr along one
                if (node == head) {
                    head = node->next;
                }
                return ptr;
            }
            last = node;
            node = node->next;
        }

        return nullptr;
    }

    void deallocate(void* ptr, size_t size) {
        // insert node at head of linked list
        Node* node = (Node*)((char*)ptr - sizeof(Node));
        node->next = head;
        node->size = size;
        head = node;
    }
};