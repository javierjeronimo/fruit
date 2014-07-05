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

#ifndef FRUIT_MODULE_H
#define FRUIT_MODULE_H

#include "impl/component_storage.h"
#include "impl/component.utils.h"
#include "impl/injection_errors.h"
#include "impl/basic_utils.h"
#include "impl/fruit_assert.h"

namespace fruit {

namespace impl {

template <typename Comp>
struct Identity;

template <typename Comp, typename I, typename C>
struct Bind;

template <typename Comp, typename I, typename C>
struct BindNonFactory;

template <typename Comp, typename I, typename C>
struct AddMultibinding;

template <typename Comp, typename Signature>
struct RegisterProvider;

template <typename Comp, typename Signature>
struct RegisterMultibindingProvider;

template <typename Comp, typename AnnotatedSignature>
struct RegisterFactory;

template <typename Comp, typename C>
struct RegisterInstance;

template <typename Comp, typename C>
struct AddInstanceMultibinding;

template <typename Comp, typename Signature>
struct RegisterConstructor;

template <typename Comp, typename AnnotatedSignature>
struct RegisterConstructorAsFactory;

template <typename Comp, typename OtherComp>
struct InstallComponent;

template <typename Comp, typename ToRegister>
struct ComponentConversionHelper;

template <typename Comp, typename TargetRequirements, bool is_already_provided, typename C>
struct EnsureProvidedTypeHelper;

/**
 * This class contains the methods of Component (which will be used by this library's users) but should not
 * be instantiated directly by client code. I.e. a user of the library should never write `ComponentImpl'.
 * Always start the construction of a component with createComponent().
 */
template <typename RsParam, typename PsParam, typename DepsParam, typename BindingsParam>
struct ComponentImpl {
public:
  // Just for convenience.
  using Rs = RsParam;
  using Ps = PsParam;
  using Deps = DepsParam;
  using Bindings = BindingsParam;
  using This = ComponentImpl<Rs, Ps, Deps, Bindings>;
  
  // Invariants:
  // * all types appearing as arguments of Deps are in Rs
  // * all types in Ps are at the head of one (and only one) Dep.
  //   (note that the types in Rs can appear in deps any number of times, 0 is also ok)
  // * Deps is of the form List<Dep...> with each Dep of the form T(Args...) and where List<Args...> is a set (no repetitions).
  // * Bindings is of the form List<I1*(C1*), ..., In*(Cn*)> and is a set (no repetitions).
  
private:
  FruitStaticAssert(std::is_same<AddDeps<Deps, List<>>, Deps>::value,
                    "Internal error: ComponentImpl instantiated with non-normalized deps");
    
  // Invariant: all types in Ps must be bound in storage.
  ComponentStorage storage;
  
  ComponentImpl() = default;
  
  ComponentImpl(ComponentStorage&& storage);
    
  template <typename... Types>
  friend class fruit::Injector;
  
  template <typename... Types>
  friend class fruit::Component;
  
  template <typename OtherRs, typename OtherPs, typename OtherDeps, typename OtherBindings>
  friend struct fruit::impl::ComponentImpl;
  
  template <typename Comp, typename ToRegister>
  friend struct fruit::impl::ComponentConversionHelper;
  
  template <typename Comp, typename TargetRequirements, bool is_already_provided, typename C>
  friend struct fruit::impl::EnsureProvidedTypeHelper;
    
  template <typename Comp>
  friend struct fruit::impl::Identity;
  
  template <typename Comp, typename I, typename C>
  friend struct fruit::impl::Bind;
  
  template <typename Comp, typename I, typename C>
  friend struct fruit::impl::BindNonFactory;
  
  template <typename Comp, typename C>
  friend struct fruit::impl::RegisterInstance;
  
  template <typename Comp, typename Signature>
  friend struct fruit::impl::RegisterProvider;
  
  template <typename Comp, typename I, typename C>
  friend struct fruit::impl::AddMultibinding;
  
  template <typename Comp, typename C>
  friend struct fruit::impl::AddInstanceMultibinding;
  
  template <typename Comp, typename Signature>
  friend struct fruit::impl::RegisterMultibindingProvider;
  
  template <typename Comp, typename AnnotatedSignature>
  friend struct fruit::impl::RegisterFactory;
  
  template <typename Comp, typename Signature>
  friend struct RegisterConstructor;
  
  template <typename Comp, typename AnnotatedSignature>
  friend struct fruit::impl::RegisterConstructorAsFactory;
  
  template <typename Comp, typename OtherM>
  friend struct fruit::impl::InstallComponent;
  
  template <typename Source_Rs, typename Source_Ps, typename Source_Deps, typename Source_Bindings>
  ComponentImpl(const ComponentImpl<Source_Rs, Source_Ps, Source_Deps, Source_Bindings>& sourceComponent);
  
public:
  /**
   * Binds the base class (typically, an interface or abstract class) I to the implementation C.
   */
  template <typename I, typename C>
  FunctorResult<Bind<This, I, C>, This&&>
  bind() && {
    return Bind<This, I, C>()(std::move(*this));
  }
  
  /**
   * Registers Signature as the constructor signature to use to inject a type.
   * For example, registerConstructor<C(U,V)>() registers the constructor C::C(U,V).
   * 
   * It's usually more convenient to use an Inject typedef or INJECT macro instead, e.g.:
   * 
   * class C {
   * public:
   *   // This also declares the constructor
   *   INJECT(C(U u, V v));
   * ...
   * };
   * 
   * or
   * 
   * class C {
   * public:
   *   using Inject = C(U,V);
   * ...
   * };
   * 
   * Use registerConstructor() when you want to inject the class C in different ways
   * in different components, or when C is a third-party class that can't be modified.
   */
  template <typename Signature>
  FunctorResult<RegisterConstructor<This, Signature>, This&&>
  registerConstructor() && {
    return RegisterConstructor<This, Signature>()(std::move(*this));
  }
  
  /**
   * Use this method to bind the type C to a specific instance.
   * The caller must ensure that the provided reference is valid for the lifetime of this
   * component and of any injectors using this component, and must ensure that the object
   * is deleted after the last components/injectors using it are destroyed.
   * 
   * This should be used sparingly, but in some cases it can be useful; for example, if
   * a web server creates an injector to handle each request, this method can be used
   * to inject the request itself.
   */
  template <typename C>
  FunctorResult<RegisterInstance<This, C>, This&&, C&>
  bindInstance(C& instance) && {
    return RegisterInstance<This, C>()(std::move(*this), instance);
  }
  
  /**
   * Registers `provider' as a provider of C, where provider is a function
   * returning either C or C* (returning C* is preferable). A lambda with no captures
   * can be used as a function.
   * When an instance of C is needed, the arguments of the provider will be injected
   * and the provider will be called to create the instance of C, that will then be
   * stored in the injector.
   * `provider' must return a non-null pointer, otherwise the program will abort.
   * Example:
   * 
   * registerProvider([](U* u, V* v) {
   *    C* c = new C(u, v);
   *    c->initialize();
   *    return c;
   * })
   * 
   * As in the previous example, it's not necessary to specify the signature, it will
   * be inferred by the compiler.
   * 
   * Registering stateful functors (including lambdas with captures) is not supported.
   * However, instead of registering a functor F to provide a C, it's possible to bind F
   * (binding an instance if necessary) and then use this method to register a provider
   * function that takes a F and any other needed parameters, calls F with such parameters
   * and returns a C*.
   */
  template <typename Function>
  FunctorResult<RegisterProvider<This, FunctionSignature<Function>>, This&&, FunctionSignature<Function>*, void(*)(void*)>
  registerProvider(Function provider) && {
    return RegisterProvider<This, FunctionSignature<Function>>()(std::move(*this), provider, SimpleDeleter<SignatureType<FunctionSignature<Function>>>::f);
  }
  
  /**
   * Similar to bind(), but adds a multibinding instead.
   * 
   * Note that multibindings are independent from bindings; creating a binding with bind doesn't count as a multibinding,
   * and adding a multibinding doesn't allow to inject the type (only allows to retrieve multibindings through the
   * getMultibindings method of the injector).
   */
  template <typename I, typename C>
  FunctorResult<AddMultibinding<This, I, C>, This&&>
  addMultibinding() && {
    return AddMultibinding<This, I, C>()(std::move(*this));
  }
  
  /**
   * Similar to bindInstance, but adds a multibinding instead.
   * 
   * Note that multibindings are independent from bindings; creating a binding with bindInstance doesn't count as a
   * multibinding, and adding a multibinding doesn't allow to inject the type (only allows to retrieve
   * multibindings through the getMultibindings method of the injector).
   */
  template <typename C>
  FunctorResult<AddInstanceMultibinding<This, C>, This&&, C&>
  addInstanceMultibinding(C& instance) && {
    return AddInstanceMultibinding<This, C>()(std::move(*this), instance);
  }
  
  /**
   * Similar to registerProvider, but adds a multibinding instead.
   * 
   * Note that multibindings are independent from bindings; creating a binding with registerProvider doesn't count as a
   * multibinding, and adding a multibinding doesn't allow to inject the type (only allows to retrieve multibindings
   * through the getMultibindings method of the injector).
   */
  template <typename Function>
  FunctorResult<RegisterMultibindingProvider<This, FunctionSignature<Function>>, This&&, FunctionSignature<Function>*, void(*)(void*)>
  addMultibindingProvider(Function provider) && {
    return RegisterMultibindingProvider<This, FunctionSignature<Function>>()(std::move(*this), provider, SimpleDeleter<SignatureType<FunctionSignature<Function>>>::f);
  }
    
  /**
   * Registers `factory' as a factory of C, where `factory' is a function returning either C or C*
   * (returning C* is preferable). A lambda with no captures can be used as a function.
   * 
   * registerFactory<C(Assisted<U*>, V*)>([](U* u, V* v) {
   *   return new C(u, v);
   * })
   * 
   * As in the previous example, this is usually used for assisted injection. Unlike
   * registerProvider, where the signature is inferred, for this method the signature
   * must be specified explicitly.
   * This is usually used for assisted injection: some parameters are marked as Assisted
   * and are not injected. Instead of calling injector.get<C*>(), in this example we will
   * call injector.get<std::function<C(U*)>() (or we will declare std::function<C(U*)> as
   * an injected parameter to another provider or class).
   * 
   * If the only thing that the factory does is to call the constructor of C, it's usually
   * more convenient to use an Inject typedef or INJECT macro instead, e.g.:
   * 
   * class C {
   * public:
   *   // This also declares the constructor
   *   INJECT(C(ASSISTED(U) u, V v));
   * ...
   * };
   * 
   * or
   * 
   * class C {
   * public:
   *   using Inject = C(Assisted<U>,V);
   * ...
   * };
   * 
   * Use registerFactory() when you want to inject the class C in different ways
   * in different components, or when C is a third-party class that can't be modified.
   * 
   * Registering stateful functors (including lambdas with captures) is not supported.
   * However, instead of registering a functor F to provide a C, it's possible to bind F
   * (binding an instance if necessary) and then use this method to register a provider
   * function that takes a F and any other needed parameters, calls F with such parameters
   * and returns a C*.
   */
  template <typename AnnotatedSignature>
  FunctorResult<RegisterFactory<This, AnnotatedSignature>, This&&, RequiredSignatureForAssistedFactory<AnnotatedSignature>*>
  registerFactory(RequiredSignatureForAssistedFactory<AnnotatedSignature>* factory) && {
    return RegisterFactory<This, AnnotatedSignature>()(std::move(*this), factory);
  }
  
  /**
   * Adds the bindings in `component' to the current component.
   * For example:
   * 
   * createComponent()
   *    .install(getComponent1())
   *    .install(getComponent2())
   *    .bind<I, C>()
   * 
   * As seen in the example, the template parameters will be inferred by the compiler,
   * it's not necessary to specify them explicitly.
   */
  template <typename OtherRs, typename OtherPs, typename OtherDeps, typename OtherBindings>
  FunctorResult<InstallComponent<This, ComponentImpl<OtherRs, OtherPs, OtherDeps, OtherBindings>>, This&&, const ComponentImpl<OtherRs, OtherPs, OtherDeps, OtherBindings>&>
  install(const ComponentImpl<OtherRs, OtherPs, OtherDeps, OtherBindings>& component) && {
    return InstallComponent<This, ComponentImpl<OtherRs, OtherPs, OtherDeps, OtherBindings>>()(std::move(*this), component);
  }
};

} // namespace impl

// Used to group the requirements of Component.
template <typename... Types>
struct Required {};

// Used to annotate T as a type that uses assisted injection.
template <typename T>
struct Assisted;

/**
 * The parameters must be of the form <Required<R...>, P...> where R are the required types and P are the provided ones.
 * If the list of requirements is empty it can be omitted (e.g. Component<Foo, Bar>).
 * No type can appear twice. Not even once in R and once in P.
 */
template <typename... Types>
class Component;

template <typename... Types>
class Component : public Component<Required<>, Types...> {
private:
  Component() = default;
  
  friend Component<> createComponent();
  
public:
  /**
   * Converts a component to another, auto-injecting the missing types (if any).
   * This is typically called implicitly when returning a component from a function.
   * 
   * To copy a component, the most convenient way is to call createComponent().install(m).
   */
  template <typename Comp>
  Component(Comp&& m) 
    : Component<Required<>, Types...>(std::forward<Comp>(m)) {
  }
};

template <typename... R, typename... P>
class Component<Required<R...>, P...> 
  : public fruit::impl::ComponentImpl<fruit::impl::List<R...>,
                                      fruit::impl::List<P...>,
                                      fruit::impl::ConstructDeps<fruit::impl::List<R...>, P...>,
                                      fruit::impl::List<>> {
private:
  FruitDelegateCheck(fruit::impl::CheckNoRepeatedTypes<R..., P...>);
  FruitDelegateChecks(fruit::impl::CheckClassType<R, fruit::impl::GetClassForType<R>>);  
  FruitDelegateChecks(fruit::impl::CheckClassType<P, fruit::impl::GetClassForType<P>>);  
  
  using Impl = fruit::impl::ComponentImpl<fruit::impl::List<R...>,
                                          fruit::impl::List<P...>,
                                          fruit::impl::ConstructDeps<fruit::impl::List<R...>, P...>,
                                          fruit::impl::List<>>;
  
  Component() = default;
  
  template <typename... Types>
  friend class Component;
  
public:
  template <typename OtherRs, typename OtherPs, typename OtherDeps, typename OtherBindings>
  Component(fruit::impl::ComponentImpl<OtherRs, OtherPs, OtherDeps, OtherBindings>&& component)
    : Impl(std::move(component)) {
  }
};

inline Component<> createComponent() {
  return {};
}


} // namespace fruit

#include "impl/component.templates.h"


#endif // FRUIT_MODULE_H
