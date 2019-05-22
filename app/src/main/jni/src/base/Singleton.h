#ifndef _SINGLETON_H__
#define _SINGLETON_H__
#include <cassert>
template <typename T> class Singleton{
private:
    /** @brief Explicit private copy constructor. This is a forbidden operation.*/
    Singleton(const Singleton<T> &);
    /** @brief Private operator= . This is a forbidden operation. */
    Singleton& operator=(const Singleton<T> &);
protected:
    static T* msSingleton;
public:
    Singleton( void ){
        assert( !msSingleton );
        msSingleton = static_cast< T* >( this );
    }
    virtual ~Singleton( void ){
        assert( msSingleton );
        msSingleton = nullptr;
    }
    static T& getSingleton( void ){
        assert( msSingleton );
        return ( *msSingleton );
    }
    static T* getSingletonPtr( void ){
        return msSingleton;
    }
};

#endif
