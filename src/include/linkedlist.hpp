#pragma once
#include <cstddef>
#include <type_traits>

template <typename T>
concept Pointer = std::is_pointer<T>::value;

template <Pointer T, std::size_t capacity>
class LinkedList {
    struct Item {
        T data;
        Item* next;
    };
    Item* first = nullptr;
    Item* last = nullptr;
    std::size_t count = 0;
    Item array[capacity] = {0};

    constexpr Item* find_empty() const {
        for (std::size_t i = 0; i < capacity; ++i) {
            if (array[i].data == nullptr) {
                return array[i];
            }
        }
        return nullptr;
    }

    constexpr void push(T data) {
        Item* item = find_empty();
        item->data = data;
        count++;

        if (first == nullptr) {
            first = item;
        }
        if (last != nullptr) {
            last->next = item;
        } 
        last = item;
    }

    constexpr std::size_t size() {
        return count;
    }

};
