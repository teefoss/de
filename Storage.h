//
//  Storage.h
//  de
//

#ifndef Storage_h
#define Storage_h

#include <stdlib.h>

template <typename T>
class Storage {
public:
    Storage() { };
    Storage(int numSlots);
    ~Storage();

    void clear() { numElements = 0; }
    void resize(int numSlots);
    int  count() const { return numElements; }
    void insert(const T& data, int index);
    void append(const T& data);
    void remove(int index);
    void removeFast(int index);

    T& operator[](int index) { return buffer[index]; }
    const T& operator[](int index) const { return buffer[index]; }
    T * pointer() { return buffer; }
    const T * pointer() const { return buffer; }

private:
    int numSlots = 0; // Total number of array slots currently allocated.
    int numElements = 0; // Current number of elements in the array.
    T * buffer = nullptr;
};

template <typename T>
inline Storage<T>::Storage(int numSlots)
{
    this->buffer = (T *)malloc(numSlots * sizeof(T));
    this->numSlots = numSlots;
    this->numElements = 0;
}

template <typename T>
inline Storage<T>::~Storage()
{
    if ( buffer ) {
        free(buffer);
        buffer = nullptr;
        numSlots = 0;
        numElements = 0;
    }
}

template <typename T>
inline void Storage<T>::resize(int numSlots)
{
    buffer = (T *)realloc(buffer, sizeof(T) * numSlots);
    this->numSlots = numSlots;
}

template <typename T>
inline void Storage<T>::insert(const T &data, int index)
{
    ++numElements;
    if ( numElements > numSlots ) {
        buffer = (T *)realloc(buffer, sizeof(T) * numElements);
        numSlots = numElements;
    }

    // Move all elements to the right of index one to the right.
    for ( int i = numElements - 1; i > index; i-- ) {
        buffer[i] = buffer[i - 1];
    }

    buffer[index] = data;
}

template <typename T>
inline void Storage<T>::append(const T& data)
{
    insert(data, numElements);
}

template <typename T>
inline void Storage<T>::remove(int index)
{
    // Most the everything to the right over one to the left.
    for ( int i = index; i < numElements - 1; i++ ) {
        buffer[i] = buffer[i + 1];
    }

    --numElements;
}

template <typename T>
inline void Storage<T>::removeFast(int index)
{
    buffer[index] = buffer[--numElements];
}

#endif /* Storage_h */
