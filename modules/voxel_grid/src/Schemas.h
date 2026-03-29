#ifndef SCHEMAS_H
#define SCHEMAS_H

#include <sys/types.h>
#include <concepts>


namespace VG {

template <typename T>
concept TaskScheme = requires(T t) {
    typename T::Types;
    { t.after_task() } -> std::same_as<void>; // called from WorkerThread
    { t.operator()() } -> std::same_as<void>; // called from WorkerThread
    { t.before_task() } -> std::same_as<void>; // called from MainThread
};

template <typename Data>
concept DataScheme = requires(Data d) {
    { Data::commit() } -> std::same_as<void>;
    { d.add_to_commit() } -> std::same_as<void>;
};



template <typename T, template <typename...> class C>
struct is_specialization_of : std::false_type {};

template <template <typename...> class C, typename... Args>
struct is_specialization_of<C<Args...>, C> : std::true_type {};

template <typename T, template <typename...> class C>
concept specialization_of = is_specialization_of<T, C>::value;





}; //namespace VVG


#endif // SCHEMAS_H