// Umbrella module for Nandina reactive primitives.
// Consumers only need `import Nandina.Reactive`.
//
// Sub-modules:
//   Nandina.Core.Signal        — Connection, ScopedConnection, EventSignal<>
//   Nandina.Reactive.State     — State<T>, ReadState<T>, Computed<F>,
//                                Effect, EffectScope
//   Nandina.Reactive.Prop      — Prop<T>: unified static/reactive property
//   Nandina.Reactive.StateList — StateList<T>: observable vector
export module Nandina.Reactive;

export import Nandina.Core.Signal;
export import Nandina.Reactive.State;
export import Nandina.Reactive.Prop;
export import Nandina.Reactive.StateList;

