#ifndef LCALLBACK_H
#define LCALLBACK_H

#include "stdint.h"
#include "lockfree.h"

#ifdef WIN32
#define __SIZE_TYPE__ size_t
#define PACK __attribute__((packed, aligned(1)))
#endif

template<typename R, typename ...Args>
class LCallback {
private:
    class Function {
    public:
        virtual R operator()(Args ...a) = 0;
    };
    uint8_t func[64]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    void copy(uint8_t *other) {
        for(uint8_t *p = func; p<(func+64); p++) *p = *other++;
    }
    LockFree lock;
public:
    /**
   * @brief LCallback - для пустой
   */
    LCallback(){}
    virtual ~LCallback(){
        lock.lock();
        *(uint32_t*)func = 0xffffffff;
        lock.free();
    }
    /**
   * @brief LCallback - конструктор для объект->метод
   */
    template<typename Object, typename Method>
    LCallback(Object *o, Method m) {
        class Func : public Function {
            friend class LCallback<R,Args ...>;
            constexpr void* operator new(__SIZE_TYPE__ sz, uint8_t *p){
                return p;
            }
            constexpr Func(Object *o, Method m):_o(o),_m(m){}
            Object *_o;
            Method _m;
            R operator()(Args ...a){return (_o->*_m)(a...);}
        };
        new(func) Func(o,m);
    }

    /**
   * @brief LCallback - конструктор для лямбда функции
   */
    template<typename Lambda>
    LCallback(Lambda l) {
        class Func : public Function {
            friend class LCallback<R,Args ...>;
            constexpr void* operator new(__SIZE_TYPE__ sz, uint8_t *p){return p;}
            constexpr Func(Lambda l):_l(l){}
            Lambda _l;
            R operator()(Args ...a){return _l(a...);}
        };
        new(func) Func(l);
    }

    template<typename Lambda>
    LCallback& operator=(const Lambda& l){
        class Func : public Function {
            friend class LCallback<R,Args ...>;
            constexpr void* operator new(__SIZE_TYPE__ sz, uint8_t *p){return p;}
            constexpr Func(Lambda l):_l(l){}
            Lambda _l;
            R operator()(Args ...a){return _l(a...);}
        };
        new(func) Func(l);
        return *this;
    }
    /**
   * @brief operator ()
   * @param a
   * @return
   */
    R operator()(Args ...a){
        lock.lock();
        if(*(uint32_t*)func == 0) {
            lock.free();
            return R();
        }
        if(*(uint32_t*)func == 0xffffffff) {
            lock.free();
            return R();
        }
        return resolveVoid((R*)0, a...);
    }
    template <typename rv>
    R resolveVoid(rv*, Args ...a) {
        R&& tmp = (*((Function*)func))(a...);
        lock.free();
        return tmp;
    }
    void resolveVoid(void *, Args ...a) {
        (*((Function*)func))(a...);
        lock.free();
        return;
    }




    /**
   * @brief LCallback - конструктор для функции
   */
    LCallback(R(*cfunc)(Args...)) {
        class Func : public Function {
            friend class LCallback<R,Args ...>;
            constexpr void* operator new(__SIZE_TYPE__ sz, uint8_t *p){return p;}
            constexpr Func(R(*cfunc)(Args...)):_cfunc(cfunc){}
            R(*_cfunc)(Args...);
            R operator()(Args ...a){return _cfunc(a...);}
        };
        new(func) Func(cfunc);
    }
    LCallback& operator=(R(*cfunc)(Args...)) {
        class Func : public Function {
            friend class LCallback<R,Args ...>;
            constexpr void* operator new(__SIZE_TYPE__ sz, uint8_t *p){return p;}
            constexpr Func(R(*cfunc)(Args...)):_cfunc(cfunc){}
            R(*_cfunc)(Args...);
            R operator()(Args ...a){return _cfunc(a...);}
        };
        new(func) Func(cfunc);
        return *this;
    }

    /**
   * @brief с помощью этой штуки можно LCallback приравнивать к указателю на функцию
   */
    typedef R(*ccall_t)(void *lcallback, Args...);
    operator ccall_t() {return &ccall;}
    static R ccall(void *lcallback, Args ...a) {return (*(LCallback<R,Args...>*)lcallback)(a...);}
    /**
   * @brief правило пяти
   */
    LCallback(const LCallback& other) {copy((uint8_t *)other.func);}
    LCallback(LCallback&& other) noexcept {copy((uint8_t *)other.func);}
    LCallback& operator=(const LCallback& other){copy((uint8_t *)other.func); return *this;}
    LCallback& operator=(LCallback&& other){copy((uint8_t *)other.func); return *this;}
};

#endif // LCALLBACK_H
