#ifndef RING_H_INCLUDED
#define RING_H_INCLUDED

//#include "Streaming.h"

//#define RING_DEBUG
/*! \defgroup ring Ring: An STL-style templated Ring class
 *  @{
*/

//
// The ring class implements a templated ring structure.  It implements many of
// the methods of an STL collection.


template<class T> class ring;   // forward declaration

// Definition of a single node in the ring.  This is the actual data.
// It is a doubly-linked list that loops in both directions.  A pointer to
// one of the elements defines the "current" element in the ring.
template<class T>class _ringNode {
    friend class ring<T>;
protected:
    T m_val;
    _ringNode* m_next;
    _ringNode* m_prev;
#if defined(RING_DEBUG)
    void DebugDump(Print& p) const;
#endif
};

#if defined(RING_DEBUG)
template<class T> void _ringNode<T>::DebugDump(Print& p) const {
    p.print(F("[val: ")); p.print(m_val);
    p.print(F(" next: 0x")); p.print((int)m_next, HEX);
    p.print(F(" prev: 0x")); p.print((int)m_prev, HEX);
    p.print(F("]"));
}
#endif

// The ring object.  It contains a pointer to the "current" element of
// a series of _ringNode objects/
//
// Note that front() and back() return a reference to the first data element.
// This allows operations like myRing->front()++ to increment the value in
// a ring of ints, for example.  front() and back() do not remove values.
// If the ring is empty, front() and back() will blow up.  This is keeping
// with STL standard:  these operations are not defined on empty collections.

/*! \class ring
	\brief Implements a templated ring data structure.

    This is a templated class that implements a ring of arbitrary objects.  Each object
    must have a constructor and a destructor as well as operator=.  It should also
    have the printTo member function if any form of serialization is to be done.
*/
#if defined(RING_DEBUG)
template<class T>class ring: public Printable {
#else
template<class T>class ring {
#endif
	private:
    _ringNode<T>* m_cur;
    char* m_prefix;

public:
    //***** Constructor, destructor
    //! \brief Construct an empty ring
    ring(): m_cur(NULL), m_prefix((char*)"r: ") {}
    //! \brief Destroy a ring.
    /*!
    	As part of this process, the destructors for all of the stored objects will be called.
    	After the destruction, the ring will be empty.
    */
    ~ring() {}


    void push_front(T val);

    void push_back(T val);

    //***** removal

    void pop_front();
    void pop_back();
    void clear();

    // access

    T& front() const;
    T& back() const;

    // moving around

    void move_next();
	void move_prev();

    // query for info
    bool empty() const;
    size_t size() const;

    // assignment and comparison (equality)
    ring<T>& operator=(ring<T>& r);
    bool operator==(ring<T>& r) const;

#if defined(RING_DEBUG)
	// extension for Printable to allow the class to be serialized/streamed
    size_t printTo(Print& p) const;

    // debugging stuff
    void DebugDump(Print& p) const;
#endif

    // mapping operations
    void map(void (*fn)(T& ));
    void map(void (*fn)(T& , void* ), void* );
};

//! \brief Push an element onto the front of the ring.  The element will be the new current element
/*!
	A more elaborate description of push_front
	\param val - The object being pushed onto the front.  The object is copied.  As such, the object will
	require a copier (operator=) member function.
	\sa front(), push_back()
*/
template<class T>void ring<T>::push_front(T val) {
    _ringNode<T>* newNode;
    newNode = new _ringNode<T>();
    newNode->m_val = val;
    if(m_cur==NULL) {
        newNode->m_next = newNode;
        newNode->m_prev = newNode;
    } else {
        newNode->m_next = m_cur;
        newNode->m_prev = m_cur->m_prev;
        m_cur->m_prev->m_next = newNode;
        m_cur->m_prev = newNode;
    }
    m_cur = newNode;
}

/*! \brief Push an element onto the back of the ring.  The current element is left
	unchanged.

	\param val - The object being pushed onto the back.  The object is copied.  As such, the object will
	require a copier (operator=) member function.
	\sa back(), push_back();
*/
template<class T> inline void ring<T>::push_back(T val) {
    this->push_front(val);
    this->move_next();
}

/*! \brief Removes the first element from the ring.

	Removes the first element from the ring.  This will call the destructor of the stored object.
	Calling on an empty ring will have no effect.
	Returns nothing.
	\sa pop_back()
*/
template<class T> void ring<T>::pop_front() {
    _ringNode<T>* toGo; // node being deleted
    if(m_cur==NULL) return;
    toGo = m_cur;
    if(m_cur=m_cur->m_next) { m_cur=NULL; }
    else {
        m_cur->m_prev->m_next = m_cur->m_next;
        m_cur->m_next->m_prev = m_cur->m_prev;
    }
    m_cur = m_cur->m_next;
    delete toGo;
}

/*! \brief Removes the last element from the ring.

	Removes the last element from the ring.  This will call the destructor of the stored object.
	Calling on an empty ring will have no effect.
	Returns nothing.
	\sa pop_front()
*/
template<class T>inline void ring<T>::pop_back() {
    move_prev();
    pop_front();
}

/*! \brief Removes all elements from a ring.

	Removes all elements from a ring. This will call the destructor of all stored objects.
	Calling on an empty ring will have no effect.
	The ring will be empty after this is called.
	Returns nothing.
*/
template<class T>void ring<T>::clear() {
    // temmpting to do this, but it wastes time copying pointers...
    //  while(!empty() pop_front();
    _ringNode<T>* cur;
    _ringNode<T>* last;
    _ringNode<T>* toGo;
    if(empty()) return;
    for(cur=m_cur, last=m_cur->m_prev; cur!=last; ) {
        toGo = cur;
        cur = cur->m_next;
        delete toGo;
    }
    delete cur;
    m_cur = NULL;
}

/*!	\brief Returns a reference to the first stored data element.

	This returns a reference to the first stored data element.  Any changes to this will change the
	referenced (on-ring) value as well.  Calling on an empty ring will result in unpredictable actions.
	\sa back
*/
template<class T> inline T& ring<T>::front() const {
    return (m_cur->m_val);
}

/*!	\brief Returns a reference to the last stored data element.

	This returns a reference to the last stored data element.  Any changes to this will change the
	referenced (on-ring) value as well.  Calling on an empty ring will result in unpredictable actions.
	\sa front
*/
template<class T> inline T& ring<T>::back() const {
    return (m_cur->m_prev->m_val);
}

/*!	\brief Moves the ring to the next element in sequence.

	This moves the ring to the next element in sequence.  If the ring is empty (or has only one member),
	it will be unchanged.  The previous first element will now be the last element.
	\sa move_prev
*/
template<class T>inline void ring<T>::move_next() {
    if(m_cur!=NULL) m_cur = m_cur->m_next;
}

/*!	\brief Moves the ring to the previous element in sequence.

	This moves the ring to the previous element in sequence.  If the ring is empty (or has only one member),
	it will be unchanged.  The previous last element will now be the first element.
	\sa move_next
*/
template<class T>inline void ring<T>::move_prev() {
    if(m_cur!=NULL) m_cur=m_cur->m_prev;
}

/*! \brief Tells whether or not a ring is empty.

	Returns true if the ring is empty, false if the ring has values on it.
*/
template<class T> inline bool ring<T>::empty() const{
    return m_cur==NULL;
}

/*!	\brief Returns the size of the ring.
*/
template<class T> size_t ring<T>::size() const {
    _ringNode<T>* cur;
    _ringNode<T>* last;
    size_t ret;
    if(m_cur==NULL) return 0;
    for(cur=m_cur, last=m_cur->m_prev, ret=1; cur!=last; cur=cur->m_next)
        ret++;
    return ret;
}

/*!	\brief Produces a lightweight assignnment/copy of a ring

	Produces a lightweight copy of the ring.  That is, the top level data of the ring
	is copied, but all of the data nodes are shared between the source and destination
	ring.
	\param r - the ring that is to be "copied"
*/
template<class T> inline ring<T>& ring<T>::operator=(ring<T>& r) {
    m_cur = r.m_cur;
}

/*! \brief Compare two rings for equality

	Compares the two rings.  This is a lightweight comparion.  It compares the top
	level data (are they pointing to the same data components), but does not compare data
	components.  As such, two rings separately constructed with the same data values will
	not be equal.  But two rings constructed with the operator= will (provided neither have
	been modified).
	\param r - the ring that the current ring is being compared against.
*/
template<class T> inline bool ring<T>::operator==(ring<T>& r) const {
    return m_cur==r.m_cur;
}

#if defined(RING_DEBUG)
/*!	\brief Implements the Printable interface functionality.

	This prints out the ring in a human-readable form to the given Print stream.
	It assumes that each ring data value is either natively printable or
	also implements the Printable interface.
	\param p - the Print stream
*/
template<class T>size_t ring<T>::printTo(Print& p) const {
    _ringNode<T>* last;
    _ringNode<T>* cur;
    size_t len = 0;
    len += p.print("[");
    len += p.print(m_prefix);
    if(m_cur!=NULL) {
        for(cur=m_cur, last=m_cur->m_prev; cur!=last; cur=cur->m_next) {
            len += p.print(cur->m_val);
            len += p.print(" ");
        }
        len += p.print(last->m_val);
    }
    len += p.print("]");
    return len;
}

//! \brief Print out the contents of the ring to a Print object.
/*!
	Print out the contents of the ring to the Print object.  This requires
	that the class T also be Printable (either natively or by implementing
	the Printable interface).
	\param p - a Print object (e.g., Serial).
*/
template<class T>void ring<T>::DebugDump(Print& p) const {
    Serial.print("m_cur: 0x"); Serial.print((int)m_cur, HEX);
    _ringNode<T>* cur;
    _ringNode<T>* last;
    if(m_cur==NULL) { p.print("[]"); return; }
    p.print("[");
    for(cur=m_cur, last=m_cur->m_prev; cur!=last; cur=cur->m_next) {
        cur->DebugDump(p);
    }
    last->DebugDump(p);
    p.print("]");
}
#endif

// Mapping functions

//! \brief Map a function across each data element on a ring.
/*!
	Map a function that takes a single T as its param.  T is passed as a reference
	so the function may modify T.
	\param fn - The function to be applied to each element.
	\sa map(void (*fn)(T&, void*), void*)
*/
template<class T>void ring<T>::map(void (*fn)(T&)) {
    _ringNode<T>* cur;
    _ringNode<T>* last;
    if(empty()) return;
    for(cur=m_cur, last=m_cur->m_prev; cur!=last; cur=cur->m_next) {
        (fn)(cur->m_val);
    }
    (fn)(cur->m_val);
}

//!	\brief Map a function with one secondary parameter across all of the elements of a ring
/*!
// Map a function that takes a T and a void* as a param.  T is pased as a reference
// so the function may modify T.  THe void* is a parameter to map and is passed to
// the function.  It may be a pointer to anything, and can be used to accumulate
// results, etc.
//	\param fn - A function that takes a ring's data object and
//	an arbitrary data pointer.  The function is applied to the pair.
//	\param p - An arbitrary data object.
//	\sa map(void (*fn)())
*/
template<class T>void ring<T>::map(void (*fn)(T&, void*), void* p) {
    _ringNode<T>* cur;
    _ringNode<T>* last;
    if(empty()) return;
    for(cur=m_cur, last=m_cur->m_prev; cur!=last; cur=cur->m_next) {
        (fn)(cur->m_val, p);
    }
    (fn)(cur->m_val, p);
}

/*! @} */ // end ring
#endif // RING_H_INCLUDED
