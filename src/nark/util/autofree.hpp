#ifndef __nark_util_autofree_hpp__
#define __nark_util_autofree_hpp__

#include <boost/noncopyable.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits.hpp>
#include <stdlib.h>
#include <stdexcept>
#include <memory>
#include <utility>
#include <algorithm>

#if defined(__GNUC__) && __GNUC__ >= 4 && !defined(__GXX_EXPERIMENTAL_CXX0X__)
	#include <ext/memory> // for uninitialized_copy_n
	#include <ext/algorithm> // for copy_n
	#define STDEXT_copy_n               __gnu_cxx::copy_n
	#define STDEXT_uninitialized_copy_n __gnu_cxx::uninitialized_copy_n
#else
	#define STDEXT_copy_n               std::copy_n
	#define STDEXT_uninitialized_copy_n std::uninitialized_copy_n
#endif

#if defined(__GXX_EXPERIMENTAL_CXX0X__) || __cplusplus >= 201103L || \
	defined(_MSC_VER) && _MSC_VER >= 1700
	#include <initializer_list>
	#ifndef HSM_HAS_MOVE
		#define HSM_HAS_MOVE
	#endif
#endif

#ifndef __HAS_RVALUE_REFERENCE
  #if (__GNUC__ > 4 || __GNUC__ == 4 && __GNUC_MINOR__ >= 3) || \
								       _MSC_VER >= 1600
    #if __EDG_VERSION__ > 0
      #define __HAS_RVALUE_REFERENCE (__EDG_VERSION__ >= 410)
    #else
      #define __HAS_RVALUE_REFERENCE 1
    #endif
  #elif __clang__
    #define __HAS_RVALUE_REFERENCE __has_feature(cxx_rvalue_references)
  #else
    #define __HAS_RVALUE_REFERENCE 0
  #endif
#endif

namespace nark {

	template<class T>
	class AutoFree : boost::noncopyable {
		BOOST_STATIC_ASSERT(boost::has_trivial_destructor<T>::value);

	public:
		T* p; // just the pointer, no any overhead

		AutoFree() : p(NULL) {}
		explicit AutoFree(T* q) : p(q) {}
		explicit AutoFree(size_t n) {
			p = (T*)::malloc(sizeof(T) * n);
			if (NULL == p)
				throw std::bad_alloc();
		}
		AutoFree(size_t n, T val) {
			AutoFree tmp(n);
			std::uninitialized_fill_n(tmp.p, n, val);
			p = tmp.release();
		}
		template<class U>
		AutoFree(size_t n, const U* src) {
			AutoFree tmp(n);
			STDEXT_uninitialized_copy_n(src, n, tmp.p);
			p = tmp.release();
		}
		~AutoFree() { if (p) ::free(p); }

		void operator=(T* q) {
		   	if (p)
			   	::free(p);
		   	p = q;
	   	}

		void free() {
		   	assert(p);
		   	::free(p);
		   	p = NULL;
	   	}

		void alloc(size_t n) {
			if (p) ::free(p);
			p = (T*)::malloc(sizeof(T) * n);
			if (NULL == p)
				throw std::bad_alloc();
		}

		void resize(size_t n) {
			T* q = (T*)::realloc(p, sizeof(T) * n);
			if (NULL == q)
				throw std::bad_alloc();
			p = q;
		}

		void resize(size_t oldsize, size_t newsize, T val) {
			resize(newsize);
			if (oldsize < newsize)
				std::uninitialized_fill_n(p+oldsize, newsize-oldsize, val);
		}

		T* release_and_set(T* newptr) {
			T* oldptr = p;
			p = newptr;
			return oldptr;
		}

		T* release() {
			T* q = p;
			p = NULL;
			return q;
		}

		void swap(AutoFree& y) { T* tmp = p; p = y.p; y.p = tmp; }

		operator T*  () const { return  p; }
		T* operator->() const { return  p; } // ? direct, simple and stupid ?
		T& operator* () const { return *p; } // ? direct, simple and stupid ?
	//	T& operator[](int i) const { return p[i]; }
	};

} // namespace nark

namespace std {
	template<class T>
	void swap(nark::AutoFree<T>& x, nark::AutoFree<T>& y) { x.swap(y); }
}

#endif


