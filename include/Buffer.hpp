#pragma once

#include <cstdlib>
#include <stdexcept>
#include <cstring>
#include <iostream>
class Buffer {
public:
    char *data;
    size_t size;
    size_t pos;

    Buffer() : data(NULL), size(0), pos(0) {}

    Buffer(size_t s) : size(s) , pos(0) {
        data = new char[size];
    }

    ~Buffer() {
        if (data) {
            delete [] data;
        }
    }

    // Buffer(const Buffer & buff) {
    //     *this = buff;
    // }

    // Buffer & operator=(const Buffer & buff) {
    //     data = buff.data;
    //     size = buff.size;
    //     pos = buff.pos;
    //     return *this;
    // }

    size_t length() const {
        return size - pos;
    }

    void resize(size_t new_size) {
        // std::cerr << "size: " << new_size << std::endl;
        if (size == new_size) {
            pos = 0;
            return ;   
        }
        if (new_size == 0) {
            if (data) {
                delete [] data;
                data = NULL;
            }
            pos = 0;
            size = 0;
        }

        char *new_data = new char[new_size];

        if (data) {
            size_t copyn = size > new_size ? new_size : size;
            std::memcpy(new_data, data, copyn);
            delete [] data;
        }
        
        size = new_size;
        pos = 0;
        data = new_data;
    }

    void setData(const char *new_data, size_t len) {
        resize(len);
        std::memcpy(data, new_data, len);
    }

    char operator[] (size_t p) throw (std::out_of_range) {
        if (p < size) {
            return data[p];
        }
        throw std::out_of_range("buffer out of range");
    }
    
    ssize_t read(char *buff, size_t size) {
        buff = data + pos;
        pos += size;
        return size <= length() ? size : length();
    }

    char getc() {
        if (pos < size) {
            return data[pos++];
        }
        return -1;
    }
    char peekc() {
        if (pos < size) {
            return data[pos];
        }
        return -1;
    }
};

