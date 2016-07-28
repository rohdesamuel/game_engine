#ifndef STDHEADER__H
#define STDHEADER__H

#ifndef __LP64__
#define __LP32__
#endif

#if ((defined _WIN32 || defined __LP32__) && !defined _WIN64) 
#define __COMPILE_AS_32__
#elif (defined _WIN64 || defined __LP64__)
#define __COMPILE_AS_64__
#endif

#if (defined __WIN32__ || defined __CYGWIN32__ || defined _WIN32 || defined _WIN64 || defined _MSC_VER)
#define __COMPILE_AS_WINDOWS__
#elif (defined __linux__ || defined __GNUC__)
#define __COMPILE_AS_LINUX__

typedef signed char         INT8, *PINT8;
typedef signed short        INT16, *PINT16;
typedef signed int          INT32, *PINT32;
typedef signed __int64      INT64, *PINT64;
typedef unsigned char       UINT8, *PUINT8;
typedef unsigned short      UINT16, *PUINT16;
typedef unsigned int        UINT32, *PUINT32;
typedef unsigned __int64    UINT64, *PUINT64;
#endif

#define __ENGINE_DEBUG__

#if (defined __COMPILE_AS_32__)
typedef INT32 INT;
typedef UINT32 UINT;
#elif (defined __COMPILE_AS_64__)
#endif

#ifdef __COMPILE_AS_WINDOWS__
#include <Windows.h>

#ifdef __ENGINE_DEBUG__
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#define AssertOrDie(assert) \
do{ if (!(assert)) { int* a = 0; *a = 0; } } while (0)

#else
#define AssertOrDie(assert) do{}while(0)
#endif

#elif (defined __COMPILE_AS_LINUX__)
#endif

/* In case we already defined our foreach loop */
#ifdef in
#undef in
#endif

/* All C includes */
#include <intsafe.h>

/* All C++ includes */
#include <algorithm>
#include <array>
#include <exception>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <regex>
#include <set>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

/* Forward declarations */
namespace radiant_engine
{

}

typedef UINT32 TypeId;
typedef UINT64 Id;

struct InstId {
	UINT32 type_id;
	UINT64 inst_id;
};

class AbstractGameLoop;
class AbstractObject;
class AbstractObjectType;
class AbstractObjectTypeLibrary;
class AbstractObjectManager;
class AbstractObjectBuffer;
class AbstractWorkBuffer;
class AbstractUpdater;
class AbstractRenderer;
class AbstractInterpolator;
class AbstractWork;
class InstanceList;
class Args;
class ArgsFactory;
class ChangeRequest;
class ChangeRequestFactory;
class ChangeRequestFactoryModule;
class ObjectRef;


template <class ItTy_, class RetTy_>
class IteratorHolder;

// Abstract object capable of holding any data types stored in a Var.
class Object;

// Slightly modified boost Any class.
class Var;


/* Template Library */
// ???

#define CACHE_LINE_SIZE 64
#ifdef __COMPILE_AS_LINUX__
#define __CACHE_ALIGNED__ __attribute__((align(64)))

template<typename T>
struct  cache_line_storage {
	T data;
};
#elif defined __COMPILE_AS_WINDOWS__
#define __CACHE_ALIGNED__ __declspec(align(CACHE_LINE_SIZE))
template<typename T>
struct __CACHE_ALIGNED__ cache_line_storage {
	T data;
};
#endif

/* Foreach Loop */
#define _in_ :
#define in _in_
#define foreach(expr) \
for (auto& expr)

/* Macro Defines */
//#define IncludePackage(module) \

#define ConextSwitchHint() //Sleep(0)

#define DefinePrototype(name) \
	typedef class name __PROTOTYPE__; \
	typedef Instance< name > __INSTANCE_TYPE__; \
	typedef InstanceModule< __INSTANCE_TYPE__ >::Instances __INSTANCES_TYPE__; \
class name : public Prototype

#define DefineCallback(name) \
	auto name = [this](Entity * entity)

#define InstallModule(module) \
	friend class module<__PROTOTYPE__>; \
	typedef module<__PROTOTYPE__> __PROTOTYPE_##module##__; \
	__PROTOTYPE_##module##__ My##module

#define InitModule(module) \
	My##module = __PROTOTYPE_##module##__(this)

#define DefineModule(module) \
	template <class Ty_> \
class module##Module : \
	private Module

#define CastInstance(entity) \
	static_cast<__INSTANCE_TYPE__ *>(entity);

#define CastPrototype(entity) \
	static_cast<__PROTOTYPE__ *>(entity);

#endif 