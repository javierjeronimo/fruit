/*
 * Copyright 2014 Google Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef FRUIT_UNSAFE_MODULE_TEMPLATES_H
#define FRUIT_UNSAFE_MODULE_TEMPLATES_H

#include "metaprogramming.h"
#include "demangle_type_name.h"
#include "type_info.h"
#include "../injector.h"
#include "fruit_assert.h"

namespace fruit {
namespace impl {

template <typename AnnotatedSignature>
struct BindAssistedFactory;

// General case, value
template <typename C>
struct GetHelper {
  C operator()(UnsafeModule& module) {
    return *(module.getPtr<C>());
  }
};

template <typename C>
struct GetHelper<const C> {
  const C operator()(UnsafeModule& module) {
    return *(module.getPtr<C>());
  }
};

template <typename C>
struct GetHelper<std::shared_ptr<C>> {
  std::shared_ptr<C> operator()(UnsafeModule& module) {
    return std::shared_ptr<C>(std::shared_ptr<char>(), module.getPtr<C>());
  }
};

template <typename C>
struct GetHelper<C*> {
  C* operator()(UnsafeModule& module) {
    return module.getPtr<C>();
  }
};

template <typename C>
struct GetHelper<const C*> {
  const C* operator()(UnsafeModule& module) {
    return module.getPtr<C>();
  }
};

template <typename C>
struct GetHelper<C&> {
  C& operator()(UnsafeModule& module) {
    return *(module.getPtr<C>());
  }
};

template <typename C>
struct GetHelper<const C&> {
  const C& operator()(UnsafeModule& module) {
    return *(module.getPtr<C>());
  }
};

template <typename... Ps>
struct GetHelper<Injector<Ps...>> {
  Injector<Ps...> operator()(UnsafeModule& unsafeModule) {
    return Injector<Ps...>(unsafeModule);
  }
};

// Non-assisted case.
template <int numAssistedBefore, typename Arg, typename ParamTuple>
struct GetAssistedArgHelper {
  auto operator()(UnsafeModule& m, ParamTuple) -> decltype(m.get<Arg>()) {
    return m.get<Arg>();
  }
};

// Assisted case.
template <int numAssistedBefore, typename Arg, typename ParamTuple>
struct GetAssistedArgHelper<numAssistedBefore, Assisted<Arg>, ParamTuple> {
  auto operator()(UnsafeModule&, ParamTuple paramTuple) -> decltype(std::get<numAssistedBefore>(paramTuple)) {
    return std::get<numAssistedBefore>(paramTuple);
  }
};

template <int index, typename AnnotatedArgs, typename ParamTuple>
struct GetAssistedArg : public GetAssistedArgHelper<NumAssistedBefore<index, AnnotatedArgs>::value, GetNthType<index, AnnotatedArgs>, ParamTuple> {};

template <typename AnnotatedSignature, typename InjectedFunctionType, typename Sequence>
class BindAssistedFactoryHelper {};

template <typename AnnotatedSignature, typename C, typename... Params, int... indexes>
class BindAssistedFactoryHelper<AnnotatedSignature, C(Params...), IntList<indexes...>> {
private:
  /* std::function<C(Params...)>, C(Args...) */
  using RequiredSignature = RequiredSignatureForAssistedFactory<AnnotatedSignature>;
  
  UnsafeModule& m;
  RequiredSignature* factory;
  
public:
  BindAssistedFactoryHelper(UnsafeModule& m, RequiredSignature* factory) 
    :m(m), factory(factory) {}

  C operator()(Params... params) {
      return factory(GetAssistedArg<indexes, SignatureArgs<AnnotatedSignature>, decltype(std::tie(params...))>()(m, std::tie(params...))...);
  }
};

template <typename AnnotatedSignature>
struct BindAssistedFactory : public BindAssistedFactoryHelper<
      AnnotatedSignature,
      InjectedFunctionTypeForAssistedFactory<AnnotatedSignature>,
      GenerateIntSequence<
        list_size<
          SignatureArgs<RequiredSignatureForAssistedFactory<AnnotatedSignature>>
        >::value
      >> {
  BindAssistedFactory(UnsafeModule& m, RequiredSignatureForAssistedFactory<AnnotatedSignature>* factory) 
    : BindAssistedFactoryHelper<
      AnnotatedSignature,
      InjectedFunctionTypeForAssistedFactory<AnnotatedSignature>,
      GenerateIntSequence<
        list_size<
          SignatureArgs<RequiredSignatureForAssistedFactory<AnnotatedSignature>>
        >::value
      >>(m, factory) {}
};


template <typename MessageGenerator>
inline void UnsafeModule::check(bool b, MessageGenerator messageGenerator) {
  if (!b) {
    printError(messageGenerator());
    abort();
  }
}

template <typename C>
inline void UnsafeModule::createTypeInfo(std::pair<void*, void(*)(void*)> (*create)(UnsafeModule&, void*),
                                         void* createArgument) {
  createTypeInfo(getTypeIndex<C>(), create, createArgument);
}

template <typename C>
inline void UnsafeModule::createTypeInfo(void* instance,
                                         void (*destroy)(void*)) {
  createTypeInfo(getTypeIndex<C>(), instance, destroy);
}

template <typename C>
inline C* UnsafeModule::getPtr() {
  void* p = getPtr(getTypeIndex<C>());
  return reinterpret_cast<C*>(p);
}

// I, C must not be pointers.
template <typename I, typename C>
inline void UnsafeModule::bind() {
  FruitStaticAssert(!std::is_pointer<I>::value, "I should not be a pointer");
  FruitStaticAssert(!std::is_pointer<C>::value, "C should not be a pointer");
  auto create = [](UnsafeModule& m, void*) {
    C* cPtr = m.getPtr<C>();
    // This step is needed when the cast C->I changes the pointer
    // (e.g. for multiple inheritance).
    I* iPtr = static_cast<I*>(cPtr);
    return std::pair<void*, void(*)(void*)>(reinterpret_cast<void*>(iPtr), [](void*){});
  };
  createTypeInfo<I>(create, nullptr);
}

template <typename C>
inline void UnsafeModule::bindInstance(C* instance) {
  check(instance != nullptr, "attempting to register nullptr as instance");
  createTypeInfo<C>(instance, [](void*){});
}

template <typename C, typename... Args>
inline void UnsafeModule::registerProvider(C* (*provider)(Args...)) {
  FruitStaticAssert(!std::is_pointer<C>::value, "C should not be a pointer");
  check(provider != nullptr, "attempting to register nullptr as provider");
  using provider_type = decltype(provider);
  auto create = [](UnsafeModule& m, void* arg) {
    provider_type provider = reinterpret_cast<provider_type>(arg);
    C* cPtr = provider(m.get<Args>()...);
    auto deleteOperation = [](void* p) {
      delete reinterpret_cast<C*>(p);
    };
    return std::pair<void*, void(*)(void*)>(reinterpret_cast<void*>(cPtr), deleteOperation);
  };
  createTypeInfo<C>(create, reinterpret_cast<void*>(provider));
}

template <typename C, typename... Args>
inline void UnsafeModule::registerProvider(C (*provider)(Args...)) {
  FruitStaticAssert(!std::is_pointer<C>::value, "C should not be a pointer");
  // TODO: Move this check into ModuleImpl.
  static_assert(std::is_move_constructible<C>::value, "C should be movable");
  check(provider != nullptr, "attempting to register nullptr as provider");
  using provider_type = decltype(provider);
  auto create = [](UnsafeModule& m, void* arg) {
    provider_type provider = reinterpret_cast<provider_type>(arg);
    C* cPtr = new C(provider(m.get<Args>()...));
    auto deleteOperation = [](void* p) {
      delete reinterpret_cast<C*>(p);
    };
    return std::pair<void*, void(*)(void*)>(reinterpret_cast<void*>(cPtr), deleteOperation);
  };
  createTypeInfo<C>(create, reinterpret_cast<void*>(provider));
}

template <typename AnnotatedSignature>
inline void UnsafeModule::registerFactory(RequiredSignatureForAssistedFactory<AnnotatedSignature>* factory) {
  check(factory != nullptr, "attempting to register nullptr as factory");
  using Signature = RequiredSignatureForAssistedFactory<AnnotatedSignature>;
  using InjectedFunctionType = InjectedFunctionTypeForAssistedFactory<AnnotatedSignature>;
  auto create = [](UnsafeModule& m, void* arg) {
    Signature* factory = reinterpret_cast<Signature*>(arg);
    std::function<InjectedFunctionType>* fPtr = 
        new std::function<InjectedFunctionType>(BindAssistedFactory<AnnotatedSignature>(m, factory));
    auto deleteOperation = [](void* p) {
      delete reinterpret_cast<std::function<InjectedFunctionType>*>(p);
    };
    return std::pair<void*, void(*)(void*)>(reinterpret_cast<void*>(fPtr), deleteOperation);
  };
  createTypeInfo<std::function<InjectedFunctionType>>(create, reinterpret_cast<void*>(factory));
}

template <typename C>
inline UnsafeModule::TypeInfo& UnsafeModule::getTypeInfo() {
  TypeIndex typeIndex = getTypeIndex<C>();
  auto itr = typeRegistry.find(typeIndex);
  FruitCheck(itr != typeRegistry.end(), [=](){return "attempting to getTypeInfo() on a non-registered type: " + demangleTypeName(typeIndex.name());});
  return itr->second;
}

} // namespace fruit
} // namespace impl


#endif // FRUIT_UNSAFE_MODULE_TEMPLATES_H